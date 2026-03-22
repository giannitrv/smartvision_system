#include "command_server.h"
#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

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
    serverThread = std::thread(&CommandServer::serverLoop, this);
}

void CommandServer::stop() {
    running = false;
    if (serverThread.joinable()) {
        serverThread.join();
    }
}

void CommandServer::serverLoop() {
    char buffer[5];
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    std::vector<uint8_t> response;

    while (running) {
        int n = recvfrom(socketFd, buffer, sizeof(buffer), 
                         0, (struct sockaddr *) &clientAddr, &clientLen);
        if (n > 0) {
            std::vector<uint8_t> command(buffer, buffer + n);
            response = smartvision.parseCommand(command);
            sendto(socketFd, response.data(), response.size(), 
                   0, (const struct sockaddr *) &clientAddr, sizeof(clientAddr));
        }
    }
}
