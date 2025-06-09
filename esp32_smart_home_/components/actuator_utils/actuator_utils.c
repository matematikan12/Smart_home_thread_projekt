#include "actuator_utils.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

static const char *TAG = "actuator_utils";

// GPIO для реле
#define RELAY_GPIO_1    5
#define RELAY_GPIO_2    18

// Параметри для PWM-сервоприводів (LEDC)
#define SERVO_LEDC_TIMER      LEDC_TIMER_0
#define SERVO_LEDC_MODE       LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_CHANNEL    LEDC_CHANNEL_0
#define SERVO_LEDC_GPIO       19
#define SERVO_FREQ_HZ         50     // Частота 50 Hz
#define SERVO_DUTY_MIN        40     // Мінімальний duty (1 ms)
#define SERVO_DUTY_MAX        115    // Максимальний duty (2.5 ms)

/*
 * Ініціалізує GPIO для керування реле та PWM для сервопривода
 */
void actuator_init(void) {
    // 1. Конфігурація реле
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_GPIO_1) | (1ULL << RELAY_GPIO_2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(RELAY_GPIO_1, 0);
    gpio_set_level(RELAY_GPIO_2, 0);

    // 2. Налаштування LEDC для серво
    ledc_timer_config_t ledc_timer = {
        .speed_mode = SERVO_LEDC_MODE,
        .timer_num = SERVO_LEDC_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = SERVO_LEDC_MODE,
        .channel = SERVO_LEDC_CHANNEL,
        .timer_sel = SERVO_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SERVO_LEDC_GPIO,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);

    ESP_LOGI(TAG, "Актуатори ініціалізовані");
}

/*
 * relay_on: вмикає реле на заданому каналі (1 або 2)
 */
void relay_on(uint8_t channel) {
    if (channel == 1) {
        gpio_set_level(RELAY_GPIO_1, 1);
        ESP_LOGI(TAG, "Реле 1 увімкнено");
    } else if (channel == 2) {
        gpio_set_level(RELAY_GPIO_2, 1);
        ESP_LOGI(TAG, "Реле 2 увімкнено");
    }
}

/*
 * relay_off: вимикає реле на каналі (1 або 2)
 */
void relay_off(uint8_t channel) {
    if (channel == 1) {
        gpio_set_level(RELAY_GPIO_1, 0);
        ESP_LOGI(TAG, "Реле 1 вимкнено");
    } else if (channel == 2) {
        gpio_set_level(RELAY_GPIO_2, 0);
        ESP_LOGI(TAG, "Реле 2 вимкнено");
    }
}

/*
 * servo_set_angle: встановлює кут сервопривода (0..180)
 */
void servo_set_angle(uint8_t angle) {
    if (angle > 180) angle = 180;
    // Переводимо кут у duty
    uint32_t duty = ((angle * (SERVO_DUTY_MAX - SERVO_DUTY_MIN)) / 180) + SERVO_DUTY_MIN;
    ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
    ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);
    ESP_LOGI(TAG, "Сервопривід: кут=%d°, duty=%d", angle, duty);
}

/*
 * compute_crc8: обчислення CRC-8 (поліном 0x8C)
 */
uint8_t compute_crc8(const uint8_t *data, size_t length) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < length; i++) {
        uint8_t extract = data[i];
        for (uint8_t tempI = 8; tempI; tempI--) {
            uint8_t sum = (crc ^ extract) & 0x01;
            crc >>= 1;
            if (sum) {
                crc ^= 0x8C;
            }
            extract >>= 1;
        }
    }
    return crc;
}
