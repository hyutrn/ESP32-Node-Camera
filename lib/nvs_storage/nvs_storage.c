#include "nvs_storage.h"

// Hàm lưu SSID và Password vào NVS
void save_wifi_credentials(const char* ssid, const char* password) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi_store", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS handle!\n");
        return;
    }

    // Lưu SSID và Password
    err = nvs_set_str(nvs_handle, "ssid", ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(nvs_handle, "password", password);
    }

    if (err == ESP_OK) {
        nvs_commit(nvs_handle);
        printf("WiFi credentials saved successfully!\n");
    } else {
        printf("Failed to save WiFi credentials: %s\n", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}

// Hàm lưu id_node vào NVS
void save_id_node(const char* id_node){
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("id_node_store", NVS_READWRITE, &nvs_handle);
    if(err != ESP_OK){
        printf("Error opening NVS handle!\n");
        return;
    }

    //Save id_node 
    err = nvs_set_str(nvs_handle, "id_node", id_node);
    if(err == ESP_OK){
        nvs_commit(nvs_handle);
        printf("id_node saved successfully!\n");
    } else {
        printf("Failed to save id_node: %s\n", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
}

void save_ip_gateway(const char* ip_gateway) {
    nvs_handle_t nvs_handle_ip;
    esp_err_t err = nvs_open("ip_gw", NVS_READWRITE, &nvs_handle_ip);
    
    // Kiểm tra lỗi khi mở NVS
    if (err != ESP_OK) {
        printf("Error opening NVS handle: %s\n", esp_err_to_name(err));
        return;
    }

    // Lưu IP Gateway với khóa ngắn hơn
    err = nvs_set_str(nvs_handle_ip, "gw_ip", ip_gateway);  // Thay "gw_ip" thay vì "ip_gateway"
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle_ip);
        if (err != ESP_OK) {
            printf("Failed to commit NVS: %s\n", esp_err_to_name(err));
        } else {
            printf("IP Gateway saved successfully!\n");
        }
    } else {
        printf("Failed to save IP Gateway: %s\n", esp_err_to_name(err));
    }

    nvs_close(nvs_handle_ip);
}


// Hàm đọc SSID và Password từ NVS
bool load_wifi_credentials(char* ssid, size_t ssid_size, char* password, size_t password_size) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("wifi_store", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        printf("No WiFi credentials found in NVS!\n");
        return false;
    }

    // Đọc SSID và Password
    err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_size);
    if (err == ESP_OK) {
        err = nvs_get_str(nvs_handle, "password", password, &password_size);
    }

    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        printf("WiFi credentials loaded: SSID=%s\n", ssid);
        return true;
    } else {
        printf("Failed to load WiFi credentials: %s\n", esp_err_to_name(err));
        return false;
    }
}

// Hàm đọc id_node từ NVS
bool load_id_node(char* id_node, size_t id_node_size) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("id_node_store", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        printf("No id_node found in NVS!\n");
        strncpy(id_node, "-1", id_node_size);  // Đặt giá trị mặc định
        return false;
    }

    // Đọc id_node
    size_t required_size = id_node_size;
    err = nvs_get_str(nvs_handle, "id_node", id_node, &required_size);

    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        printf("id_node loaded: %s\n", id_node);
        return true;
    } else {
        printf("Failed to load id_node: %s\n", esp_err_to_name(err));
        strncpy(id_node, "-1", id_node_size);  // Đặt giá trị mặc định
        return false;
    }
}

bool load_ip_gateway(char* ip_gateway, size_t ip_gateway_size) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("ip_gw", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        printf("No IP Gateway found in NVS!\n");
        return false;
    }

    // Đọc IP Gateway từ key "gw_ip"
    err = nvs_get_str(nvs_handle, "gw_ip", ip_gateway, &ip_gateway_size);  // Thay "gw_ip" thay vì "ip_gateway"

    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        printf("IP Gateway loaded: %s\n", ip_gateway);
        return true;
    } else {
        printf("Failed to load IP Gateway: %s\n", esp_err_to_name(err));
        return false;
    }
}
