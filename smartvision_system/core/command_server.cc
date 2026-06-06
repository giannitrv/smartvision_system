#include "command_server.h"
#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

CommandServer::CommandServer(SmartVision& vision) : smartvision(vision), socketFd(-1) {
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0) {
        std::cerr << "[CommandServer] ERROR: Could not create socket.\n";
        return;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(5005);

    if (bind(socketFd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "[CommandServer] ERROR: Could not bind socket to port 5005.\n";
        close(socketFd);
        socketFd = -1;
        return;
    }

    // Set a timeout for recvfrom so the thread can stop gracefully
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if (setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "[CommandServer] ERROR: Could not set socket timeout.\n";
    }

    std::cout << "[CommandServer] Socket created and bound to port 5005.\n";
}

CommandServer::~CommandServer() {
    stop();
    if (socketFd >= 0) {
        close(socketFd);
    }
    std::cout << "[CommandServer] CommandServer stopped.\n";
}

void CommandServer::run() {
    if (socketFd < 0) return;
    running = true;
    serverThread   = std::thread(&CommandServer::serverLoop,   this);
    telemetryThread = std::thread(&CommandServer::telemetryLoop, this);
}

void CommandServer::stop() {
    running = false;
    if (serverThread.joinable()) {
        serverThread.join();
    }
    if (telemetryThread.joinable()) {
        telemetryThread.join();
    }
}

void CommandServer::serverLoop() {
    char buffer[5];
    sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);

    while (running) {
        int n = recvfrom(socketFd, buffer, sizeof(buffer),
                         0, (struct sockaddr *)&addr, &addrLen);
        if (n > 0) {
            // Store client address for telemetry
            {
                std::lock_guard<std::mutex> lock(clientAddrMutex);
                clientAddr      = addr;
                clientAddrValid = true;
            }
            // Parse and apply command (no ACK sent)
            std::vector<uint8_t> command(buffer, buffer + n);
            smartvision.parseCommand(command);
        }
    }
}

void CommandServer::telemetryLoop() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(TELEMETRY_INTERVAL_MS));

        sockaddr_in dest;
        bool valid;
        {
            std::lock_guard<std::mutex> lock(clientAddrMutex);
            dest  = clientAddr;
            valid = clientAddrValid;
        }

        if (!valid) continue;

        VisionMetadata meta = smartvision.getMetadata();

        json payload;
        payload["zoom"] = meta.zoom;
        payload["pan"]  = meta.pan;
        payload["tilt"] = meta.tilt;
        payload["autoZoom"] = meta.autoZoom;
        payload["recording"] = meta.recording;

        std::string msg = payload.dump();
        sendto(socketFd, msg.c_str(), msg.size(),
               0, (const struct sockaddr *)&dest, sizeof(dest));
    }
}
