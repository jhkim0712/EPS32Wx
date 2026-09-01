idf_component_register(
    SRCS "weather_interface.c" "weather_task.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config
    PRIV_REQUIRES esp_http_client cjson esp_timer nvs_storage wifi_net ui
)
