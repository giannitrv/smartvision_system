#ifndef PANTILT_H
#define PANTILT_H

#include "servo.h"
#include "PCA9685.h"

class PanTilt {
    public:
        PanTilt(const char* device, int address);
        ~PanTilt();
        void update(uint8_t panSetpoint, uint8_t tiltSetpoint);
        uint8_t getPanAngle() const;
        uint8_t getTiltAngle() const;
    private:
        PCA9685* pca;
        Servo* panServo;
        Servo* tiltServo;
        uint8_t currentPanAngle;
        uint8_t currentTiltAngle;
};

#endif // PANTILT_H