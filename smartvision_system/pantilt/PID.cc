#include "PID.h"

PID::PID(double Kp, double Ki, double Kd) : Kp(Kp), Ki(Ki), Kd(Kd), integral(0), previous_error(0) {
    max_integral = 100;
}

PID::~PID() {
    // Destructor
}

double PID::calculate(double setpoint, double measured_value, double dt) {
    double error = setpoint - measured_value;
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