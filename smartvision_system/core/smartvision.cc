#include <ctime>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "smartvision.h"
#include "protocol.h"
#include "image_utils.h"

using json = nlohmann::json;

const uint8_t SmartVision::MIN_PAN_ANGLE = 30;
const uint8_t SmartVision::MAX_PAN_ANGLE = 150;
const uint8_t SmartVision::MIN_TILT_ANGLE = 30;
const uint8_t SmartVision::MAX_TILT_ANGLE = 150;

SmartVision::SmartVision(const std::string &configPath) {
    // --- Default parameters ---
    s_Parameters_t parameters;
    parameters.modelPath = "./models/yolov8n.rknn";
    parameters.i2cDevice = "/dev/i2c-8";
    parameters.i2cAddr = 0x40;
    parameters.panKp = 12.0f;
    parameters.panKi = 0.0f;
    parameters.panKd = 0.5f;
    parameters.tiltKp = 7.0f;
    parameters.tiltKi = 0.0f;
    parameters.tiltKd = 0.3f;
    width = 1280;
    height = 720;
    fps = 30;
    refTargetSize = -1.0f;
    maxZoomFactor = 5.0f;
    osdEnabled = true;
    osdScale = 1.0;
    osdPosX = 10;
    osdPosY = 30;
    textColor = cv::Scalar(0, 255, 0);
    // --- Load config from JSON ---
    loadConfig(configPath, &parameters);

    defaultPanAngle = panAngle;
    defaultTiltAngle = tiltAngle;
    zoomFactor = 1.0;
    fpsCounter = 0;
    textFont = cv::FONT_HERSHEY_SIMPLEX;
    textThickness = 1;
    textSize = 1;
    autoReturnTimeoutMs = 3000;
    isManualGimbalMove = false;
    lastTrackTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch());
    this->panAngle = defaultPanAngle;
    this->tiltAngle = defaultTiltAngle;
    // Open camera
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
    ai = new AI(parameters.modelPath.c_str());
    // Init pan tilt
    panTilt = new PanTilt(parameters.i2cDevice.c_str(), parameters.i2cAddr);
    panTilt->update(defaultPanAngle, defaultTiltAngle);
    // Init PID
    panPid = new PID(parameters.panKp, parameters.panKi, parameters.panKd);
    tiltPid = new PID(parameters.tiltKp, parameters.tiltKi, parameters.tiltKd);
    // Init video recorder
    recorder = new VideoRecorder();
    // Print camera info
    std::cout << "[SmartVision] Camera opened: "
              << cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
              << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << " @ "
              << cap.get(cv::CAP_PROP_FPS) << " fps\n";
}

SmartVision::~SmartVision() {
    panTilt->update(defaultPanAngle, defaultTiltAngle);
    recorder->stopRecording();
    stop();
    delete ai;
    delete panTilt;
    delete panPid;
    delete tiltPid;
    delete recorder;
    std::cout << "[SmartVision] SmartVision stopped.\n";
}

void SmartVision::start(void) {
    if (!cap.isOpened()) {
        cap.open(0, cv::CAP_V4L2);
        cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
        cap.set(cv::CAP_PROP_FRAME_WIDTH,  width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        cap.set(cv::CAP_PROP_FPS, fps);
        if (!cap.isOpened()) {
            std::cerr << "[SmartVision] ERROR: Could not re-open camera in start().\n";
            return;
        }
    }
    running = true;
    fpsStart = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch());
    captureThread = std::thread(&SmartVision::captureLoop, this);
    trackingThread = std::thread(&SmartVision::trackingLoop, this);
}

void SmartVision::stop(void) {
    running = false;
    if (trackingThread.joinable()) {
        trackingThread.join();
    }
    if (captureThread.joinable()) {
        captureThread.join();
    }
}

cv::Mat SmartVision::getLatestFrame(void) {
    std::lock_guard<std::mutex> lock(frameMutex);
    return latestFrame.clone();
}

VisionMetadata SmartVision::getMetadata(void) {
    std::lock_guard<std::mutex> lock(frameMutex);
    return {zoomFactor, panAngle, tiltAngle, autoZoom, osdEnabled, recorder->isRecording()};
}

void SmartVision::parseCommand(const std::vector<uint8_t> &command) {
    uint8_t reset;

    switch (command[0]) {
        case SET_ZOOM_CMD_ID:
            parseSetZoomCmd(command, &zoomFactor, maxZoomFactor);
            break;
        case SET_GIMBAL_CMD_ID:
            parseSetGimbalCmd(command, &panAngle, &tiltAngle, &reset);
            if (reset) {
                panAngle = defaultPanAngle;
                tiltAngle = defaultTiltAngle;
                isManualGimbalMove = false;
            } else {
                panAngle = std::clamp<uint8_t>(panAngle, MIN_PAN_ANGLE, MAX_PAN_ANGLE);
                tiltAngle = std::clamp<uint8_t>(tiltAngle, MIN_TILT_ANGLE, MAX_TILT_ANGLE);
                isManualGimbalMove = true;
            }
            break;
        case CAMERA_CONTROL_CMD_ID:
            parseCameraControlCmd(command, &cameraControl, &osdEnabled);
            if (cameraControl == 1) {
                saveSnapshot();
            } else if (cameraControl == 2) {
                if (recorder->isRecording()) {
                    recorder->stopRecording();
                } else {
                    recorder->startRecording(width, height, fps);
                }
            } else {
                // Unknown camera control command
            }
            break;
        case TARGET_TRACKING_CMD_ID:
            parseTargetTrackingCmd(command, &targetX, &targetY);
            targetX = targetX * width / 255;
            targetY = targetY * height / 255;
            targetId = ai->getTargetIdAt(targetX, targetY);
            targetTrackingEnabled = (targetId != -1);
            if (targetTrackingEnabled) {
                isManualGimbalMove = false;
            }
            refTargetSize = -1.0f; // will be captured on the first valid tracking frame
            lastTrackTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch());
            break;
        case TRACKING_MODE_CMD_ID:
            parseTrackingModeCmd(command, &autoZoom);
            break;
        default:
            break;
    }
}

void SmartVision::loadConfig(const std::string &configPath, s_Parameters_t *parameters) {
    std::ifstream f(configPath);
    if (f.is_open()) {
        try {
            json j = json::parse(f);
            if (j.contains("width"))             width          = j["width"];
            if (j.contains("height"))            height         = j["height"];
            if (j.contains("fps"))               fps            = j["fps"];
            if (j.contains("max_zoom_factor"))   maxZoomFactor  = j["max_zoom_factor"];
            if (j.contains("model_path"))        parameters->modelPath     = j["model_path"].get<std::string>();
            if (j.contains("i2c_device"))        parameters->i2cDevice     = j["i2c_device"].get<std::string>();
            if (j.contains("i2c_addr"))          parameters->i2cAddr       = j["i2c_addr"];
            if (j.contains("pan_angle"))         panAngle       = j["pan_angle"];
            if (j.contains("tilt_angle"))        tiltAngle      = j["tilt_angle"];
            if (j.contains("pan_kp"))            parameters->panKp         = j["pan_kp"];
            if (j.contains("pan_ki"))            parameters->panKi         = j["pan_ki"];
            if (j.contains("pan_kd"))            parameters->panKd         = j["pan_kd"];
            if (j.contains("tilt_kp"))           parameters->tiltKp        = j["tilt_kp"];
            if (j.contains("tilt_ki"))           parameters->tiltKi        = j["tilt_ki"];
            if (j.contains("tilt_kd"))           parameters->tiltKd        = j["tilt_kd"];
            if (j.contains("auto_return_timeout_ms")) autoReturnTimeoutMs = j["auto_return_timeout_ms"];
            if (j.contains("osd_enabled"))       osdEnabled     = j["osd_enabled"];
            if (j.contains("osd_scale"))         osdScale       = j["osd_scale"];
            if (j.contains("osd_thickness"))     textThickness  = j["osd_thickness"];
            if (j.contains("osd_pos_x"))         osdPosX        = j["osd_pos_x"];
            if (j.contains("osd_pos_y"))         osdPosY        = j["osd_pos_y"];
            if (j.contains("osd_color") && j["osd_color"].is_array() && j["osd_color"].size() == 3) {
                textColor = cv::Scalar(j["osd_color"][0], j["osd_color"][1], j["osd_color"][2]);
            }
            std::cout << "[SmartVision] Loaded configuration from " << configPath << "\n";
        } catch (...) {
            std::cerr << "[SmartVision] Failed to parse " << configPath << ", using defaults\n";
        }
    } else {
        std::cerr << "[SmartVision] Could not open " << configPath << ", using defaults\n";
    }
}

cv::Mat SmartVision::process(cv::Mat &frame) {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char text[128];

    if (osdEnabled) {
        snprintf(text, sizeof(text), "Zoom: %.1f Pan: %d Tilt: %d FPS: %d Date: %02d/%02d/%d Time: %02d:%02d:%02d",
            zoomFactor,
            panAngle,
            tiltAngle,
            currentFps,
            ltm->tm_mday, ltm->tm_mon + 1, ltm->tm_year + 1900,
            ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
        cv::putText(frame, text, cv::Point(osdPosX, osdPosY), textFont, osdScale, textColor, textThickness);
    }
    return frame;
}

void SmartVision::captureLoop(void) {
    cv::Mat rawFrame;
    cv::Point currTargetCenter;
    int error_x = 0;
    int error_y = 0;
    double pid_x = 0;
    double pid_y = 0;
    int newPan = 0;
    int newTilt = 0;
    float trackedSize;
    std::chrono::milliseconds currTime;

    while (running) {
        currTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch());
        // Update FPS
        updateFps(currTime);
        // Read frame
        cap.read(rawFrame);
        if (rawFrame.empty()) {
            std::cerr << "[SmartVision] WARNING: Empty frame captured, skipping.\n";
            continue;
        }
        rawFrame = applyZoom(rawFrame);
        trackedSize = -1.0f;
        currTargetCenter = ai->process(rawFrame, targetTrackingEnabled ? targetId : -1,
                                   targetTrackingEnabled ? &trackedSize : nullptr);
        // Auto-zoom: lock to bbox size captured at selection time
        if ((targetTrackingEnabled) && (trackedSize > 0.0f) && (autoZoom)) {
            zoomTracking(trackedSize);
        }
        // Track tracking
        if ((targetTrackingEnabled) && (currTargetCenter.x != -1)) {
            // Target is tracked
            lastTrackTime = currTime;
            std::lock_guard<std::mutex> lock(trackingMutex);
            targetCenter = currTargetCenter;
            // targetTrackingEnabled already true; refresh center atomically
        } else if (isManualGimbalMove == false) {
            // Return to zero when target is lost for more than autoReturnTimeoutMs
            if ((autoReturnTimeoutMs > 0) && ((currTime - lastTrackTime).count() > autoReturnTimeoutMs)) {
                if (panAngle > defaultPanAngle) {
                    panAngle--;
                } else if (panAngle < defaultPanAngle) {
                    panAngle++;
                }
                if (tiltAngle > defaultTiltAngle) {
                    tiltAngle--;
                } else if (tiltAngle < defaultTiltAngle) {
                    tiltAngle++;
                }
            }
        } else {
            // No tracking and manual gimbal mode: do nothing
        }
        // Let the application process the frame (detection, drawing, etc.)
        cv::Mat processed = process(rawFrame);
        // Update last frame
        std::lock_guard<std::mutex> lock(frameMutex);
        latestFrame = processed.clone();
        // Push to video recorder
        if (recorder->isRecording()) {
            recorder->pushFrame(processed, currentFps);
        }
    }
    cap.release();
}

void SmartVision::trackingLoop(void) {
    int errX;
    int errY;
    double normX;
    double normY;
    double dtMs;
    double pidX;
    double pidY;
    int newPan;
    int newTilt;
    cv::Point center;
    bool tracking;

    while (running) {
        // Snapshot shared state under lock
        {
            std::lock_guard<std::mutex> lock(trackingMutex);
            center   = targetCenter;
            tracking = targetTrackingEnabled;
        }
        if (tracking && center.x != -1) {
            errX = center.x - (width / 2);
            errY = center.y - (height / 2);
            // Normalize error to [-1, 1] and compensate for zoom:
            // at zoom Z the FOV is 1/Z narrower, so the same pixel offset
            // corresponds to a proportionally smaller angular movement.
            normX = (double)errX / (width  * zoomFactor);
            normY = (double)errY / (height * zoomFactor);
            // Dead zone
            if (std::abs(normX) < 0.05) {
                normX = 0.0;
            }
            if (std::abs(normY) < 0.05) {
                normY = 0.0;
            }
            pidX = panPid->calculate(normX, 10.0);
            pidY = tiltPid->calculate(normY, 10.0);
            newPan  = panAngle - (int)pidX;
            newTilt = tiltAngle - (int)pidY;
            panAngle  = std::clamp<int>(newPan,  MIN_PAN_ANGLE,  MAX_PAN_ANGLE);
            tiltAngle = std::clamp<int>(newTilt, MIN_TILT_ANGLE, MAX_TILT_ANGLE);
        }
        // Drive servo at 100 Hz regardless (holds last position when not tracking)
        panTilt->update(panAngle, tiltAngle);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SmartVision::updateFps(std::chrono::milliseconds currTime) {
    fpsCounter++;
    if ((currTime - fpsStart) > std::chrono::seconds(2)) {
        currentFps = (fpsCounter * 1000) / (currTime - fpsStart).count();
        fpsStart = currTime;
        fpsCounter = 0;
    }
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

void SmartVision::zoomTracking(float currSize) {
    float targetZoom;

    if (refTargetSize < 0.0f) {
        // First valid frame after target selection: capture reference size
        refTargetSize = currSize;
        return;
    }
    if (currSize <= 0.0f)
        return;

    targetZoom = zoomFactor * (refTargetSize / currSize);
    targetZoom = std::clamp(targetZoom, 1.0f, maxZoomFactor);
    zoomFactor += zoomAlpha * (targetZoom - zoomFactor);
}

void SmartVision::saveSnapshot(void) {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char buffer[64];

    sprintf(buffer, "./snapshots/%d_%02d_%02d-%02d:%02d:%02d.jpg",
        ltm->tm_year + 1900,
        ltm->tm_mon + 1,
        ltm->tm_mday,
        ltm->tm_hour,
        ltm->tm_min,
        ltm->tm_sec
    );
    cv::imwrite(buffer, latestFrame);
}
