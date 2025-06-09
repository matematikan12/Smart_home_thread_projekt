#pragma once

#include <stdint.h>

/* Зовнішні змінні, що містять PEM-сертифікати */
extern const uint8_t broker_ca_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t broker_ca_pem_end[]   asm("_binary_ca_cert_pem_end");
extern const uint8_t client_cert_pem_start[] asm("_binary_client_cert_pem_start");
extern const uint8_t client_cert_pem_end[]   asm("_binary_client_cert_pem_end");
extern const uint8_t client_key_pem_start[] asm("_binary_client_key_pem_start");
extern const uint8_t client_key_pem_end[]   asm("_binary_client_key_pem_end");

/*
 * mqtt_init: ініціалізує MQTT-клієнт із TLS
 */
void mqtt_init(const char *broker_uri, const char *client_id);

/*
 * mqtt_publish: публікує payload у топік topic
 */
void mqtt_publish(const char *topic, const char *payload);
