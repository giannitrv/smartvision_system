#include "protocol.h"

void parseSetZoomCmd(const std::vector<uint8_t> &command, float *zoomFactor) {
    float desiredZoom;

    desiredZoom = command[1] + command[2] / 10.0f;
    if (desiredZoom < 1.0f) {
        desiredZoom = 1.0f;
    } else if (desiredZoom > 5.0f) {
        desiredZoom = 5.0f;
    }
    *zoomFactor = desiredZoom;
}

void parseSetGimbalCmd(const std::vector<uint8_t> &command, uint8_t *panAngle, uint8_t *tiltAngle, uint8_t *reset) {
    if (command[1] == 43) {
        *panAngle += 5;
    } else if (command[1] == 45) {
        *panAngle -= 5;
    }
    if (command[2] == 43) {
        *tiltAngle += 5;
    } else if (command[2] == 45) {
        *tiltAngle -= 5;
    }
    *reset = command[3];
}

void parseTargetTrackingCmd(const std::vector<uint8_t> &command, int *targetX, int *targetY) {
    *targetX = command[1];
    *targetY = command[2];
}

void parseTrackingModeCmd(const std::vector<uint8_t> &command, bool *autoZoom) {
    *autoZoom = (command[1] > 0);
}
