#ifndef CONSTANTS_H
#define CONSTANTS_H

// =============================================================================
// NVS 키 정의
// =============================================================================

// NVS 네임스페이스
#define NVS_NAMESPACE          "weather_cfg"

// WiFi 관련 NVS 키
#define NVS_KEY_WIFI_SSID      "wifi_ssid"
#define NVS_KEY_WIFI_PASS      "wifi_pass"
#define NVS_KEY_WIFI_CONFIGURED "wifi_configured"

// API 및 날씨 관련 NVS 키
#define NVS_KEY_API_KEY        "api_key"
#define NVS_KEY_CITY_NAME      "city_name"
#define NVS_WEATHER_API_KEY    NVS_KEY_API_KEY        // 호환성 유지
#define NVS_WEATHER_CITY       NVS_KEY_CITY_NAME      // 호환성 유지

// 시스템 설정 NVS 키
#define NVS_KEY_FIRST_BOOT     "first_boot"
#define NVS_KEY_UPDATE_INTERVAL "update_interval"

// =============================================================================
// WiFi 설정 상수
// =============================================================================

// WiFi 기본 설정
#define WIFI_SSID_MAX_LEN       32
#define WIFI_PASS_MAX_LEN       64
#define WIFI_CONNECT_TIMEOUT    10000  // 10초

// AP 모드 설정 (WiFi 설정 포털용)
#define DEFAULT_AP_SSID         "ESP32_Wx_Config"
#define DEFAULT_AP_PASSWORD     ""
#define AP_CHANNEL              1
#define AP_MAX_CONN             4

// 호환성을 위한 기존 매크로들
#define AP_SSID                 DEFAULT_AP_SSID
#define AP_PASSWORD             DEFAULT_AP_PASSWORD

// =============================================================================
// 날씨 API 설정 상수
// =============================================================================

// API 키 및 도시명 길이 제한
#define API_KEY_MAX_LEN         64
#define CITY_NAME_MAX_LEN       32

// 업데이트 간격 (밀리초)
#define UPDATE_INTERVAL_MS      600000  // 10분
#define DEFAULT_UPDATE_INTERVAL_SEC 300  // 5분

// OpenWeatherMap API URLs
#define OPENWEATHERMAP_CURRENT_API_URL "http://api.openweathermap.org/data/2.5/weather"
#define OPENWEATHERMAP_FORECAST_API_URL "http://api.openweathermap.org/data/2.5/forecast"
#define OPENWEATHERMAP_GEOCODING_API_URL "http://api.openweathermap.org/geo/1.0/direct"
#define OPENWEATHERMAP_API_TIMEOUT_MS 5000  // 5초

// =============================================================================
// 웹 서버 설정 상수
// =============================================================================

#define CONFIG_SERVER_PORT      80
#define WEB_SERVER_MAX_CONNECTIONS 4

// =============================================================================
// 시스템 설정 상수
// =============================================================================

// 태스크 스택 크기 (power of two, bytes 단위)
#define MAIN_TASK_STACK_SIZE    8192
#define WEATHER_TASK_STACK_SIZE 4096
#define UI_TASK_STACK_SIZE      4096

// 태스크 우선순위
#define MAIN_TASK_PRIORITY      5
#define WEATHER_TASK_PRIORITY   4
#define UI_TASK_PRIORITY        3

// 태스크 이름
#define MAIN_TASK_NAME          "main_task"
#define WEATHER_TASK_NAME       "weather_task"
#define UI_TASK_NAME            "ui_task"

// CPU 코어 설정
#define APP_TASK_CORE_ID       1   // 애플리케이션용 코어 (코어1)
#define SYSTEM_CORE_ID          0   // 시스템용 코어 (코어0)

// 시스템 타임아웃
#define SYSTEM_RESTART_DELAY_MS 5000
#define CONFIG_PORTAL_TIMEOUT_MS 300000  // 5분

// =============================================================================
// 하드웨어 설정 상수
// =============================================================================

// 디스플레이 설정
#define DISPLAY_WIDTH           320
#define DISPLAY_HEIGHT          480
#define DISPLAY_ROTATION        0

// 터치 스크린 설정
#define TOUCH_I2C_SDA_PIN       6
#define TOUCH_I2C_SCL_PIN       5
#define TOUCH_I2C_ADDR          0x38

// =============================================================================
// 로그 태그 상수
// =============================================================================

#define LOG_TAG_MAIN            "MAIN"
#define LOG_TAG_WIFI            "WIFI_MANAGER"
#define LOG_TAG_WEATHER         "WEATHER_TASK"
#define LOG_TAG_UI              "UI_APP"
#define LOG_TAG_NVS             "NVS_MANAGER"
#define LOG_TAG_CONFIG_PORTAL   "CONFIG_PORTAL"
#define LOG_TAG_OTA             "OTA_MANAGER"
#define LOG_TAG_SPIFFS          "SPIFFS_MANAGER"

#endif // CONSTANTS_H