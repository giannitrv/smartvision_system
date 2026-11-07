#ifndef PAN_TILT_NODE_H
#define PAN_TILT_NODE_H

#include "can_bus.h"
#include "can_protocol.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

// Bring common types into scope for existing code
using can_protocol::NodeCommunicationState;
using can_protocol::PanTiltNodeState;
using can_protocol::sPanTiltCommand;
using can_protocol::sPanTiltResponse;

class PanTiltNode {
public:
  explicit PanTiltNode(std::shared_ptr<CanBus> bus = nullptr);
  ~PanTiltNode();

  PanTiltNode(const PanTiltNode &) = delete;
  PanTiltNode &operator=(const PanTiltNode &) = delete;

  int Open(uint16_t defaultPanAngle = 90, uint16_t defaultTiltAngle = 90);
  void Close();
  void UpdateSetpoint(PanTiltNodeState state, uint16_t panAngle, uint16_t tiltAngle);
  bool GetPosition(PanTiltNodeState &state, uint16_t &panAngle, uint16_t &tiltAngle);
  NodeCommunicationState GetCommunicationState() const { return comState.load(); }

private:
  std::shared_ptr<CanBus> canBus;
  bool ownsCanBus{false};
  std::atomic<bool> running{false};
  std::thread txThread;

  std::mutex setpointMutex;
  PanTiltNodeState targetState{PanTiltNodeState::IDLE};
  uint16_t targetPan{90};
  uint16_t targetTilt{90};

  std::mutex rxMutex;
  PanTiltNodeState currentState{PanTiltNodeState::IDLE};
  uint16_t actualPan{90};
  uint16_t actualTilt{90};
  bool hasNewResponse{false};

  uint8_t txSeqNumber{0};
  std::atomic<uint8_t> lastTxSeqNumber{0};
  uint64_t openTimestamp{0};
  uint64_t lastReceivedTimestamp{0};
  bool firstPacketReceived{false};
  std::atomic<NodeCommunicationState> comState{NodeCommunicationState::OK};

  void txLoop();
  void onFrameReceived(uint8_t msgId, const std::array<uint8_t, 8> &payload);
};

#endif // PAN_TILT_NODE_H
