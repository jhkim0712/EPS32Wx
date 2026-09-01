idf_component_register(
    SRCS "lvgl_driver.c" "backlight.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl app_config
    PRIV_REQUIRES driver esp_lcd esp_timer
)
