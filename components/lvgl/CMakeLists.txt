# LVGL v8.3.11 component build script

# Collect all C sources under lvgl/src
file(GLOB_RECURSE LVGL_SRC_FILES "${CMAKE_CURRENT_LIST_DIR}/lvgl/src/*.c")

idf_component_register(
    SRCS        ${LVGL_SRC_FILES}
    INCLUDE_DIRS 
        "."            # for lv_conf.h
        "lvgl"
        "lvgl/src"
)

# Define to allow simple include of lv_conf.h
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_CONF_INCLUDE_SIMPLE)
