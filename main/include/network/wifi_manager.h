#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "common/types.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi_types.h"

/**
 * @brief WiFi 매니저 초기화
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief WiFi STA 모드로 연결 시작
 * @param ssid WiFi SSID
 * @param password WiFi 패스워드
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_connect_sta(const char* ssid, const char* password);

/**
 * @brief WiFi AP 모드 시작 (설정 포털용)
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_start_ap(void);

/**
 * @brief WiFi 연결 해제
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief 현재 WiFi 상태 얻기
 * @return 현재 WiFi 상태
 */
wifi_state_t wifi_manager_get_state(void);

/**
 * @brief WiFi 연결 상태 확인
 * @return true if connected to internet
 */
bool wifi_manager_is_connected(void);

/**
 * @brief AP 모드 여부 확인
 * @return true if in AP mode
 */
bool wifi_manager_is_ap_mode(void);

/**
 * @brief 연결된 WiFi 정보 얻기
 * @param ssid SSID를 저장할 버퍼
 * @param ip_addr IP 주소를 저장할 버퍼
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_get_info(char* ssid, char* ip_addr);

/**
 * @brief WiFi 스캔 시작
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_scan(void);

/**
 * @brief 스캔된 WiFi 목록 얻기
 * @param ap_info AP 정보를 저장할 배열
 * @param max_ap 최대 AP 수
 * @return 실제 찾은 AP 수
 */
uint16_t wifi_manager_get_scan_results(wifi_ap_record_t* ap_info, uint16_t max_ap);

/**
 * @brief WiFi 이벤트 핸들러 설정
 * @param handler 이벤트 핸들러 함수
 * @param arg 핸들러에 전달할 인자
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_set_event_handler(esp_event_handler_t handler, void* arg);

/**
 * @brief Setup network and configure web portal
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_setup_network(void);

#endif // WIFI_MANAGER_H