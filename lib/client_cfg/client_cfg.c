#include "client_cfg.h"

#define code_len 256  // Điều chỉnh kích thước theo yêu cầu
int client_flag;
char id_node[100];  // Biến lưu trữ id_node

//GET
esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);

        // Nếu có dữ liệu, xử lý JSON
        if(evt->data_len > 0) {
            // Chuyển đổi dữ liệu thành chuỗi JSON
            cJSON *json_response = cJSON_Parse((char *)evt->data);
            if (json_response == NULL) {
                ESP_LOGE("HTTP", "Error parsing JSON response");
                client_flag = 0;  // Không thành công
            } else {
                // Trích xuất node_id từ JSON
                cJSON *node_id_item = cJSON_GetObjectItem(json_response, "node_id");
                if (node_id_item != NULL && cJSON_IsNumber(node_id_item)) {
                    // Lưu giá trị node_id vào biến id_node dưới dạng chuỗi
                    snprintf(id_node, sizeof(id_node), "%d", node_id_item->valueint);
                    ESP_LOGI("HTTP", "Received node_id: %s", id_node);  // In ra giá trị của id_node
                    client_flag = 1;  // Thành công
                } else {
                    ESP_LOGE("HTTP", "node_id not found in JSON response");
                    client_flag = 0;  // Không thành công
                }
                // Giải phóng bộ nhớ cho đối tượng JSON
                cJSON_Delete(json_response);
            }
        }
        break;

    default:
        break;
    }
    return ESP_OK;
}

esp_err_t client_get()
{
    // Tạo chuỗi URL cho phương thức POST
    //char client_get_url[100];
    //snprintf(client_get_url, sizeof(client_get_url), "http://%s/get", ip_gateway);

    esp_http_client_config_t config_get = {
        .url = "client_get_url",
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        .event_handler = client_event_get_handler};
        
    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    return ESP_OK;
}

//POST
esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);

        // Nếu có dữ liệu, xử lý JSON
        if(evt->data_len > 0) {
            // Chuyển đổi dữ liệu thành chuỗi JSON
            cJSON *json_response = cJSON_Parse((char *)evt->data);
            if (json_response == NULL) {
                ESP_LOGE("HTTP", "Error parsing JSON response");
                client_flag = 0;  // Không thành công
            } else {
                // Trích xuất node_id từ JSON
                cJSON *node_id_item = cJSON_GetObjectItem(json_response, "node_id");
                if (node_id_item != NULL && cJSON_IsNumber(node_id_item)) {
                    // Lưu giá trị node_id vào biến id_node dưới dạng chuỗi
                    snprintf(id_node, sizeof(id_node), "%d", node_id_item->valueint);
                    ESP_LOGI("HTTP", "Received node_id: %s", id_node);  // In ra giá trị của id_node
                    client_flag = 1;  // Thành công
                } else {
                    ESP_LOGE("HTTP", "node_id not found in JSON response");
                    client_flag = 0;  // Không thành công
                }
                // Giải phóng bộ nhớ cho đối tượng JSON
                cJSON_Delete(json_response);
            }
        }
        break;

    default:
        break;
    }
    return ESP_OK;
}


esp_err_t client_post()
{
    // Tạo chuỗi URL cho phương thức POST
    char client_post_url[code_len];
    snprintf(client_post_url, sizeof(client_post_url), "http://%s:8080/register_node?code=CKABSKJCcabkjsb2kbjkcabsjkabsj2bjkbjcaksbcj", ip_gateway);

    //register_node?code=CKABSKJCcabkjsb2kbjkcabsjkabsj2bjkbjcaksbcjkasb298@21^%26$cbaskjbcajkcbajskbcjaksbcajskbcasjk
    //register_node?code=CKABSKJCcabkjsb2kbjkcabsjkabsj2bjkbjcaksbcjkasb298@21^&$cbaskjbcajkcbajskbcjaksbcajskbcasjk
    printf("%s\n", client_post_url);

    esp_http_client_config_t config_post = {
        .url = client_post_url,
        .host = ip_gateway,
        .port = 8080,
        .username = NULL,
        .password = NULL,
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        .event_handler = client_event_post_handler};

    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    if(client_flag == 1) {
        return ESP_OK;
    }
    else {
        return ESP_FAIL;
    }
}
