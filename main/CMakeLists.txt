# Register main component sources and include paths

# Collect sources recursively within main/ subfolders
file(GLOB_RECURSE MAIN_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/common/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/network/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/storage/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/weather/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/ui/*.c"
)

idf_component_register(
    SRCS        ${MAIN_SOURCES}
    INCLUDE_DIRS 
        "${CMAKE_CURRENT_LIST_DIR}/include"
    REQUIRES
        nvs_flash
        esp_wifi
        esp_netif
        esp_event
        esp_timer
        lvgl
        esp_http_client
        esp_http_server
        esp-tls
        mbedtls
        spiffs
        app_update
        esp_https_ota
        driver
        esp_lcd
        spi_flash
        cjson
)
