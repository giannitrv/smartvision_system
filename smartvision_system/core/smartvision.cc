#include "smartvision.h"
#include "protocol.h"
#include "ai.h"
#include "image_utils.h"

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
    if (zoomFactor <= 1.01f) {
        return frame;
    }

    cv::Size originalSize = frame.size();
    int centerX = originalSize.width / 2;
    int centerY = originalSize.height / 2;
    int newWidth = originalSize.width / zoomFactor;
    int newHeight = originalSize.height / zoomFactor;
    int x = centerX - newWidth / 2;
    int y = centerY - newHeight / 2;

    image_buffer_t src_img;
    memset(&src_img, 0, sizeof(image_buffer_t));
    src_img.width = originalSize.width;
    src_img.height = originalSize.height;
    src_img.format = IMAGE_FORMAT_BGR888;
    src_img.virt_addr = frame.data;

    cv::Mat zoomedFrame(originalSize, CV_8UC3);
    image_buffer_t dst_img;
    memset(&dst_img, 0, sizeof(image_buffer_t));
    dst_img.width = originalSize.width;
    dst_img.height = originalSize.height;
    dst_img.format = IMAGE_FORMAT_BGR888;
    dst_img.virt_addr = zoomedFrame.data;

    image_rect_t src_box = {x, y, x + newWidth - 1, y + newHeight - 1};
    image_rect_t dst_box = {0, 0, originalSize.width - 1, originalSize.height - 1};

    int ret = convert_image(&src_img, &dst_img, &src_box, &dst_box, 0);
    if (ret != 0) {
        std::cerr << "[SmartVision] WARNING: RGA zoom failed, falling back to CPU.\n";
        cv::Rect roi(x, y, newWidth, newHeight);
        zoomedFrame = frame(roi);
        cv::resize(zoomedFrame, zoomedFrame, originalSize, 0, 0, cv::INTER_LINEAR);
    }

    return zoomedFrame;
}
