// actuator_node.c

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cJSON.h"
#include "thread_utils.h"
#include "actuator_utils.h"

static const char *TAG = "actuator_node";

/**
 * @brief Callback invoked when a Thread packet arrives.
 *
 * @param data    Pointer to buffer containing JSON command + CRC8
 * @param length  Total length of buffer (including CRC byte)
 * @param src_id  Thread node ID of the sender (hub)
 */
static void thread_receive_callback(const uint8_t *data, size_t length, uint16_t src_id)
{
    ESP_LOGI(TAG, "Thread RX from node %u, length=%u", src_id, length);

    // 1. Validate packet length (must contain at least 1-byte payload + 1-byte CRC)
    if (length < 2) {
        ESP_LOGW(TAG, "Packet too short, dropping");
        return;
    }

    // 2. Verify CRC8
    uint8_t received_crc = data[length - 1];
    uint8_t computed_crc = compute_crc8(data, length - 1);
    if (received_crc != computed_crc) {
        ESP_LOGW(TAG, "CRC mismatch: got 0x%02X expected 0x%02X", received_crc, computed_crc);
        return;
    }

    // 3. Extract JSON payload (exclude CRC byte)
    size_t json_len = length - 1;
    char json_str[256];
    if (json_len >= sizeof(json_str)) {
        json_len = sizeof(json_str) - 1;
    }
    memcpy(json_str, data, json_len);
    json_str[json_len] = '\0';
    ESP_LOGI(TAG, "Command JSON: %s", json_str);

    // 4. Parse JSON for relay command
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        ESP_LOGW(TAG, "JSON parse error");
        return;
    }
    cJSON *relay = cJSON_GetObjectItem(root, "relay");
    cJSON *state = cJSON_GetObjectItem(root, "state");
    if (!cJSON_IsNumber(relay) || !cJSON_IsNumber(state)) {
        ESP_LOGW(TAG, "Invalid JSON format");
        cJSON_Delete(root);
        return;
    }
    uint8_t relay_id    = (uint8_t)relay->valueint;
    uint8_t relay_state = (uint8_t)state->valueint;
    cJSON_Delete(root);

    // 5. Execute relay action
    if (relay_state) {
        relay_on(relay_id);
        ESP_LOGI(TAG, "Relay %u → ON", relay_id);
    } else {
        relay_off(relay_id);
        ESP_LOGI(TAG, "Relay %u → OFF", relay_id);
    }

    // 6. Send ACK back to the hub: { "status": 0 }
    cJSON *ack = cJSON_CreateObject();
    cJSON_AddNumberToObject(ack, "status", 0);
    char *ack_str = cJSON_PrintUnformatted(ack);
    cJSON_Delete(ack);
    if (ack_str) {
        size_t ack_len = strlen(ack_str);
        uint8_t buf[128];
        if (ack_len + 1 <= sizeof(buf)) {
            memcpy(buf, ack_str, ack_len);
            buf[ack_len] = compute_crc8(buf, ack_len);
            thread_send(buf, ack_len + 1, src_id);
            ESP_LOGI(TAG, "ACK sent to node %u", src_id);
        }
        cJSON_free(ack_str);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting actuator node (ESP32-H2)");

    // Initialize Thread stack
    ESP_ERROR_CHECK(thread_init());

    // Initialize actuators (GPIOs, PWM, etc.)
    actuator_init();

    // Register callback for incoming Thread commands
    thread_register_receive_cb(thread_receive_callback);

    // Main loop: process Thread events
    while (true) {
        thread_process();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}