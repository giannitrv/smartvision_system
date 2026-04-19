#ifndef SMARTVISION_H
#define SMARTVISION_H

#include <atomic>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include "ai.h"
#include "pantilt.h"
#include "PID.h"

class SmartVision {
public:
    SmartVision(int width, int height, int fps, const char *modelPath, uint8_t panAngle, uint8_t tiltAngle);
    ~SmartVision();
    void start(void);
    void stop(void);
    cv::Mat getLatestFrame(void);
    std::vector<uint8_t> parseCommand(const std::vector<uint8_t> &command);

private:
    static const uint8_t MIN_PAN_ANGLE;
    static const uint8_t MAX_PAN_ANGLE;
    static const uint8_t MIN_TILT_ANGLE;
    static const uint8_t MAX_TILT_ANGLE;
    cv::Mat process(cv::Mat &frame);
    void captureLoop(void);
    cv::Mat applyZoom(cv::Mat frame);
    void trackTarget(cv::Point targetCenter);
    AI *ai;
    PanTilt *panTilt;
    PID *panPid;
    PID *tiltPid;
    cv::Mat latestFrame;
    std::mutex frameMutex;
    cv::VideoCapture cap;
    int width;
    int height;
    int fps;
    float zoomFactor;
    int currentFps;
    int fpsCounter;
    std::chrono::milliseconds fpsStart;
    std::thread captureThread;
    std::atomic<bool> running{false};
    cv::Point textPos;
    cv::Scalar textColor;
    int textFont;
    int textThickness;
    int textSize;
    uint8_t defaultPanAngle;
    uint8_t defaultTiltAngle;
    uint8_t panAngle;
    uint8_t tiltAngle;
    int targetId;
    bool targetTrackingEnabled;
};

#endif // SMARTVISION_H
