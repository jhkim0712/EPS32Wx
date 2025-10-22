#ifndef UI_APP_H
#define UI_APP_H

#include "esp_err.h"

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

#ifdef __cplusplus
}
#endif

#endif // UI_APP_H
