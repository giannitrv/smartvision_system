#include "smartvision.h"
#include "ai.h"
#include <cstring>

SmartVision::SmartVision(int width, int height, int fps, const char *model_path)
    : width(width), height(height), fps(fps) {

    fpsCounter = 0;
    // Init text variables
    text_pos = cv::Point(10, 30);
    text_color = cv::Scalar(0, 255, 0);
    text_font = cv::FONT_HERSHEY_SIMPLEX;
    text_thickness = 1;
    text_size = 1;
    // Open camera. CAP_V4L2 + MJPG fourcc = same path as the old v4l2src
    // pipeline, but OpenCV transparently decodes the JPEG before handing us a BGR
    // frame.
    cap.open(0, cv::CAP_V4L2);
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    cap.set(cv::CAP_PROP_FPS, fps);

    if (!cap.isOpened()) {
        std::cerr << "[SmartVision] ERROR: Could not open camera.\n";
        return;
    }
    // Init ai model
    ai = new AI(model_path);
    std::cout << "[SmartVision] Camera opened: "
              << cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
              << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << " @ "
              << cap.get(cv::CAP_PROP_FPS) << " fps\n";
}

SmartVision::~SmartVision() {
    stop();
    delete ai;
}

cv::Mat SmartVision::getLatestFrame() {
    std::lock_guard<std::mutex> lock(frame_mutex);
    return latest_frame.clone();
}

void SmartVision::start() {
    running = true;
    fpsStart = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch());
    capture_thread = std::thread(&SmartVision::captureLoop, this);
}

void SmartVision::stop() {
    running = false;
    if (capture_thread.joinable()) {
    capture_thread.join();
  }
}

cv::Mat SmartVision::process(cv::Mat &frame) {
    // Default: passthrough. Override in a subclass to apply detection, drawing,
    // etc.
    std::string text = "FPS: " + std::to_string(currentFps);
    cv::putText(frame, text, text_pos, text_font, text_size, text_color,
                text_thickness);
    return frame;
}

void SmartVision::captureLoop() {
    cv::Mat raw_frame;
    std::chrono::milliseconds currTime;

    while (running) {
        fpsCounter++;
        currTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch());
        if (currTime - fpsStart > std::chrono::seconds(2)) {
            currentFps = fpsCounter / 2;
            fpsStart = currTime;
            fpsCounter = 0;
        }
        cap >> raw_frame;
        if (raw_frame.empty()) {
            std::cerr << "[SmartVision] WARNING: Empty frame captured, skipping.\n";
            continue;
        }
        ai->process(raw_frame);
        // Let the application process the frame (detection, drawing, etc.)
        cv::Mat processed = process(raw_frame);
        // Update last frame
        std::lock_guard<std::mutex> lock(frame_mutex);
        latest_frame = processed.clone();
    }

    cap.release();
    std::cout << "[SmartVision] Capture thread stopped.\n";
}
