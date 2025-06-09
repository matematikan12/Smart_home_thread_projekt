#pragma once
#include <stdint.h>
#include <stddef.h>

/*
 * actuator_init: ініціалізує GPIO для реле та PWM для сервоприводів
 */
void actuator_init(void);

/*
 * relay_on: вмикає реле на каналі (1 або 2)
 */
void relay_on(uint8_t channel);

/*
 * relay_off: вимикає реле на каналі (1 або 2)
 */
void relay_off(uint8_t channel);

/*
 * servo_set_angle: задає кут сервопривода (0..180°)
 */
void servo_set_angle(uint8_t angle);

/*
 * compute_crc8: обчислення CRC-8 для масиву data довжиною length
 */
uint8_t compute_crc8(const uint8_t *data, size_t length);
