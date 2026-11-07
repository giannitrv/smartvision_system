
#ifndef I2C_H
#define I2C_H

#include <array>
#include <cstdint>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>

class I2C {
public:
  I2C(const char *device, int address);
  ~I2C();

  I2C(const I2C &) = delete;
  I2C &operator=(const I2C &) = delete;

  int read(uint8_t reg, uint8_t buffer[], size_t length);
  int write(uint8_t reg, uint8_t buffer[], size_t length);

private:
  int address;
  int file{-1};
  std::array<uint8_t, 32> txBuffer;
};

#endif // I2C_H
