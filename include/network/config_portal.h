#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include "esp_err.h"
#include "common/types.h"

/**
 * @brief 설정 포털 웹 서버 시작
 * @return ESP_OK on success
 */
esp_err_t config_portal_start(void);

/**
 * @brief 설정 포털 웹 서버 중지
 * @return ESP_OK on success
 */
esp_err_t config_portal_stop(void);

/**
 * @brief 설정 포털 실행 상태 확인
 * @return true if running
 */
bool config_portal_is_running(void);

/**
 * @brief 설정 완료 콜백 함수 타입
 * @param config 설정된 앱 구성
 */
typedef void (*config_complete_cb_t)(const app_config_t* config);

/**
 * @brief 설정 완료 콜백 함수 등록
 * @param callback 콜백 함수
 */
void config_portal_set_callback(config_complete_cb_t callback);

#endif // CONFIG_PORTAL_H