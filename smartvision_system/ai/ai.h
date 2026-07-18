#ifndef AI_H
#define AI_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <mutex>
#include <set>
#include "yolov8.h"
#include "image_utils.h"
#include "BYTETracker.h"

class AI {
public:
    AI(const char *modelPath, const std::vector<int> &selectedClasses);
    ~AI();
    cv::Point process(cv::Mat &frame, int target_id = -1, float *trackedSize = nullptr);
    void showResults(cv::Mat &frame, int target_id);
    // Returns the track_id of the tracked object whose bounding box contains
    // the point (x, y), or -1 if no match is found.
    int getTargetIdAt(int x, int y) const;
private:
    rknn_app_context_t rknn_app_ctx;
    image_buffer_t src_image;
    image_buffer_t dst_img;
    object_detect_result_list od_results;
    BYTETracker tracker;
    std::set<int> selectedClasses;
    std::vector<STrack> output_stracks;
    std::vector<STrack> lastTracks; // cached result of the last tracker update
    mutable std::mutex tracksMutex; // protects lastTracks (written by capture thread, read by command thread)
};

#endif // AI_H