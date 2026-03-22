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

std::vector<uint8_t> createUnknownCmdAck(const std::vector<uint8_t> &command) {
    std::vector<uint8_t> response;

    response.push_back(command[0]);
    response.push_back(0x00);
    response.push_back(0x00);
    response.push_back(0x00);
    response.push_back(0x00);

    return response;
}
