#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include "common/types.h"
#include "esp_err.h"
#include <esp_wifi.h>  // for wifi_config_t

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

/**
 * @brief Read WiFi configuration (STA) from NVS into wifi_config_t
 * @param wifi_config Output wifi_config_t buffer
 * @return ESP_OK if SSID was found and copied
 */
esp_err_t nvs_get_wifi_config(wifi_config_t* wifi_config);

/**
 * @brief Read a string value by key into provided buffer (safe copy)
 * @param key NVS key
 * @param out Destination buffer
 * @param out_len Destination buffer length
 * @return ESP_OK on success
 */
esp_err_t nvs_get_string(const char* key, char* out, size_t out_len);

#endif // NVS_MANAGER_H