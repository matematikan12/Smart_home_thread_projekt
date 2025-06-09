// hub_main.c

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include "thread_utils.h"
#include "mqtt_utils.h"

static const char *TAG = "hub_main";

/**
 * @brief Callback invoked when data is received from the Thread network.
 *
 * @param data Pointer to received payload (last byte is CRC)
 * @param length Total length of received buffer (including CRC)
 * @param src_id  Thread node ID of sender
 */
void thread_receive_callback(const uint8_t *data, size_t length, uint16_t src_id)
{
    if (length < 2) {
        ESP_LOGW(TAG, "Thread packet too short");
        return;
    }

    ESP_LOGI(TAG, "Thread RX from node %d, len=%d", src_id, length);

    // Verify CRC8
    uint8_t received_crc = data[length - 1];
    uint8_t computed_crc = compute_crc8(data, length - 1);
    if (received_crc != computed_crc) {
        ESP_LOGW(TAG, "CRC mismatch (got 0x%02X, expected 0x%02X)", received_crc, computed_crc);
        return;
    }

    // Copy JSON payload (without CRC)
    size_t json_len = length - 1;
    char json_str[256];
    if (json_len >= sizeof(json_str)) {
        json_len = sizeof(json_str) - 1;
    }
    memcpy(json_str, data, json_len);
    json_str[json_len] = '\0';

    ESP_LOGI(TAG, "Thread JSON: %s", json_str);

    // Publish to MQTT under topic "home/sensors/<src_id>"
    char topic[64];
    snprintf(topic, sizeof(topic), "home/sensors/%u", src_id);
    mqtt_publish(topic, json_str);
    ESP_LOGI(TAG, "MQTT PUB → %s", topic);
}

/**
 * @brief MQTT event handler for incoming control messages.
 */
static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
    case MQTT_EVENT_DATA: {
        // Copy topic and payload into C-strings
        char topic[128], payload[128];
        int tlen = MIN(event->topic_len, sizeof(topic)-1);
        int plen = MIN(event->data_len,  sizeof(payload)-1);
        memcpy(topic, event->topic, tlen); topic[tlen] = '\0';
        memcpy(payload, event->data, plen); payload[plen] = '\0';

        ESP_LOGI(TAG, "MQTT RX: %s → %s", topic, payload);

        // Parse control topic: home/control/<node_id>
        int node_id;
        if (sscanf(topic, "home/control/%d", &node_id) == 1) {
            cJSON *root = cJSON_Parse(payload);
            if (root) {
                cJSON *relay  = cJSON_GetObjectItem(root, "relay");
                cJSON *state  = cJSON_GetObjectItem(root, "state");
                if (cJSON_IsNumber(relay) && cJSON_IsNumber(state)) {
                    uint8_t relay_id    = relay->valueint;
                    uint8_t relay_state = state->valueint;

                    // Build Thread command JSON
                    cJSON *cmd = cJSON_CreateObject();
                    cJSON_AddNumberToObject(cmd, "relay", relay_id);
                    cJSON_AddNumberToObject(cmd, "state", relay_state);
                    char *cmd_str = cJSON_PrintUnformatted(cmd);
                    if (cmd_str) {
                        size_t len = strlen(cmd_str);
                        uint8_t buf[256];
                        memcpy(buf, cmd_str, len);
                        buf[len] = compute_crc8(buf, len);
                        thread_send(buf, len + 1, (uint16_t)node_id);
                        ESP_LOGI(TAG, "Thread TX to %d: %s", node_id, cmd_str);
                        cJSON_free(cmd_str);
                    }
                    cJSON_Delete(cmd);
                }
                cJSON_Delete(root);
            }
        }
        break;
    }

    default:
        break;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== Hub (ESP32-S3) Starting ===");

    // Initialize Thread stack and register receive callback
    ESP_ERROR_CHECK(thread_init());
    thread_register_receive_cb(thread_receive_callback);

    // Initialize MQTT over TLS & register event handler
    const char *mqtt_uri = "mqtts://192.168.0.65:8883";
    const char *client_id = "hub_esp32s3";
    ESP_ERROR_CHECK(mqtt_init(mqtt_uri, client_id));
    mqtt_register_event_handler(mqtt_event_handler);

    // Subscribe to control topic for all nodes
    mqtt_subscribe("home/control/#", 1);

    // Main loop: poll Thread and yield to MQTT
    while (true) {
        // Process any pending Thread events
        thread_process();

        // Allow FreeRTOS to schedule MQTT tasks
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
