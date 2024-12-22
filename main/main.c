#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "string.h"
#include <sys/param.h>
#include <stdbool.h>
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_camera.h"

#include "wifi_pro.h"
#include "server_cfg.h"
#include "mqtt_cfg.h" 
#include "shared.h"

#define EVENT_CAMERA_INIT_DONE BIT0
// Khai báo Event Group
EventGroupHandle_t event_group;

#define BOARD_ESP32CAM_AITHINKER
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22
// Định nghĩa các tham số chỉnh kích thước ảnh và buffer chứa ảnh
#define CAMERA_FRAME_SIZE FRAMESIZE_QVGA // Kích thước khung hình (240x240)
#define CAMERA_JPEG_QUALITY 12           // Chất lượng JPEG (0-63)#define CAMERA_BUFFER_SIZE 64 * 1024     // 64KB buffer cho ảnh JPEG
#define CAMERA_BUFFER_SIZE 64 * 1024     // 64KB buffer cho ảnh JPEG
char image_buffer[CAMERA_BUFFER_SIZE]; // Khai báo biến string toàn cục

// Cấu hình camera
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_RGB565, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};


static esp_err_t init_camera(void)
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE("CAMERA", "Camera Init Failed");
        return err;
    }
    // Sau khi khởi tạo thành công, set bit trong Event Group
    xEventGroupSetBits(event_group, EVENT_CAMERA_INIT_DONE);
    return ESP_OK;
}

/*
// Hàm chụp ảnh và trả về chuỗi chứa ảnh
char* captureImage() {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE("CAMERA", "Camera capture failed");
        return NULL;
    }

    // Cấp phát bộ nhớ cho buffer ảnh nếu cần
    if (image_buffer == NULL || fb->len > CAMERA_BUFFER_SIZE) {
        if (image_buffer) free(image_buffer); // Giải phóng bộ nhớ cũ
        image_buffer = (uint8_t*) malloc(fb->len); // Cấp phát bộ nhớ mới
        if (image_buffer == NULL) {
            ESP_LOGE("CAMERA", "Failed to allocate memory for image");
            esp_camera_fb_return(fb);
            return NULL;
        }
    }

    // Sao chép dữ liệu ảnh vào buffer
    memcpy(image_buffer, fb->buf, fb->len);

    // Trả lại buffer cho driver
    esp_camera_fb_return(fb);

    ESP_LOGI("CAMERA", "Image captured successfully");
    return (char*)image_buffer;
}
*/

// Semaphore 
SemaphoreHandle_t xSemaphoreCamera = NULL;

EventGroupHandle_t event_group;
const int EVENT_INIT_DONE = BIT0;

void cameraTask(void *arg) {
    // Chờ cho đến khi camera được khởi tạo xong
    xEventGroupWaitBits(event_group, EVENT_CAMERA_INIT_DONE, pdTRUE, pdTRUE, portMAX_DELAY);
    
    while (1) {
        ESP_LOGI("MAIN", "Taking picture...");
        camera_fb_t *pic = esp_camera_fb_get();

        if (pic == NULL) {
            ESP_LOGE("MAIN", "Camera capture failed");
            vTaskDelay(5000 / portTICK_PERIOD_MS); // Đợi và thử lại
            continue;
        }

        // Lấy thông tin kích thước ảnh
        size_t width = pic->width;
        size_t height = pic->height;

        ESP_LOGI("MAIN", "Picture taken! Its size was: %zu bytes", pic->len);
        // Gửi ảnh qua MQTT
        pictureSend(client, pic->buf, pic->len, width, height, "/camera/image");
        //pictureSend(client, pic->buf, pic->len, "/camera/image");
        //testSend(client);

        esp_camera_fb_return(pic);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

/*
#define IMAGE_BUFFER_SIZE  153600 // Kích thước ảnh cụ thể (153600 bytes)

static uint8_t* image_buffer = NULL; // Biến chứa bộ đệm ảnh
void cameraTask(void *pvParameters) {
    camera_fb_t *pic = NULL;

    // Lấy ảnh từ camera
    pic = esp_camera_fb_get();
    if (!pic) {
        ESP_LOGE("CAMERA", "Camera capture failed");
        vTaskDelete(NULL);
        return;
    }

    // Kiểm tra kích thước bộ đệm cần thiết và cấp phát bộ nhớ mới nếu cần
    if (image_buffer == NULL || pic->len != IMAGE_BUFFER_SIZE) {
        if (image_buffer) {
            free(image_buffer); // Giải phóng bộ nhớ cũ nếu có
        }
        
        // Cấp phát bộ nhớ đủ cho ảnh mới (153600 bytes)
        image_buffer = (uint8_t*) malloc(pic->len);
        if (image_buffer == NULL) {
            ESP_LOGE("CAMERA", "Failed to allocate memory for image");
            esp_camera_fb_return(pic);
            vTaskDelete(NULL);
            return;
        }
    }

    // Sao chép dữ liệu ảnh vào bộ đệm đã cấp phát
    memcpy(image_buffer, pic->buf, pic->len);
    ESP_LOGI("CAMERA", "Captured image of size %d bytes", pic->len);

    // Xử lý ảnh hoặc lưu ảnh tại đây

    // Giải phóng bộ nhớ ảnh sau khi xử lý xong
    esp_camera_fb_return(pic);

    // Tạm dừng task này một chút (nếu cần)
    vTaskDelay(5000 / portTICK_PERIOD_MS);
}
*/
void app_main(void) {
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    //Khởi tạo Event Group
    shared_event_group_init();
    // Khởi tạo Event Group
    event_group = xEventGroupCreate();

    initialise_wifi();

    //Chờ đợi sự kiện EVENT_CLIENT_POSTED
    printf("Waiting for init node...\n");
    xEventGroupWaitBits(shared_event_group, EVENT_CLIENT_POSTED, pdTRUE, pdTRUE, portMAX_DELAY);
    printf("Init Wifi and Server done!\n");

    // Khởi tạo camera và kiểm tra lỗi
    if (init_camera() != ESP_OK) {
        ESP_LOGE("MAIN", "Camera initialization failed");
        return;
    }
    xSemaphoreCamera = xSemaphoreCreateBinary();

    vTaskDelay(2000/ portTICK_PERIOD_MS);
    mqtt_app_start();

    // Tạo task xử lý từng task
    xTaskCreate(cameraTask, "CameraTask", 8192, NULL, 9, NULL);
}
