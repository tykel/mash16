#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <iostream>
int use_verbose = 0;
#include "core/inspector.h"
#include "core/json_server.h"

using namespace mash16;

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

    // Connect as client (retry a few times until server is ready)
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
        std::string req = "{\"method\":\"getRegisters\"}";
        write(fd, req.c_str(), req.size());
        shutdown(fd, SHUT_WR);
        char buf[4096]; ssize_t n = read(fd, buf, sizeof(buf)-1);
        if(n>0) {
            buf[n]=0; std::cout << "response: " << buf << std::endl;
        }
        close(fd);
    } else {
        std::cerr << "client connect failed after retries: errno=" << errno << " (" << strerror(errno) << ")" << std::endl;
    }

    srv.stop();
    free(cpu.m);
    return 0;
}
