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
    if (ret != 0)
    {
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
    if (ret != 0)
    {
        printf("release_yolov8_model fail! ret=%d\n", ret);
    }

    if (dst_img.virt_addr != NULL) {
        free(dst_img.virt_addr);
    }

    if (src_image.virt_addr != NULL)
    {
        free(src_image.virt_addr);
    }
}

void AI::process(cv::Mat &frame) {
    int ret;
    int bg_color = 114;
    letterbox_t letter_box;

    // Initialize or Update src_image if needed
    if (src_image.virt_addr == NULL || src_image.width != frame.cols || src_image.height != frame.rows) {
        if (src_image.virt_addr != NULL) {
            free(src_image.virt_addr);
        }
        
        src_image.width = frame.cols;
        src_image.height = frame.rows;
        src_image.format = IMAGE_FORMAT_RGB888;
        src_image.size = get_image_size(&src_image);
        src_image.virt_addr = (unsigned char *)malloc(src_image.size);
    }

    // Convert OpenCV BGR to RGB and copy to src_image
    cv::Mat rgb_frame;
    cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGB);
    memcpy(src_image.virt_addr, rgb_frame.data, src_image.size);

    // Pre Process
    memset(&letter_box, 0, sizeof(letterbox_t));
    ret = convert_image_with_letterbox(&src_image, &dst_img, &letter_box, bg_color);
    if (ret < 0) {
        printf("convert_image_with_letterbox fail! ret=%d\n", ret);
        return;
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
        return;
    }

    // Inference
    ret = rknn_run(rknn_app_ctx.rknn_ctx, nullptr);
    if (ret < 0) {
        printf("rknn_run fail! ret=%d\n", ret);
        return;
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
        return;
    }

    // Post Process
    memset(&od_results, 0x00, sizeof(od_results));
    post_process(&rknn_app_ctx, dynamic_outputs, &letter_box, BOX_THRESH, NMS_THRESH, &od_results);
    rknn_outputs_release(rknn_app_ctx.rknn_ctx, rknn_app_ctx.io_num.n_output, dynamic_outputs);

    // Draw
    char text[256];
    for (int i = 0; i < od_results.count; i++)
    {
        object_detect_result *det_result = &(od_results.results[i]);
        int x1 = det_result->box.left;
        int y1 = det_result->box.top;
        int x2 = det_result->box.right;
        int y2 = det_result->box.bottom;

        draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_GREEN, 1);

        sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
        //sprintf(text, "%d %.1f%%", det_result->cls_id, det_result->prop * 100);
        draw_text(&src_image, text, x1, y1 - 20, COLOR_GREEN, 10);
    }

    // Copy back from src_image (drawing modified it) to OpenCV frame
    cv::Mat rgb_result(src_image.height, src_image.width, CV_8UC3, src_image.virt_addr);
    cv::cvtColor(rgb_result, frame, cv::COLOR_RGB2BGR);
}
