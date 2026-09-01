// LCD 백라이트 LEDC(PWM) 드라이버 — 기존 GPIO on/off(lcd_backlight_set)를 대체한다.
#include "display/backlight.h"

#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_log.h"

#include "common/constants.h"

static const char *TAG = LOG_TAG_DISPLAY;

#define BACKLIGHT_LEDC_TIMER    LEDC_TIMER_0
#define BACKLIGHT_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define BACKLIGHT_LEDC_CHANNEL  LEDC_CHANNEL_0
#define BACKLIGHT_LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define BACKLIGHT_LEDC_FREQ_HZ  5000
#define BACKLIGHT_DUTY_MAX      ((1 << 10) - 1) // 10-bit duty resolution -> 1023

static bool s_inited = false;
static uint8_t s_percent = 0;

static uint32_t percent_to_duty(uint8_t percent)
{
    if (percent > 100) percent = 100;
    uint32_t duty = ((uint32_t)percent * BACKLIGHT_DUTY_MAX) / 100;
    return LCD_BL_ACTIVE_HIGH ? duty : (BACKLIGHT_DUTY_MAX - duty);
}

esp_err_t backlight_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }

    ledc_timer_config_t timer_cfg = {
        .speed_mode      = BACKLIGHT_LEDC_MODE,
        .timer_num       = BACKLIGHT_LEDC_TIMER,
        .duty_resolution = BACKLIGHT_LEDC_DUTY_RES,
        .freq_hz         = BACKLIGHT_LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "ledc_timer_config failed");

    ledc_channel_config_t channel_cfg = {
        .gpio_num   = LCD_BL_PIN,
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .channel    = BACKLIGHT_LEDC_CHANNEL,
        .timer_sel  = BACKLIGHT_LEDC_TIMER,
        .duty       = percent_to_duty(0), // 초기값: 꺼짐(0%)
        .hpoint     = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_cfg), TAG, "ledc_channel_config failed");

    // backlight_fade_to()에서 사용
    esp_err_t err = ledc_fade_func_install(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) { // ALREADY installed는 무시
        ESP_LOGW(TAG, "ledc_fade_func_install failed: %s (fade disabled)", esp_err_to_name(err));
    }

    s_percent = 0;
    s_inited = true;
    ESP_LOGI(TAG, "Backlight LEDC initialized on GPIO%d", LCD_BL_PIN);
    return ESP_OK;
}

esp_err_t backlight_set_percent(uint8_t percent)
{
    if (!s_inited) {
        esp_err_t err = backlight_init();
        if (err != ESP_OK) return err;
    }
    if (percent > 100) percent = 100;

    uint32_t duty = percent_to_duty(percent);
    ESP_RETURN_ON_ERROR(ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, duty), TAG, "ledc_set_duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL), TAG, "ledc_update_duty failed");

    s_percent = percent;
    return ESP_OK;
}

uint8_t backlight_get_percent(void)
{
    return s_percent;
}

esp_err_t backlight_fade_to(uint8_t percent, uint32_t fade_ms)
{
    if (!s_inited) {
        esp_err_t err = backlight_init();
        if (err != ESP_OK) return err;
    }
    if (percent > 100) percent = 100;

    uint32_t duty = percent_to_duty(percent);
    esp_err_t err = ledc_set_fade_with_time(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, duty, (int)fade_ms);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ledc_set_fade_with_time failed: %s, falling back to immediate set", esp_err_to_name(err));
        s_percent = percent;
        return backlight_set_percent(percent);
    }
    ESP_RETURN_ON_ERROR(ledc_fade_start(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, LEDC_FADE_NO_WAIT), TAG, "ledc_fade_start failed");

    s_percent = percent;
    return ESP_OK;
}
