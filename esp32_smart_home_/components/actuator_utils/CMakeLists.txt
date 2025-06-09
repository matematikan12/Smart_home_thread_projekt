idf_component_register(SRCS "actuator_utils.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_openthread esp_mqtt cjson)