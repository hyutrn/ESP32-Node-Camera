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
#include "mbedtls/base64.h"

// Semaphore 
SemaphoreHandle_t xSemaphoreCamera = NULL;
const int EVENT_INIT_DONE = BIT0;
#define EVENT_CAMERA_INIT_DONE BIT0

#define BOARD_ESP32CAM_AITHINKER
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 
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
#define CAMERA_FRAME_SIZE FRAMESIZE_VGA  // Kích thước khung hình (640x480)
#define CAMERA_JPEG_QUALITY 10           // Chất lượng JPEG (0-63)#define CAMERA_BUFFER_SIZE 64 * 1024     // 64KB buffer cho ảnh JPEG

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

    .xclk_freq_hz = 20000000,               //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,         //JPEG
    .frame_size = CAMERA_FRAME_SIZE,        //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.
    .jpeg_quality = CAMERA_JPEG_QUALITY,    //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,                          //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

// Hàm chuyển đổi ảnh JPEG sang Base64
char* jpeg_to_base64(camera_fb_t* fb) {
    if (!fb || fb->len == 0) {
        printf("Invalid frame buffer\n");
        return NULL;
    }

    // Tính toán độ dài chuỗi Base64
    size_t input_len = fb->len; // Kích thước ảnh JPEG
    size_t output_len = 4 * ((input_len + 2) / 3) + 1; // Độ dài Base64 + ký tự NULL

    // Cấp phát bộ nhớ cho chuỗi Base64
    char* base64_buf = malloc(output_len);
    if (!base64_buf) {
        printf("Failed to allocate memory for Base64\n");
        return NULL;
    }

    // Mã hóa Base64
    size_t olen = 0; // Độ dài thực sự sau mã hóa
    int ret = mbedtls_base64_encode((unsigned char*)base64_buf, output_len, &olen, fb->buf, input_len);
    if (ret != 0) {
        printf("Base64 encoding failed: %d\n", ret);
        free(base64_buf);
        return NULL;
    }

    return base64_buf; // Trả về chuỗi Base64
}

static esp_err_t init_camera(void)
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE("CAMERA", "Camera Init Failed");
        return err;
    }
    // Sau khi khởi tạo thành công, set bit EVENT_CAMERA_INIT_DONE thành True
    xEventGroupSetBits(event_group, EVENT_CAMERA_INIT_DONE);
    return ESP_OK;
}


void cameraTask(void *arg) {
    while (1) {
        ESP_LOGI("MAIN", "Taking picture...");
        camera_fb_t *pic = esp_camera_fb_get();

        if (pic == NULL) {
            ESP_LOGE("MAIN", "Camera capture failed");
            vTaskDelay(30000 / portTICK_PERIOD_MS); // Đợi và thử lại
            continue;
        }

        ESP_LOGI("MAIN", "Picture taken! Its size was: %zu bytes", pic->len);
    
        // Xử lý pic->buf thành chuỗi Base64
        char* base64_image = jpeg_to_base64(pic);
        if (base64_image) {
            //printf("Base64 Encoded Image:\n%s\n", base64_image);
            load_id_node(id_node, sizeof(id_node)); 
            //Chuỗi topic MQTT cho ảnh
            char topic_image[150];  // Tạo một mảng để chứa chuỗi topic

            //Nối chuỗi "nodes/cameras/" với id_node
            snprintf(topic_image, sizeof(topic_image), "nodes/cameras/%s", id_node);
            printf("Topic: %s\n", topic_image);

            pictureSend(client, base64_image, topic_image);
            xEventGroupWaitBits(event_group, PUBLISH_IMAGE, pdTRUE, pdTRUE, portMAX_DELAY);

            free(base64_image); // Giải phóng bộ nhớ sau khi sử dụng
        }

        esp_camera_fb_return(pic);  // Đảm bảo giải phóng bộ nhớ cho camera
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}



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
    save_ip_gateway(ip_gateway);
    printf("Init Wifi and Server done!\n");

    // Khởi tạo camera và kiểm tra lỗi
    if (init_camera() != ESP_OK) {
       ESP_LOGE("MAIN", "Camera initialization failed");
        return;
    }
    // Chờ cho đến khi camera được khởi tạo xong
    xEventGroupWaitBits(event_group, EVENT_CAMERA_INIT_DONE, pdTRUE, pdTRUE, portMAX_DELAY);
    vTaskDelay(2000/ portTICK_PERIOD_MS);

    mqtt_app_start();
    xEventGroupWaitBits(event_group, EVEN_INIT_MQTT_DONE, pdTRUE, pdTRUE, portMAX_DELAY);

    xSemaphoreCamera = xSemaphoreCreateBinary();
    // Tạo task xử lý từng task
    xTaskCreate(cameraTask, "CameraTask", 4096, NULL, 9, NULL);
}
