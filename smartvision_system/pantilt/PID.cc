#include "PID.h"

PID::PID(double Kp, double Ki, double Kd) {
    this->Kp = Kp;
    this->Ki = Ki;
    this->Kd = Kd;
    this->integral = 0;
    this->previous_error = 0;
    this->max_integral = 100;
}

PID::~PID() {
    // Destructor
}

double PID::calculate(double error, double dt) {
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