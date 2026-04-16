#ifndef SERVO_H
#define SERVO_H

#include <cstdint>
#include "PCA9685.h"

class Servo {
    public:
        Servo(PCA9685* pca, uint8_t channel);
        ~Servo();
        void setAngle(uint8_t angle);
        uint16_t degreeTosPWM(uint8_t degree);
    private:
        PCA9685* pca;
        uint8_t channel;
        static const uint16_t MIN_PULSE;  // 1ms at 50Hz
        static const uint16_t MAX_PULSE;  // 2ms at 50Hz
};

#endif // SERVO_H