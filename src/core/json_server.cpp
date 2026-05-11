#include "json_server.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <cctype>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <iomanip>
#include <optional>

static const char *B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

namespace mash16 {
namespace {

struct Json {
    enum Type { Null, Bool, Number, String, Array, Object } type = Null;
    bool b = false;
    double n = 0;
    std::string s;
    std::vector<Json> a;
    std::map<std::string, Json> o;

    static Json null() { return {}; }
    static Json boolean(bool v) { Json j; j.type = Bool; j.b = v; return j; }
    static Json number(double v) { Json j; j.type = Number; j.n = v; return j; }
    static Json string(std::string v) { Json j; j.type = String; j.s = std::move(v); return j; }
    static Json array() { Json j; j.type = Array; return j; }
    static Json object() { Json j; j.type = Object; return j; }

    const Json *get(const std::string& key) const {
        if (type != Object) return nullptr;
        auto it = o.find(key);
        return it == o.end() ? nullptr : &it->second;
    }
};

static std::string json_escape(const std::string &s) {
    std::ostringstream o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if (c < 0x20) {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

static std::string dump_json(const Json& j) {
    std::ostringstream o;
    switch (j.type) {
        case Json::Null: o << "null"; break;
        case Json::Bool: o << (j.b ? "true" : "false"); break;
        case Json::Number:
            if (j.n == static_cast<long long>(j.n)) o << static_cast<long long>(j.n);
            else o << j.n;
            break;
        case Json::String: o << "\"" << json_escape(j.s) << "\""; break;
        case Json::Array:
            o << "[";
            for (size_t i = 0; i < j.a.size(); ++i) {
                if (i) o << ",";
                o << dump_json(j.a[i]);
            }
            o << "]";
            break;
        case Json::Object:
            o << "{";
            for (auto it = j.o.begin(); it != j.o.end(); ++it) {
                if (it != j.o.begin()) o << ",";
                o << "\"" << json_escape(it->first) << "\":" << dump_json(it->second);
            }
            o << "}";
            break;
    }
    return o.str();
}

class Parser {
public:
    explicit Parser(const std::string& input) : in_(input) {}

    bool parse(Json& out, std::string& error) {
        skip_ws();
        if (!value(out)) {
            error = "parse error";
            return false;
        }
        skip_ws();
        if (pos_ != in_.size()) {
            error = "trailing characters";
            return false;
        }
        return true;
    }

private:
    bool value(Json& out) {
        skip_ws();
        if (pos_ >= in_.size()) return false;
        char c = in_[pos_];
        if (c == 'n') return literal("null", Json::null(), out);
        if (c == 't') return literal("true", Json::boolean(true), out);
        if (c == 'f') return literal("false", Json::boolean(false), out);
        if (c == '"') return string(out);
        if (c == '[') return array(out);
        if (c == '{') return object(out);
        return number(out);
    }

    bool literal(const char *lit, const Json& val, Json& out) {
        size_t len = strlen(lit);
        if (in_.compare(pos_, len, lit) != 0) return false;
        pos_ += len;
        out = val;
        return true;
    }

    bool string(Json& out) {
        if (in_[pos_++] != '"') return false;
        std::string result;
        while (pos_ < in_.size()) {
            char c = in_[pos_++];
            if (c == '"') {
                out = Json::string(result);
                return true;
            }
            if (c == '\\') {
                if (pos_ >= in_.size()) return false;
                char e = in_[pos_++];
                switch (e) {
                    case '"': result.push_back('"'); break;
                    case '\\': result.push_back('\\'); break;
                    case '/': result.push_back('/'); break;
                    case 'b': result.push_back('\b'); break;
                    case 'f': result.push_back('\f'); break;
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    case 'u':
                        if (pos_ + 4 > in_.size()) return false;
                        result.push_back('?');
                        pos_ += 4;
                        break;
                    default:
                        return false;
                }
            } else {
                result.push_back(c);
            }
        }
        return false;
    }

    bool number(Json& out) {
        size_t start = pos_;
        if (pos_ < in_.size() && in_[pos_] == '-') ++pos_;
        if (pos_ + 1 < in_.size() && in_[pos_] == '0' && (in_[pos_ + 1] == 'x' || in_[pos_ + 1] == 'X')) {
            pos_ += 2;
            while (pos_ < in_.size() && isxdigit((unsigned char)in_[pos_])) ++pos_;
            char *end = nullptr;
            std::string token = in_.substr(start, pos_ - start);
            unsigned long v = strtoul(token.c_str(), &end, 0);
            if (!end || *end != '\0') return false;
            out = Json::number(v);
            return true;
        }
        while (pos_ < in_.size() && isdigit((unsigned char)in_[pos_])) ++pos_;
        if (pos_ < in_.size() && in_[pos_] == '.') {
            ++pos_;
            while (pos_ < in_.size() && isdigit((unsigned char)in_[pos_])) ++pos_;
        }
        if (pos_ < in_.size() && (in_[pos_] == 'e' || in_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < in_.size() && (in_[pos_] == '+' || in_[pos_] == '-')) ++pos_;
            while (pos_ < in_.size() && isdigit((unsigned char)in_[pos_])) ++pos_;
        }
        if (pos_ == start) return false;
        char *end = nullptr;
        std::string token = in_.substr(start, pos_ - start);
        double v = strtod(token.c_str(), &end);
        if (!end || *end != '\0') return false;
        out = Json::number(v);
        return true;
    }

    bool array(Json& out) {
        if (in_[pos_++] != '[') return false;
        out = Json::array();
        skip_ws();
        if (pos_ < in_.size() && in_[pos_] == ']') { ++pos_; return true; }
        for (;;) {
            Json item;
            if (!value(item)) return false;
            out.a.push_back(std::move(item));
            skip_ws();
            if (pos_ < in_.size() && in_[pos_] == ',') { ++pos_; continue; }
            if (pos_ < in_.size() && in_[pos_] == ']') { ++pos_; return true; }
            return false;
        }
    }

    bool object(Json& out) {
        if (in_[pos_++] != '{') return false;
        out = Json::object();
        skip_ws();
        if (pos_ < in_.size() && in_[pos_] == '}') { ++pos_; return true; }
        for (;;) {
            Json key;
            if (!string(key)) return false;
            skip_ws();
            if (pos_ >= in_.size() || in_[pos_++] != ':') return false;
            Json val;
            if (!value(val)) return false;
            out.o[key.s] = std::move(val);
            skip_ws();
            if (pos_ < in_.size() && in_[pos_] == ',') { ++pos_; skip_ws(); continue; }
            if (pos_ < in_.size() && in_[pos_] == '}') { ++pos_; return true; }
            return false;
        }
    }

    void skip_ws() {
        while (pos_ < in_.size() && isspace((unsigned char)in_[pos_])) ++pos_;
    }

    const std::string& in_;
    size_t pos_ = 0;
};

static bool parse_json(const std::string& text, Json& out, std::string& error) {
    return Parser(text).parse(out, error);
}

static const Json *params_for(const Json& req) {
    if (auto params = req.get("params"); params && params->type == Json::Object) return params;
    return &req;
}

static bool uint_param(const Json& obj, const char *key, uint32_t& out) {
    auto v = obj.get(key);
    if (!v) return false;
    if (v->type == Json::Number) {
        if (v->n < 0 || v->n > 0xffffffffu) return false;
        out = static_cast<uint32_t>(v->n);
        return true;
    }
    if (v->type == Json::String) {
        char *end = nullptr;
        unsigned long n = strtoul(v->s.c_str(), &end, 0);
        if (!end || *end != '\0' || n > 0xffffffffu) return false;
        out = static_cast<uint32_t>(n);
        return true;
    }
    return false;
}

static bool string_param(const Json& obj, const char *key, std::string& out) {
    auto v = obj.get(key);
    if (!v || v->type != Json::String) return false;
    out = v->s;
    return true;
}

static Json registers_json(const Inspector::Registers& regs) {
    Json result = Json::object();
    result.o["pc"] = Json::number(regs.pc);
    result.o["sp"] = Json::number(regs.sp);
    Json r = Json::array();
    for (int i = 0; i < 16; ++i) r.a.push_back(Json::number(regs.r[i]));
    result.o["r"] = r;
    Json flags = Json::object();
    flags.o["c"] = Json::number(regs.f.c);
    flags.o["z"] = Json::number(regs.f.z);
    flags.o["o"] = Json::number(regs.f.o);
    flags.o["n"] = Json::number(regs.f.n);
    result.o["flags"] = flags;
    return result;
}

static Json instruction_json(const Inspector::Instruction& inst) {
    Json result = Json::object();
    result.o["addr"] = Json::number(inst.addr);
    result.o["raw"] = Json::number(inst.raw);
    result.o["op"] = Json::number(inst.op);
    result.o["mnemonic"] = Json::string(inst.mnemonic);
    result.o["text"] = Json::string(inst.text);
    if (inst.immediate) result.o["immediate"] = Json::number(*inst.immediate);
    if (!inst.symbol.empty()) result.o["symbol"] = Json::string(inst.symbol);
    return result;
}

static Json state_json(Inspector *inspector) {
    auto regs = inspector->getRegisters();
    Json result = Json::object();
    result.o["registers"] = registers_json(regs);
    result.o["pc"] = Json::number(regs.pc);
    result.o["instruction"] = instruction_json(inspector->disassembleOne(regs.pc));
    auto stop = inspector->stopState();
    Json stop_j = Json::object();
    stop_j.o["reason"] = Json::string(stop.reason);
    stop_j.o["pc"] = Json::number(stop.pc);
    stop_j.o["sequence"] = Json::number(stop.sequence);
    result.o["stop"] = stop_j;
    Json nearby = Json::array();
    uint16_t start = regs.pc >= 8 ? regs.pc - 8 : 0;
    for (auto& inst : inspector->disassemble(start, 5))
        nearby.a.push_back(instruction_json(inst));
    result.o["nearby"] = nearby;
    return result;
}

static std::string response_result(const Json& id, const Json& result, bool rpc) {
    if (!rpc) return std::string("{\"result\":") + dump_json(result) + "}\n";
    Json resp = Json::object();
    resp.o["jsonrpc"] = Json::string("2.0");
    resp.o["id"] = id;
    resp.o["result"] = result;
    return dump_json(resp) + "\n";
}

static std::string response_error(const Json& id, int code, const std::string& message, bool rpc) {
    if (!rpc) return std::string("{\"error\":\"") + json_escape(message) + "\"}\n";
    Json resp = Json::object();
    resp.o["jsonrpc"] = Json::string("2.0");
    resp.o["id"] = id;
    Json err = Json::object();
    err.o["code"] = Json::number(code);
    err.o["message"] = Json::string(message);
    resp.o["error"] = err;
    return dump_json(resp) + "\n";
}

static std::string base64_encode(const std::vector<uint8_t>& data) {
    std::string b64;
    int val=0, valb=-6;
    for (unsigned char c : data) {
        val = (val<<8) + c;
        valb += 8;
        while (valb>=0) {
            b64.push_back(B64[(val>>valb)&0x3F]);
            valb-=6;
        }
    }
    if (valb>-6) b64.push_back(B64[((val<<8)>>(valb+8))&0x3F]);
    while (b64.size()%4) b64.push_back('=');
    return b64;
}

static std::vector<uint8_t> base64_decode(const std::string& b64) {
    std::string tbl(256, -1);
    for (int i = 0; i < 64; ++i) tbl[(unsigned char)B64[i]] = i;
    std::vector<uint8_t> out;
    int val=0, valb=-8;
    for (unsigned char c : b64) {
        if (isspace(c) || c=='=') break;
        if (tbl[c] == (char)-1) continue;
        val = (val<<6) + tbl[c];
        valb += 6;
        if (valb>=0) {
            out.push_back((uint8_t)((val>>valb)&0xFF));
            valb -= 8;
        }
    }
    return out;
}

static bool request_is_rpc(const Json& req) {
    auto v = req.get("jsonrpc");
    return v && v->type == Json::String && v->s == "2.0";
}

} // namespace

JsonServer::JsonServer() : running_(false), inspector_(nullptr), server_fd_(-1) {}

JsonServer::~JsonServer() {
    stop();
}

bool JsonServer::start(const std::string& socket_path, Inspector *inspector) {
    if (running_) return false;
    inspector_ = inspector;

    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        perror("socket");
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path)-1);
    unlink(socket_path.c_str());

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    if (listen(server_fd_, 5) < 0) {
        perror("listen");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    running_ = true;
    thread_ = std::thread(&JsonServer::run, this, socket_path);
    return true;
}

void JsonServer::stop() {
    if (!running_) return;
    running_ = false;
    if (server_fd_ != -1) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }
    if (inspector_) inspector_->wakeEventWaiters();

    if (thread_.joinable()) thread_.join();
    for (auto &thread : subscribe_threads_) {
        if (thread.joinable()) thread.join();
    }
    subscribe_threads_.clear();
}

static void read_request(int client, std::string &req) {
    char buf[4096];
    for (;;) {
        struct pollfd pfd;
        pfd.fd = client;
        pfd.events = POLLIN | POLLHUP;
#ifdef POLLRDHUP
        pfd.events |= POLLRDHUP;
#endif
        int rv = poll(&pfd, 1, 1000);
        if (rv < 0) {
            if (errno == EINTR) continue;
            perror("poll client");
            break;
        }
        if (rv == 0) break;

        for (;;) {
            ssize_t n = read(client, buf, sizeof(buf));
            if (n > 0) {
                req.append(buf, buf + n);
                continue;
            }
            if (n == 0) return;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            perror("read");
            return;
        }

        if (pfd.revents & POLLHUP) break;
#ifdef POLLRDHUP
        if (pfd.revents & POLLRDHUP) break;
#endif
    }
}

static bool bytes_param(const Json& params, std::vector<uint8_t>& bytes) {
    auto v = params.get("bytes");
    if (!v || v->type != Json::Array) return false;
    for (const auto& item : v->a) {
        if (item.type != Json::Number || item.n < 0 || item.n > 255) return false;
        bytes.push_back(static_cast<uint8_t>(item.n));
    }
    return true;
}

std::string JsonServer::handleRequest(const std::string& text, int client, bool& keep_open) {
    Json req;
    std::string parse_error;
    if (!parse_json(text, req, parse_error) || req.type != Json::Object)
        return response_error(Json::null(), -32700, parse_error, true);

    bool rpc = request_is_rpc(req);
    Json id = req.get("id") ? *req.get("id") : Json::null();
    auto method_v = req.get("method");
    if (!method_v || method_v->type != Json::String)
        return response_error(id, -32600, "missing method", rpc);
    std::string method = method_v->s;
    const Json& params = *params_for(req);

    if (method == "subscribe") {
        subscribe_threads_.emplace_back(&JsonServer::streamSubscription, this, client, rpc, dump_json(id));
        keep_open = true;
        return "";
    }

    if (method == "initialize" || method == "capabilities") {
        Json result = Json::object();
        result.o["protocol"] = Json::string("mash16-jsonrpc");
        result.o["version"] = Json::number(1);
        Json methods = Json::array();
        const char *names[] = {
            "initialize","capabilities","state","pause","resume","step","waitStopped",
            "runUntil","runFor","setBreakpoint","clearBreakpoint","listBreakpoints",
            "setWatchpoint","clearWatchpoint","listWatchpoints","readMemory","writeMemory",
            "readRegisters","writeRegister","disassemble","symbols","resolveSymbol",
            "evaluate","snapshot","restore","trace","subscribe",
            "getRegisters","run"
        };
        for (auto name : names) methods.a.push_back(Json::string(name));
        result.o["methods"] = methods;
        return response_result(id, result, rpc);
    }
    if (method == "state") return response_result(id, state_json(inspector_), rpc);
    if (method == "getRegisters" || method == "readRegisters")
        return response_result(id, registers_json(inspector_->getRegisters()), rpc);
    if (method == "readMemory") {
        uint32_t addr = 0, size = 0;
        if (!uint_param(params, "addr", addr) || !uint_param(params, "size", size))
            return response_error(id, -32602, "missing addr/size", rpc);
        auto data = inspector_->readMemory(static_cast<uint16_t>(addr), size);
        Json result = Json::array();
        for (uint8_t b : data) result.a.push_back(Json::number(b));
        return response_result(id, result, rpc);
    }
    if (method == "writeMemory") {
        uint32_t addr = 0;
        std::vector<uint8_t> bytes;
        if (!uint_param(params, "addr", addr)) return response_error(id, -32602, "missing addr", rpc);
        if (!bytes_param(params, bytes)) return response_error(id, -32602, "missing bytes", rpc);
        bool ok = inspector_->writeMemory(static_cast<uint16_t>(addr), bytes);
        return ok ? response_result(id, Json::boolean(true), rpc) : response_error(id, -32000, "write failed", rpc);
    }
    if (method == "writeRegister") {
        std::string name;
        uint32_t value = 0;
        if (!string_param(params, "name", name) || !uint_param(params, "value", value))
            return response_error(id, -32602, "missing name/value", rpc);
        bool ok = inspector_->writeRegister(name, static_cast<int32_t>(value));
        return ok ? response_result(id, Json::boolean(true), rpc) : response_error(id, -32602, "unknown register", rpc);
    }
    if (method == "setBreakpoint" || method == "clearBreakpoint") {
        uint32_t addr = 0;
        std::string target;
        std::optional<uint16_t> resolved;
        if (uint_param(params, "addr", addr)) resolved = static_cast<uint16_t>(addr);
        else if (string_param(params, "target", target)) resolved = inspector_->resolveAddressOrSymbol(target);
        if (!resolved) return response_error(id, -32602, "missing or unknown addr/target", rpc);
        if (method == "setBreakpoint") inspector_->setBreakpoint(*resolved);
        else inspector_->clearBreakpoint(*resolved);
        return response_result(id, Json::boolean(true), rpc);
    }
    if (method == "listBreakpoints") {
        Json result = Json::array();
        for (auto bp : inspector_->listBreakpoints()) result.a.push_back(Json::number(bp));
        return response_result(id, result, rpc);
    }
    if (method == "setWatchpoint" || method == "clearWatchpoint") {
        uint32_t addr = 0;
        if (!uint_param(params, "addr", addr)) return response_error(id, -32602, "missing addr", rpc);
        if (method == "setWatchpoint") inspector_->setWatchpoint(static_cast<uint16_t>(addr));
        else inspector_->clearWatchpoint(static_cast<uint16_t>(addr));
        return response_result(id, Json::boolean(true), rpc);
    }
    if (method == "listWatchpoints") {
        Json result = Json::array();
        for (auto wp : inspector_->listWatchpoints()) result.a.push_back(Json::number(wp));
        return response_result(id, result, rpc);
    }
    if (method == "run" || method == "resume") {
        inspector_->run();
        return response_result(id, Json::boolean(true), rpc);
    }
    if (method == "pause") {
        inspector_->pause();
        return response_result(id, state_json(inspector_), rpc);
    }
    if (method == "step") {
        uint32_t count = 1;
        uint_param(params, "count", count);
        inspector_->step(count);
        return response_result(id, state_json(inspector_), rpc);
    }
    if (method == "runFor") {
        uint32_t count = 0;
        if (!uint_param(params, "instructionCount", count) && !uint_param(params, "instructions", count))
            return response_error(id, -32602, "missing instructionCount", rpc);
        inspector_->step(count);
        return response_result(id, state_json(inspector_), rpc);
    }
    if (method == "waitStopped") {
        uint32_t timeout = 1000, after = 0;
        uint_param(params, "timeoutMs", timeout);
        uint_param(params, "afterSequence", after);
        Inspector::StopState stop;
        if (!inspector_->waitStopped(stop, timeout, after))
            return response_error(id, -32001, "timeout", rpc);
        return response_result(id, state_json(inspector_), rpc);
    }
    if (method == "runUntil") {
        uint32_t timeout = 1000, max_instructions = 0, addr = 0;
        std::string target;
        std::optional<uint16_t> resolved;
        uint_param(params, "timeoutMs", timeout);
        uint_param(params, "maxInstructions", max_instructions);
        if (uint_param(params, "addr", addr)) resolved = static_cast<uint16_t>(addr);
        else if (string_param(params, "target", target)) resolved = inspector_->resolveAddressOrSymbol(target);
        if (!resolved) return response_error(id, -32602, "missing or unknown addr/target", rpc);
        if (inspector_->getRegisters().pc == *resolved) return response_result(id, state_json(inspector_), rpc);
        inspector_->setBreakpoint(*resolved);
        auto seq = inspector_->stopState().sequence;
        inspector_->run();
        Inspector::StopState stop;
        bool ok = inspector_->waitStopped(stop, timeout, seq);
        inspector_->clearBreakpoint(*resolved);
        if (!ok) return response_error(id, -32001, "timeout", rpc);
        if (max_instructions) (void)max_instructions;
        return response_result(id, state_json(inspector_), rpc);
    }
    if (method == "disassemble" || method == "trace") {
        uint32_t addr = inspector_->getRegisters().pc, count = method == "trace" ? 8 : 1;
        std::string target;
        uint_param(params, "addr", addr);
        uint_param(params, "count", count);
        if (string_param(params, "target", target)) {
            auto resolved = inspector_->resolveAddressOrSymbol(target);
            if (!resolved) return response_error(id, -32602, "unknown target", rpc);
            addr = *resolved;
        }
        Json result = Json::array();
        for (auto& inst : inspector_->disassemble(static_cast<uint16_t>(addr), count))
            result.a.push_back(instruction_json(inst));
        return response_result(id, result, rpc);
    }
    if (method == "symbols") {
        Json result = Json::array();
        for (const auto& [addr, name] : inspector_->listSymbols()) {
            Json item = Json::object();
            item.o["addr"] = Json::number(addr);
            item.o["name"] = Json::string(name);
            result.a.push_back(item);
        }
        return response_result(id, result, rpc);
    }
    if (method == "resolveSymbol") {
        std::string name;
        if (!string_param(params, "name", name)) return response_error(id, -32602, "missing name", rpc);
        auto addr = inspector_->resolveSymbol(name);
        if (!addr) return response_error(id, -32004, "symbol not found", rpc);
        Json result = Json::object();
        result.o["name"] = Json::string(name);
        result.o["addr"] = Json::number(*addr);
        return response_result(id, result, rpc);
    }
    if (method == "evaluate") {
        std::string expr;
        if (!string_param(params, "expr", expr)) return response_error(id, -32602, "missing expr", rpc);
        auto regs = inspector_->getRegisters();
        std::optional<uint16_t> value;
        if (expr == "pc") value = regs.pc;
        else if (expr == "sp") value = regs.sp;
        else if (expr.size() >= 2 && expr[0] == 'r') {
            char *end = nullptr;
            long idx = strtol(expr.c_str() + 1, &end, 0);
            if (end && *end == '\0' && idx >= 0 && idx < 16) value = static_cast<uint16_t>(regs.r[idx]);
        }
        if (!value) value = inspector_->resolveAddressOrSymbol(expr);
        if (!value) return response_error(id, -32005, "could not evaluate expression", rpc);
        Json result = Json::object();
        result.o["value"] = Json::number(*value);
        std::ostringstream hex;
        hex << "0x" << std::hex << std::setw(4) << std::setfill('0') << *value;
        result.o["hex"] = Json::string(hex.str());
        return response_result(id, result, rpc);
    }
    if (method == "snapshot") {
        return response_result(id, Json::string(base64_encode(inspector_->snapshot())), rpc);
    }
    if (method == "restore") {
        std::string b64;
        if (!string_param(params, "data", b64)) return response_error(id, -32602, "missing data", rpc);
        bool ok = inspector_->restore(base64_decode(b64));
        return ok ? response_result(id, Json::boolean(true), rpc) : response_error(id, -32000, "restore failed", rpc);
    }
    return response_error(id, -32601, "unknown method", rpc);
}

void JsonServer::run(const std::string& socket_path) {
    if (server_fd_ < 0) {
        running_ = false;
        return;
    }

    while (running_) {
        struct pollfd pfd;
        pfd.fd = server_fd_;
        pfd.events = POLLIN;
        int rv = poll(&pfd, 1, 500);
        if (rv < 0) {
            if (!running_) break;
            perror("poll");
            break;
        }
        if (rv == 0) continue;
        if (!(pfd.revents & POLLIN)) continue;

        int client = accept(server_fd_, nullptr, nullptr);
        if (client < 0) {
            if (!running_) break;
            perror("accept");
            break;
        }
        int flags = fcntl(client, F_GETFL, 0);
        if (flags >= 0) fcntl(client, F_SETFL, flags | O_NONBLOCK);

        std::string req;
        read_request(client, req);
        if (req.empty()) {
            close(client);
            continue;
        }

        std::vector<std::string> requests;
        if (req.find('\n') == std::string::npos) {
            requests.push_back(req);
        } else {
            std::istringstream lines(req);
            std::string line;
            while (std::getline(lines, line)) {
                if (!line.empty()) requests.push_back(line);
            }
        }

        bool keep_open = false;
        for (const auto& one : requests) {
            std::string resp = handleRequest(one, client, keep_open);
            if (!resp.empty()) send(client, resp.c_str(), resp.size(), MSG_NOSIGNAL);
            if (keep_open) break;
        }
        if (!keep_open) close(client);
    }

    unlink(socket_path.c_str());
}

void JsonServer::streamSubscription(int client, bool rpc, std::string id_json) {
    if (rpc) {
        std::string ack = std::string("{\"id\":") + id_json +
            ",\"jsonrpc\":\"2.0\",\"result\":\"subscribed\"}\n";
        send(client, ack.c_str(), ack.size(), MSG_NOSIGNAL);
    } else {
        std::string ack = "{\"result\":\"subscribed\"}\n";
        send(client, ack.c_str(), ack.size(), MSG_NOSIGNAL);
    }

    auto sub = inspector_->createEventSubscription();
    while (running_) {
        Inspector::Event ev;
        bool ok = inspector_->popEventBlocking(sub, ev, 5000);
        if (!ok) continue;
        std::string line;
        if (rpc) {
            line = std::string("{\"jsonrpc\":\"2.0\",\"method\":\"event\",\"params\":") + ev.payload + "}\n";
        } else {
            line = ev.payload + "\n";
        }
        ssize_t w = send(client, line.c_str(), line.size(), MSG_NOSIGNAL);
        if (w < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            perror("send");
            break;
        } else if (w == 0) {
            break;
        }
    }
    inspector_->closeEventSubscription(sub);
    close(client);
}

} // namespace mash16
