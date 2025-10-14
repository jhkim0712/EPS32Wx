#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "storage/nvs_manager.h"
#include "storage/spiffs_manager.h"
#include "network/wifi_manager.h"
#include "network/config_portal.h"
#include "weather/weather_interface.h"
#include "common/config.h"
#include "common/types.h"

static const char* TAG = "MAIN";

static app_state_t current_app_state = APP_STATE_INIT;
static app_config_t app_config;

// 날씨 응답 콜백 함수
static void weather_response_callback(const weather_data_t *weather_data, esp_err_t result)
{
    if (result == ESP_OK && weather_validate_data(weather_data)) {
        ESP_LOGI(TAG, "✅ 날씨 정보 업데이트 성공");
        weather_log_data(weather_data);
        
        // TODO: 디스플레이에 날씨 정보 표시
        // display_show_weather(weather_data);
        
    } else {
        ESP_LOGE(TAG, "❌ 날씨 정보 요청 실패: %s", esp_err_to_name(result));
    }
}

// NVS에서 설정을 로드하여 날씨 정보 요청
static void weather_get_current_from_nvs(void)
{
    if (strlen(app_config.api_key) == 0 || strlen(app_config.city_name) == 0) {
        ESP_LOGW(TAG, "API 키 또는 도시명이 설정되지 않았습니다");
        return;
    }
    
    ESP_LOGI(TAG, "🌤️ 날씨 정보 요청: %s", app_config.city_name);
    esp_err_t err = weather_get_current(app_config.api_key, app_config.city_name, weather_response_callback);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "날씨 API 호출 실패: %s", esp_err_to_name(err));
    }
}

// 설정 완료 콜백
static void config_complete_callback(const app_config_t* config)
{
    ESP_LOGI(TAG, "Configuration completed, restarting...");
    vTaskDelay(pdMS_TO_TICKS(2000));  // 2초 대기
    esp_restart();
}

// WiFi 이벤트 핸들러
static void wifi_event_handler(void* arg, esp_event_base_t event_base, 
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, trying to reconnect...");
        // 재연결 로직 추가 가능
    }
}

static void init_system(void)
{
    ESP_LOGI(TAG, "Initializing system components...");
    
    // NVS 초기화
    ESP_ERROR_CHECK(nvs_manager_init());
    
    // SPIFFS 초기화
    ESP_ERROR_CHECK(spiffs_manager_init());
    
    // WiFi 매니저 초기화
    ESP_ERROR_CHECK(wifi_manager_init());
    wifi_manager_set_event_handler(wifi_event_handler, NULL);
    
    // 날씨 API 초기화
    ESP_ERROR_CHECK(weather_init(WEATHER_PROVIDER_OPENWEATHERMAP));
    
    // 설정 로드
    ESP_ERROR_CHECK(nvs_manager_load_config(&app_config));
    
    ESP_LOGI(TAG, "System initialization completed");
}

static void start_config_mode(void)
{
    ESP_LOGI(TAG, "Starting configuration mode...");
    current_app_state = APP_STATE_CONFIG_MODE;
    
    // AP 모드로 WiFi 시작
    ESP_ERROR_CHECK(wifi_manager_start_ap());
    
    // 설정 포털 시작
    config_portal_set_callback(config_complete_callback);
    ESP_ERROR_CHECK(config_portal_start());
    
    ESP_LOGI(TAG, "Configuration mode started");
    ESP_LOGI(TAG, "Connect to WiFi: %s", AP_SSID);
    ESP_LOGI(TAG, "Open browser: http://192.168.4.1");
}

static void start_normal_operation(void)
{
    ESP_LOGI(TAG, "Starting normal operation mode...");
    current_app_state = APP_STATE_WIFI_CONNECTING;
    
    // 저장된 WiFi 설정으로 연결
    esp_err_t ret = wifi_manager_connect_sta(app_config.wifi.ssid, app_config.wifi.password);
    
    if (ret == ESP_OK) {
        current_app_state = APP_STATE_NORMAL_OPERATION;
        ESP_LOGI(TAG, "WiFi connected successfully");
        
        ESP_LOGI(TAG, "Weather station ready!");
        ESP_LOGI(TAG, "API Key: %s", app_config.api_key[0] ? "Configured" : "Not set");
        ESP_LOGI(TAG, "City: %s", app_config.city_name);
        
        // 첫 번째 날씨 정보 요청 (5초 후)
        vTaskDelay(pdMS_TO_TICKS(5000));
        weather_get_current_from_nvs();
        
    } else {
        ESP_LOGE(TAG, "Failed to connect to WiFi, entering config mode");
        start_config_mode();
    }
}

static void main_task(void* pvParameters)
{
    // 시스템 정보 출력
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    
    ESP_LOGI(TAG, "ESP32 Weather Station Starting...");
    ESP_LOGI(TAG, "Chip: %s with %d CPU cores", CONFIG_IDF_TARGET, chip_info.cores);
    
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        ESP_LOGI(TAG, "Flash: %lu MB", flash_size / (1024 * 1024));
    }
    
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    
    // 시스템 초기화
    init_system();
    
    // 첫 부팅이거나 WiFi 설정이 없으면 설정 모드로 진입
    if (app_config.first_boot || !app_config.wifi.configured || strlen(app_config.api_key) == 0) {
        ESP_LOGI(TAG, "First boot or incomplete configuration detected");
        start_config_mode();
    } else {
        ESP_LOGI(TAG, "Configuration found, starting normal operation");
        start_normal_operation();
    }
    
    // 메인 루프
    while (1) {
        switch (current_app_state) {
            case APP_STATE_CONFIG_MODE:
                // 설정 모드에서는 대기
                vTaskDelay(pdMS_TO_TICKS(5000));
                break;
                
            case APP_STATE_NORMAL_OPERATION:
                // 주기적 날씨 데이터 업데이트 (10분마다)
                weather_get_current_from_nvs();
                
                // 10분 대기 (600,000ms = 10분)
                vTaskDelay(pdMS_TO_TICKS(UPDATE_INTERVAL_MS));
                break;
                
            default:
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
        }
        
        // 메모리 상태 모니터링
        size_t free_heap = esp_get_free_heap_size();
        if (free_heap < 20000) {  // 20KB 미만이면 경고
            ESP_LOGW(TAG, "Low memory warning: %zu bytes free", free_heap);
        }
    }
}

void app_main(void)
{
    // 메인 태스크 생성
    xTaskCreate(main_task, "main_task", 8192, NULL, 5, NULL);
}