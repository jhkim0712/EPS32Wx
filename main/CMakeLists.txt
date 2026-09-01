# Thin orchestrator component: main.c only wires the other components
# together at boot. All feature logic lives in components/.
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        app_config
        sys_util
        nvs_storage
        spiffs_storage
        sd_storage
        display
        ui
        wifi_net
        webui
        weather
        ota_updater
        nvs_flash
        esp_timer
        esp_netif
        lwip
        spi_flash
)
