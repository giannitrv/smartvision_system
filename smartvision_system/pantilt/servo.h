#ifndef SERVO_H
#define SERVO_H

#include "PCA9685.h"
#include <cstdint>


class Servo {
public:
  Servo(PCA9685 &pca, uint8_t channel);
  ~Servo() = default;

  Servo(const Servo &) = delete;
  Servo &operator=(const Servo &) = delete;

  void setAngle(uint8_t angle);

private:
  PCA9685 &pca;
  uint8_t channel;
};

#endif // SERVO_H