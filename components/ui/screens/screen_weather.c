// Weather 화면: 온도/습도/날씨상태 카드
#include "screens.h"
#include <stdio.h>
#include "common/constants.h"

static lv_obj_t *s_card = NULL;
static lv_obj_t *s_label_city = NULL;
static lv_obj_t *s_label_temp = NULL;
static lv_obj_t *s_label_hum = NULL;
static lv_obj_t *s_label_cond = NULL;

lv_obj_t *screen_weather_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    s_label_city = lv_label_create(scr);
    lv_obj_set_style_text_color(s_label_city, lv_color_white(), 0);
    lv_label_set_text(s_label_city, "Weather");
    lv_obj_align(s_label_city, LV_ALIGN_TOP_MID, 0, 16);

    s_card = lv_obj_create(scr);
    lv_obj_set_size(s_card, 260, 220);
    lv_obj_align(s_card, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_color(s_card, lv_color_hex(0x1E2A3A), 0);
    lv_obj_set_style_radius(s_card, 16, 0);
    lv_obj_clear_flag(s_card, LV_OBJ_FLAG_SCROLLABLE);

    s_label_temp = lv_label_create(s_card);
#if LV_FONT_MONTSERRAT_28
    lv_obj_set_style_text_font(s_label_temp, &lv_font_montserrat_28, 0);
#endif
    lv_obj_set_style_text_color(s_label_temp, lv_color_white(), 0);
    lv_label_set_text(s_label_temp, "--.- \xC2\xB0" "C");
    lv_obj_align(s_label_temp, LV_ALIGN_TOP_MID, 0, 20);

    s_label_hum = lv_label_create(s_card);
    lv_obj_set_style_text_color(s_label_hum, lv_color_hex(0xB0D0FF), 0);
    lv_label_set_text(s_label_hum, "Humidity: -- %");
    lv_obj_align(s_label_hum, LV_ALIGN_CENTER, 0, 10);

    s_label_cond = lv_label_create(s_card);
    lv_obj_set_style_text_color(s_label_cond, lv_color_hex(0xB0D0FF), 0);
    lv_label_set_text(s_label_cond, "Condition: --");
    lv_obj_align(s_label_cond, LV_ALIGN_BOTTOM_MID, 0, -20);

    return scr;
}

void screen_weather_set_data(float temperature, int humidity, const char *condition_desc)
{
    if (!condition_desc) condition_desc = "";

    if (s_label_temp) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f \xC2\xB0" "C", temperature);
        lv_label_set_text(s_label_temp, buf);
    }
    if (s_label_hum) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Humidity: %d %%", humidity);
        lv_label_set_text(s_label_hum, buf);
    }
    if (s_label_cond) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Condition: %s", condition_desc);
        lv_label_set_text(s_label_cond, buf);
    }

    // Clock 화면 하단 요약 줄도 같은 데이터로 갱신
    screen_clock_set_weather_summary(temperature, condition_desc);
}
