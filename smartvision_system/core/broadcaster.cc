#include "broadcaster.h"
#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

Broadcaster::Broadcaster() {
    int broadcast = 1;

    sprintf(message, "{\"device\":\"multivision_system\",\"rtsp\":\"8554\",\"cmd\":5005}");
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0) {
        std::cerr << "[Broadcaster] ERROR: Could not create socket.\n";
        return;
    }
    if (setsockopt(socketFd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        std::cerr << "[Broadcaster] ERROR: Could not set socket options.\n";
        return;
    }
    std::cout << "[Broadcaster] Socket created and set for broadcast.\n";
}

Broadcaster::~Broadcaster() {
    stop();
    close(socketFd);
    std::cout << "[Broadcaster] Broadcast stopped.\n";
}

void Broadcaster::run() {
    running = true;
    broadcastThread = std::thread(&Broadcaster::broadcastLoop, this);
}

void Broadcaster::stop() {
    running = false;
    if (broadcastThread.joinable()) {
        broadcastThread.join();
    }
}

void Broadcaster::broadcastLoop() {
    sockaddr_in broadcastAddr;
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(6000);
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    while (running) {
        sendto(socketFd, message, strlen(message), 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr));
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
