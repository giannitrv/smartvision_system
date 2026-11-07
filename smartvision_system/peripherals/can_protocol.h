#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace can_protocol {

// 5 MSB for Device ID (0x00 .. 0x1F)
enum class DeviceId : uint8_t {
  BROADCAST   = 0x00,
  PAN_TILT    = 0x01
  // Expandable up to 31 devices
};

// 11-bit standard CAN ID encoding:
// Bits [10..6] (5 bits) = Device ID (0..31)
// Bits [5..0]  (6 bits) = Message ID (0..63)
constexpr uint16_t MakeCanId(DeviceId dev, uint8_t msgId) {
  return ((static_cast<uint16_t>(dev) & 0x1F) << 6) | (static_cast<uint16_t>(msgId) & 0x3F);
}

constexpr DeviceId GetDeviceId(uint32_t canId) {
  return static_cast<DeviceId>((canId >> 6) & 0x1F);
}

constexpr uint8_t GetMessageId(uint32_t canId) {
  return static_cast<uint8_t>(canId & 0x3F);
}

// CRC8 calculation
namespace crc {
constexpr std::array<uint8_t, 256> GenerateCRC8Table(uint8_t poly = 0x07) {
  std::array<uint8_t, 256> table{};
  for (int i = 0; i < 256; ++i) {
    uint8_t curr = static_cast<uint8_t>(i);
    for (int bit = 0; bit < 8; ++bit) {
      curr = (curr & 0x80) ? static_cast<uint8_t>((curr << 1) ^ poly)
                           : static_cast<uint8_t>(curr << 1);
    }
    table[i] = curr;
  }
  return table;
}
inline constexpr auto CRC8_TABLE = GenerateCRC8Table(0x07);
inline uint8_t CalculateCRC8(const uint8_t *data, size_t length) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < length; ++i) {
    crc = CRC8_TABLE[crc ^ data[i]];
  }
  return crc;
}
} // namespace crc

enum class NodeCommunicationState : uint8_t {
  OK = 0x00,
  COMMUNICATION_TIMEOUT = 0x01,
  SEQUENCE_NUMBER_MISMATCH = 0x02,
  CRC_ERROR = 0x03,
  OFFLINE = 0x04
};

inline const char *NodeCommStateToString(NodeCommunicationState state) {
  switch (state) {
    case NodeCommunicationState::OK:                       return "OK";
    case NodeCommunicationState::COMMUNICATION_TIMEOUT:    return "TIMEOUT";
    case NodeCommunicationState::SEQUENCE_NUMBER_MISMATCH: return "SEQ_ERR";
    case NodeCommunicationState::CRC_ERROR:                return "CRC_ERR";
    case NodeCommunicationState::OFFLINE:                  return "OFFLINE";
    default:                                               return "UNKNOWN";
  }
}

// ============================================================================
// 1. PAN / TILT MESSAGES
// ============================================================================
enum class PanTiltMsgId : uint8_t {
  CMD_SETPOINT    = 0x01, // Host -> PanTilt
  STATUS_RESPONSE = 0x02  // PanTilt -> Host
};

enum class PanTiltNodeState : uint8_t {
  INIT        = 0x00,
  IDLE        = 0x01,
  MOVING      = 0x02,
  POSE_LOCKED = 0x03,
  FAULT       = 0x04
};

inline const char *PanTiltStateToString(PanTiltNodeState state) {
  switch (state) {
    case PanTiltNodeState::INIT:        return "INIT";
    case PanTiltNodeState::IDLE:        return "IDLE";
    case PanTiltNodeState::MOVING:      return "MOVING";
    case PanTiltNodeState::POSE_LOCKED: return "LOCKED";
    case PanTiltNodeState::FAULT:       return "FAULT";
    default:                            return "UNKNOWN";
  }
}

struct sPanTiltCommand {
  uint8_t sequenceNumber{0};
  PanTiltNodeState nodeState{PanTiltNodeState::IDLE};
  uint16_t panAngle{0};
  uint16_t tiltAngle{0};

  std::array<uint8_t, 8> toPayload() const {
    std::array<uint8_t, 8> payload{};
    payload[0] = sequenceNumber;
    payload[1] = static_cast<uint8_t>(nodeState);
    payload[2] = panAngle & 0x00FF;
    payload[3] = panAngle >> 8;
    payload[4] = tiltAngle & 0x00FF;
    payload[5] = tiltAngle >> 8;
    payload[6] = 0x00;
    payload[7] = crc::CalculateCRC8(payload.data(), 7);
    return payload;
  }
};

struct sPanTiltResponse {
  uint8_t sequenceNumber{0};
  PanTiltNodeState nodeState{PanTiltNodeState::IDLE};
  uint16_t panAngle{0};
  uint16_t tiltAngle{0};

  static std::optional<sPanTiltResponse> fromPayload(const std::array<uint8_t, 8> &payload) {
    uint8_t expectedCrc = crc::CalculateCRC8(payload.data(), 7);
    if (expectedCrc != payload[7]) {
      return std::nullopt;
    }
    sPanTiltResponse resp;
    resp.sequenceNumber = payload[0];
    resp.nodeState = static_cast<PanTiltNodeState>(payload[1]);
    resp.panAngle = (payload[3] << 8) | payload[2];
    resp.tiltAngle = (payload[5] << 8) | payload[4];
    return resp;
  }
};
} // namespace can_protocol

#endif // CAN_PROTOCOL_H

