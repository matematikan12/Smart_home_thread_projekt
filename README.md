# Система керування розумним будинком на базі ESP32 & Thread

## Опис проєкту  
Цей проєкт реалізує високомодульну, енергоефективну та безпечну систему “Розумний дім” із використанням IoT-технологій:  
- **Thread (IEEE 802.15.4)** для надійної mesh-мережі сенсорів та актуаторів  
- **MQTT over TLS** для захищеного обміну даними з Home Assistant  
- **ESP32-H2 / ESP32-S3 + nRF52840** як апаратна платформа  
- **Home Assistant** для візуалізації, автоматизацій та керування

## Ключові можливості  
- **Сенсорні вузли** (ESP32-H2): температура/вологість, CO₂/TVOC, освітленість, рух, витік води із Deep-Sleep (< 10 µA)  
- **Виконавчі вузли** (ESP32-H2): керування реле та серводвигунами жалюзі/заслінок  
- **Центральний хаб** (ESP32-S3 + nRF52840): Thread Border Router → IPv6 + MQTT/TLS → Home Assistant  
- **Safe Mode**, **Watchdog**, **LWT** та **QoS 1** для відмовостійкості  
- **Plug-and-Play**: нові вузли додаються без зміни хаба

## Структура репозиторію
├── actuator_node/ # Код вузла-актуатора (ESP32-H2)
├── hub_esp32s3/ # Код центрального хаба (ESP32-S3)
├── sensor_node/ # Код сенсорного вузла (ESP32-H2)
├── components/ # Спільні утиліти (thread_utils, mqtt_utils, actuator_utils, sensor_utils)

## ПЗ та середовище  
- **Espressif ESP-IDF v4.x**  
- **OpenThread SDK**  
- **Mosquitto** (TLS)  
- **Home Assistant** (MQTT Integration, Lovelace UI)

**Налаштування ESP-IDF**
cd hub_esp32s3
idf.py set-target esp32s3
idf.py menuconfig    # вказати Wi-Fi, Thread PSK, MQTT TLS налаштування
idf.py build flash monitor

**Збірка сенсорних/актуаторних вузлів**
cd ../sensor_node
idf.py build flash monitor

cd ../actuator_node
idf.py build flash monitor
