set(CJSON_SRC_FILES
    "cJSON.c"
    "cJSON_Utils.c"
)
idf_component_register(
    SRCS ${CJSON_SRC_FILES}
    INCLUDE_DIRS "include"
)