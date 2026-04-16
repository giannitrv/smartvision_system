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
    panPID = new PID(1.0, 0.0, 0.0);
    tiltPID = new PID(1.0, 0.0, 0.0);
    std::cout << "[PanTilt] PanTilt initialized" << std::endl;
}

PanTilt::~PanTilt() {
    delete panServo;
    delete tiltServo;
    delete panPID;
    delete tiltPID;
    delete pca;
}

void PanTilt::update(double panSetpoint, double tiltSetpoint, double panMeasured, double tiltMeasured, double dt) {
    //double panOutput = panPID->calculate(panSetpoint, panMeasured, dt);
    //double tiltOutput = tiltPID->calculate(tiltSetpoint, tiltMeasured, dt);

    // Convert PID output to servo angle (0-180)
    uint8_t panAngle = static_cast<uint8_t>(std::max(0.0, std::min(180.0, panSetpoint)));
    uint8_t tiltAngle = static_cast<uint8_t>(std::max(0.0, std::min(180.0, tiltSetpoint)));

    panServo->setAngle(panAngle);
    tiltServo->setAngle(tiltAngle);
    currentPanAngle = panAngle;
    currentTiltAngle = tiltAngle;
}

double PanTilt::getPanAngle() const {
    // Placeholder for getting current pan angle
    return currentPanAngle;
}

double PanTilt::getTiltAngle() const {
    // Placeholder for getting current tilt angle
    return currentTiltAngle;
}