idf_component_register(
    SRCS "uart_console.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES app_config console wifi_net sd_storage display ota_updater sys_util
)
