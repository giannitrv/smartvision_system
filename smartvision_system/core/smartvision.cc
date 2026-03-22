#include "smartvision.h"
#include "protocol.h"
#include "ai.h"

SmartVision::SmartVision(int width, int height, int fps, const char *modelPath)
    : width(width), height(height), fps(fps) {

    zoomFactor = 1.0;
    fpsCounter = 0;
    // Init text variables
    textPos = cv::Point(10, 30);
    textColor = cv::Scalar(0, 255, 0);
    textFont = cv::FONT_HERSHEY_SIMPLEX;
    textThickness = 1;
    textSize = 1;
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
    ai = new AI(modelPath);
    std::cout << "[SmartVision] Camera opened: "
              << cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
              << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << " @ "
              << cap.get(cv::CAP_PROP_FPS) << " fps\n";
}

SmartVision::~SmartVision() {
    stop();
    delete ai;
    std::cout << "[SmartVision] SmartVision stopped.\n";
}

void SmartVision::start() {
    running = true;
    fpsStart = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch());
    captureThread = std::thread(&SmartVision::captureLoop, this);
}

void SmartVision::stop() {
    running = false;
    if (captureThread.joinable()) {
        captureThread.join();
    }
}

cv::Mat SmartVision::getLatestFrame() {
    std::lock_guard<std::mutex> lock(frameMutex);
    return latestFrame.clone();
}

std::vector<uint8_t> SmartVision::parseCommand(const std::vector<uint8_t> &command) {
    std::vector<uint8_t> response;

    switch (command[0]) {
        case SET_COOM_CMD_ID:
            parseSetZoomCmd(command, &zoomFactor);
            response = createSetZoomAck(zoomFactor);
            break;
        default:
            response = createUnknownCmdAck(command);
        break;
    }

    return response;
}

cv::Mat SmartVision::process(cv::Mat &frame) {
    char text[100];
    sprintf(text, "Zoom : %.1f FPS: %d", zoomFactor, currentFps);
    cv::putText(frame, text, textPos, textFont, textSize, textColor, textThickness);
    return frame;
}

void SmartVision::captureLoop() {
    cv::Mat rawFrame;
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
        cap >> rawFrame;
        if (rawFrame.empty()) {
            std::cerr << "[SmartVision] WARNING: Empty frame captured, skipping.\n";
            continue;
        }
        rawFrame = applyZoom(rawFrame);
        ai->process(rawFrame);
        // Let the application process the frame (detection, drawing, etc.)
        cv::Mat processed = process(rawFrame);
        // Update last frame
        std::lock_guard<std::mutex> lock(frameMutex);
        latestFrame = processed.clone();
    }
    cap.release();
}

cv::Mat SmartVision::applyZoom(cv::Mat frame) {
    cv::Mat zoomedFrame;

    cv::Size originalSize = frame.size();
    int centerX = originalSize.width / 2;
    int centerY = originalSize.height / 2;
    int newWidth = originalSize.width / zoomFactor;
    int newHeight = originalSize.height / zoomFactor;
    int x = centerX - newWidth / 2;
    int y = centerY - newHeight / 2;
    cv::Rect roi(x, y, newWidth, newHeight);
    zoomedFrame = frame(roi);
    cv::resize(zoomedFrame, zoomedFrame, originalSize, 0, 0, cv::INTER_LINEAR);

    return zoomedFrame;
}
