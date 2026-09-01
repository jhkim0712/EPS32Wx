idf_component_register(
    SRCS "config_portal.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config
    PRIV_REQUIRES esp_http_server cjson nvs_storage spiffs_storage sd_storage wifi_net display ota_updater
)
