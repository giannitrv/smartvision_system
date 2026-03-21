#ifndef SMARTVISION_H
#define SMARTVISION_H

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>

class SmartVision {
public:
    // appsrc: the GStreamer element that will receive our frames
    // width, height, fps: must match what OpenCV captures and what the pipeline expects
    SmartVision(GstElement* appsrc, int width, int height, int fps);
    ~SmartVision();

    // Start/stop the background capture thread
    void start();
    void stop();

protected:
    // Override this in a subclass to apply any processing (detection, drawing, etc.)
    // Receives a BGR frame, must return a BGR frame of the same dimensions.
    virtual cv::Mat process(cv::Mat& frame);

private:
    void captureLoop();

    GstElement*         appsrc_;
    int                 width_;
    int                 height_;
    int                 fps_;
    std::thread         capture_thread_;
    std::atomic<bool>   running_{false};
    guint64             frame_count_{0};
};

#endif // SMARTVISION_H