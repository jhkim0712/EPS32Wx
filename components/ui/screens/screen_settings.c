// Settings 화면: 밝기/회전/WiFi/야간감광/OTA/SD/버전 정보를 한 화면에서 다룬다.
// 매니페스트 URL 등 긴 문자열 입력은 웹 System 탭 전용이며, 여기서는 트리거/상태만 제공한다.
#include "screens.h"
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "common/config.h"
#include "common/constants.h"
#include "common/types.h"
#include "display/backlight.h"
#include "ui/lvgl_driver.h"
#include "storage/nvs_manager.h"
#include "storage/sd_card_manager.h"
#include "network/wifi_manager.h"
#include "ota/ota_manager.h"
#include "util/system_restart.h"

static const char *TAG = LOG_TAG_UI;

static lv_obj_t *s_wifi_status_label = NULL;
static lv_obj_t *s_sd_status_label = NULL;
static lv_obj_t *s_ota_status_label = NULL;
static lv_obj_t *s_ota_bar = NULL;
static lv_timer_t *s_ota_poll_timer = NULL;

static lv_obj_t *make_section_title(lv_obj_t *parent, const char *text)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_color(l, lv_color_hex(0x60A0FF), 0);
    lv_label_set_text(l, text);
    return l;
}

static lv_obj_t *make_row(lv_obj_t *parent)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    return row;
}

// ---- 밝기 ----
static void brightness_slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    backlight_set_percent((uint8_t)val);

    if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
        nvs_manager_save_brightness((uint8_t)val); // 플래시 마모 방지: 뗐을 때만 저장
    }
}

// ---- 회전 ----
static void rotation_btn_event_cb(lv_event_t *e)
{
    int deg = (int)(intptr_t)lv_event_get_user_data(e);
    if (display_set_rotation(deg) == ESP_OK) {
        nvs_manager_save_rotation((uint16_t)deg);
    }
}

// ---- WiFi ----
static void wifi_reset_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ESP_LOGW(TAG, "WiFi reset requested from Settings screen");
    nvs_manager_clear_wifi_config();
    system_restart_delayed(1000);
}

static void refresh_wifi_status(void)
{
    if (!s_wifi_status_label) return;
    char buf[96];
    if (wifi_manager_is_connected()) {
        char ssid[33] = {0}, ip[32] = {0};
        wifi_manager_get_info(ssid, ip);
        snprintf(buf, sizeof(buf), "WiFi: %s (%s)", ssid, ip);
    } else if (wifi_manager_is_ap_mode()) {
        snprintf(buf, sizeof(buf), "WiFi: AP setup mode");
    } else {
        snprintf(buf, sizeof(buf), "WiFi: not connected");
    }
    lv_label_set_text(s_wifi_status_label, buf);
}

// ---- 야간 감광 ----
static void night_dim_switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    app_config_t cfg;
    nvs_manager_load_config(&cfg);
    nvs_manager_save_night_dim(enabled, cfg.night_dim_start_hour, cfg.night_dim_end_hour, cfg.night_dim_brightness_percent);
}

static void build_hour_roller_options(char *out, size_t out_len)
{
    out[0] = '\0';
    for (int h = 0; h < 24; ++h) {
        char item[8];
        snprintf(item, sizeof(item), "%02d\n", h);
        strncat(out, item, out_len - strlen(out) - 1);
    }
    size_t len = strlen(out);
    if (len > 0 && out[len - 1] == '\n') out[len - 1] = '\0'; // 마지막 개행 제거
}

static void night_dim_start_roller_event_cb(lv_event_t *e)
{
    uint16_t sel = lv_roller_get_selected(lv_event_get_target(e));
    app_config_t cfg;
    nvs_manager_load_config(&cfg);
    nvs_manager_save_night_dim(cfg.night_dim_enabled, (uint8_t)sel, cfg.night_dim_end_hour, cfg.night_dim_brightness_percent);
}

static void night_dim_end_roller_event_cb(lv_event_t *e)
{
    uint16_t sel = lv_roller_get_selected(lv_event_get_target(e));
    app_config_t cfg;
    nvs_manager_load_config(&cfg);
    nvs_manager_save_night_dim(cfg.night_dim_enabled, cfg.night_dim_start_hour, (uint8_t)sel, cfg.night_dim_brightness_percent);
}

// ---- SD 카드 ----
static void refresh_sd_status(void)
{
    if (!s_sd_status_label) return;
    char buf[64];
    if (sd_card_is_mounted()) {
        uint64_t total = 0, freeb = 0;
        sd_card_get_info(&total, &freeb);
        snprintf(buf, sizeof(buf), "SD: %llu/%llu MB free",
                 (unsigned long long)(freeb / (1024 * 1024)), (unsigned long long)(total / (1024 * 1024)));
    } else {
        snprintf(buf, sizeof(buf), "SD: not mounted");
    }
    lv_label_set_text(s_sd_status_label, buf);
}

// ---- OTA ----
static void ota_poll_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    ota_status_t status = ota_manager_get_status();
    int progress = ota_manager_get_progress();

    if (s_ota_bar) lv_bar_set_value(s_ota_bar, progress, LV_ANIM_ON);

    const char *text = "";
    switch (status) {
        case OTA_STATUS_IDLE:        text = "OTA: idle"; break;
        case OTA_STATUS_DOWNLOADING: text = "OTA: downloading"; break;
        case OTA_STATUS_INSTALLING:  text = "OTA: installing"; break;
        case OTA_STATUS_SUCCESS:     text = "OTA: done, restarting..."; break;
        case OTA_STATUS_FAILED:      text = "OTA: failed"; break;
        default: break;
    }
    if (s_ota_status_label && text[0]) lv_label_set_text(s_ota_status_label, text);
}

static void ota_check_btn_event_cb(lv_event_t *e)
{
    (void)e;
    esp_err_t err = ota_manager_check_update(NULL); // NULL -> NVS에 저장된 매니페스트 URL 사용
    if (err == ESP_OK) {
        ota_info_t info;
        ota_manager_get_info(&info);
        char buf[64];
        snprintf(buf, sizeof(buf), "New version %s available", info.available_version);
        lv_label_set_text(s_ota_status_label, buf);
    } else if (err == ESP_ERR_NOT_FOUND) {
        lv_label_set_text(s_ota_status_label, "OTA: up to date");
    } else if (err == ESP_ERR_INVALID_STATE) {
        lv_label_set_text(s_ota_status_label, "OTA: no manifest URL set");
    } else {
        lv_label_set_text(s_ota_status_label, "OTA: check failed");
    }
}

static void ota_update_btn_event_cb(lv_event_t *e)
{
    (void)e;
    ota_manager_start_update(NULL); // NULL -> check_update()가 채운 펌웨어 URL 사용
}

// 설정 화면으로 돌아올 때마다 WiFi/SD 상태를 새로 읽어 표시한다.
static void settings_screen_loaded_cb(lv_event_t *e)
{
    (void)e;
    refresh_wifi_status();
    refresh_sd_status();
}

lv_obj_t *screen_settings_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(scr, 10, 0);
    lv_obj_set_style_pad_row(scr, 8, 0);
    lv_obj_set_style_pad_bottom(scr, 44, 0); // 하단 내비게이션 바에 가리지 않도록

    app_config_t cfg;
    nvs_manager_load_config(&cfg);

    // ---- 밝기 ----
    make_section_title(scr, "Brightness");
    lv_obj_t *brightness_slider = lv_slider_create(scr);
    lv_obj_set_width(brightness_slider, lv_pct(100));
    lv_slider_set_range(brightness_slider, 0, 100);
    lv_slider_set_value(brightness_slider, cfg.brightness_percent, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightness_slider, brightness_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(brightness_slider, brightness_slider_event_cb, LV_EVENT_RELEASED, NULL);

    // ---- 회전 ----
    make_section_title(scr, "Rotation");
    lv_obj_t *rot_row = make_row(scr);
    const int rotations[4] = {0, 90, 180, 270};
    for (int i = 0; i < 4; ++i) {
        lv_obj_t *btn = lv_btn_create(rot_row);
        lv_obj_set_size(btn, 60, 32);
        lv_obj_add_event_cb(btn, rotation_btn_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)rotations[i]);
        lv_obj_t *l = lv_label_create(btn);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d\xC2\xB0", rotations[i]);
        lv_label_set_text(l, buf);
        lv_obj_center(l);
    }

    // ---- WiFi ----
    make_section_title(scr, "WiFi");
    lv_obj_t *wifi_row = make_row(scr);
    s_wifi_status_label = lv_label_create(wifi_row);
    lv_obj_set_style_text_color(s_wifi_status_label, lv_color_white(), 0);
    lv_obj_t *wifi_reset_btn = lv_btn_create(wifi_row);
    lv_obj_add_event_cb(wifi_reset_btn, wifi_reset_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *wifi_reset_label = lv_label_create(wifi_reset_btn);
    lv_label_set_text(wifi_reset_label, "Reset");

    // ---- 야간 감광 ----
    make_section_title(scr, "Night Dimming");
    lv_obj_t *nd_row = make_row(scr);
    lv_obj_t *nd_switch = lv_switch_create(nd_row);
    if (cfg.night_dim_enabled) lv_obj_add_state(nd_switch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(nd_switch, night_dim_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    char hour_opts[24 * 3 + 1];
    build_hour_roller_options(hour_opts, sizeof(hour_opts));

    lv_obj_t *nd_start_roller = lv_roller_create(nd_row);
    lv_roller_set_options(nd_start_roller, hour_opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(nd_start_roller, 1);
    lv_roller_set_selected(nd_start_roller, cfg.night_dim_start_hour, LV_ANIM_OFF);
    lv_obj_add_event_cb(nd_start_roller, night_dim_start_roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *nd_tilde = lv_label_create(nd_row);
    lv_label_set_text(nd_tilde, "~");

    lv_obj_t *nd_end_roller = lv_roller_create(nd_row);
    lv_roller_set_options(nd_end_roller, hour_opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(nd_end_roller, 1);
    lv_roller_set_selected(nd_end_roller, cfg.night_dim_end_hour, LV_ANIM_OFF);
    lv_obj_add_event_cb(nd_end_roller, night_dim_end_roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // ---- OTA ----
    make_section_title(scr, "Firmware Update (OTA)");
    s_ota_status_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_ota_status_label, lv_color_hex(0xB0B0B0), 0);
    lv_label_set_text(s_ota_status_label, "OTA: idle");

    s_ota_bar = lv_bar_create(scr);
    lv_obj_set_size(s_ota_bar, lv_pct(100), 12);
    lv_bar_set_range(s_ota_bar, 0, 100);
    lv_bar_set_value(s_ota_bar, 0, LV_ANIM_OFF);

    lv_obj_t *ota_row = make_row(scr);
    lv_obj_t *ota_check_btn = lv_btn_create(ota_row);
    lv_obj_add_event_cb(ota_check_btn, ota_check_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ota_check_label = lv_label_create(ota_check_btn);
    lv_label_set_text(ota_check_label, "Check");

    lv_obj_t *ota_update_btn = lv_btn_create(ota_row);
    lv_obj_add_event_cb(ota_update_btn, ota_update_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ota_update_label = lv_label_create(ota_update_btn);
    lv_label_set_text(ota_update_label, "Update");

    s_ota_poll_timer = lv_timer_create(ota_poll_timer_cb, 1000, NULL);

    // ---- SD 카드 / 버전 정보 ----
    lv_obj_t *bottom_row = make_row(scr);
    s_sd_status_label = lv_label_create(bottom_row);
    lv_obj_set_style_text_color(s_sd_status_label, lv_color_hex(0x909090), 0);

    lv_obj_t *version_label = lv_label_create(bottom_row);
    lv_obj_set_style_text_color(version_label, lv_color_hex(0x909090), 0);
    char vbuf[32];
    snprintf(vbuf, sizeof(vbuf), "FW %s", FIRMWARE_VERSION);
    lv_label_set_text(version_label, vbuf);

    refresh_wifi_status();
    refresh_sd_status();

    lv_obj_add_event_cb(scr, settings_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);

    return scr;
}
