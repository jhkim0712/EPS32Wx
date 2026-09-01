file(GLOB_RECURSE UI_EXTRA_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/screens/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/widgets/*.c"
)

idf_component_register(
    SRCS "ui_app.c" "screen_manager.c" ${UI_EXTRA_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES lvgl display app_config
    PRIV_REQUIRES nvs_storage sd_storage wifi_net ota_updater sys_util
)
