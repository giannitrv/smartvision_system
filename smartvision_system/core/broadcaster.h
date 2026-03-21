#ifndef BROADCASTER_H
#define BROADCASTER_H

#include <atomic>
#include <thread>
#include <sys/socket.h>

class Broadcaster {
public:
    Broadcaster();
    ~Broadcaster();
    void run();
    void stop();
private:
    void broadcastLoop();
    std::thread broadcastThread;
    std::atomic<bool> running{false};
    char message[100];
    int socketFd;
};

#endif // BROADCASTER_H