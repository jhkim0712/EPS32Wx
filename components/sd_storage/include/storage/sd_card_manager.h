#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 외장 SPI microSD 모듈을 SD_MOUNT_POINT("/sdcard")에 마운트한다.
 *        카드가 없거나 배선이 안 된 경우에도 앱 전체가 죽지 않도록,
 *        실패는 항상 non-fatal 하게 다뤄야 한다 (호출자가 로그만 남기고 계속 진행).
 *        이미 마운트되어 있으면 ESP_OK를 반환한다.
 * @return ESP_OK on success
 */
esp_err_t sd_card_init(void);

/**
 * @brief SD 카드를 언마운트하고 SPI 버스를 해제한다.
 */
void sd_card_deinit(void);

/**
 * @brief 현재 마운트 상태를 반환한다.
 */
bool sd_card_is_mounted(void);

/**
 * @brief 총 용량과 여유 용량을 바이트 단위로 반환한다.
 * @return ESP_OK on success (마운트되어 있지 않으면 ESP_ERR_INVALID_STATE)
 */
esp_err_t sd_card_get_info(uint64_t *total_bytes, uint64_t *free_bytes);

// 갤러리 등에서 쓰는 파일 목록 항목
typedef struct {
    char name[64];       // 파일명만 (예: "sunset.jpg")
    char full_path[128]; // VFS 절대 경로 (예: "/sdcard/photos/sunset.jpg")
    size_t size;          // 바이트
} sd_file_entry_t;

/**
 * @brief rel_dir(SD_MOUNT_POINT 기준 상대 경로, 예: "/photos") 안의 이미지 파일
 *        (.jpg/.jpeg/.png/.gif, 대소문자 무관) 목록을 out에 채운다.
 * @param rel_dir SD_MOUNT_POINT 기준 상대 디렉터리 경로 (예: "/photos")
 * @param out 결과를 담을 버퍼
 * @param max_entries out 배열 크기
 * @return 채운 항목 수 (0 이상), 마운트되지 않았거나 디렉터리가 없으면 0
 */
int sd_card_list_dir(const char *rel_dir, sd_file_entry_t *out, int max_entries);

#ifdef __cplusplus
}
#endif

#endif // SD_CARD_MANAGER_H
