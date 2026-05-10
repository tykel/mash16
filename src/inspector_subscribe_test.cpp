#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <thread>
#include "core/inspector.h"
#include "core/json_server.h"

int use_verbose = 0;
using namespace mash16;

int main()
{
    cpu_state cpu;
    memset(&cpu, 0, sizeof(cpu));
    cpu.m = (uint8_t*)malloc(MEM_SIZE);
    for (size_t i=0;i<MEM_SIZE;++i) cpu.m[i] = (uint8_t)(i&0xff);

    Inspector insp(&cpu);
    JsonServer srv;
    const std::string path = "/tmp/mash16_subscribe.sock";
    if(!srv.start(path, &insp)) {
        std::cerr << "server failed to start" << std::endl;
        return 2;
    }

    // Connect as client (retry until server ready)
    int fd = -1;
    struct sockaddr_un addr; memset(&addr,0,sizeof(addr)); addr.sun_family = AF_UNIX; strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path)-1);
    int attempts = 0;
    while (attempts++ < 50) {
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) { perror("socket"); break; }
        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) break;
        close(fd);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (fd >= 0) {
        std::string req = "{\"method\":\"subscribe\"}";
        write(fd, req.c_str(), req.size());
        shutdown(fd, SHUT_WR);

        // set a recv timeout so test won't block forever if no events arrive
        struct timeval tv; tv.tv_sec = 2; tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        char buf[8192]; ssize_t n = read(fd, buf, sizeof(buf)-1);
        if(n<=0) {
            std::cerr << "subscribe ack failed: errno=" << errno << " (" << strerror(errno) << ")" << std::endl;
            close(fd);
            srv.stop();
            free(cpu.m);
            return 3;
        }
        buf[n]=0; std::cout << "subscribe response: " << buf << std::endl;

        int fd2 = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd2 < 0 || connect(fd2, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            std::cerr << "second client connect failed: errno=" << errno << " (" << strerror(errno) << ")" << std::endl;
            close(fd);
            if (fd2 >= 0) close(fd2);
            srv.stop();
            free(cpu.m);
            return 4;
        }
        std::string req2 = "{\"method\":\"getRegisters\"}";
        write(fd2, req2.c_str(), req2.size());
        shutdown(fd2, SHUT_WR);
        setsockopt(fd2, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        char buf2[4096]; ssize_t n2 = read(fd2, buf2, sizeof(buf2)-1);
        close(fd2);
        if (n2 <= 0) {
            std::cerr << "second client did not receive response" << std::endl;
            close(fd);
            srv.stop();
            free(cpu.m);
            return 5;
        }
        buf2[n2] = 0;
        std::string response2(buf2);
        if (response2.find("\"pc\"") == std::string::npos) {
            std::cerr << "unexpected second client response: " << response2 << std::endl;
            close(fd);
            srv.stop();
            free(cpu.m);
            return 6;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        Inspector::Event ev;
        ev.type = "test_event";
        ev.payload = "{\"type\":\"test_event\",\"msg\":\"ping\"}";
        insp.pushEvent(ev);
        n = read(fd, buf, sizeof(buf)-1);
        if (n <= 0) {
            std::cerr << "subscribe event failed: errno=" << errno << " (" << strerror(errno) << ")" << std::endl;
            close(fd);
            srv.stop();
            free(cpu.m);
            return 7;
        }
        buf[n] = 0;
        std::string event_response(buf);
        if (event_response.find("\"test_event\"") == std::string::npos) {
            std::cerr << "unexpected subscribe event: " << event_response << std::endl;
            close(fd);
            srv.stop();
            free(cpu.m);
            return 8;
        }
        close(fd);
    } else {
        std::cerr << "client connect failed after retries: errno=" << errno << " (" << strerror(errno) << ")" << std::endl;
        srv.stop();
        free(cpu.m);
        return 9;
    }

    srv.stop();
    free(cpu.m);
    return 0;
}
