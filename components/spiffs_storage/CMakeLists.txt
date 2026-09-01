idf_component_register(
    SRCS "spiffs_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config
    PRIV_REQUIRES spiffs
)
