#ifndef SMARTVISION_H
#define SMARTVISION_H

#include "PID.h"
#include "ai.h"
#include "can_bus.h"
#include "can_protocol.h"
#include "pan_tilt_node.h"
#include "pantilt.h"
#include "videorecorder.h"
#include <atomic>
#include <chrono>
#include <gst/gst.h>
#include <memory>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <vector>

struct VisionMetadata {
  float zoom;
  int pan;
  int tilt;
  bool autoZoom;
  bool osdEnabled;
  bool recording;
};

class SmartVision {
public:
  explicit SmartVision(const std::string &configPath = "config.json");
  ~SmartVision();

  SmartVision(const SmartVision &) = delete;
  SmartVision &operator=(const SmartVision &) = delete;

  int getWidth() const { return width; }
  int getHeight() const { return height; }
  int getFps() const { return fps; }
  void start(void);
  void stop(void);
  cv::Mat getLatestFrame(void);
  VisionMetadata getMetadata(void);
  void parseCommand(const std::vector<uint8_t> &command);

private:
  typedef struct {
    std::unique_ptr<PanTiltNode> node;
    PanTiltNodeState desiredState;
    PanTiltNodeState currentState;
    NodeCommunicationState comState;
  } sGimbalNode_t;
  typedef struct {
    std::string modelPath;
    std::string i2cDevice;
    int i2cAddr;
    float panKp;
    float panKi;
    float panKd;
    float tiltKp;
    float tiltKi;
    float tiltKd;
    std::vector<int> objectClass;
  } sParameters_t;
  static const uint8_t INFERENCE_RATE;
  void loadConfig(const std::string &configPath, sParameters_t *parameters);
  cv::Mat process(cv::Mat &frame);
  void zoomTracking(float currSize);
  void captureLoop(void);
  void updateFps(std::chrono::milliseconds currTime);
  cv::Mat applyZoom(cv::Mat frame);
  void trackingLoop(void);
  void saveSnapshot(void);
  std::unique_ptr<AI> ai;
  std::unique_ptr<PanTilt> panTilt;
  std::unique_ptr<PID> panPid;
  std::unique_ptr<PID> tiltPid;
  sGimbalNode_t gimbalNode;
  std::shared_ptr<CanBus> canBus;
  std::unique_ptr<VideoRecorder> recorder;
  cv::Mat latestFrame;
  std::mutex frameMutex;
  std::mutex trackingMutex;
  cv::VideoCapture cap;
  cv::Point targetCenter{-1, -1};
  int width;
  int height;
  int fps;
  float maxZoomFactor;
  float zoomFactor;
  int currentFps;
  int fpsCounter;
  std::chrono::milliseconds fpsStart;
  std::thread captureThread;
  std::thread trackingThread;
  std::atomic<bool> running{false};
  cv::Point textPos;
  cv::Scalar textColor;
  int textFont;
  int textThickness;
  int textSize;
  uint8_t cameraControl;
  int defaultPanAngle;
  int defaultTiltAngle;
  int maxPanAngle;
  int minPanAngle;
  int maxTiltAngle;
  int minTiltAngle;
  int panAngle;
  int tiltAngle;
  int actualPanAngle;
  int actualTiltAngle;
  float gimbalDeadzone;
  int targetX;
  int targetY;
  int targetId;
  bool targetTrackingEnabled;
  bool autoZoom;
  float refTargetSize;
  static constexpr float zoomAlpha = 0.05f;
  bool useRemoteControl;
  bool osdEnabled;
  float osdScale;
  int osdPosX;
  int osdPosY;
  int autoReturnTimeoutMs;
  std::chrono::milliseconds lastTrackTime;
  bool isManualGimbalMove;
};

#endif // SMARTVISION_H
