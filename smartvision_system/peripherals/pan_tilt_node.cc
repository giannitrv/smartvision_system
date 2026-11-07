#include "pan_tilt_node.h"
#include <chrono>
#include <iostream>

PanTiltNode::PanTiltNode(std::shared_ptr<CanBus> bus) : canBus(std::move(bus)) {
  if (!canBus) {
    canBus = std::make_shared<CanBus>();
    ownsCanBus = true;
  }
}

PanTiltNode::~PanTiltNode() {
  Close();
}

int PanTiltNode::Open(uint16_t defaultPanAngle, uint16_t defaultTiltAngle) {
  if (ownsCanBus) {
    if (canBus->Open() != 0) {
      return -1;
    }
    canBus->Run();
  }

  canBus->RegisterDeviceHandler(
      can_protocol::DeviceId::PAN_TILT,
      [this](uint8_t msgId, const std::array<uint8_t, 8> &payload) {
        this->onFrameReceived(msgId, payload);
      });

  openTimestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
  lastReceivedTimestamp = 0;
  firstPacketReceived = false;
  comState = NodeCommunicationState::OK;

  {
    std::lock_guard<std::mutex> lock(setpointMutex);
    targetState = PanTiltNodeState::IDLE;
    targetPan = defaultPanAngle;
    targetTilt = defaultTiltAngle;
  }
  {
    std::lock_guard<std::mutex> lock(rxMutex);
    currentState = PanTiltNodeState::IDLE;
    actualPan = defaultPanAngle;
    actualTilt = defaultTiltAngle;
    hasNewResponse = false;
  }

  running = true;
  txThread = std::thread(&PanTiltNode::txLoop, this);

  return 1;
}

void PanTiltNode::Close() {
  running = false;
  if (txThread.joinable()) {
    txThread.join();
  }
  if (canBus) {
    canBus->UnregisterDeviceHandler(can_protocol::DeviceId::PAN_TILT);
    if (ownsCanBus) {
      canBus->Stop();
      canBus->Close();
    }
  }
}

void PanTiltNode::UpdateSetpoint(PanTiltNodeState state, uint16_t panAngle, uint16_t tiltAngle) {
  std::lock_guard<std::mutex> lock(setpointMutex);
  targetState = state;
  targetPan = panAngle;
  targetTilt = tiltAngle;
}

bool PanTiltNode::GetPosition(PanTiltNodeState &state, uint16_t &panAngle, uint16_t &tiltAngle) {
  auto now = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());

  // Watchdog timeout check (500 ms)
  if (!firstPacketReceived) {
    if (now - openTimestamp > 2000) {
      comState = NodeCommunicationState::COMMUNICATION_TIMEOUT;
      std::cout << "PanTiltNode: First packet not received" << std::endl;
    }
  } else if (now - lastReceivedTimestamp > 2000) {
    comState = NodeCommunicationState::COMMUNICATION_TIMEOUT;
    std::cout << "PanTiltNode: Last packet not received" << std::endl;
  }

  std::lock_guard<std::mutex> lock(rxMutex);
  if (!hasNewResponse) {
    return false;
  }
  hasNewResponse = false;
  state = currentState;
  panAngle = actualPan;
  tiltAngle = actualTilt;

  return true;
}

void PanTiltNode::onFrameReceived(uint8_t msgId, const std::array<uint8_t, 8> &payload) {
  if (msgId != static_cast<uint8_t>(can_protocol::PanTiltMsgId::STATUS_RESPONSE)) {
    return;
  }

  auto now = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());

  auto ack = can_protocol::sPanTiltResponse::fromPayload(payload);
  if (!ack.has_value()) {
    comState = NodeCommunicationState::CRC_ERROR;
    return;
  }

  uint8_t lastTx = lastTxSeqNumber.load();
  uint8_t rxSeq = ack.value().sequenceNumber;
  uint8_t latency = static_cast<uint8_t>(lastTx - rxSeq);
  if (latency > 2) {
    comState = NodeCommunicationState::SEQUENCE_NUMBER_MISMATCH;
    return;
  }

  lastReceivedTimestamp = now;
  firstPacketReceived = true;
  comState = NodeCommunicationState::OK;

  std::lock_guard<std::mutex> lock(rxMutex);
  currentState = ack.value().nodeState;
  actualPan = ack.value().panAngle;
  actualTilt = ack.value().tiltAngle;
  hasNewResponse = true;
}

void PanTiltNode::txLoop() {
  const uint16_t cmdCanId = can_protocol::MakeCanId(
      can_protocol::DeviceId::PAN_TILT,
      static_cast<uint8_t>(can_protocol::PanTiltMsgId::CMD_SETPOINT));

  while (running.load()) {
    can_protocol::sPanTiltCommand cmd{};
    {
      std::lock_guard<std::mutex> lock(setpointMutex);
      cmd.nodeState = targetState;
      cmd.panAngle = targetPan;
      cmd.tiltAngle = targetTilt;
    }
    cmd.sequenceNumber = txSeqNumber;
    lastTxSeqNumber.store(txSeqNumber);
    txSeqNumber++;

    std::array<uint8_t, 8> payload = cmd.toPayload();
    canBus->Send(cmdCanId, payload);

    std::this_thread::sleep_for(std::chrono::milliseconds(25)); // 40 Hz
  }
}
