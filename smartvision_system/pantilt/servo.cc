#include "servo.h"
#include <iostream>

// PCA9685 at 50Hz: 1ms = 205, 2ms = 410 (out of 4096)
const uint16_t Servo::MIN_PULSE = 205;   // 1ms pulse (0 degrees)
const uint16_t Servo::MAX_PULSE = 410;   // 2ms pulse (180 degrees)

Servo::Servo(PCA9685* pca, uint8_t channel) : pca(pca), channel(channel) {
    if (pca == nullptr) {
        std::cerr << "Error: PCA9685 pointer is null" << std::endl;
    }
}

Servo::~Servo() {
    // Destructor
}

// Convert servo angle (0-180 degrees) to PCA9685 PWM value
uint16_t Servo::degreeTosPWM(uint8_t degree) {
    // Clamp degree to 0-180 range
    if (degree > 180) {
        degree = 180;
    }
    
    // Linear interpolation: PWM = MIN + (degree/180) * (MAX - MIN)
    uint16_t pwm = MIN_PULSE + ((degree * (MAX_PULSE - MIN_PULSE)) / 180);
    return pwm;
}

void Servo::setAngle(uint8_t angle) {
    uint16_t pwm = degreeTosPWM(angle);
    pca->setPWM(channel, 0, pwm);
}
