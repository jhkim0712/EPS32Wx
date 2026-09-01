// UI 애플리케이션 진입점. 실제 화면 구성은 screen_manager.c + screens/*.c가 담당하고,
// 이 파일은 main.c/weather_task 등 다른 컴포넌트에 대한 안정적인 공개 API만 제공한다.
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "common/constants.h"
#include "ui/ui_app.h"
#include "ui/screen_manager.h"
#include "lvgl.h"
#include "ui/lvgl_driver.h"
#include "display/backlight.h"
#include "screens/screens.h"

static const char* TAG = LOG_TAG_UI;
static bool s_ui_started = false;

esp_err_t ui_app_start(void)
{
    if (s_ui_started) {
        ESP_LOGI(TAG, "UI already started");
        return ESP_OK;
    }

    // Initialize LVGL port (display + touch + tasks)
    esp_err_t err = lvgl_driver_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LVGL driver init failed: %d", (int)err);
        return err;
    }

    if (lvgl_lock(500)) {
        err = screen_manager_init();
        lvgl_unlock();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "screen_manager_init failed: %d", (int)err);
            return err;
        }
    }
    s_ui_started = true;
    ESP_LOGI(TAG, "UI started");
    return ESP_OK;
}

void ui_update_weather_info(float temperature, int humidity, const char* condition_desc)
{
    if (!s_ui_started) {
        (void)ui_app_start();
    }

    if (!condition_desc) condition_desc = "";
    ESP_LOGI(TAG, "UI update - Temp: %.1f C, Humidity: %d%%, Condition: %s",
             temperature, humidity, condition_desc);

    if (lvgl_lock(200)) {
        screen_weather_set_data(temperature, humidity, condition_desc);
        lvgl_unlock();
    }
}

void ui_apply_config(const app_config_t *cfg)
{
    if (!cfg || !s_ui_started) return;

    if (lvgl_lock(200)) {
        backlight_set_percent(cfg->brightness_percent);
        display_set_rotation((int)cfg->display_rotation_deg); // 현재값과 같으면 내부적으로 no-op
        screen_gallery_set_enabled(cfg->slideshow_enabled);
        screen_gallery_set_interval(cfg->slideshow_interval_sec);
        lvgl_unlock();
    }
}

// AP 설정 모드 안내 화면. screen_manager의 4개 화면과는 별개의 독립 화면으로,
// 최초 WiFi 설정이 끝나기 전(정상 화면들을 보여줄 데이터가 아직 없을 때)에만 쓰인다.
void ui_show_config_portal_info(const char* ap_ssid, const char* ap_password, const char* portal_url)
{
    if (!s_ui_started) {
        (void)ui_app_start();
    }
    if (!ap_ssid) ap_ssid = DEFAULT_AP_SSID;
    if (!ap_password) ap_password = DEFAULT_AP_PASSWORD;
    if (!portal_url) portal_url = DEFAULT_AP_IP_ADDR;

    if (lvgl_lock(300)) {
        lv_obj_t *scr = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
        lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *title = lv_label_create(scr);
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_label_set_text(title, "WiFi Setup Mode");
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

        lv_obj_t *line1 = lv_label_create(scr);
        lv_obj_set_style_text_color(line1, lv_color_white(), 0);
        char buf1[96];
        snprintf(buf1, sizeof(buf1), "AP SSID: %s", ap_ssid);
        lv_label_set_text(line1, buf1);
        lv_obj_align(line1, LV_ALIGN_LEFT_MID, 10, -30);

        lv_obj_t *line2 = lv_label_create(scr);
        lv_obj_set_style_text_color(line2, lv_color_white(), 0);
        char buf2[96];
        if (strlen(ap_password) > 0) {
            snprintf(buf2, sizeof(buf2), "Password: %s", ap_password);
        } else {
            snprintf(buf2, sizeof(buf2), "Password: (OPEN)");
        }
        lv_label_set_text(line2, buf2);
        lv_obj_align(line2, LV_ALIGN_LEFT_MID, 10, 0);

        lv_obj_t *line3 = lv_label_create(scr);
        lv_obj_set_style_text_color(line3, lv_color_white(), 0);
        char buf3[128];
        snprintf(buf3, sizeof(buf3), "Open %s in your browser", portal_url);
        lv_label_set_text(line3, buf3);
        lv_obj_align(line3, LV_ALIGN_LEFT_MID, 10, 30);

        lv_scr_load(scr);
        lvgl_unlock();
    }
}
