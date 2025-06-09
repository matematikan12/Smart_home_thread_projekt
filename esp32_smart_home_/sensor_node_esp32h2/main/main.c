#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "thread_utils.h"
#include "sensor_utils.h"
#include "actuator_utils.h" // Для compute_crc8
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "sensor_node";

#define WINDOW_SIZE 5  // Розмір вікна для ковзного середнього

// Структура для зберігання останніх вимірів (ковзне середнє)
typedef struct {
    float buffer[WINDOW_SIZE];
    int index;
    int count;
    float sum;
} moving_avg_t;

/*
 * Ініціалізація структури ковзного середнього
 */
static void moving_avg_init(moving_avg_t *ma) {
    for (int i = 0; i < WINDOW_SIZE; i++) {
        ma->buffer[i] = 0.0f;
    }
    ma->index = 0;
    ma->count = 0;
    ma->sum = 0.0f;
}

/*
 * Оновлення ковзного середнього:
 * new_val: нове значення; повертає усереднене
 */
static float moving_avg_update(moving_avg_t *ma, float new_val) {
    if (ma->count < WINDOW_SIZE) {
        ma->buffer[ma->index] = new_val;
        ma->sum += new_val;
        ma->count++;
    } else {
        ma->sum -= ma->buffer[ma->index];
        ma->buffer[ma->index] = new_val;
        ma->sum += new_val;
    }
    ma->index = (ma->index + 1) % WINDOW_SIZE;
    return ma->sum / ma->count;
}

/*
 * Завдання для читання даних із датчиків і відправки їх у Thread
 */
void sensor_task(void *pvParameters) {
    moving_avg_t temp_avg;
    moving_avg_init(&temp_avg);

    // Ініціалізуємо I2C для сенсорів
    if (sensor_i2c_init() != ESP_OK) {
        ESP_LOGE(TAG, "Помилка ініціалізації I2C для сенсорів");
        vTaskDelete(NULL);
    }

    while (1) {
        // 1. Зчитуємо температуру та вологість (ковзне середнє для температури)
        float raw_temp = read_temperature();
        float avg_temp = moving_avg_update(&temp_avg, raw_temp);
        float humidity = read_humidity();

        // 2. Зчитуємо CO2 та TVOC (CCS811)
        uint16_t co2 = read_co2();

        // 3. Зчитуємо освітленість (BH1750)
        uint16_t light = read_light();

        // 4. Зчитуємо PIR-датчик руху 
        bool motion = read_motion();

        // 5. Зчитуємо датчик витоку води (GPIO)
        bool leak = read_leak();

        // 6. Формуємо JSON
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "temperature", avg_temp);
        cJSON_AddNumberToObject(root, "humidity", humidity);
        cJSON_AddNumberToObject(root, "co2", co2);
        cJSON_AddNumberToObject(root, "light", light);
        cJSON_AddBoolToObject(root, "motion", motion);
        cJSON_AddBoolToObject(root, "leak", leak);
        char *json_str = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);

        if (json_str) {
            size_t len = strlen(json_str);
            // 7. Додаємо CRC до кінця
            uint8_t send_buf[512];
            memcpy(send_buf, json_str, len);
            uint8_t crc = compute_crc8(send_buf, len);
            send_buf[len] = crc;
            size_t total_len = len + 1;

            // 8. Відправляємо через Thread до хаба (ID 0)
            thread_send(send_buf, total_len, 0x00);
            ESP_LOGI(TAG, "Відправлено: %s", json_str);
            cJSON_free(json_str);
        }

        // 9. Затримка 10 секунд, Deep-Sleep
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_LOGI(TAG, "Запуск вузла-датчика (ESP32-H2)");

    // Ініціалізуємо Thread-стек
    thread_init();

    // Запускаємо завдання сенсора
    xTaskCreate(sensor_task, "sensor_task", 8192, NULL, 5, NULL);
}
