// ui 컴포넌트 내부 전용 헤더 (public include/ 에는 두지 않음).
// 각 화면(screen_*.c)의 생성 함수와, 외부 데이터를 받아 갱신하는 함수를 선언한다.
#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// 각 화면은 lv_obj_create(NULL)로 독립된 screen 객체를 만들어 반환한다.
// 위젯 생성과 화면 자체의 lv_timer 등록(시계 tick, 슬라이드쇼 타이머)까지 여기서 끝낸다.
lv_obj_t *screen_clock_create(void);
lv_obj_t *screen_weather_create(void);
lv_obj_t *screen_gallery_create(void);
lv_obj_t *screen_settings_create(void);

// weather_task 등 외부에서 새 날씨 데이터가 도착했을 때 호출 (screen_weather.c 구현)
void screen_weather_set_data(float temperature, int humidity, const char *condition_desc);

// screen_clock.c가 화면 하단에 표시하는 요약 정보도 같은 데이터를 공유한다.
void screen_clock_set_weather_summary(float temperature, const char *condition_desc);

// Settings 화면에서 슬라이드쇼 간격/사용 여부가 바뀌면 호출 (screen_gallery.c 구현)
void screen_gallery_set_interval(uint16_t seconds);
void screen_gallery_set_enabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREENS_H
