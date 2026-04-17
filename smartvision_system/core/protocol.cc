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

void parseSetGimbalCmd(const std::vector<uint8_t> &command, uint8_t *panAngle, uint8_t *tiltAngle) {
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
}

std::vector<uint8_t> createSetZoomAck(float zoomFactor) {
    std::vector<uint8_t> response;
    uint8_t integerPart = (uint8_t)zoomFactor;
    uint8_t decimalPart = (uint8_t)((zoomFactor - integerPart) * 10);

    response.push_back(SET_COOM_CMD_ID);
    response.push_back(integerPart);
    response.push_back(decimalPart);
    response.push_back(0x00);
    response.push_back(0x00);
    return response;
}

std::vector<uint8_t> createSetGimbalAck(uint8_t panAngle, uint8_t tiltAngle) {
    std::vector<uint8_t> response;

    response.push_back(SET_GIMBAL_CMD_ID);
    response.push_back(panAngle);
    response.push_back(tiltAngle);
    response.push_back(0x00);
    response.push_back(0x00);
    return response;
}

std::vector<uint8_t> createUnknownCmdAck(const std::vector<uint8_t> &command) {
    std::vector<uint8_t> response;

    response.push_back(command[0]);
    response.push_back(0x00);
    response.push_back(0x00);
    response.push_back(0x00);
    response.push_back(0x00);

    return response;
}
