#include "broadcaster.h"
#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <string>
#include <utility>

namespace {
// Returns a pair of (interface_ip, broadcast_ip)
std::pair<std::string, std::string> getEthernetIP() {
    struct ifaddrs *ifAddrStruct = nullptr;
    struct ifaddrs *ifa = nullptr;
    void *tmpAddrPtr = nullptr;
    std::string ipAddress = "127.0.0.1";
    std::string broadcastAddress = "255.255.255.255"; // Default limited broadcast

    if (getifaddrs(&ifAddrStruct) == -1) {
        std::cerr << "[Broadcaster] ERROR: getifaddrs failed.\n";
        return {ipAddress, broadcastAddress};
    }

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        // Check for IPv4
        if (ifa->ifa_addr->sa_family == AF_INET) {
            // Check that interface is UP and not LOOPBACK
            if ((ifa->ifa_flags & IFF_UP) && !(ifa->ifa_flags & IFF_LOOPBACK)) {
                std::string ifName(ifa->ifa_name);
                // Match common ethernet prefixes: eth*, en*, end*
                if (ifName.rfind("eth", 0) == 0 || 
                    ifName.rfind("en", 0) == 0 || 
                    ifName.rfind("end", 0) == 0) {
                    
                    tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
                    char addressBuffer[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
                    ipAddress = addressBuffer;
                    
                    // Retrieve the subnet-directed broadcast address if available
                    if (ifa->ifa_flags & IFF_BROADCAST) {
                        void *broadAddrPtr = &((struct sockaddr_in *)ifa->ifa_broadaddr)->sin_addr;
                        char broadBuffer[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, broadAddrPtr, broadBuffer, INET_ADDRSTRLEN);
                        broadcastAddress = broadBuffer;
                    }
                    
                    // If we found a valid non-empty address, stop searching
                    if (ipAddress != "0.0.0.0" && !ipAddress.empty()) {
                        break;
                    }
                }
            }
        }
    }
    
    if (ifAddrStruct != nullptr) {
        freeifaddrs(ifAddrStruct);
    }
    return {ipAddress, broadcastAddress};
}
}

Broadcaster::Broadcaster() {
    int broadcast = 1;

    std::pair<std::string, std::string> netInfo = getEthernetIP();
    std::string ip = netInfo.first;
    broadcastIp = netInfo.second;

    std::cout << "[Broadcaster] Detected Ethernet IP: " << ip 
              << " | Directed Broadcast IP: " << broadcastIp << "\n";

    snprintf(message, sizeof(message), 
             "{\"device\":\"multivision_system\",\"ip\":\"%s\",\"rtsp\":\"8554\",\"cmd\":5005}", 
             ip.c_str());

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
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(6000);

    if (inet_pton(AF_INET, broadcastIp.c_str(), &broadcastAddr.sin_addr) <= 0) {
        std::cerr << "[Broadcaster] ERROR: Invalid broadcast IP " << broadcastIp << ", falling back to limited broadcast.\n";
        broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;
    }

    while (running) {
        sendto(socketFd, message, strlen(message), 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr));
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
