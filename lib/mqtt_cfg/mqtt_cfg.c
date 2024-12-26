#include "mqtt_cfg.h" 

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1 
static const uint8_t mqtt_cert_key_pem_start[] = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDDEN "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_cert_key_pem_start[]   asm("_binary_mqtt_cert_key_pem_start");
#endif
extern const uint8_t mqtt_cert_key_pem_end[]   asm("_binary_mqtt_cert_key_pem_end");

esp_mqtt_client_handle_t client = NULL;

esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id = -1;
    // your_context_t *context = event->context;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI("MQTT", "MQTT_EVENT_CONNECTED");

        //Khi connect thành công, set bit EVENT_INIT_MQTT_DONE bằng true
        xEventGroupSetBits(event_group, EVEN_INIT_MQTT_DONE);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI("MQTT", "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI("MQTT", "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI("MQTT", "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI("MQTT", "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI("MQTT", "MQTT_EVENT_DATA");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI("MQTT", "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI("MQTT", "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    /* The argument passed to esp_mqtt_client_register_event can de accessed as handler_args*/
    ESP_LOGD("MQTT", "Event dispatched from event loop base=%s, event_id=%" PRId32, base, event_id);
    mqtt_event_handler_cb(event_data);
}

// Hàm chuỗi base64 qua MQTT
void pictureSend(esp_mqtt_client_handle_t client, const char* base64_image, const char *topic) {
    if (!client || !topic || !base64_image) {
        printf("Invalid parameters for MQTT publish\n");
        return;
    }

    // Publish dữ liệu Base64
    int msg_id = esp_mqtt_client_publish(client, topic, base64_image, 0, 1, 0);
    if (msg_id >= 0) {
        printf("Base64 data published successfully, msg_id=%d\n", msg_id);
    } else {
        printf("Failed to publish Base64 data, msg_id=%d\n", msg_id);
    }

    // Gán bit sự kiện cho EVENT_CLIENT_POSTED 
    xEventGroupSetBits(event_group, PUBLISH_IMAGE);
}


void mqtt_app_start(void)
{
    load_id_node(id_node, sizeof(id_node));
    char last_topic[150];
    snprintf(last_topic, sizeof(last_topic), "nodes/cameras/%s", id_node);

    //Config for using local broker
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://rasp.local",
        //.broker.address.hostname = "mqtt://rasp.local",
        .broker.address.port = 8003,
        .credentials.username = "node",
        //.credentials.client_id = "node camera",
        .credentials.authentication.password = "test",
        .session.keepalive = 10,
        //.last_will.topic = "nodes/sensors",
        .session.last_will.msg = "offline",
        .session.last_will.msg_len = 7,
        .session.last_will.qos = 1,
        .session.last_will.topic = last_topic,
        .session.last_will.retain = 0,
    };

    /*
    //Config for using flepsi.io broker
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://mqtt.flespi.io",
        .broker.address.hostname = "mqtt://mqtt.flespi.io",
        .broker.address.port = 1883,
        .credentials.username = "ejkH5zHIZGRWVmg8cjgSKuDeWZoNhhgzrRl6BsOixEEEIgJ6bauQbQYGcPs5vyUd",
        .credentials.client_id = "NODE CAMERA",
        .credentials.authentication.password = NULL,
        .session.keepalive = 90
    };
    */

    ESP_LOGI("MQTT", "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
