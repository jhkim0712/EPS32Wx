idf_component_register(
    SRCS "ota_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config app_update esp_partition
    PRIV_REQUIRES esp_https_ota esp_http_client esp-tls mbedtls cjson nvs_storage sys_util
)
