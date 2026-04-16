#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
#include "PCA9685.h"

PCA9685::PCA9685(const char* device, int address) : i2c(device, address), frequency(200.0f) {
    // Constructor
}

PCA9685::~PCA9685() {
    // Destructor
}

void PCA9685::reset() {
    // Software reset
    writeRegister(PCA9685_MODE1, 0x80);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void PCA9685::init() {
    reset();
    
    // Set MODE1 register
    uint8_t mode1 = readRegister(PCA9685_MODE1);
    mode1 = (mode1 & ~PCA9685_MODE1_SLEEP) | PCA9685_MODE1_AUTO_INC;
    writeRegister(PCA9685_MODE1, mode1);
    
    std::cout << "PCA9685 initialized" << std::endl;
}

void PCA9685::setFrequency(float freq) {
    // Put device into SLEEP mode
    uint8_t mode1 = readRegister(PCA9685_MODE1);
    mode1 |= PCA9685_MODE1_SLEEP;
    writeRegister(PCA9685_MODE1, mode1);
    // Calculate prescaler value
    // prescaler = (25MHz / (4096 * frequency)) - 1
    uint8_t prescaler = (uint8_t)(25000000 / (4096 * freq)) - 1;
    writeRegister(PCA9685_PRESCALER, prescaler);
    // Wake up device
    mode1 &= ~PCA9685_MODE1_SLEEP;
    writeRegister(PCA9685_MODE1, mode1);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void PCA9685::setPWM(uint8_t channel, uint16_t on, uint16_t off) {
    if (channel >= PCA9685_NUM_CHANNELS) {
        std::cerr << "Invalid channel: " << (int)channel << std::endl;
        return;
    }
    
    // Clamp values to 0-4095
    on = on & 0x0FFF;
    off = off & 0x0FFF;
    
    // Calculate register addresses for this channel
    uint8_t on_l = PCA9685_LEDX_ON_L + (4 * channel);
    uint8_t on_h = PCA9685_LEDX_ON_H + (4 * channel);
    uint8_t off_l = PCA9685_LEDX_OFF_L + (4 * channel);
    uint8_t off_h = PCA9685_LEDX_OFF_H + (4 * channel);
    
    // Write ON and OFF values
    writeRegister(on_l, on & 0xFF);
    writeRegister(on_h, (on >> 8) & 0xFF);
    writeRegister(off_l, off & 0xFF);
    writeRegister(off_h, (off >> 8) & 0xFF);
}

void PCA9685::getFrequency(float& freq) {
    freq = frequency;
}

void PCA9685::writeRegister(uint8_t reg, uint8_t value) {
    uint8_t buffer = value;
    if (i2c.write(reg, &buffer, 1) < 0) {
        std::cerr << "Error writing to register 0x" << std::hex << (int)reg << std::dec << std::endl;
    }
}

uint8_t PCA9685::readRegister(uint8_t reg) {
    uint8_t buffer;
    if (i2c.read(reg, &buffer, 1) < 0) {
        std::cerr << "Error reading from register 0x" << std::hex << (int)reg << std::dec << std::endl;
        return 0;
    }
    return buffer;
}
