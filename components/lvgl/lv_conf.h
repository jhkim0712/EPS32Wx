#ifndef LV_CONF_H
#define LV_CONF_H

/* Minimal LVGL 8.3.11 config tuned for ESP32-S3 + WT32-SC01-PLUS */

/*====================
 * Color settings
 *====================*/
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 1

/*====================
 * OS settings
 *====================*/
#define LV_USE_OS 1
#define LV_OS_FREE_RTOS 1

/*====================
 * Log settings
 *====================*/
#define LV_USE_LOG 1
#define LV_LOG_PRINTF 1

/*====================
 * HAL settings
 *====================*/
#define LV_TICK_CUSTOM 0

/*====================
 * Feature toggles (keep defaults lightweight)
 *====================*/
#define LV_USE_LABEL 1
#define LV_USE_IMG 1
#define LV_USE_ARC 1
#define LV_USE_BTN 1
#define LV_USE_SLIDER 0
#define LV_USE_ROLLER 0
#define LV_USE_TABLE 0

/*====================
 * Memory settings
 *====================*/
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64U * 1024U)

/*====================
 * Others
 *====================*/
#define LV_FONT_MONTSERRAT_14 1

#endif /* LV_CONF_H */
