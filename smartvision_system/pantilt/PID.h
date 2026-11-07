#ifndef PID_H
#define PID_H

class PID {
public:
  PID(double Kp, double Ki, double Kd);
  ~PID() = default;
  double calculate(double error, double dt);

private:
  double Kp{0.0};
  double Ki{0.0};
  double Kd{0.0};
  double max_integral{100.0};
  double integral{0.0};
  double previous_error{0.0};
};
#endif // PID_H