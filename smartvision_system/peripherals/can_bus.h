#ifndef CAN_BUS_H
#define CAN_BUS_H

#include "can_protocol.h"
#include <array>
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

class CanBus {
public:
  using FrameCallback =
      std::function<void(uint8_t messageId, const std::array<uint8_t, 8> &payload)>;

  CanBus();
  ~CanBus();

  CanBus(const CanBus &) = delete;
  CanBus &operator=(const CanBus &) = delete;

  int Open(const std::string &interfaceName = "can0");
  void Close();
  void Run();
  void Stop();
  bool IsRunning() const { return running.load(); }

  // Send a CAN frame with the specified 11-bit CAN ID (thread-safe)
  bool Send(uint16_t canId, const std::array<uint8_t, 8> &payload);

  // Register / Unregister callback for a specific device ID (0..31)
  void RegisterDeviceHandler(can_protocol::DeviceId devId, FrameCallback callback);
  void UnregisterDeviceHandler(can_protocol::DeviceId devId);

private:
  int socket_fd{-1};
  std::atomic<bool> running{false};
  std::thread rxThread;
  std::mutex txMutex;
  std::mutex handlersMutex;
  std::unordered_map<uint8_t, FrameCallback> deviceHandlers;

  void rxLoop();
};

#endif // CAN_BUS_H

