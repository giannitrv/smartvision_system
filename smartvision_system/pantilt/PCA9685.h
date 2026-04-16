#ifndef PCA9685_H
#define PCA9685_H

#include <cstdint>
#include "I2C.h"

// PCA9685 Register Addresses
#define PCA9685_MODE1           0x00
#define PCA9685_MODE2           0x01
#define PCA9685_SUBADR1         0x02
#define PCA9685_SUBADR2         0x03
#define PCA9685_SUBADR3         0x04
#define PCA9685_ALLCALLADR      0x05
#define PCA9685_LEDX_ON_L       0x06
#define PCA9685_LEDX_ON_H       0x07
#define PCA9685_LEDX_OFF_L      0x08
#define PCA9685_LEDX_OFF_H      0x09
#define PCA9685_ALL_LED_ON_L    0xFA
#define PCA9685_ALL_LED_ON_H    0xFB
#define PCA9685_ALL_LED_OFF_L   0xFC
#define PCA9685_ALL_LED_OFF_H   0xFD
#define PCA9685_PRESCALER       0xFE

#define PCA9685_MODE1_SLEEP     0x10
#define PCA9685_MODE1_AUTO_INC  0x20
#define PCA9685_MODE1_ENABLE    0x01

#define PCA9685_NUM_CHANNELS    16

class PCA9685 {
    public:
        PCA9685(const char* device, int address = 0x40);
        ~PCA9685();
        void init();
        void setFrequency(float freq);
        void setPWM(uint8_t channel, uint16_t on, uint16_t off);
        void getFrequency(float& freq);
        void reset();
    private:
        I2C i2c;
        float frequency;
        void writeRegister(uint8_t reg, uint8_t value);
        uint8_t readRegister(uint8_t reg);
};

#endif // PCA9685_H