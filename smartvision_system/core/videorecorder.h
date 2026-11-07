#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H

#include "opencv2/opencv.hpp"
#include <atomic>
#include <gst/gst.h>
#include <mutex>
#include <string>

class VideoRecorder {
public:
  VideoRecorder(void);
  ~VideoRecorder(void);

  VideoRecorder(const VideoRecorder &) = delete;
  VideoRecorder &operator=(const VideoRecorder &) = delete;

  void startRecording(int width, int height, int fps);
  void stopRecording(void);
  bool isRecording(void);
  void pushFrame(cv::Mat &frame, int fps);

private:
  std::atomic<bool> recording{false};
  GstElement *recordPipeline{nullptr};
  GstElement *recordAppsrc{nullptr};
  std::mutex recordMutex;
};

#endif // VIDEORECORDER_H
