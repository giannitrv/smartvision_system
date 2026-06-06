#include <gst/gst.h>
#include <atomic>
#include <mutex>
#include <string>
#include "opencv2/opencv.hpp"

class VideoRecorder {
public:
    VideoRecorder(void);
    ~VideoRecorder(void);
    void startRecording(int width, int height, int fps);
    void stopRecording(void);
    bool isRecording(void);
    void pushFrame(cv::Mat &frame, int fps);
private:
    bool recording;
    GstElement *recordPipeline{nullptr};
    GstElement *recordAppsrc{nullptr};
    std::mutex recordMutex;
};