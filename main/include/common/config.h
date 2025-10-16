#ifndef CONFIG_H
#define CONFIG_H

// 공통 상수 정의 포함
#include "constants.h"

// SPIFFS 설정
#define SPIFFS_PARTITION_LABEL  "spiffs"
#define WEB_FILES_PATH          "/web"

// OTA 업데이트 설정
#define OTA_PARTITION_SIZE      0x180000     // 1.5MB per partition
#define OTA_URL_MAX_LEN         256
#define OTA_TIMEOUT_MS          30000        // 30초
#define FIRMWARE_VERSION        "1.0.0"

// ESP32-S3 특화 설정
#define ESP32S3_FLASH_SIZE      16*1024*1024  // 16MB (WT32-SC01-PLUS)
#define ESP32S3_PSRAM_SIZE      2*1024*1024   // 2MB PSRAM

// 디스플레이 설정 (WT32-SC01-PLUS용 - 3.5" 320x480 ST7796)
#define DISPLAY_SPI_HOST        SPI2_HOST

#endif // CONFIG_H