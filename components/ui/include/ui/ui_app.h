#ifndef UI_APP_H
#define UI_APP_H

#include "esp_err.h"
#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize/start the UI application layer.
 * Safe to call multiple times. Returns ESP_OK on success.
 */
esp_err_t ui_app_start(void);

/**
 * Update the weather information on the UI.
 * Parameters:
 *  - temperature: Celsius
 *  - humidity: percentage (0~100)
 *  - condition_desc: short description (e.g., "Clear", "Rain")
 */
void ui_update_weather_info(float temperature, int humidity, const char* condition_desc);

/**
 * Show WiFi configuration instructions (AP mode info) on the screen.
 * If UI is not started yet, it will start it.
 * @param ap_ssid AP SSID to display
 * @param ap_password AP password to display (can be empty for open AP)
 * @param portal_url URL or IP to open in browser (e.g., "http://192.168.4.1")
 */
void ui_show_config_portal_info(const char* ap_ssid, const char* ap_password, const char* portal_url);

/**
 * NVS에서 불러온 설정(밝기/회전/슬라이드쇼)을 UI 레이어(백라이트/디스플레이/갤러리)에
 * 적용한다. main.c가 부팅 시(및 웹 설정 변경 후 필요하다면) 호출한다.
 * UI가 아직 시작되지 않았으면 아무 것도 하지 않는다.
 */
void ui_apply_config(const app_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif // UI_APP_H
