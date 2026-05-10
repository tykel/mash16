#ifndef JSON_SERVER_H
#define JSON_SERVER_H

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include "inspector.h"

namespace mash16 {

class JsonServer {
public:
    JsonServer();
    ~JsonServer();

    // Start server listening on unix domain socket path. Returns true on success.
    bool start(const std::string& socket_path, Inspector *inspector);
    void stop();

private:
    void run(const std::string& socket_path);
    void streamSubscription(int client);

    std::thread thread_;
    std::vector<std::thread> subscribe_threads_;
    std::atomic_bool running_;
    Inspector *inspector_;
    int server_fd_;
};

} // namespace mash16

#endif // JSON_SERVER_H
