#include <algorithm>
#include <iostream>
#include "pantilt.h"

PanTilt::PanTilt(const char* device, int address) {
    pca = new PCA9685(device, address);
    pca->init();
    pca->setFrequency(47.0f);  // Set to 50Hz for servo control
    // Initialize servos and PID controllers
    panServo = new Servo(pca, 0);
    tiltServo = new Servo(pca, 1);
    std::cout << "[PanTilt] PanTilt initialized" << std::endl;
}

PanTilt::~PanTilt() {
    delete panServo;
    delete tiltServo;
    delete pca;
}

void PanTilt::update(uint8_t panSetpoint, uint8_t tiltSetpoint) {
    // Convert PID output to servo angle (0-180)
    uint8_t panAngle = std::clamp<uint8_t>(panSetpoint, 0, 180);
    uint8_t tiltAngle = std::clamp<uint8_t>(tiltSetpoint, 0, 180);

    panServo->setAngle(panAngle);
    tiltServo->setAngle(tiltAngle);
    currentPanAngle = panAngle;
    currentTiltAngle = tiltAngle;
}

uint8_t PanTilt::getPanAngle() const {
    // Placeholder for getting current pan angle
    return currentPanAngle;
}

uint8_t PanTilt::getTiltAngle() const {
    // Placeholder for getting current tilt angle
    return currentTiltAngle;
}