#include "mqtt_utils.h"
#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt_utils";
static esp_mqtt_client_handle_t client;

/*
 * Ініціалізуємо MQTT-клієнт із TLS-з'єднанням
 * broker_uri: URI брокера (наприклад, "mqtts://broker.local:8883")
 * client_id: унікальний ідентифікатор клієнта
 */
void mqtt_init(const char *broker_uri, const char *client_id) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = broker_uri,
        .client_id = client_id,
        .cert_pem = (const char *)broker_ca_pem_start, // CA-сертифікат
        .client_cert_pem = (const char *)client_cert_pem_start,
        .client_key_pem = (const char *)client_key_pem_start
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Помилка ініціалізації MQTT-клієнта");
        return;
    }

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

/*
 * mqtt_publish: публікує payload у топік topic з QoS 1
 */
void mqtt_publish(const char *topic, const char *payload) {
    if (client) {
        int msg_id = esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
        ESP_LOGI(TAG, "MQTT публікація ID: %d, топік: %s, payload: %s", msg_id, topic, payload);
    }
}

/*
 * Колбек для обробки подій MQTT
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT підключено");
            // Можна підписатися на топіки:
            // esp_mqtt_client_subscribe(client, "home/control/#", 1);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT дані отримано: топік: %.*s, payload: %.*s",
                     event->topic_len, event->topic, event->data_len, event->data);
            // Обробка даних в колбеці consumer-складова
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT помилка");
            break;
        default:
            break;
    }
}
