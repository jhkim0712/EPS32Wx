idf_component_register(
    SRCS "sd_card_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config
    PRIV_REQUIRES fatfs sdmmc driver
)
