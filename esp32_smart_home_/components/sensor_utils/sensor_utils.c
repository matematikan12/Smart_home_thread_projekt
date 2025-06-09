#include "sensor_utils.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include <math.h>

static const char *TAG = "sensor_utils";

#define I2C_MASTER_SCL_IO           22 /*!< GPIO номер SCL порту */
#define I2C_MASTER_SDA_IO           21 /*!< GPIO номер SDA порту */
#define I2C_MASTER_FREQ_HZ          100000 /*!< Частота I2C */
#define I2C_MASTER_NUM              I2C_NUM_0 /*!< I2C порт */

#define BH1750_SENSOR_ADDR          0x23 /*!< Адреса BH1750 */
#define AM2320_SENSOR_ADDR          0x5C /*!< Адреса AM2320 */
#define CCS811_SENSOR_ADDR          0x5A /*!< Адреса CCS811 */

#define BH1750_CMD_START            0x10 /*!< Команда старту вимірювання (H-Resolution Mode) */

static esp_err_t i2c_master_init(void) {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) return err;
    return i2c_driver_install(i2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);
}

/*
 * read_bh1750: зчитує освітленість у люксах
 */
uint16_t read_light(void) {
    uint8_t data[2] = {0};
    // Ініціюємо зчитування
    i2c_master_write_to_device(I2C_MASTER_NUM, BH1750_SENSOR_ADDR, (uint8_t*)&BH1750_CMD_START, 1, 1000 / portTICK_RATE_MS);
    vTaskDelay(pdMS_TO_TICKS(180)); // Час вимірювання ~180ms
    // Читаємо два байти даних
    i2c_master_read_from_device(I2C_MASTER_NUM, BH1750_SENSOR_ADDR, data, 2, 1000 / portTICK_RATE_MS);
    uint16_t raw = (data[0] << 8) | data[1];
    uint16_t lux = raw / 1.2; // Переведення у люкси
    ESP_LOGI(TAG, "BH1750: освітленість: %d lx", lux);
    return lux;
}

/*
 * read_am2320: зчитує температуру та вологість
 * повертає температуру через вказівник temp,
 * та вологість через вказівник hum
 */
static esp_err_t read_am2320(float *temp, float *hum) {
    uint8_t cmd[2] = {0x03, 0x00}; // Команда читання даних
    // Запит 4 байти від регістру 0
    cmd[1] = 0x04;
    // Затримка перед роботою датчика
    vTaskDelay(pdMS_TO_TICKS(2));
    i2c_master_write_to_device(I2C_MASTER_NUM, AM2320_SENSOR_ADDR, cmd, 2, 1000 / portTICK_RATE_MS);
    vTaskDelay(pdMS_TO_TICKS(2));
    uint8_t data[8];
    i2c_master_read_from_device(I2C_MASTER_NUM, AM2320_SENSOR_ADDR, data, 8, 1000 / portTICK_RATE_MS);
    // Перевірка CRC пропущено
    *hum = ((data[2] << 8) | data[3]) / 10.0f;
    *temp = (((data[4] & 0x7F) << 8) | data[5]) / 10.0f;
    if (data[4] & 0x80) *temp = -*temp;
    ESP_LOGI(TAG, "AM2320: T=%.2f °C, H=%.2f %%", *temp, *hum);
    return ESP_OK;
}

/*
 * Ініціалізація CCS811 - зчитує дані IAQ
 * (реалізовано базове - початкове налаштування)
 */
static esp_err_t read_ccs811(uint16_t *co2, uint16_t *tvoc) {
    // Ініціалізація сенсора CCS811
    uint8_t init_cmd[2] = {0xF4, 0x00}; // Режим Idle
    i2c_master_write_to_device(I2C_MASTER_NUM, CCS811_SENSOR_ADDR, init_cmd, 2, 1000 / portTICK_RATE_MS);
    vTaskDelay(pdMS_TO_TICKS(100));
    // Встановлюємо режим вимірювання 1х/сек
    uint8_t meas_mode = 0x10; // DriveMode=1
    i2c_master_write_to_device(I2C_MASTER_NUM, CCS811_SENSOR_ADDR, &meas_mode, 1, 1000 / portTICK_RATE_MS);
    vTaskDelay(pdMS_TO_TICKS(100));
    // Зчитуємо дані
    uint8_t buf[8];
    i2c_master_read_from_device(I2C_MASTER_NUM, CCS811_SENSOR_ADDR, buf, 8, 1000 / portTICK_RATE_MS);
    *co2 = (buf[0] << 8) | buf[1];
    *tvoc = (buf[2] << 8) | buf[3];
    ESP_LOGI(TAG, "CCS811: CO2=%d ppm, TVOC=%d ppb", *co2, *tvoc);
    return ESP_OK;
}

/*
 * Ініціалізація I2C і повернення ESP_OK/ESP_ERR
 */
esp_err_t sensor_i2c_init(void) {
    return i2c_master_init();
}

/*
 * read_temperature: викликає read_am2320, повертає температуру
 */
float read_temperature(void) {
    float temp = 0.0f, hum = 0.0f;
    if (read_am2320(&temp, &hum) == ESP_OK) {
        return temp;
    }
    return NAN;
}

/*
 * read_humidity: повертає вологість
 */
float read_humidity(void) {
    float temp = 0.0f, hum = 0.0f;
    if (read_am2320(&temp, &hum) == ESP_OK) {
        return hum;
    }
    return NAN;
}

/*
 * read_co2: повертає CO2
 */
uint16_t read_co2(void) {
    uint16_t co2 = 0, tvoc = 0;
    read_ccs811(&co2, &tvoc);
    return co2;
}
