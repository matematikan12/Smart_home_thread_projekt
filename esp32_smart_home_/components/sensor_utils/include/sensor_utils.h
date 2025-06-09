#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

/*
 * Ініціалізує I2C для сенсорів, повертає ESP_OK/ESP_ERR
 */
esp_err_t sensor_i2c_init(void);

/*
 * read_light: зчитує освітленість у люксах (BH1750)
 */
uint16_t read_light(void);

/*
 * read_temperature: зчитує температуру у °C (AM2320)
 */
float read_temperature(void);

/*
 * read_humidity: зчитує вологість у % (AM2320)
 */
float read_humidity(void);

/*
 * read_co2: зчитує CO2 у ppm (CCS811)
 */
uint16_t read_co2(void);

/*
 * read_motion: повертає true, якщо виявлено рух (PIR, реалізація залежить від GPIO)
 */
bool read_motion(void);

/*
 * read_leak: повертає true, якщо виявлено витік води (реалізація GPIO)
 */
bool read_leak(void);
