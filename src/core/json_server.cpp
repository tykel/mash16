#include "json_server.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <cctype>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

static const char *B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

namespace mash16 {

JsonServer::JsonServer() : running_(false), inspector_(nullptr), server_fd_(-1) {}

JsonServer::~JsonServer() {
    stop();
}

bool JsonServer::start(const std::string& socket_path, Inspector *inspector) {
    if (running_) return false;
    inspector_ = inspector;

    // create, bind and listen here so start() only returns once the socket is ready
    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        perror("socket");
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path)-1);
    unlink(socket_path.c_str()); // remove existing

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
    // shutdown listening socket to break accept
    if (server_fd_ != -1) {
        shutdown(server_fd_, SHUT_RDWR);
        close(server_fd_);
        server_fd_ = -1;
    }
    // Wake any inspector waiters (subscribe loops) so they can exit promptly
    if (inspector_) inspector_->wakeEventWaiters();

    if (thread_.joinable()) thread_.join();
    for (auto &thread : subscribe_threads_) {
        if (thread.joinable()) thread.join();
    }
    subscribe_threads_.clear();
}

static std::string json_escape(const std::string &s) {
    std::ostringstream o;
    for (auto c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\n': o << "\\n"; break;
            default: o << c;
        }
    }
    return o.str();
}

// Very small ad-hoc parser for "method":"..." and simple params extraction.
static bool extract_method(const std::string &req, std::string &method) {
    auto p = req.find("\"method\"");
    if (p == std::string::npos) return false;
    auto colon = req.find(':', p);
    if (colon == std::string::npos) return false;
    auto quote = req.find('"', colon);
    if (quote == std::string::npos) return false;
    auto quote2 = req.find('"', quote + 1);
    if (quote2 == std::string::npos) return false;
    method = req.substr(quote + 1, quote2 - quote - 1);
    return true;
}

static bool extract_uint_param(const std::string &req, const std::string &key, uint32_t &out) {
    auto p = req.find("\"" + key + "\"");
    if (p == std::string::npos) return false;
    auto colon = req.find(':', p);
    if (colon == std::string::npos) return false;
    // read number after colon
    size_t i = colon + 1;
    while (i < req.size() && isspace((unsigned char)req[i])) ++i;
    size_t j = i;
    while (j < req.size() && (isdigit((unsigned char)req[j]) || req[j]=='x' || (req[j]>='a' && req[j]<='f') || (req[j]>='A' && req[j]<='F') || req[j]=='X')) ++j;
    if (j==i) return false;
    std::string token = req.substr(i, j-i);
    // handle hex 0x...
    char *endptr = nullptr;
    unsigned long val = strtoul(token.c_str(), &endptr, 0);
    if (endptr == token.c_str()) return false;
    out = static_cast<uint32_t>(val);
    return true;
}

static bool extract_string_param(const std::string &req, const std::string &key, std::string &out) {
    auto p = req.find("\"" + key + "\"");
    if (p == std::string::npos) return false;
    auto colon = req.find(':', p);
    if (colon == std::string::npos) return false;
    auto quote = req.find('"', colon + 1);
    if (quote == std::string::npos) return false;

    out.clear();
    bool escaped = false;
    for (size_t i = quote + 1; i < req.size(); ++i) {
        char c = req[i];
        if (escaped) {
            switch (c) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default: out.push_back(c); break;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else if (c == '"') {
            return true;
        } else {
            out.push_back(c);
        }
    }
    return false;
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

void JsonServer::run(const std::string& socket_path) {
    // server_fd_ should already be created/bound/listening by start()
    if (server_fd_ < 0) {
        running_ = false;
        return;
    }

    while (running_) {
        // Wait with timeout so we can exit promptly when running_ becomes false
        struct pollfd pfd;
        pfd.fd = server_fd_;
        pfd.events = POLLIN;
        int rv = poll(&pfd, 1, 500); // 500ms
        if (rv < 0) {
            if (!running_) break;
            perror("poll");
            break;
        }
        if (rv == 0) continue; // timeout, loop back and check running_
        if (!(pfd.revents & POLLIN)) continue;

        int client = accept(server_fd_, nullptr, nullptr);
        if (client < 0) {
            if (!running_) break;
            perror("accept");
            break;
        }
        // Set client socket non-blocking so slow clients can't block the server thread
        int flags = fcntl(client, F_GETFL, 0);
        if (flags >= 0) fcntl(client, F_SETFL, flags | O_NONBLOCK);
        fprintf(stderr, "> accepted client fd=%d (non-blocking=%d)\n", client, flags>=0);

        std::string req;
        read_request(client, req);
        fprintf(stderr, "> request len=%zu\n", req.size());
        if (req.size() > 0) {
            std::string shown = req.substr(0, std::min<size_t>(req.size(), 512));
            fprintf(stderr, "> request: %s\n", shown.c_str());
        }

        std::string method;
        if (!extract_method(req, method)) {
            std::string resp = "{\"error\":\"missing method\"}\n";
            send(client, resp.c_str(), resp.size(), MSG_NOSIGNAL);
            close(client);
            continue;
        }

        std::string resp;
        if (method == "getRegisters") {
            auto regs = inspector_->getRegisters();
            std::ostringstream o;
            o << "{\"result\":{\"pc\":" << regs.pc << ",\"sp\":" << regs.sp << ",\"r\": [";
            for (int i=0;i<16;++i) { if (i) o<<","; o<<regs.r[i]; }
            o << "],\"flags\":{\"c\":" << (int)regs.f.c << ",\"z\":" << (int)regs.f.z << ",\"o\":" << (int)regs.f.o << ",\"n\":" << (int)regs.f.n << "}}}\n";
            resp = o.str();
        } else if (method == "readMemory") {
            uint32_t addr = 0, size = 0;
            if (!extract_uint_param(req, "addr", addr) || !extract_uint_param(req, "size", size)) {
                resp = "{\"error\":\"missing addr/size\"}\n";
            } else {
                auto data = inspector_->readMemory(static_cast<uint16_t>(addr), size);
                std::ostringstream o;
                o << "{\"result\":[";
                for (size_t i=0;i<data.size();++i) { if (i) o<<","; o<<(int)data[i]; }
                o << "]}\n";
                resp = o.str();
            }
        } else if (method == "writeMemory") {
            uint32_t addr = 0;
            if (!extract_uint_param(req, "addr", addr)) {
                resp = "{\"error\":\"missing addr\"}\n";
            } else {
                // naive: expect params.bytes = [n,n,...]
                auto p = req.find("\"bytes\"");
                if (p == std::string::npos) { resp = "{\"error\":\"missing bytes\"}\n"; }
                else {
                    auto b1 = req.find('[', p);
                    auto b2 = req.find(']', b1);
                    if (b1==std::string::npos || b2==std::string::npos) { resp = "{\"error\":\"bad bytes\"}\n"; }
                    else {
                        std::vector<uint8_t> bytes;
                        std::string list = req.substr(b1+1, b2-b1-1);
                        std::istringstream is(list);
                        std::string tok;
                        while (std::getline(is, tok, ',')) {
                            // trim
                            size_t s=0; while (s<tok.size() && isspace((unsigned char)tok[s])) ++s;
                            size_t e=tok.size(); while (e>0 && isspace((unsigned char)tok[e-1])) --e;
                            if (e<=s) continue;
                            int v = strtol(tok.substr(s,e-s).c_str(), nullptr, 0);
                            bytes.push_back((uint8_t)v);
                        }
                        bool ok = inspector_->writeMemory(static_cast<uint16_t>(addr), bytes);
                        resp = ok ? "{\"result\":true}\n" : "{\"error\":\"write failed\"}\n";
                    }
                }
            }
        } else if (method == "setBreakpoint") {
            uint32_t addr=0; if (!extract_uint_param(req, "addr", addr)) { resp = "{\"error\":\"missing addr\"}\n"; }
            else { inspector_->setBreakpoint(static_cast<uint16_t>(addr)); resp = "{\"result\":true}\n"; }
        } else if (method == "clearBreakpoint") {
            uint32_t addr=0; if (!extract_uint_param(req, "addr", addr)) { resp = "{\"error\":\"missing addr\"}\n"; }
            else { inspector_->clearBreakpoint(static_cast<uint16_t>(addr)); resp = "{\"result\":true}\n"; }
        } else if (method == "listBreakpoints") {
            auto bps = inspector_->listBreakpoints();
            std::ostringstream o; o<<"{\"result\":[";
            for (size_t i=0;i<bps.size();++i) { if (i) o<<","; o<<bps[i]; }
            o<<"]}\n"; resp = o.str();
        } else if (method == "subscribe") {
            subscribe_threads_.emplace_back(&JsonServer::streamSubscription, this, client);
            continue;
        } else if (method == "run") {
            inspector_->run();
            resp = "{\"result\":true}\n";
        } else if (method == "pause") {
            inspector_->pause();
            resp = "{\"result\":true}\n";
        } else if (method == "step") {
            inspector_->step();
            resp = "{\"result\":true}\n";
        } else if (method == "snapshot") {
            auto data = inspector_->snapshot();
            // base64-encode
            static const char *B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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
            std::ostringstream o;
            o << "{\"result\":\"" << json_escape(b64) << "\"}\n";
            resp = o.str();
        } else if (method == "restore") {
            // expect param: data: "base64..."
            std::string b64;
            if (!extract_string_param(req, "data", b64)) { resp = "{\"error\":\"missing data\"}\n"; }
            else {
                // decode base64
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
                bool ok = inspector_->restore(out);
                resp = ok ? "{\"result\":true}\n" : "{\"error\":\"restore failed\"}\n";
            }
        } else {
            resp = "{\"error\":\"unknown method\"}\n";
        }

        write(client, resp.c_str(), resp.size());
        close(client);
    }

    // unlink socket
    unlink(socket_path.c_str());
}

void JsonServer::streamSubscription(int client) {
    // Keep this connection open and stream events as newline-delimited JSON.
    std::string ack = "{\"result\":\"subscribed\"}\n";
    send(client, ack.c_str(), ack.size(), MSG_NOSIGNAL);
    inspector_->eventSubscriberAttached();
    while (running_) {
        Inspector::Event ev;
        bool ok = inspector_->popEventBlocking(ev, 5000);
        if (!ok) continue;
        std::string line = ev.payload + "\n";
        ssize_t w = send(client, line.c_str(), line.size(), MSG_NOSIGNAL);
        if (w < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            perror("send");
            break;
        } else if (w == 0) {
            break;
        }
    }
    inspector_->eventSubscriberDetached();
    close(client);
}

} // namespace mash16
