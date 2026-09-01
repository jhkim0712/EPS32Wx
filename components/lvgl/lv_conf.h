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
 * Feature toggles
 *====================*/
#define LV_USE_LABEL 1
#define LV_USE_IMG 1
#define LV_USE_ARC 1
#define LV_USE_BTN 1
#define LV_USE_SLIDER 1
#define LV_USE_ROLLER 1
#define LV_USE_TABLE 0
#define LV_USE_BAR 1
#define LV_USE_SWITCH 1

/* 사진/GIF 슬라이드쇼(디지털 액자)용 이미지 디코더 */
#define LV_USE_SJPG 1   /* 소프트웨어 JPEG 디코더 */
#define LV_USE_GIF  1
#define LV_USE_PNG  1
#define LV_USE_BMP  0
#define LV_USE_ANIMIMG 1

/* SD카드(FATFS, VFS 경유)의 파일을 lv_img/lv_gif 소스로 바로 열기 위함
 * ("S:/sdcard/photos/x.jpg" -> fopen("/sdcard/photos/x.jpg")) */
#define LV_USE_FS_STDIO 1
#if LV_USE_FS_STDIO
    #define LV_FS_STDIO_LETTER 'S'
    #define LV_FS_STDIO_PATH ""
    #define LV_FS_STDIO_CACHE_SIZE 0
#endif

/*====================
 * Memory settings
 *====================*/
/* LVGL 내부 힙을 PSRAM에 두어야 디코딩된 사진/GIF 프레임 버퍼가 들어간다.
 * (PSRAM이 sdkconfig에서 꺼져 있으면 heap_caps_malloc이 실패하므로,
 *  이 설정은 반드시 CONFIG_SPIRAM=y 와 함께 적용해야 한다.) */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM
    #define LV_MEM_CUSTOM_INCLUDE "esp_heap_caps.h"
    #define LV_MEM_CUSTOM_ALLOC(size)       heap_caps_malloc((size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
    #define LV_MEM_CUSTOM_FREE(p)           heap_caps_free(p)
    #define LV_MEM_CUSTOM_REALLOC(p, size)  heap_caps_realloc((p), (size), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#else
    #define LV_MEM_SIZE (64U * 1024U)
#endif

/*====================
 * Others
 *====================*/
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_36 1

#endif /* LV_CONF_H */
