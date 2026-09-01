#ifndef UI_SCREEN_MANAGER_H
#define UI_SCREEN_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCREEN_CLOCK = 0,
    SCREEN_WEATHER,
    SCREEN_GALLERY,
    SCREEN_SETTINGS,
    SCREEN_COUNT
} app_screen_id_t;

/**
 * @brief Clock/Weather/Gallery/Settings 화면과 하단 내비게이션 바를 생성하고
 *        SCREEN_CLOCK을 표시한다. lvgl_driver_init() 이후, LVGL 락을 호출자가
 *        쥔 상태에서 호출해야 한다 (다른 ui_app.c 진입점들과 동일한 규약).
 */
esp_err_t screen_manager_init(void);

/**
 * @brief 지정한 화면으로 전환한다.
 * @param animate true면 좌우 슬라이드 애니메이션 사용
 */
void screen_manager_show(app_screen_id_t id, bool animate);

app_screen_id_t screen_manager_current(void);

// 현재 화면 기준 다음/이전 화면으로 순환 이동 (스와이프 제스처에서 사용)
void screen_manager_next(void);
void screen_manager_prev(void);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_MANAGER_H
