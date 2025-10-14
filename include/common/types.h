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

// 날씨 데이터 구조체
typedef struct {
    char city[33];
    char description[65];
    float temperature;
    float feels_like;
    int humidity;
    int pressure;
    float wind_speed;
    int wind_deg;
    char icon[4];
    uint32_t timestamp;
} weather_data_t;

// 앱 설정 구조체
typedef struct {
    app_wifi_config_t wifi;
    char api_key[65];
    char city_name[33];
    bool first_boot;
    uint32_t update_interval;
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