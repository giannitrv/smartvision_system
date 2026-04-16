#ifndef SMARTVISION_H
#define SMARTVISION_H

#include <atomic>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include "ai.h"
#include "pantilt.h"

class SmartVision {
public:
    SmartVision(int width, int height, int fps, const char *modelPath, uint8_t panAngle, uint8_t tiltAngle);
    ~SmartVision();
    void start();
    void stop();
    cv::Mat getLatestFrame();
    std::vector<uint8_t> parseCommand(const std::vector<uint8_t> &command);

private:
    cv::Mat process(cv::Mat &frame);
    void captureLoop();
    cv::Mat applyZoom(cv::Mat frame);
    AI *ai;
    PanTilt *panTilt;
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
};

#endif // SMARTVISION_H
