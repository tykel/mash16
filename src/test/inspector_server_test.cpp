#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cstdio>
int use_verbose = 0;
#include "inspector/inspector.h"
#include "inspector/json_server.h"

using namespace mash16;

static std::string base64_encode(const std::vector<uint8_t>& data)
{
    static const char *B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            b64.push_back(B64[(val >> valb) & 0x3f]);
            valb -= 6;
        }
    }
    if (valb > -6) b64.push_back(B64[((val << 8) >> (valb + 8)) & 0x3f]);
    while (b64.size() % 4) b64.push_back('=');
    return b64;
}

static int connect_client(const std::string& path)
{
    struct sockaddr_un addr; memset(&addr,0,sizeof(addr)); addr.sun_family = AF_UNIX; strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path)-1);
    for (int attempts = 0; attempts < 50; ++attempts) {
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) return -1;
        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) return fd;
        close(fd);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return -1;
}

static std::string send_request(const std::string& path, const std::string& req)
{
    int fd = connect_client(path);
    if (fd < 0) return "";
    size_t written = 0;
    while (written < req.size()) {
        ssize_t n = write(fd, req.data() + written, req.size() - written);
        if (n <= 0) break;
        written += (size_t)n;
    }
    shutdown(fd, SHUT_WR);
    std::string resp;
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) resp.append(buf, buf + n);
    close(fd);
    return resp;
}

static std::string send_split_request(const std::string& path, const std::string& first, const std::string& second)
{
    int fd = connect_client(path);
    if (fd < 0) return "";
    write(fd, first.data(), first.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    write(fd, second.data(), second.size());
    shutdown(fd, SHUT_WR);
    std::string resp;
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) resp.append(buf, buf + n);
    close(fd);
    return resp;
}

int main()
{
    cpu_state cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.m = (uint8_t*)malloc(MEM_SIZE);
    for (size_t i=0;i<MEM_SIZE;++i) cpu.m[i] = (uint8_t)(i&0xff);

    Inspector insp(&cpu);
    JsonServer srv;
    const std::string path = "/tmp/mash16_test.sock";
    if(!srv.start(path, &insp)) {
        std::cerr << "server failed to start" << std::endl;
        return 2;
    }

    std::string resp = send_request(path, "{\"method\":\"getRegisters\"}");
    if (!resp.empty()) {
        std::cout << "response: " << resp << std::endl;
    } else {
        std::cerr << "client connect failed after retries: errno=" << errno << " (" << strerror(errno) << ")" << std::endl;
        srv.stop();
        free(cpu.m);
        return 3;
    }

    resp = send_split_request(path, "{\"method\":\"get", "Registers\"}");
    if (resp.find("\"pc\"") == std::string::npos) {
        std::cerr << "split request failed: response=" << resp << std::endl;
        srv.stop();
        free(cpu.m);
        return 4;
    }

    resp = send_request(path, "{\"jsonrpc\":\"2.0\",\"id\":7,\"method\":\"initialize\"}\n");
    if (resp.find("\"jsonrpc\":\"2.0\"") == std::string::npos ||
        resp.find("\"id\":7") == std::string::npos ||
        resp.find("\"state\"") == std::string::npos) {
        std::cerr << "json-rpc initialize failed: response=" << resp << std::endl;
        srv.stop();
        free(cpu.m);
        return 5;
    }

    cpu.pc = 0;
    cpu.m[0] = 0x00; cpu.m[1] = 0x00; cpu.m[2] = 0x00; cpu.m[3] = 0x00;
    resp = send_request(path, "{\"jsonrpc\":\"2.0\",\"id\":\"s\",\"method\":\"state\"}\n");
    if (resp.find("\"instruction\"") == std::string::npos ||
        resp.find("\"text\":\"nop\"") == std::string::npos) {
        std::cerr << "json-rpc state failed: response=" << resp << std::endl;
        srv.stop();
        free(cpu.m);
        return 6;
    }

    std::string cli_cmd =
        "if [ -x ./mash16-inspect ]; then ./mash16-inspect --socket " + path +
        " --json status; else ./build-debug/mash16-inspect --socket " + path +
        " --json status; fi";
    FILE *pipe = popen(cli_cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "mash16-inspect popen failed" << std::endl;
        srv.stop();
        free(cpu.m);
        return 7;
    }
    char cli_buf[4096];
    std::string cli_resp;
    while (fgets(cli_buf, sizeof(cli_buf), pipe)) cli_resp += cli_buf;
    int cli_status = pclose(pipe);
    if (cli_status != 0 || cli_resp.find("\"instruction\"") == std::string::npos) {
        std::cerr << "mash16-inspect status failed: status=" << cli_status
                  << " response=" << cli_resp << std::endl;
        srv.stop();
        free(cpu.m);
        return 8;
    }

    cli_cmd =
        "if [ -x ./mash16-inspect ]; then ./mash16-inspect --socket " + path +
        " press b; else ./build-debug/mash16-inspect --socket " + path +
        " press b; fi";
    pipe = popen(cli_cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "mash16-inspect press popen failed" << std::endl;
        srv.stop();
        free(cpu.m);
        return 9;
    }
    cli_resp.clear();
    while (fgets(cli_buf, sizeof(cli_buf), pipe)) cli_resp += cli_buf;
    cli_status = pclose(pipe);
    if (cli_status != 0 || cli_resp.find("ok") == std::string::npos ||
        !(cpu.m[IO_PAD1_ADDR] & PAD_B)) {
        std::cerr << "mash16-inspect press failed: status=" << cli_status
                  << " response=" << cli_resp
                  << " pad1=" << (int)cpu.m[IO_PAD1_ADDR] << std::endl;
        srv.stop();
        free(cpu.m);
        return 9;
    }

    cli_cmd =
        "if [ -x ./mash16-inspect ]; then ./mash16-inspect --socket " + path +
        " release b; else ./build-debug/mash16-inspect --socket " + path +
        " release b; fi";
    pipe = popen(cli_cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "mash16-inspect release popen failed" << std::endl;
        srv.stop();
        free(cpu.m);
        return 10;
    }
    cli_resp.clear();
    while (fgets(cli_buf, sizeof(cli_buf), pipe)) cli_resp += cli_buf;
    cli_status = pclose(pipe);
    if (cli_status != 0 || cli_resp.find("ok") == std::string::npos ||
        (cpu.m[IO_PAD1_ADDR] & PAD_B)) {
        std::cerr << "mash16-inspect release failed: status=" << cli_status
                  << " response=" << cli_resp
                  << " pad1=" << (int)cpu.m[IO_PAD1_ADDR] << std::endl;
        srv.stop();
        free(cpu.m);
        return 10;
    }

    resp = send_request(path,
        "{\"jsonrpc\":\"2.0\",\"id\":8,\"method\":\"setBreakpoint\",\"params\":{\"addr\":512}}\n"
        "{\"jsonrpc\":\"2.0\",\"id\":9,\"method\":\"listBreakpoints\"}\n");
    if (resp.find("\"id\":8") == std::string::npos ||
        resp.find("\"id\":9") == std::string::npos ||
        resp.find("512") == std::string::npos) {
        std::cerr << "newline-delimited json-rpc failed: response=" << resp << std::endl;
        srv.stop();
        free(cpu.m);
        return 11;
    }

    auto snapshot = insp.snapshot();
    std::string restore_req = std::string("{\"method\":\"restore\",\"data\":\"") + base64_encode(snapshot) + "\"}";
    cpu.m[0] = 0xaa;
    resp = send_request(path, restore_req);
    if (resp.find("\"result\":true") == std::string::npos || cpu.m[0] != 0) {
        std::cerr << "restore failed: response=" << resp << " mem0=" << (int)cpu.m[0] << std::endl;
        srv.stop();
        free(cpu.m);
        return 12;
    }

    cpu.m[IO_PAD1_ADDR] = 0;
    resp = send_request(path,
        "{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"pressControllerButton\","
        "\"params\":{\"button\":\"a\",\"player\":1}}\n");
    if (resp.find("\"result\":true") == std::string::npos || !(cpu.m[IO_PAD1_ADDR] & PAD_A)) {
        std::cerr << "controller press failed: response=" << resp
                  << " pad1=" << (int)cpu.m[IO_PAD1_ADDR] << std::endl;
        srv.stop();
        free(cpu.m);
        return 13;
    }

    resp = send_request(path,
        "{\"jsonrpc\":\"2.0\",\"id\":12,\"method\":\"releaseControllerButton\","
        "\"params\":{\"button\":\"a\",\"player\":1}}\n");
    if (resp.find("\"result\":true") == std::string::npos || (cpu.m[IO_PAD1_ADDR] & PAD_A)) {
        std::cerr << "controller release failed: response=" << resp
                  << " pad1=" << (int)cpu.m[IO_PAD1_ADDR] << std::endl;
        srv.stop();
        free(cpu.m);
        return 14;
    }

    cpu.m[IO_PAD2_ADDR] = 0;
    resp = send_request(path,
        "{\"jsonrpc\":\"2.0\",\"id\":13,\"method\":\"pressControllerButton\","
        "\"params\":{\"button\":\"start\",\"player\":2}}\n");
    if (resp.find("\"result\":true") == std::string::npos || !(cpu.m[IO_PAD2_ADDR] & PAD_START)) {
        std::cerr << "controller player 2 press failed: response=" << resp
                  << " pad2=" << (int)cpu.m[IO_PAD2_ADDR] << std::endl;
        srv.stop();
        free(cpu.m);
        return 15;
    }

    cpu.m[IO_PAD1_ADDR] = 0;
    resp = send_request(path,
        "{\"jsonrpc\":\"2.0\",\"id\":14,\"method\":\"pressControllerButton\","
        "\"params\":{\"button\":\"b\",\"player\":257}}\n");
    if (resp.find("\"error\"") == std::string::npos || (cpu.m[IO_PAD1_ADDR] & PAD_B)) {
        std::cerr << "invalid controller player accepted: response=" << resp
                  << " pad1=" << (int)cpu.m[IO_PAD1_ADDR] << std::endl;
        srv.stop();
        free(cpu.m);
        return 16;
    }

    srv.stop();
    free(cpu.m);
    return 0;
}
