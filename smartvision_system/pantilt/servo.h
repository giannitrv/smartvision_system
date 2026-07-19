#ifndef SERVO_H
#define SERVO_H

#include <cstdint>
#include "PCA9685.h"

class Servo {
    public:
        Servo(PCA9685* pca, uint8_t channel);
        ~Servo();
        void setAngle(uint8_t angle);
    private:
        PCA9685* pca;
        uint8_t channel;
};

#endif // SERVO_H