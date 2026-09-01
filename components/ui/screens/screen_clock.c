// Clock 화면: 큰 시:분 표시 + 날짜 + 하단 날씨 요약 한 줄
#include "screens.h"
#include <stdio.h>
#include <time.h>
#include "common/constants.h"

static lv_obj_t *s_time_label = NULL;
static lv_obj_t *s_date_label = NULL;
static lv_obj_t *s_weather_label = NULL;

static const char *s_weekday_kr[7] = {
    "\xEC\x9D\xBC", "\xEC\x9B\x94", "\xED\x99\x94", "\xEC\x88\x98", "\xEB\xAA\xA9", "\xEA\xB8\x88", "\xED\x86\xA0" // 일 월 화 수 목 금 토
};

static void clock_tick_cb(lv_timer_t *timer)
{
    (void)timer;
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", tm_now.tm_hour, tm_now.tm_min);
    lv_label_set_text(s_time_label, buf);

    char dbuf[48];
    snprintf(dbuf, sizeof(dbuf), "%04d-%02d-%02d (%s)",
             tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
             s_weekday_kr[tm_now.tm_wday]);
    lv_label_set_text(s_date_label, dbuf);
}

lv_obj_t *screen_clock_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    s_time_label = lv_label_create(scr);
#if LV_FONT_MONTSERRAT_36
    lv_obj_set_style_text_font(s_time_label, &lv_font_montserrat_36, 0);
#endif
    lv_obj_set_style_text_color(s_time_label, lv_color_white(), 0);
    lv_label_set_text(s_time_label, "--:--");
    lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, -40);

    s_date_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_date_label, lv_color_hex(0xB0B0B0), 0);
    lv_label_set_text(s_date_label, "---------------");
    lv_obj_align_to(s_date_label, s_time_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);

    s_weather_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_weather_label, lv_color_hex(0x80C0FF), 0);
    lv_label_set_text(s_weather_label, "");
    lv_obj_align(s_weather_label, LV_ALIGN_BOTTOM_MID, 0, -48);

    lv_timer_create(clock_tick_cb, 1000, NULL);
    clock_tick_cb(NULL); // 즉시 한 번 갱신

    return scr;
}

void screen_clock_set_weather_summary(float temperature, const char *condition_desc)
{
    if (!s_weather_label) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "%.1f\xC2\xB0" "C  %s", temperature, condition_desc ? condition_desc : "");
    lv_label_set_text(s_weather_label, buf);
}
