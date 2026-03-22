#ifndef COMMAND_SERVER_H
#define COMMAND_SERVER_H

#include <atomic>
#include <thread>
#include <vector>
#include "smartvision.h"

class CommandServer {
public:
    CommandServer(SmartVision& vision);
    ~CommandServer();
    void run();
    void stop();
private:
    void serverLoop();
    SmartVision& smartvision;
    std::thread serverThread;
    std::atomic<bool> running{false};
    int socketFd;
};

#endif // COMMAND_SERVER_H
