#ifndef CONFIG_H
#define CONFIG_H

// WiFi 설정
#define WIFI_SSID_MAX_LEN       32
#define WIFI_PASS_MAX_LEN       64
#define WIFI_CONNECT_TIMEOUT    10000  // 10초

// AP 모드 설정 (WiFi 설정 포털용)
#define AP_SSID                 "ESP32_Wx_Config"
#define AP_PASSWORD             ""
#define AP_CHANNEL              1
#define AP_MAX_CONN             4

// 웹 서버 설정
#define CONFIG_SERVER_PORT      80

// 날씨 API 설정
#define API_KEY_MAX_LEN         64
#define CITY_NAME_MAX_LEN       32
#define UPDATE_INTERVAL_MS      600000  // 10분

/* OpenWeatherMap API 설정 */
#define OPENWEATHERMAP_CURRENT_API_URL "http://api.openweathermap.org/data/2.5/weather"
#define OPENWEATHERMAP_FORECAST_API_URL "http://api.openweathermap.org/data/2.5/forecast"
#define OPENWEATHERMAP_GEOCODING_API_URL "http://api.openweathermap.org/geo/1.0/direct"
#define OPENWEATHERMAP_API_TIMEOUT_MS 5000  // 5초

// NVS 네임스페이스
#define NVS_NAMESPACE          "weather_cfg"
#define NVS_KEY_WIFI_SSID      "wifi_ssid"
#define NVS_KEY_WIFI_PASS      "wifi_pass"
#define NVS_KEY_API_KEY        "api_key"
#define NVS_KEY_CITY_NAME      "city_name"
#define NVS_KEY_FIRST_BOOT     "first_boot"

// SPIFFS 설정
#define SPIFFS_PARTITION_LABEL  "spiffs"
#define WEB_FILES_PATH          "/web"

// OTA 업데이트 설정
#define OTA_PARTITION_SIZE      0x180000     // 1.5MB per partition
#define OTA_URL_MAX_LEN         256
#define OTA_TIMEOUT_MS          30000        // 30초
#define FIRMWARE_VERSION        "1.0.0"

// ESP32-S3 특화 설정
#define ESP32S3_FLASH_SIZE      8*1024*1024  // 8MB
#define ESP32S3_PSRAM_SIZE      8*1024*1024  // 8MB

// 디스플레이 설정 (ESP32-S3-WROOM-1용)
#define DISPLAY_WIDTH           320
#define DISPLAY_HEIGHT          240
#define DISPLAY_SPI_HOST        SPI2_HOST

#endif // CONFIG_H