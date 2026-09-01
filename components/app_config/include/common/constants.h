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

// System 탭 / 온디바이스 설정 관련 NVS 키 (15자 이내, NVS 키 길이 제한)
#define NVS_KEY_BRIGHTNESS       "brightness"
#define NVS_KEY_ROTATION         "rotation"
#define NVS_KEY_TIMEZONE         "tz"
#define NVS_KEY_NIGHT_DIM_EN     "nd_en"
#define NVS_KEY_NIGHT_DIM_START  "nd_start"
#define NVS_KEY_NIGHT_DIM_END    "nd_end"
#define NVS_KEY_NIGHT_DIM_BRIGHT "nd_bright"
#define NVS_KEY_WEB_AUTH_EN      "web_auth_en"
#define NVS_KEY_WEB_AUTH_USER    "web_user"
#define NVS_KEY_WEB_AUTH_PASS    "web_pass"
#define NVS_KEY_SLIDESHOW_EN     "slide_en"
#define NVS_KEY_SLIDESHOW_INT    "slide_int"
#define NVS_KEY_OTA_MANIFEST_URL "ota_manifest"
#define NVS_KEY_OTA_AUTO_CHECK   "ota_auto"

// 기본값
#define DEFAULT_TIMEZONE_POSIX          "UTC0"
#define DEFAULT_NIGHT_DIM_START_HOUR    22
#define DEFAULT_NIGHT_DIM_END_HOUR      7
#define DEFAULT_NIGHT_DIM_BRIGHTNESS    20
#define DEFAULT_SLIDESHOW_INTERVAL_SEC  5

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
#define DEFAULT_AP_IP_ADDR      "192.168.4.1"
#define DEFAULT_AP_NETMASK      "255.255.255.0"

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
#define WEATHER_TASK_STACK_SIZE 4096
#define UI_TASK_STACK_SIZE      4096

// 태스크 우선순위
#define WEATHER_TASK_PRIORITY   4
#define UI_TASK_PRIORITY        3

// 태스크 이름
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
// 보드 배선/핀 번호는 board_pins.h로 분리되어 있다 (다른 보드로 포팅 시 그 파일만
// 바꾸면 됨). 여기서는 그 핀들을 사용하는 소프트웨어 쪽 튜닝 값만 남긴다.
#include "common/board_pins.h"

#define DEFAULT_BRIGHTNESS_PERCENT 80

// LVGL 렌더 버퍼 설정 (라인 수)
// 120(320*120*2B=76.8KB)으로 키웠을 때 실기기에서 "gdma_link_mount_buffers: no
// more space for buffer mounting" -> ISR 안 abort() 크래시가 났었다. 버퍼 크기를
// 다시 40으로 줄여도, 더블버퍼를 꺼도 크래시가 그대로여서 범인이 크기가 아니라는
// 게 드러났다 — 진짜 원인은 lvgl_driver.c에서 이 버퍼를 PSRAM에 할당하고 있었던
// 것(i80 LCD의 GDMA가 직접 읽어가는 버퍼는 내부 DMA RAM에 있어야 함, 지금은
// 고쳐짐). 그 문제가 해결됐으니 이 값은 다시 키워도 되지만, 지금 값(40)이 안전
// 확인된 것이라 일단 유지 — 갤러리 성능 보고 키울 것.
#define LVGL_BUFFER_LINES       40
#define LVGL_TICK_PERIOD_MS      5
#define LVGL_TASK_PERIOD_MS      10
#define LVGL_TASK_STACK_SIZE     4096
#define LVGL_TASK_PRIORITY       5
#define LVGL_TASK_CORE_ID        1

// SD카드 설정 (핀/SPI 호스트는 board_pins.h)
#define SD_MOUNT_POINT          "/sdcard"
#define SD_PHOTOS_SUBDIR        "/photos"
#define SD_SPI_MAX_FILES        5
#define SD_SPI_MAX_TRANSFER_SZ  4000

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
#define LOG_TAG_SYS_UTIL        "SYS_UTIL"
#define LOG_TAG_SD              "SD_STORAGE"
#define LOG_TAG_DISPLAY         "DISPLAY"
#define LOG_TAG_CONSOLE         "UART_CONSOLE"

// =============================================================================
// 폰트 설정 (한글 표시용)
// =============================================================================
// 한글 표시를 위해 커스텀 폰트를 사용할지 여부 (0: 사용 안 함, 1: 사용)
// 1로 설정하고, ui/fonts 디렉토리에 lv_font_conv로 생성한 한글 폰트 C 파일을 추가하세요.
#ifndef USE_FONT_KOREAN
#define USE_FONT_KOREAN         0
#endif

#endif // CONSTANTS_H