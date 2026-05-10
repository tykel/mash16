#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
int use_verbose = 0;
#include "core/inspector.h"
#include "core/json_server.h"

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

    auto snapshot = insp.snapshot();
    std::string restore_req = std::string("{\"method\":\"restore\",\"data\":\"") + base64_encode(snapshot) + "\"}";
    cpu.m[0] = 0xaa;
    resp = send_request(path, restore_req);
    if (resp.find("\"result\":true") == std::string::npos || cpu.m[0] != 0) {
        std::cerr << "restore failed: response=" << resp << " mem0=" << (int)cpu.m[0] << std::endl;
        srv.stop();
        free(cpu.m);
        return 5;
    }

    srv.stop();
    free(cpu.m);
    return 0;
}
