// Simple LVGL UI to present weather data on WT32-SC01-PLUS
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "common/constants.h"
#include "ui/ui_app.h"
#include "lvgl.h"
#include "ui/lvgl_driver.h"

static const char* TAG = LOG_TAG_UI;
static bool s_ui_started = false;
static lv_obj_t *s_label_title = NULL;
static lv_obj_t *s_label_temp = NULL;
static lv_obj_t *s_label_hum = NULL;
static lv_obj_t *s_label_cond = NULL;

static void ui_build_main_screen(void)
{
    lv_obj_t *scr = lv_scr_act();
    s_label_title = lv_label_create(scr);
    lv_label_set_text(s_label_title, "Weather");
    lv_obj_align(s_label_title, LV_ALIGN_TOP_MID, 0, 10);

    s_label_temp = lv_label_create(scr);
    lv_label_set_text(s_label_temp, "Temp: --.- C");
    lv_obj_align(s_label_temp, LV_ALIGN_LEFT_MID, 10, -30);

    s_label_hum = lv_label_create(scr);
    lv_label_set_text(s_label_hum, "Humidity: -- %");
    lv_obj_align(s_label_hum, LV_ALIGN_LEFT_MID, 10, 0);

    s_label_cond = lv_label_create(scr);
    lv_label_set_text(s_label_cond, "Condition: --");
    lv_obj_align(s_label_cond, LV_ALIGN_LEFT_MID, 10, 30);
}

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
        ui_build_main_screen();
        lvgl_unlock();
    }
    s_ui_started = true;
    ESP_LOGI(TAG, "UI started");
    return ESP_OK;
}

void ui_update_weather_info(float temperature, int humidity, const char* condition_desc)
{
    if (!s_ui_started) {
        // Auto-start UI if not started
        (void)ui_app_start();
    }

    if (!condition_desc) condition_desc = "";
    ESP_LOGI(TAG, "UI update - Temp: %.1f C, Humidity: %d%%, Condition: %s",
             temperature, humidity, condition_desc);

    if (lvgl_lock(200)) {
        char buf[64];
        if (s_label_temp) {
            snprintf(buf, sizeof(buf), "Temp: %.1f C", temperature);
            lv_label_set_text(s_label_temp, buf);
        }
        if (s_label_hum) {
            snprintf(buf, sizeof(buf), "Humidity: %d %%", humidity);
            lv_label_set_text(s_label_hum, buf);
        }
        if (s_label_cond) {
            snprintf(buf, sizeof(buf), "Condition: %s", condition_desc);
            lv_label_set_text(s_label_cond, buf);
        }
        lvgl_unlock();
    }
}
