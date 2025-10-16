#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <esp_err.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#ifdef __cplusplus
extern "C" {
#endif

// OTA 상태 열거형
typedef enum {
    OTA_STATUS_IDLE,
    OTA_STATUS_DOWNLOADING,
    OTA_STATUS_INSTALLING,
    OTA_STATUS_SUCCESS,
    OTA_STATUS_FAILED,
    OTA_STATUS_ROLLBACK
} ota_status_t;

// OTA 정보 구조체
typedef struct {
    char current_version[32];
    char available_version[32];
    char update_url[256];
    size_t firmware_size;
    ota_status_t status;
    int progress_percent;
} ota_info_t;

/**
 * @brief OTA 매니저 초기화
 * @return ESP_OK on success
 */
esp_err_t ota_manager_init(void);

/**
 * @brief 현재 펌웨어 정보 얻기
 * @param info OTA 정보 구조체 포인터
 * @return ESP_OK on success
 */
esp_err_t ota_manager_get_info(ota_info_t *info);

/**
 * @brief 펌웨어 업데이트 확인
 * @param update_url 업데이트 서버 URL
 * @return ESP_OK if update available
 */
esp_err_t ota_manager_check_update(const char *update_url);

/**
 * @brief 펌웨어 업데이트 시작
 * @param update_url 업데이트 서버 URL
 * @return ESP_OK on success
 */
esp_err_t ota_manager_start_update(const char *update_url);

/**
 * @brief OTA 상태 확인
 * @return 현재 OTA 상태
 */
ota_status_t ota_manager_get_status(void);

/**
 * @brief OTA 진행률 확인
 * @return 진행률 (0-100%)
 */
int ota_manager_get_progress(void);

/**
 * @brief 업데이트 후 재부팅
 * @return ESP_OK on success
 */
esp_err_t ota_manager_reboot(void);

/**
 * @brief 이전 버전으로 롤백
 * @return ESP_OK on success
 */
esp_err_t ota_manager_rollback(void);

#ifdef __cplusplus
}
#endif

#endif // OTA_MANAGER_H