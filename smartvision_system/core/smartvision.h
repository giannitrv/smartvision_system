#ifndef SMARTVISION_H
#define SMARTVISION_H

#include <atomic>
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include "ai.h"

class SmartVision {
public:
  // appsrc: the GStreamer element that will receive our frames
  // width, height, fps: must match what OpenCV captures and what the pipeline
  // expects
  SmartVision(int width, int height, int fps, const char *model_path);
  ~SmartVision();
  cv::Mat getLatestFrame();
  // Start/stop the background capture thread
  void start();
  void stop();

protected:
  // Override this in a subclass to apply any processing (detection, drawing,
  // etc.) Receives a BGR frame, must return a BGR frame of the same dimensions.
  virtual cv::Mat process(cv::Mat &frame);

private:
  void captureLoop();
  AI *ai;
  cv::Mat latest_frame;
  std::mutex frame_mutex;
  cv::VideoCapture cap;
  int width;
  int height;
  int fps;
  int currentFps;
  int fpsCounter;
  std::chrono::milliseconds fpsStart;
  std::thread capture_thread;
  std::atomic<bool> running{false};
  cv::Point text_pos;
  cv::Scalar text_color;
  int text_font;
  int text_thickness;
  int text_size;
};

#endif // SMARTVISION_H
