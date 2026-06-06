
#include "videorecorder.h"
#include <filesystem>
#include <ctime>

VideoRecorder::VideoRecorder(void) {
    gst_init(nullptr, nullptr);
    recording = false;
}

VideoRecorder::~VideoRecorder(void) {
    stopRecording();
}

void VideoRecorder::startRecording(int width, int height, int fps) {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char filename[256];

    std::lock_guard<std::mutex> lock(recordMutex);
    if (recording) {
        std::cout << "[SmartVision] Already recording.\n";
        return;
    }

    std::filesystem::create_directories("./recordings");

    sprintf(filename, "./recordings/rec_%d%02d%02d_%02d%02d%02d.mp4",
            ltm->tm_year + 1900,
            ltm->tm_mon + 1,
            ltm->tm_mday,
            ltm->tm_hour,
            ltm->tm_min,
            ltm->tm_sec);

    std::string pipelineStr = 
        "appsrc name=recordsrc is-live=true format=time do-timestamp=true ! "
        "videoconvert ! "
        "video/x-raw,format=NV12 ! "
        "mpph265enc rc-mode=vbr bps=4000000 gop=30 ! "
        "h265parse ! "
        "qtmux ! "
        "filesink location=" + std::string(filename);

    GError *error = nullptr;
    recordPipeline = gst_parse_launch(pipelineStr.c_str(), &error);
    if (!recordPipeline) {
        std::cerr << "[SmartVision] ERROR: Failed to create recording pipeline: " 
                  << (error ? error->message : "Unknown error") << "\n";
        if (error) g_error_free(error);
        return;
    }

    recordAppsrc = gst_bin_get_by_name(GST_BIN(recordPipeline), "recordsrc");
    if (!recordAppsrc) {
        std::cerr << "[SmartVision] ERROR: Failed to get appsrc from recording pipeline.\n";
        gst_object_unref(recordPipeline);
        recordPipeline = nullptr;
        return;
    }

    // Configure appsrc caps
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "BGR",
        "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height,
        "framerate", GST_TYPE_FRACTION, fps, 1,
        NULL);
    g_object_set(G_OBJECT(recordAppsrc), "caps", caps, NULL);
    gst_caps_unref(caps);

    // Set format and do-timestamp on appsrc explicitly
    g_object_set(G_OBJECT(recordAppsrc), "format", GST_FORMAT_TIME, "do-timestamp", TRUE, NULL);

    GstStateChangeReturn state_ret = gst_element_set_state(recordPipeline, GST_STATE_PLAYING);
    if (state_ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "[SmartVision] ERROR: Failed to set recording pipeline to PLAYING state.\n";
        gst_object_unref(recordAppsrc);
        gst_object_unref(recordPipeline);
        recordAppsrc = nullptr;
        recordPipeline = nullptr;
        return;
    }

    recording = true;
    std::cout << "[SmartVision] Started recording to: " << filename << "\n";
}

bool VideoRecorder::isRecording(void) {
    return recording;
}

void VideoRecorder::pushFrame(cv::Mat &frame, int fps) {
    std::lock_guard<std::mutex> lock(recordMutex);
    if (!recording || !recordAppsrc) {
        return;
    }

    gsize size = frame.total() * frame.elemSize();
    GstBuffer *buffer = gst_buffer_new_allocate(NULL, size, NULL);

    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    memcpy(map.data, frame.data, size);
    gst_buffer_unmap(buffer, &map);

    GST_BUFFER_DURATION(buffer) = GST_SECOND / static_cast<GstClockTime>(fps);

    GstFlowReturn ret;
    g_signal_emit_by_name(recordAppsrc, "push-buffer", buffer, &ret);

    gst_buffer_unref(buffer);
}

void VideoRecorder::stopRecording(void) {
    std::lock_guard<std::mutex> lock(recordMutex);
    if (!recording || !recordPipeline) {
        return;
    }

    recording = false;

    // Send EOS to pipeline so filesink completes the MP4 file
    gst_element_send_event(recordPipeline, gst_event_new_eos());

    // Wait for the EOS message on the bus
    GstBus *bus = gst_element_get_bus(recordPipeline);
    if (bus) {
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
            (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
        if (msg) {
            gst_message_unref(msg);
        }
        gst_object_unref(bus);
    }

    gst_element_set_state(recordPipeline, GST_STATE_NULL);
    
    if (recordAppsrc) {
        gst_object_unref(recordAppsrc);
        recordAppsrc = nullptr;
    }
    gst_object_unref(recordPipeline);
    recordPipeline = nullptr;

    std::cout << "[SmartVision] Stopped recording and finalized file.\n";
}