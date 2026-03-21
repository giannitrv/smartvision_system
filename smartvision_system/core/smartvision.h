#ifndef SMARTVISION_H
#define SMARTVISION_H

#include <atomic>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include "ai.h"

class SmartVision {
public:
  SmartVision(int width, int height, int fps, const char *modelPath);
  ~SmartVision();
  cv::Mat getLatestFrame();
  void start();
  void stop();

protected:
  // Override this in a subclass to apply any processing (detection, drawing,
  // etc.) Receives a BGR frame, must return a BGR frame of the same dimensions.
  virtual cv::Mat process(cv::Mat &frame);

private:
  void captureLoop();
  cv::Mat applyZoom(cv::Mat frame);
  AI *ai;
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
};

#endif // SMARTVISION_H
