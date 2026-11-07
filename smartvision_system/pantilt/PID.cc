#include "PID.h"

PID::PID(double Kp, double Ki, double Kd)
    : Kp(Kp), Ki(Ki), Kd(Kd), max_integral(100.0), integral(0.0),
      previous_error(0.0) {}

double PID::calculate(double error, double dt) {
  if (dt <= 0.0) {
    return (Kp * error);
  }
  integral += error * dt;

  if (integral > max_integral) {
    integral = max_integral;
  } else if (integral < -max_integral) {
    integral = -max_integral;
  }

  double derivative = (error - previous_error) / dt;
  previous_error = error;

  return (Kp * error) + (Ki * integral) + (Kd * derivative);
}