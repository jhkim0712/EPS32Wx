cmake_minimum_required(VERSION 3.16.0)

# Extra components (e.g., components/lvgl)
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/components)

# ESP-IDF project bootstrap
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(EPS32Wx)

# Create SPIFFS image from data/ for partition label 'spiffs'
# Requires a 'spiffs' partition in partitions.csv (present in this repo)
spiffs_create_partition_image(spiffs ${CMAKE_CURRENT_LIST_DIR}/data FLASH_IN_PROJECT)