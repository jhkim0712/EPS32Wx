#ifndef DISPLAY_BACKLIGHT_H
#define DISPLAY_BACKLIGHT_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LCD 백라이트 LEDC(PWM) 초기화. GPIO on/off 대신 밝기 조절이 가능하다.
 *        내부적으로 0%로 시작하며, 이후 backlight_set_percent()로 밝기를 올린다.
 * @return ESP_OK on success
 */
esp_err_t backlight_init(void);

/**
 * @brief 밝기를 즉시 설정한다 (LCD_BL_ACTIVE_HIGH를 반영해 duty를 매핑).
 * @param percent 0-100
 */
esp_err_t backlight_set_percent(uint8_t percent);

/**
 * @brief 마지막으로 설정한 밝기(%)를 반환한다.
 */
uint8_t backlight_get_percent(void);

/**
 * @brief 지정 시간(ms) 동안 부드럽게 밝기를 전환한다 (야간 감광 등에 사용).
 *        호출은 즉시 반환하며, 페이드는 LEDC 하드웨어가 백그라운드로 수행한다.
 */
esp_err_t backlight_fade_to(uint8_t percent, uint32_t fade_ms);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_BACKLIGHT_H
