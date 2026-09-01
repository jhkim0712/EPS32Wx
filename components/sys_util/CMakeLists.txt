idf_component_register(
    SRCS "system_restart.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config
    PRIV_REQUIRES freertos esp_system
)
