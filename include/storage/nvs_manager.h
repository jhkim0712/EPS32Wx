#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include "common/types.h"
#include "esp_err.h"

/**
 * @brief NVS 초기화
 * @return ESP_OK on success
 */
esp_err_t nvs_manager_init(void);

/**
 * @brief 앱 설정 로드
 * @param config 설정을 저장할 구조체
 * @return ESP_OK on success
 */
esp_err_t nvs_manager_load_config(app_config_t* config);

/**
 * @brief 앱 설정 저장
 * @param config 저장할 설정 구조체
 * @return ESP_OK on success
 */
esp_err_t nvs_manager_save_config(const app_config_t* config);

/**
 * @brief WiFi 설정 저장
 * @param ssid WiFi SSID
 * @param password WiFi 패스워드
 * @return ESP_OK on success
 */
esp_err_t nvs_manager_save_wifi_config(const char* ssid, const char* password);

/**
 * @brief API 키 저장
 * @param api_key 날씨 API 키
 * @return ESP_OK on success
 */
esp_err_t nvs_manager_save_api_key(const char* api_key);

/**
 * @brief 도시명 저장
 * @param city_name 도시명
 * @return ESP_OK on success
 */
esp_err_t nvs_manager_save_city_name(const char* city_name);

/**
 * @brief 첫 부팅 플래그 설정
 * @param first_boot 첫 부팅 여부
 * @return ESP_OK on success
 */
esp_err_t nvs_manager_set_first_boot(bool first_boot);

/**
 * @brief NVS 초기화 (모든 설정 삭제)
 * @return ESP_OK on success
 */
esp_err_t nvs_manager_erase_all(void);

#endif // NVS_MANAGER_H