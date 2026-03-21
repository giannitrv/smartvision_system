#ifndef AI_H
#define AI_H

#include <opencv2/opencv.hpp>
#include "yolov8.h"
#include "image_utils.h"

class AI {
public:
    AI(const char *model_path);
    ~AI();
    void process(cv::Mat &frame);
private:
    rknn_app_context_t rknn_app_ctx;
    image_buffer_t src_image;
    image_buffer_t dst_img;
    object_detect_result_list od_results;
};

#endif // AI_H