#ifndef PANTILT_H
#define PANTILT_H

#include "servo.h"
#include "PID.h"
#include "PCA9685.h"

class PanTilt {
    public:
        PanTilt(const char* device, int address);
        ~PanTilt();
        void update(double panSetpoint, double tiltSetpoint, double panMeasured, double tiltMeasured, double dt);
        double getPanAngle() const;
        double getTiltAngle() const;
    private:
        PCA9685* pca;
        Servo* panServo;
        Servo* tiltServo;
        PID* panPID;
        PID* tiltPID;
        double currentPanAngle;
        double currentTiltAngle;
};

#endif // PANTILT_H