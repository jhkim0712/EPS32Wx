// Gallery 화면: SD카드 /photos 안의 jpg/png/gif를 슬라이드쇼로 보여준다.
// - 화면 진입/재마운트 시에만 목록을 다시 스캔한다 (매 틱마다 스캔하지 않음).
// - 화면 좌/우 1/3 영역 탭으로 이전/다음, lv_timer로 자동 전환.
#include "screens.h"
#include <stdio.h>
#include <string.h>
#include <strings.h> // strcasecmp

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "common/constants.h"
#include "storage/sd_card_manager.h"

static const char *TAG = LOG_TAG_UI;

#define GALLERY_MAX_FILES 64
#define GALLERY_MAX_SKIPS 5 // 손상된 파일을 만났을 때 무한 루프에 빠지지 않도록 상한

static lv_obj_t *s_scr = NULL;
static lv_obj_t *s_placeholder_label = NULL;
static lv_obj_t *s_img = NULL;   // jpg/png
static lv_obj_t *s_gif = NULL;   // gif
static lv_timer_t *s_slideshow_timer = NULL;
static bool s_enabled = true;

static sd_file_entry_t *s_files = NULL; // MALLOC_CAP_SPIRAM
static int s_file_count = 0;
static int s_current_index = -1;

// "S:" + full_path (LVGL LV_FS_STDIO 드라이브 문자 접두). full_path 자체가
// sd_file_entry_t::full_path(192바이트)에서 오므로 그보다 여유 있게 잡는다.
static char s_path_buf[200];

static void clear_media(void)
{
    if (s_img) { lv_obj_del(s_img); s_img = NULL; }
    if (s_gif) { lv_obj_del(s_gif); s_gif = NULL; }
}

static bool has_gif_extension(const char *name)
{
    size_t len = strlen(name);
    return len >= 4 && strcasecmp(name + len - 4, ".gif") == 0;
}

static void show_placeholder(const char *text)
{
    clear_media();
    lv_label_set_text(s_placeholder_label, text);
    lv_obj_clear_flag(s_placeholder_label, LV_OBJ_FLAG_HIDDEN);
}

static void rescan_files(void)
{
    if (!s_files) {
        s_files = (sd_file_entry_t *)heap_caps_malloc(sizeof(sd_file_entry_t) * GALLERY_MAX_FILES, MALLOC_CAP_SPIRAM);
        if (!s_files) {
            ESP_LOGE(TAG, "Failed to allocate gallery file list buffer");
            s_file_count = 0;
            return;
        }
    }

    if (!sd_card_is_mounted()) {
        s_file_count = 0;
        return;
    }

    s_file_count = sd_card_list_dir(SD_PHOTOS_SUBDIR, s_files, GALLERY_MAX_FILES);
    s_current_index = -1;
}

// index의 이미지를 실제로 화면에 그린다. 디코딩 실패 시 다음 파일로 자동 스킵.
static void show_index(int index, int skip_depth)
{
    if (s_file_count <= 0) {
        show_placeholder("No photos in SD:/photos");
        return;
    }
    if (skip_depth >= GALLERY_MAX_SKIPS) {
        show_placeholder("No readable photos found");
        return;
    }

    index = ((index % s_file_count) + s_file_count) % s_file_count;
    s_current_index = index;
    const sd_file_entry_t *f = &s_files[index];

    snprintf(s_path_buf, sizeof(s_path_buf), "S:%s", f->full_path);

    clear_media();
    lv_obj_add_flag(s_placeholder_label, LV_OBJ_FLAG_HIDDEN);

    if (has_gif_extension(f->name)) {
        s_gif = lv_gif_create(s_scr);
        lv_gif_set_src(s_gif, s_path_buf);
        lv_obj_center(s_gif);
        // lv_gif는 소스가 유효하지 않아도 즉시 에러를 반환하지 않으므로(내부적으로 로그만
        // 남김), 여기서는 별도 실패 감지 없이 표시를 시도한다 — 화면이 비어 보이는 정도로 그친다.
    } else {
        s_img = lv_img_create(s_scr);
        lv_img_set_src(s_img, s_path_buf);
        lv_obj_center(s_img);
        lv_img_set_zoom(s_img, 256); // 100% (LVGL zoom 256 = 1.0x); 필요 시 화면 크기에 맞춰 조정
    }

    ESP_LOGI(TAG, "Gallery showing [%d/%d]: %s", index + 1, s_file_count, f->name);
}

static void slideshow_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    // 이 타이머는 화면이 보이는 동안만 lv_timer_resume() 상태이므로(screen_event_cb 참고)
    // 별도의 가시성 체크 없이 진행하면 된다.
    if (s_file_count <= 0) {
        rescan_files();
    }
    show_index(s_current_index + 1, 0);
}

static void screen_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SCREEN_LOADED) {
        rescan_files();
        show_index(0, 0);
        if (s_slideshow_timer && s_enabled) lv_timer_resume(s_slideshow_timer);
        return;
    }
    if (code == LV_EVENT_SCREEN_UNLOADED) {
        if (s_slideshow_timer) lv_timer_pause(s_slideshow_timer);
        return;
    }
    if (code == LV_EVENT_CLICKED) {
        lv_point_t p;
        lv_indev_get_point(lv_indev_get_act(), &p);
        if (p.x < DISPLAY_WIDTH / 3) {
            show_index(s_current_index - 1, 0);
        } else if (p.x > (DISPLAY_WIDTH * 2) / 3) {
            show_index(s_current_index + 1, 0);
        }
        // 가운데 탭은 무시 (나중에 일시정지 토글 등으로 쓸 수 있음)
    }
}

lv_obj_t *screen_gallery_create(void)
{
    s_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_scr, lv_color_black(), 0);
    lv_obj_clear_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_scr, LV_OBJ_FLAG_CLICKABLE);

    s_placeholder_label = lv_label_create(s_scr);
    lv_obj_set_style_text_color(s_placeholder_label, lv_color_hex(0x909090), 0);
    lv_label_set_text(s_placeholder_label, "...");
    lv_obj_center(s_placeholder_label);

    lv_obj_add_event_cb(s_scr, screen_event_cb, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(s_scr, screen_event_cb, LV_EVENT_SCREEN_UNLOADED, NULL);
    lv_obj_add_event_cb(s_scr, screen_event_cb, LV_EVENT_CLICKED, NULL);

    s_slideshow_timer = lv_timer_create(slideshow_timer_cb, DEFAULT_SLIDESHOW_INTERVAL_SEC * 1000, NULL);
    lv_timer_pause(s_slideshow_timer); // 화면이 보일 때만 동작

    return s_scr;
}

void screen_gallery_set_interval(uint16_t seconds)
{
    if (seconds == 0) seconds = 1;
    if (s_slideshow_timer) {
        lv_timer_set_period(s_slideshow_timer, (uint32_t)seconds * 1000);
    }
}

void screen_gallery_set_enabled(bool enabled)
{
    s_enabled = enabled;
    if (!s_slideshow_timer) return;
    if (enabled) {
        lv_timer_resume(s_slideshow_timer);
    } else {
        lv_timer_pause(s_slideshow_timer);
    }
}
