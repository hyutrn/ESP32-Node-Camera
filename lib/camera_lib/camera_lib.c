#include "camera_lib.h"

// Tên TAG để log
static const char* TAG = "CAMERA_LIB";

// Cấu hình camera
static camera_config_t camera_config = {
    .pin_pwdn  = -1,
    .pin_reset = -1,
    .pin_xclk = 21,
    .pin_sccb_sda = 26,
    .pin_sccb_scl = 27,
    .pin_d7 = 35,
    .pin_d6 = 34,
    .pin_d5 = 39,
    .pin_d4 = 36,
    .pin_d3 = 19,
    .pin_d2 = 18,
    .pin_d1 = 5,
    .pin_d0 = 4,
    .pin_vsync = 25,
    .pin_href = 23,
    .pin_pclk = 22,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = CAMERA_FRAME_SIZE,
    .jpeg_quality = CAMERA_JPEG_QUALITY,
    .fb_count = 1,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

// Biến toàn cục lưu chuỗi ảnh
static char image_buffer[CAMERA_BUFFER_SIZE];

// Hàm khởi tạo camera
esp_err_t initCamera() {
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Camera initialized successfully");
    return ESP_OK;
}

// Hàm chụp ảnh và trả về chuỗi chứa ảnh
char* captureImage() {
    // Lấy khung hình từ camera
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return NULL;
    }

    // Kiểm tra kích thước buffer
    if (fb->len > CAMERA_BUFFER_SIZE) {
        ESP_LOGE(TAG, "Buffer size too small");
        esp_camera_fb_return(fb);
        return NULL;
    }

    // Sao chép dữ liệu ảnh vào buffer
    memcpy(image_buffer, fb->buf, fb->len);
    image_buffer[fb->len] = '\0'; // Đảm bảo buffer là chuỗi hợp lệ

    // Trả lại buffer cho driver
    esp_camera_fb_return(fb);

    ESP_LOGI(TAG, "Image captured successfully");
    return image_buffer;
}
