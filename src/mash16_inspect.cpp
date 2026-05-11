#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::string json_escape(const std::string& s)
{
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

static bool is_numberish(const std::string& s)
{
    if (s.empty()) return false;
    char *end = nullptr;
    strtoul(s.c_str(), &end, 0);
    return end && *end == '\0';
}

static std::string request_for(const std::string& method, const std::string& params = "{}")
{
    return "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"" + method + "\",\"params\":" + params + "}\n";
}

static std::string target_params(const std::string& key, const std::string& value, const std::string& suffix = "")
{
    std::ostringstream o;
    o << "{";
    if (is_numberish(value))
        o << "\"" << key << "\":" << value;
    else
        o << "\"target\":\"" << json_escape(value) << "\"";
    if (!suffix.empty()) o << "," << suffix;
    o << "}";
    return o.str();
}

static std::string send_request(const std::string& path, const std::string& req)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket: " << strerror(errno) << "\n";
        return "";
    }
    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::cerr << "connect " << path << ": " << strerror(errno) << "\n";
        close(fd);
        return "";
    }
    size_t written = 0;
    while (written < req.size()) {
        ssize_t n = write(fd, req.data() + written, req.size() - written);
        if (n <= 0) break;
        written += static_cast<size_t>(n);
    }
    shutdown(fd, SHUT_WR);
    std::string resp;
    char buf[4096];
    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n <= 0) break;
        resp.append(buf, buf + n);
    }
    close(fd);
    return resp;
}

static std::string find_string(const std::string& text, const std::string& key)
{
    auto p = text.find("\"" + key + "\":\"");
    if (p == std::string::npos) return "";
    p = text.find('"', p + key.size() + 3);
    if (p == std::string::npos) return "";
    auto e = text.find('"', p + 1);
    if (e == std::string::npos) return "";
    return text.substr(p + 1, e - p - 1);
}

static std::string find_number(const std::string& text, const std::string& key)
{
    auto p = text.find("\"" + key + "\":");
    if (p == std::string::npos) return "";
    p += key.size() + 3;
    auto e = p;
    while (e < text.size() && (isdigit(static_cast<unsigned char>(text[e])) || text[e] == '-')) ++e;
    return text.substr(p, e - p);
}

static void print_disasm_text(const std::string& resp)
{
    size_t pos = 0;
    while ((pos = resp.find("\"text\":\"", pos)) != std::string::npos) {
        pos += 8;
        auto end = resp.find('"', pos);
        if (end == std::string::npos) break;
        std::cout << resp.substr(pos, end - pos) << "\n";
        pos = end + 1;
    }
}

static void usage()
{
    std::cerr << "usage: mash16-inspect [--socket PATH] [--json] COMMAND [ARGS]\n"
              << "commands: status, regs, mem ADDR SIZE, disasm TARGET COUNT, break TARGET,\n"
              << "          clear-break TARGET, run-until TARGET, step [COUNT]\n";
}

int main(int argc, char **argv)
{
    std::string socket_path = "/tmp/mash16.sock";
    bool json = false;
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--json") json = true;
        else if (a == "--socket" && i + 1 < argc) socket_path = argv[++i];
        else if (a.rfind("--socket=", 0) == 0) socket_path = a.substr(9);
        else args.push_back(a);
    }
    if (args.empty()) {
        usage();
        return 2;
    }

    std::string cmd = args[0];
    std::string req;
    if (cmd == "status") req = request_for("state");
    else if (cmd == "regs") req = request_for("readRegisters");
    else if (cmd == "mem" && args.size() >= 3) req = request_for("readMemory", "{\"addr\":" + args[1] + ",\"size\":" + args[2] + "}");
    else if (cmd == "disasm" && args.size() >= 2) {
        std::string count = args.size() >= 3 ? args[2] : "8";
        req = request_for("disassemble", target_params("addr", args[1], "\"count\":" + count));
    } else if (cmd == "break" && args.size() >= 2) req = request_for("setBreakpoint", target_params("addr", args[1]));
    else if (cmd == "clear-break" && args.size() >= 2) req = request_for("clearBreakpoint", target_params("addr", args[1]));
    else if (cmd == "run-until" && args.size() >= 2) req = request_for("runUntil", target_params("addr", args[1], "\"timeoutMs\":5000"));
    else if (cmd == "step") {
        std::string count = args.size() >= 2 ? args[1] : "1";
        req = request_for("step", "{\"count\":" + count + "}");
    } else {
        usage();
        return 2;
    }

    std::string resp = send_request(socket_path, req);
    if (resp.empty()) return 1;
    if (json) {
        std::cout << resp;
        return 0;
    }
    if (resp.find("\"error\"") != std::string::npos) {
        std::cerr << resp;
        return 1;
    }
    if (cmd == "status" || cmd == "step" || cmd == "run-until") {
        std::string pc = find_number(resp, "pc");
        std::string reason = find_string(resp, "reason");
        std::string instr = find_string(resp, "text");
        std::cout << "pc=" << pc << " stop=" << reason;
        if (!instr.empty()) std::cout << "  " << instr;
        std::cout << "\n";
    } else if (cmd == "disasm") {
        print_disasm_text(resp);
    } else {
        std::cout << resp;
    }
    return 0;
}
