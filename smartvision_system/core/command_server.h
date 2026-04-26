#ifndef COMMAND_SERVER_H
#define COMMAND_SERVER_H

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <arpa/inet.h>
#include "smartvision.h"

class CommandServer {
public:
    CommandServer(SmartVision& vision);
    ~CommandServer();
    void run();
    void stop();
private:
    static constexpr int TELEMETRY_INTERVAL_MS = 500;

    void serverLoop();
    void telemetryLoop();

    SmartVision& smartvision;
    std::thread serverThread;
    std::thread telemetryThread;
    std::atomic<bool> running{false};
    int socketFd;

    sockaddr_in clientAddr;
    bool clientAddrValid{false};
    std::mutex clientAddrMutex;
};

#endif // COMMAND_SERVER_H
