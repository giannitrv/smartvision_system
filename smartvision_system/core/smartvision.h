#ifndef SMARTVISION_H
#define SMARTVISION_H

#include <atomic>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include "ai.h"
#include "pantilt.h"
#include "PID.h"

struct VisionMetadata {
    float zoom;
    uint8_t pan;
    uint8_t tilt;
    bool autoZoom;
};

class SmartVision {
public:
    explicit SmartVision(const std::string &configPath = "config.json");
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getFps() const { return fps; }
    ~SmartVision();
    void start(void);
    void stop(void);
    cv::Mat getLatestFrame(void);
    VisionMetadata getMetadata(void);
    void parseCommand(const std::vector<uint8_t> &command);

private:
    static const uint8_t MIN_PAN_ANGLE;
    static const uint8_t MAX_PAN_ANGLE;
    static const uint8_t MIN_TILT_ANGLE;
    static const uint8_t MAX_TILT_ANGLE;
    void loadConfig(const std::string &configPath, std::string *modelPath, float *panKp, float *panKi, float *panKd, float *tiltKp, float *tiltKi, float *tiltKd);
    cv::Mat process(cv::Mat &frame);
    void zoomTracking(float currSize);
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
    int targetX;
    int targetY;
    int targetId;
    bool targetTrackingEnabled;
    bool autoZoom;
    float refTargetSize;
    static constexpr float zoomAlpha = 0.05f;
};

#endif // SMARTVISION_H
