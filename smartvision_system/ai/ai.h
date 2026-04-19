#ifndef AI_H
#define AI_H

#include <opencv2/opencv.hpp>
#include "yolov8.h"
#include "image_utils.h"
#include "BYTETracker.h"

class AI {
public:
    AI(const char *model_path);
    ~AI();
    cv::Point process(cv::Mat &frame, int target_id = -1);
private:
    rknn_app_context_t rknn_app_ctx;
    image_buffer_t src_image;
    image_buffer_t dst_img;
    object_detect_result_list od_results;
    BYTETracker tracker;
};

#endif // AI_H