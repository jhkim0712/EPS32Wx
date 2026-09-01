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
 * @brief 저장된 WiFi SSID/비밀번호/configured 플래그만 지운다 (다른 설정은 유지).
 *        "WiFi 재설정" 버튼에서 사용 — 재부팅 시 저장된 WiFi가 없으므로 AP 모드로 전환된다.
 * @return ESP_OK on success
 */
esp_err_t nvs_manager_clear_wifi_config(void);

/**
 * @brief NVS 초기화 (모든 설정 삭제)
 * @return ESP_OK on success
 */
esp_err_t nvs_manager_erase_all(void);

// ---- System 탭 / 온디바이스 설정 개별 setter ----
// 전체 구조체를 왕복하지 않고 단일 필드만 저장할 때 사용한다.

esp_err_t nvs_manager_save_brightness(uint8_t percent);
esp_err_t nvs_manager_save_rotation(uint16_t degrees);
esp_err_t nvs_manager_save_timezone(const char *timezone_posix);
esp_err_t nvs_manager_save_night_dim(bool enabled, uint8_t start_hour, uint8_t end_hour, uint8_t dim_brightness_percent);
esp_err_t nvs_manager_save_slideshow(bool enabled, uint16_t interval_sec);
esp_err_t nvs_manager_save_ota_manifest_url(const char *url);
esp_err_t nvs_manager_save_ota_auto_check(bool enabled);
esp_err_t nvs_manager_save_web_auth(bool enabled, const char *user, const char *pass);

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