idf_component_register(
    SRCS "nvs_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config esp_wifi
    PRIV_REQUIRES nvs_flash
)
