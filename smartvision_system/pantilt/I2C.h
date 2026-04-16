
#ifndef I2C_H
#define I2C_H

#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

class I2C {
    public:
        I2C(const char* device, int address);
        ~I2C();
        int read(uint8_t reg, uint8_t buffer[], size_t length);
        int write(uint8_t reg, uint8_t buffer[], size_t length);
    private:
        int address;
        int file;
};

#endif // I2C_H
