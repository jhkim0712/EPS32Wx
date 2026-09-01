// WT32-SC01-PLUS (ESP32-S3 + ST7796 i80 + FT6236)

#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "driver/i2c.h"

#ifndef __has_include
#define __has_include(x) 0
#endif

#if __has_include("esp_lcd/esp_lcd_panel_io.h")
#include "esp_lcd/esp_lcd_panel_io.h"
#else
#include "esp_lcd_panel_io.h"
#endif

#if __has_include("esp_lcd/esp_lcd_panel_io_i80.h")
#include "esp_lcd/esp_lcd_panel_io_i80.h"
#elif __has_include("esp_lcd_panel_io_i80.h")
#include "esp_lcd_panel_io_i80.h"
#endif

#if __has_include("esp_lcd/esp_lcd_i80_bus.h")
#include "esp_lcd/esp_lcd_i80_bus.h"
#elif __has_include("esp_lcd_i80_bus.h")
#include "esp_lcd_i80_bus.h"
#endif

#if __has_include("esp_lcd/esp_lcd_types.h")
#include "esp_lcd/esp_lcd_types.h"
#else
#include "hal/lcd_types.h"
#endif
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// Allow building without a project-specific lv_conf.h
#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif
#ifndef LV_CONF_SKIP
#define LV_CONF_SKIP
#endif

#include "lvgl.h"
#include "common/constants.h"
#include "ui/lvgl_driver.h"
#include "display/backlight.h"

static const char *TAG = LOG_TAG_UI;
static bool s_lvgl_inited = false;

static esp_lcd_i80_bus_handle_t s_i80_bus = NULL;
static esp_lcd_panel_io_handle_t s_panel_io = NULL;

static lv_color_t *s_lvgl_buf1 = NULL;
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_t *s_disp = NULL;
static lv_disp_drv_t s_disp_drv; // keep a global driver ref for flush-complete callback

static SemaphoreHandle_t s_lvgl_mutex = NULL;
static TaskHandle_t s_lvgl_task = NULL;

// 컴파일 타임 DISPLAY_ROTATION은 초기값일 뿐, 이후 display_set_rotation()으로
// 런타임에 바꿀 수 있다 (Settings 화면의 회전 옵션이 이 변수를 갱신한다).
static int s_rotation_deg = DISPLAY_ROTATION;

#define FT_REG_TD_STATUS 0x02
#define FT_REG_P1_XH     0x03

static i2c_port_t s_touch_port = 0; // I2C_NUM_0

// --- MIPI DCS helpers (generic, no vendor panel needed) ---
static inline esp_err_t panel_write_cmd(uint8_t cmd, const void *data, size_t data_size)
{
    return esp_lcd_panel_io_tx_param(s_panel_io, cmd, data, data_size);
}

static inline esp_err_t panel_write_cmd1(uint8_t cmd, uint8_t val)
{
    return panel_write_cmd(cmd, &val, 1);
}

static inline esp_err_t panel_set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t caset[4] = { (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF), (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF) };
    uint8_t raset[4] = { (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF), (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF) };
    ESP_RETURN_ON_ERROR(panel_write_cmd(0x2A, caset, 4), TAG, "CASET failed");
    ESP_RETURN_ON_ERROR(panel_write_cmd(0x2B, raset, 4), TAG, "RASET failed");
    return ESP_OK;
}

// MADCTL(0x36) 값을 회전 각도 + 미러 옵션으로부터 계산한다.
// lvgl_driver_init()과 display_set_rotation() 양쪽에서 재사용한다.
static uint8_t compute_madctl(int rotation_deg)
{
    const uint8_t MADCTL_MY  = 0x80; // Row Address Order (Y mirror)
    const uint8_t MADCTL_MX  = 0x40; // Column Address Order (X mirror)
    const uint8_t MADCTL_MV  = 0x20; // Row/Column Exchange (swap X/Y)
    const uint8_t MADCTL_BGR = 0x08; // BGR order bit
    uint8_t madctl = MADCTL_BGR;

    // Base rotation (common mapping for ST77xx/ILI9488 family)
    switch (rotation_deg) {
        case 90:  madctl |= MADCTL_MV | MADCTL_MX; break;
        case 180: madctl |= MADCTL_MX | MADCTL_MY; break;
        case 270: madctl |= MADCTL_MV | MADCTL_MY; break;
        case 0:
        default:  break; // no MV/MX/MY
    }

    // Screen-axis mirror mapping: mirror "left-right"/"up-down" on the final screen
#if LCD_MIRROR_X
    switch (rotation_deg) {
        case 90:  madctl ^= MADCTL_MY; break;
        case 180: madctl ^= MADCTL_MX; break;
        case 270: madctl ^= MADCTL_MY; break;
        case 0:
        default:  madctl ^= MADCTL_MX; break;
    }
#endif
#if LCD_MIRROR_Y
    switch (rotation_deg) {
        case 90:  madctl ^= MADCTL_MX; break;
        case 180: madctl ^= MADCTL_MY; break;
        case 270: madctl ^= MADCTL_MX; break;
        case 0:
        default:  madctl ^= MADCTL_MY; break;
    }
#endif
    return madctl;
}

static esp_err_t lcd_reset_pulse(void)
{
    if (LCD_RST_PIN >= 0) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << LCD_RST_PIN,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = 0,
            .pull_down_en = 0,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "gpio_config RST failed");
        gpio_set_level(LCD_RST_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(LCD_RST_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(LCD_RST_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }
    return ESP_OK;
}

static void lcd_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    const int x1 = area->x1;
    const int y1 = area->y1;
    const int x2 = area->x2;
    const int y2 = area->y2;

    // Set drawing window then push color data
    if (panel_set_addr_window(x1, y1, x2, y2) == ESP_OK) {
        size_t w = (size_t)(x2 - x1 + 1);
        size_t h = (size_t)(y2 - y1 + 1);
        size_t len = w * h * sizeof(lv_color_t);
        // 0x2C = RAMWR
        // async transfer; completion will trigger lv_disp_flush_ready via callback
        esp_lcd_panel_io_tx_color(s_panel_io, 0x2C, color_p, len);
    }
}

// esp_lcd color transfer done -> notify LVGL the flush completed
static bool lcd_color_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    (void)panel_io; (void)edata; (void)user_ctx;
    lv_disp_flush_ready(&s_disp_drv);
    return false; // no higher priority task woken
}

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_task(void *arg)
{
    (void)arg;
    while (1) {
        if (s_lvgl_mutex && xSemaphoreTake(s_lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGive(s_lvgl_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(LVGL_TASK_PERIOD_MS));
    }
}

static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    uint8_t buf[5];
    uint8_t reg = FT_REG_TD_STATUS;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TOUCH_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TOUCH_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buf, sizeof(buf), I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    if (i2c_master_cmd_begin(s_touch_port, cmd, pdMS_TO_TICKS(50)) != ESP_OK) {
        i2c_cmd_link_delete(cmd);
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    i2c_cmd_link_delete(cmd);

    uint8_t points = buf[0] & 0x0F;
    if (points == 0) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint16_t x = ((buf[1] & 0x0F) << 8) | buf[2];
    uint16_t y = ((buf[3] & 0x0F) << 8) | buf[4];

    // Apply mirror/rotation so touch matches what you see on screen
    int16_t tx = (int16_t)x;
    int16_t ty = (int16_t)y;

    // 1) Mirror to match MADCTL MX/MY settings
    #if LCD_MIRROR_X
        tx = (int16_t)(DISPLAY_WIDTH - 1 - tx);
    #endif
    #if LCD_MIRROR_Y
        ty = (int16_t)(DISPLAY_HEIGHT - 1 - ty);
    #endif

    // 2) Rotation (런타임 s_rotation_deg: 0/90/180/270 — display_set_rotation()으로 변경 가능)
    switch (s_rotation_deg) {
        case 90: {
            int16_t rx = ty;
            int16_t ry = (int16_t)(DISPLAY_WIDTH - 1 - tx);
            tx = rx; ty = ry;
            break;
        }
        case 180:
            tx = (int16_t)(DISPLAY_WIDTH - 1 - tx);
            ty = (int16_t)(DISPLAY_HEIGHT - 1 - ty);
            break;
        case 270: {
            int16_t rx = (int16_t)(DISPLAY_HEIGHT - 1 - ty);
            int16_t ry = tx;
            tx = rx; ty = ry;
            break;
        }
        case 0:
        default:
            break;
    }

    data->point.x = tx;
    data->point.y = ty;
    data->state = LV_INDEV_STATE_PRESSED;
}

static esp_err_t i2c_touch_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TOUCH_I2C_SDA_PIN,
        .scl_io_num = TOUCH_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = TOUCH_I2C_CLK_HZ,
        .clk_flags = 0,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(s_touch_port, &conf), TAG, "i2c_param_config failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(s_touch_port, conf.mode, 0, 0, 0), TAG, "i2c_driver_install failed");
    return ESP_OK;
}

esp_err_t lvgl_driver_init(void)
{
    if (s_lvgl_inited) {
        ESP_LOGI(TAG, "LVGL already initialized");
        return ESP_OK;
    }

    lv_init();

    s_lvgl_mutex = xSemaphoreCreateMutex();
    if (!s_lvgl_mutex) return ESP_ERR_NO_MEM;

    ESP_RETURN_ON_ERROR(backlight_init(), TAG, "backlight init failed"); // 초기 밝기 0%(꺼짐)
    ESP_RETURN_ON_ERROR(lcd_reset_pulse(), TAG, "lcd reset failed");
    // i80 (8080) bus
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = LCD_RS_PIN,
        .wr_gpio_num = LCD_WR_PIN,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums = {
            LCD_DB0_PIN, LCD_DB1_PIN, LCD_DB2_PIN, LCD_DB3_PIN,
            LCD_DB4_PIN, LCD_DB5_PIN, LCD_DB6_PIN, LCD_DB7_PIN
        },
        .bus_width = 8,
        .max_transfer_bytes = DISPLAY_WIDTH * LVGL_BUFFER_LINES * sizeof(lv_color_t),
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_i80_bus(&bus_config, &s_i80_bus), TAG, "new_i80_bus failed");

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = -1,
        .pclk_hz = 10 * 1000 * 1000,
        .trans_queue_depth = 1, // single in-flight flush
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .on_color_trans_done = lcd_color_trans_done_cb,
        .user_ctx = NULL,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i80(s_i80_bus, &io_config, &s_panel_io), TAG, "panel_io_i80 failed");

    // --- Minimal panel init sequence (generic MIPI DCS) ---
    // Already did hardware reset
    // Sleep Out
    ESP_RETURN_ON_ERROR(panel_write_cmd(0x11, NULL, 0), TAG, "SLPOUT failed");
    vTaskDelay(pdMS_TO_TICKS(120));
    // Pixel format = 16-bit
    ESP_RETURN_ON_ERROR(panel_write_cmd1(0x3A, 0x55), TAG, "COLMOD failed");
    // Memory Access Control: compute from s_rotation_deg (초기값 DISPLAY_ROTATION) and mirror options
    uint8_t madctl = compute_madctl(s_rotation_deg);
    ESP_LOGI(TAG, "Display orient: ROT=%d, MIRX=%d, MIRY=%d -> MADCTL=0x%02X",
             s_rotation_deg, (int)LCD_MIRROR_X, (int)LCD_MIRROR_Y, (unsigned)madctl);
    ESP_RETURN_ON_ERROR(panel_write_cmd1(0x36, madctl), TAG, "MADCTL failed");
    // Display ON
    ESP_RETURN_ON_ERROR(panel_write_cmd(0x29, NULL, 0), TAG, "DISPON failed");
    vTaskDelay(pdMS_TO_TICKS(20));

    // 백라이트 켜기 (기본 밝기; NVS에 저장된 값이 있으면 main.c가 부팅 후 다시 적용한다)
    backlight_set_percent(DEFAULT_BRIGHTNESS_PERCENT);

    size_t buf_pixels = DISPLAY_WIDTH * LVGL_BUFFER_LINES;
    s_lvgl_buf1 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_lvgl_buf1) s_lvgl_buf1 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!s_lvgl_buf1) return ESP_ERR_NO_MEM;

    // 두 번째 버퍼(더블버퍼): 확보 실패해도 치명적이지 않음 — NULL이면 LVGL이
    // 싱글 버퍼 모드로 계속 동작한다(끊김 없는 화면 전환 대신 안정성 우선).
    lv_color_t *lvgl_buf2 = (lv_color_t *)heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!lvgl_buf2) {
        ESP_LOGW(TAG, "Failed to allocate second LVGL draw buffer; falling back to single buffering");
    }

    lv_disp_draw_buf_init(&s_draw_buf, s_lvgl_buf1, lvgl_buf2, buf_pixels);

    lv_disp_drv_init(&s_disp_drv);
    if (s_rotation_deg == 90 || s_rotation_deg == 270) {
        s_disp_drv.hor_res = DISPLAY_HEIGHT; // swap when rotated 90/270
        s_disp_drv.ver_res = DISPLAY_WIDTH;
    } else {
        s_disp_drv.hor_res = DISPLAY_WIDTH;
        s_disp_drv.ver_res = DISPLAY_HEIGHT;
    }
    s_disp_drv.flush_cb = lcd_flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;
    s_disp = lv_disp_drv_register(&s_disp_drv);

    const esp_timer_create_args_t tick_args = {
        .callback = &lvgl_tick_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lv_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_RETURN_ON_ERROR(esp_timer_create(&tick_args, &tick_timer), TAG, "esp_timer_create failed");
    ESP_RETURN_ON_ERROR(esp_timer_start_periodic(tick_timer, LVGL_TICK_PERIOD_MS * 1000), TAG, "esp_timer_start_periodic failed");

    // LVGL handler task
    BaseType_t ok = xTaskCreatePinnedToCore(lvgl_task, "lvgl", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, &s_lvgl_task, LVGL_TASK_CORE_ID);
    if (ok != pdPASS) return ESP_ERR_NO_MEM;

    // Touch init and LVGL input device
    ESP_RETURN_ON_ERROR(i2c_touch_init(), TAG, "i2c_touch_init failed");
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);

    s_lvgl_inited = true;
    ESP_LOGI(TAG, "LVGL port initialized (WT32-SC01-PLUS)");
    return ESP_OK;
}

esp_err_t display_set_rotation(int rotation_deg)
{
    if (rotation_deg != 0 && rotation_deg != 90 && rotation_deg != 180 && rotation_deg != 270) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_lvgl_inited) {
        return ESP_ERR_INVALID_STATE;
    }
    if (rotation_deg == s_rotation_deg) {
        return ESP_OK; // no-op
    }

    bool locked = lvgl_lock(500);

    s_rotation_deg = rotation_deg;
    uint8_t madctl = compute_madctl(s_rotation_deg);
    panel_write_cmd1(0x36, madctl);

    if (s_rotation_deg == 90 || s_rotation_deg == 270) {
        s_disp_drv.hor_res = DISPLAY_HEIGHT;
        s_disp_drv.ver_res = DISPLAY_WIDTH;
    } else {
        s_disp_drv.hor_res = DISPLAY_WIDTH;
        s_disp_drv.ver_res = DISPLAY_HEIGHT;
    }
    lv_disp_drv_update(s_disp, &s_disp_drv);
    lv_obj_invalidate(lv_scr_act());

    if (locked) lvgl_unlock();

    ESP_LOGI(TAG, "Display rotation changed to %d degrees (MADCTL=0x%02X)", s_rotation_deg, (unsigned)madctl);
    return ESP_OK;
}

bool lvgl_lock(uint32_t timeout_ms)
{
    if (!s_lvgl_mutex) return false;
    return xSemaphoreTake(s_lvgl_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void lvgl_unlock(void)
{
    if (s_lvgl_mutex) xSemaphoreGive(s_lvgl_mutex);
}
