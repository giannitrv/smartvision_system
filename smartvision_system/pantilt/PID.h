#ifndef PID_H
#define PID_H

class PID {
    public:
        PID(double Kp, double Ki, double Kd);
        ~PID();
        double calculate(double error, double dt);
    private:
        double Kp, Ki, Kd;
        double max_integral;
        double integral;
        double previous_error;
};
#endif // PID_H