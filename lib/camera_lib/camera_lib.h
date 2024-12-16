#ifndef CAMERA_LIB_H
#define CAMERA_LIB_H

#include "esp_camera.h"
#include "esp_log.h"

// Định nghĩa các tham số chỉnh kích thước ảnh và buffer chứa ảnh
#define CAMERA_FRAME_SIZE FRAMESIZE_QVGA // Kích thước khung hình (240x240)
#define CAMERA_BUFFER_SIZE 64 * 1024     // 64KB buffer cho ảnh JPEG
#define CAMERA_JPEG_QUALITY 12           // Chất lượng JPEG (0-63)

// Khởi tạo camera ESP32
esp_err_t initCamera();

// Chụp ảnh và trả về chuỗi chứa ảnh
char* captureImage();

#endif // CAMERA_LIB_H
