#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <vector>
#include <cstdint>

typedef enum {
    SET_COOM_CMD_ID = 0x01,
    SET_GIMBAL_CMD_ID = 0x02,
    CAMERA_CONTROL_CMD_ID = 0x03,
    SET_IMAGE_TYPE_CMD_ID = 0x04,
    TARGET_TRACKING_CMD_ID = 0x05,
} eCommands_t;

void parseSetZoomCmd(const std::vector<uint8_t> &command, float *zoomFactor);
void parseSetGimbalCmd(const std::vector<uint8_t> &command, uint8_t *panAngle, uint8_t *tiltAngle, uint8_t *reset);
void parseTargetTrackingCmd(const std::vector<uint8_t> &command, int *targetX, int *targetY);

#endif // PROTOCOL_H
