#include "ai.h"
#include "image_drawing.h"
#include "yolov8.h"
#include <string.h>

AI::AI(const char *model_path) {
    int ret;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));
    memset(&src_image, 0, sizeof(image_buffer_t));
    memset(&dst_img, 0, sizeof(image_buffer_t));

    init_post_process();

    ret = init_yolov8_model(model_path, &rknn_app_ctx);
    if (ret != 0) {
        printf("init_yolov8_model fail! ret=%d model_path=%s\n", ret, model_path);
    }
    dst_img.width = rknn_app_ctx.model_width;
    dst_img.height = rknn_app_ctx.model_height;
    dst_img.format = IMAGE_FORMAT_RGB888;
    dst_img.size = get_image_size(&dst_img);
    dst_img.virt_addr = (unsigned char *)malloc(dst_img.size);
    if (dst_img.virt_addr == NULL) {
        printf("malloc dst_img fail!\n");
    }
}

AI::~AI() {
    int ret;

    deinit_post_process();

    ret = release_yolov8_model(&rknn_app_ctx);
    if (ret != 0) {
        printf("release_yolov8_model fail! ret=%d\n", ret);
    }
    if (dst_img.virt_addr != NULL) {
        free(dst_img.virt_addr);
    }

}

cv::Point AI::process(cv::Mat &frame, int target_id, float *trackedSize) {
    int ret;
    int bg_color = 114;
    letterbox_t letter_box;
    cv::Point target_center(-1, -1);
    std::vector<Object> objects;

    // Wrap the OpenCV BGR frame directly
    src_image.width = frame.cols;
    src_image.height = frame.rows;
    src_image.format = IMAGE_FORMAT_BGR888;
    src_image.size = get_image_size(&src_image);
    src_image.virt_addr = frame.data;

    // Pre Process
    memset(&letter_box, 0, sizeof(letterbox_t));
    ret = convert_image_with_letterbox(&src_image, &dst_img, &letter_box, bg_color);
    if (ret < 0) {
        printf("convert_image_with_letterbox fail! ret=%d\n", ret);
        return target_center;
    }

    rknn_input dynamic_inputs[rknn_app_ctx.io_num.n_input];
    memset(dynamic_inputs, 0, sizeof(dynamic_inputs));
    dynamic_inputs[0].index = 0;
    dynamic_inputs[0].type = RKNN_TENSOR_UINT8;
    dynamic_inputs[0].fmt = RKNN_TENSOR_NHWC;
    dynamic_inputs[0].size = rknn_app_ctx.model_width * rknn_app_ctx.model_height * rknn_app_ctx.model_channel;
    dynamic_inputs[0].buf = dst_img.virt_addr;

    ret = rknn_inputs_set(rknn_app_ctx.rknn_ctx, rknn_app_ctx.io_num.n_input, dynamic_inputs);
    if (ret < 0) {
        printf("rknn_input_set fail! ret=%d\n", ret);
        return target_center;
    }
    // Inference
    ret = rknn_run(rknn_app_ctx.rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        return target_center;
    }
    rknn_output dynamic_outputs[rknn_app_ctx.io_num.n_output];
    memset(dynamic_outputs, 0, sizeof(dynamic_outputs));
    for (int i = 0; i < rknn_app_ctx.io_num.n_output; i++) {
        dynamic_outputs[i].index = i;
        dynamic_outputs[i].want_float = (!rknn_app_ctx.is_quant);
    }
    ret = rknn_outputs_get(rknn_app_ctx.rknn_ctx, rknn_app_ctx.io_num.n_output, dynamic_outputs, NULL);
    if (ret < 0) {
        printf("rknn_outputs_get fail! ret=%d\n", ret);
        return target_center;
    }
    // Post Process
    memset(&od_results, 0x00, sizeof(od_results));
    post_process(&rknn_app_ctx, dynamic_outputs, &letter_box, BOX_THRESH, NMS_THRESH, &od_results);
    rknn_outputs_release(rknn_app_ctx.rknn_ctx, rknn_app_ctx.io_num.n_output, dynamic_outputs);
    // Draw and Track
    for (int i = 0; i < od_results.count; i++) {
        object_detect_result *det_result = &(od_results.results[i]);
        Object obj;
        obj.rect.x = det_result->box.left;
        obj.rect.y = det_result->box.top;
        obj.rect.width = det_result->box.right - det_result->box.left;
        obj.rect.height = det_result->box.bottom - det_result->box.top;
        obj.label = det_result->cls_id;
        obj.prob = det_result->prop;
        objects.push_back(obj);
    }
    output_stracks.clear();
    output_stracks = tracker.update(objects);
    {
        std::lock_guard<std::mutex> lock(tracksMutex);
        lastTracks = output_stracks;
    }

    for (int i = 0; i < output_stracks.size(); i++) {
        std::vector<float> tlwh = output_stracks[i].tlwh;
        int track_id = output_stracks[i].track_id;
        int x1 = (int)tlwh[0];
        int y1 = (int)tlwh[1];
        int w = (int)tlwh[2];
        int h = (int)tlwh[3];

        if (track_id == target_id) {
            target_center = cv::Point(x1 + w / 2, y1 + h / 2);
            if (trackedSize != nullptr) {
                *trackedSize = std::sqrt(static_cast<float>(w * h));
            }
        }
    }

    return target_center;
}

void AI::showResults(cv::Mat &frame, int target_id) {
    char text[256];
    cv::Scalar s;
    int thickness;

    for (int i = 0; i < output_stracks.size(); i++) {
        std::vector<float> tlwh = output_stracks[i].tlwh;
        int track_id = output_stracks[i].track_id;
        int x1 = (int)tlwh[0];
        int y1 = (int)tlwh[1];
        int w = (int)tlwh[2];
        int h = (int)tlwh[3];

        s = tracker.get_color(output_stracks[i].class_label);
        thickness = 2;

        if (track_id == target_id) {
            s = cv::Scalar(0, 0, 255);
            thickness = 4;
        }
        cv::rectangle(frame, cv::Point(x1, y1), cv::Point(x1 + w, y1 + h), s, thickness);

        sprintf(text, "ID:%d", track_id);
        cv::putText(frame, text, cv::Point(x1, y1 - 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, s, 2);
    }
}

int AI::getTargetIdAt(int x, int y) const {
    std::lock_guard<std::mutex> lock(tracksMutex);
    for (const STrack &track : lastTracks) {
        const std::vector<float> &tlwh = track.tlwh;
        int x1 = (int)tlwh[0];
        int y1 = (int)tlwh[1];
        int x2 = x1 + (int)tlwh[2];
        int y2 = y1 + (int)tlwh[3];

        if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
            return track.track_id;
        }
    }

    return -1; // no target found at the given coordinates
}
