// 온디바이스 화면 관리자: Clock/Weather/Gallery/Settings 화면을 미리 만들어두고
// 스와이프 제스처 또는 하단 내비게이션 바 탭으로 전환한다.
// (lv_tileview/lv_tabview 대신 직접 구현 — 터치 제스처를 세밀하게 제어하기 위함)
#include "ui/screen_manager.h"
#include "screens/screens.h"

#include "esp_log.h"
#include "common/constants.h"

static const char *TAG = LOG_TAG_UI;

#define NAV_BAR_HEIGHT 36

static lv_obj_t *s_screens[SCREEN_COUNT] = {0};
static lv_obj_t *s_nav_buttons[SCREEN_COUNT] = {0};
static app_screen_id_t s_current = SCREEN_CLOCK;
static bool s_inited = false;

static const char *s_nav_labels[SCREEN_COUNT] = {
    "\xEC\x8B\x9C\xEA\xB3\x84",   // 시계
    "\xEB\x82\xA0\xEC\x94\xA8",   // 날씨
    "\xEC\x82\xAC\xEC\xA7\x84",   // 사진
    "\xEC\x84\xA4\xEC\xA0\x95",   // 설정
};

static void nav_button_event_cb(lv_event_t *e)
{
    app_screen_id_t id = (app_screen_id_t)(intptr_t)lv_event_get_user_data(e);
    screen_manager_show(id, true);
}

static void update_nav_button_styles(void)
{
    for (int i = 0; i < SCREEN_COUNT; ++i) {
        if (!s_nav_buttons[i]) continue;
        if (i == (int)s_current) {
            lv_obj_set_style_bg_color(s_nav_buttons[i], lv_palette_main(LV_PALETTE_BLUE), 0);
            lv_obj_set_style_text_color(s_nav_buttons[i], lv_color_white(), 0);
        } else {
            lv_obj_set_style_bg_color(s_nav_buttons[i], lv_color_hex(0x303030), 0);
            lv_obj_set_style_text_color(s_nav_buttons[i], lv_color_hex(0xB0B0B0), 0);
        }
    }
}

static void build_nav_bar(void)
{
    lv_obj_t *bar = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, DISPLAY_WIDTH, NAV_BAR_HEIGHT);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x181818), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_90, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < SCREEN_COUNT; ++i) {
        lv_obj_t *btn = lv_btn_create(bar);
        lv_obj_set_size(btn, DISPLAY_WIDTH / SCREEN_COUNT - 8, NAV_BAR_HEIGHT - 6);
        lv_obj_add_event_cb(btn, nav_button_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, s_nav_labels[i]);
        lv_obj_center(label);

        s_nav_buttons[i] = btn;
    }

    update_nav_button_styles();
}

// 화면(screen) 위에서 좌우 스와이프하면 다음/이전 화면으로 전환한다.
static void screen_gesture_event_cb(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_LEFT) {
        screen_manager_next();
    } else if (dir == LV_DIR_RIGHT) {
        screen_manager_prev();
    }
}

esp_err_t screen_manager_init(void)
{
    if (s_inited) {
        ESP_LOGI(TAG, "Screen manager already initialized");
        return ESP_OK;
    }

    s_screens[SCREEN_CLOCK] = screen_clock_create();
    s_screens[SCREEN_WEATHER] = screen_weather_create();
    s_screens[SCREEN_GALLERY] = screen_gallery_create();
    s_screens[SCREEN_SETTINGS] = screen_settings_create();

    for (int i = 0; i < SCREEN_COUNT; ++i) {
        if (!s_screens[i]) {
            ESP_LOGE(TAG, "Failed to create screen %d", i);
            return ESP_FAIL;
        }
        lv_obj_add_event_cb(s_screens[i], screen_gesture_event_cb, LV_EVENT_GESTURE, NULL);
    }

    build_nav_bar();

    s_current = SCREEN_CLOCK;
    lv_scr_load(s_screens[SCREEN_CLOCK]);
    s_inited = true;

    ESP_LOGI(TAG, "Screen manager initialized (%d screens)", SCREEN_COUNT);
    return ESP_OK;
}

void screen_manager_show(app_screen_id_t id, bool animate)
{
    if (!s_inited || id >= SCREEN_COUNT || !s_screens[id]) return;
    if (id == s_current) return;

    app_screen_id_t prev = s_current;
    s_current = id;

    if (animate) {
        lv_scr_load_anim_t anim = (id > prev) ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
        lv_scr_load_anim(s_screens[id], anim, 200, 0, false);
    } else {
        lv_scr_load(s_screens[id]);
    }

    update_nav_button_styles();
}

app_screen_id_t screen_manager_current(void)
{
    return s_current;
}

void screen_manager_next(void)
{
    app_screen_id_t next = (app_screen_id_t)((s_current + 1) % SCREEN_COUNT);
    screen_manager_show(next, true);
}

void screen_manager_prev(void)
{
    app_screen_id_t prev = (app_screen_id_t)((s_current + SCREEN_COUNT - 1) % SCREEN_COUNT);
    screen_manager_show(prev, true);
}
