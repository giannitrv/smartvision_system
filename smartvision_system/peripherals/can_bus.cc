#include "can_bus.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

CanBus::CanBus() = default;

CanBus::~CanBus() {
  Stop();
  Close();
}

int CanBus::Open(const std::string &interfaceName) {
  socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (socket_fd < 0) {
    std::cerr << "[CAN BUS] Socket creation failed: " << std::strerror(errno) << '\n';
    return -1;
  }

  ifreq interface_request{};
  std::strncpy(interface_request.ifr_name, interfaceName.c_str(), IFNAMSIZ - 1);

  if (ioctl(socket_fd, SIOCGIFINDEX, &interface_request) < 0) {
    std::cerr << "[CAN BUS] Interface lookup (" << interfaceName
              << ") failed: " << std::strerror(errno) << '\n';
    close(socket_fd);
    socket_fd = -1;
    return -1;
  }

  sockaddr_can address{};
  address.can_family = AF_CAN;
  address.can_ifindex = interface_request.ifr_ifindex;

  if (bind(socket_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
    std::cerr << "[CAN BUS] Bind failed: " << std::strerror(errno) << '\n';
    close(socket_fd);
    socket_fd = -1;
    return -1;
  }

  std::cout << "[CAN BUS] Socket successfully bound to " << interfaceName << '\n';
  return 0;
}

void CanBus::Close() {
  if (socket_fd >= 0) {
    close(socket_fd);
    socket_fd = -1;
  }
}

void CanBus::Run() {
  if (running.load() || socket_fd < 0) {
    return;
  }
  running = true;
  rxThread = std::thread(&CanBus::rxLoop, this);
}

void CanBus::Stop() {
  running = false;
  if (rxThread.joinable()) {
    rxThread.join();
  }
}

bool CanBus::Send(uint16_t canId, const std::array<uint8_t, 8> &payload) {
  if (socket_fd < 0 || !running.load()) {
    return false;
  }
  can_frame frame{};
  frame.can_id = canId;
  frame.can_dlc = 8;
  std::copy(payload.begin(), payload.end(), frame.data);

  std::lock_guard<std::mutex> lock(txMutex);
  ssize_t written = write(socket_fd, &frame, sizeof(frame));
  if (written != sizeof(frame)) {
    std::cerr << "[CAN BUS] Write failed for CAN ID 0x" << std::hex << canId
              << std::dec << ": " << std::strerror(errno) << '\n';
    return false;
  }

  return true;
}

void CanBus::RegisterDeviceHandler(can_protocol::DeviceId devId, FrameCallback callback) {
  std::lock_guard<std::mutex> lock(handlersMutex);
  deviceHandlers[static_cast<uint8_t>(devId)] = std::move(callback);
}

void CanBus::UnregisterDeviceHandler(can_protocol::DeviceId devId) {
  std::lock_guard<std::mutex> lock(handlersMutex);
  deviceHandlers.erase(static_cast<uint8_t>(devId));
}

void CanBus::rxLoop() {
  pollfd descriptor{};
  descriptor.fd = socket_fd;
  descriptor.events = POLLIN;
  can_frame rx_frame{};

  while (running.load()) {
    int poll_result = poll(&descriptor, 1, 100); // 100ms timeout
    if (poll_result > 0) {
      if (descriptor.revents & POLLIN) {
        ssize_t received = read(socket_fd, &rx_frame, sizeof(rx_frame));
        if (received == sizeof(rx_frame)) {
          canid_t clean_id = rx_frame.can_id & CAN_SFF_MASK;
          can_protocol::DeviceId devId = can_protocol::GetDeviceId(clean_id);
          uint8_t msgId = can_protocol::GetMessageId(clean_id);

          std::array<uint8_t, 8> payload{};
          size_t copyLen = std::min(static_cast<size_t>(rx_frame.can_dlc), static_cast<size_t>(8));
          std::copy(rx_frame.data, rx_frame.data + copyLen, payload.begin());

          FrameCallback cb;
          {
            std::lock_guard<std::mutex> lock(handlersMutex);
            auto it = deviceHandlers.find(static_cast<uint8_t>(devId));
            if (it != deviceHandlers.end()) {
              cb = it->second;
            }
          }
          if (cb) {
            cb(msgId, payload);
          }
        } else if (received < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
          std::cerr << "[CAN BUS] Read error: " << std::strerror(errno) << '\n';
        }
      } else if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        std::cerr << "[CAN BUS] Poll error encountered\n";
        break;
      }
    }
  }
}

