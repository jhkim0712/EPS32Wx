#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

// WiFi 설정 구조체
typedef struct {
    char ssid[33];
    char password[65];
    bool configured;
} app_wifi_config_t;

// 앱 설정 구조체
typedef struct {
    app_wifi_config_t wifi;
    char api_key[65];
    char city_name[33];
    bool first_boot;
    uint32_t update_interval;

    // System 탭 / 온디바이스 설정 화면
    uint8_t  brightness_percent;              // 0-100, 기본 DEFAULT_BRIGHTNESS_PERCENT
    uint16_t display_rotation_deg;             // 0/90/180/270, 기본 DISPLAY_ROTATION
    char     timezone_posix[48];               // 예: "KST-9"
    bool     night_dim_enabled;
    uint8_t  night_dim_start_hour;             // 0-23
    uint8_t  night_dim_end_hour;               // 0-23
    uint8_t  night_dim_brightness_percent;     // 0-100

    // 웹 포털 기본 인증 (기본 비활성 — 초기 기기가 잠기지 않도록)
    bool     web_auth_enabled;
    char     web_auth_user[33];
    char     web_auth_pass[65];

    // Pictures 탭 / 디지털 액자 슬라이드쇼
    bool     slideshow_enabled;
    uint16_t slideshow_interval_sec;           // 기본 5

    // GitHub 매니페스트 기반 OTA
    char     ota_manifest_url[256];
    bool     ota_auto_check;
} app_config_t;

// WiFi 상태
typedef enum {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_AP_MODE,
    WIFI_STATE_ERROR
} wifi_state_t;

// 앱 상태
typedef enum {
    APP_STATE_INIT,
    APP_STATE_CONFIG_MODE,
    APP_STATE_WIFI_CONNECTING,
    APP_STATE_NORMAL_OPERATION,
    APP_STATE_ERROR
} app_state_t;

#endif // TYPES_H