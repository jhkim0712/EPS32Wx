#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_timer.h"

#include "common/constants.h"
#include "common/config.h"
#include "common/types.h"
#include "common/main_task.h"
#include "storage/nvs_manager.h"
#include "storage/spiffs_manager.h"
#include "network/wifi_manager.h"
#include "network/config_portal.h"
#include "weather/weather_task.h"
#include "ui/lvgl_driver.h"
#include "ui/ui_app.h"
#include "weather/weather_task.h"

static const char *TAG = LOG_TAG_MAIN;

static esp_err_t device_init(void)
{
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "Starting device initialization");
    
    ESP_LOGI(TAG, "Initializing NVS...");
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialization completed");
    
    ESP_LOGI(TAG, "Initializing NVS manager...");
    ret = nvs_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS manager initialization failed");
        return ret;
    }
    ESP_LOGI(TAG, "NVS manager initialization completed");
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGI(TAG, "Initializing SPIFFS...");
    ret = spiffs_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SPIFFS initialization failed: %s (web files limited)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS initialization completed");
    }
    
    ESP_LOGI(TAG, "Initializing LVGL driver...");
    ret = lvgl_driver_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LVGL driver initialization failed: %s (display limited)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "LVGL driver initialization completed");
        ui_app_start();
    }
    
    ESP_LOGI(TAG, "Initializing WiFi manager...");
    ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi manager initialization failed");
        return ret;
    }
    ESP_LOGI(TAG, "WiFi manager initialization completed");
    
    ESP_LOGI(TAG, "Device initialization completed successfully");
    return ESP_OK;
}

void main_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Main task started");
    
    // Initialize device components
    esp_err_t ret = device_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Device initialization failed in main task: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "System restarting...");
        vTaskDelay(pdMS_TO_TICKS(SYSTEM_RESTART_DELAY_MS));
        esp_restart();
    }
    

    // Main task loop
    while (true) {
        // Perform periodic main task operations here
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10초 대기
    }
    
    ESP_LOGI(TAG, "Main task terminated");
    vTaskDelete(NULL);

}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Weather Station starting...");
    
    esp_err_t ret = device_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Device initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "System restarting...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
        return;
    }
    
    ret = wifi_manager_setup_network();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Network setup failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "System restarting...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
        return;
    }
    
    ESP_LOGI(TAG, "ESP32 Weather Station ready!");
    
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG, "System Information:");
    ESP_LOGI(TAG, "  Chip: %s (%d cores)", CONFIG_IDF_TARGET, chip_info.cores);
    
    uint32_t flash_size;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        ESP_LOGI(TAG, "  Flash: %lu MB", flash_size / (1024 * 1024));
    }
    ESP_LOGI(TAG, "  Free heap: %lu bytes", esp_get_free_heap_size());
    

    // Start main task on application core (Core 1)
    /*
    ESP32는 멀티코어이므로 FreeRTOS가 코어0과 코어1을 나눠서 사용한다.
    일반적으로 코어0은 시스템용, 코어1은 어플리케이션용으로 사용한다.
    메인 애플리케이션 태스크를 코어1에 할당하여 시스템 성능을 최적화한다.

    xTaskCreatePinnedToCore 파라미터:
    - TaskFunction_t: pvTaskCode (태스크 함수)
    - Task Name: APP_TASK_NAME (태스크 이름)
    - Stack Size: APP_TASK_STACK_SIZE (스택 크기)
    - Parameters: NULL (파라미터 없음)
    - Priority: APP_TASK_PRIORITY (우선순위)
    - Task Handle: NULL (핸들 저장 불필요)
    - Core ID: APP_TASK_CORE_ID (코어1 할당)
    */
    
    BaseType_t result = xTaskCreatePinnedToCore(
        main_task_start,        // 태스크 함수
        MAIN_TASK_NAME,         // 태스크 이름
        MAIN_TASK_STACK_SIZE,   // 스택 크기 (8192 bytes)
        NULL,                   // 태스크 파라미터
        MAIN_TASK_PRIORITY,     // 태스크 우선순위 (5)
        NULL,                   // 태스크 핸들 (불필요)
        APP_TASK_CORE_ID       // 코어 ID (1 = 애플리케이션 코어)
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main task");
        esp_restart();
    } else {
        ESP_LOGI(TAG, "Main task created successfully on Core %d", APP_TASK_CORE_ID);
    }


    result = xTaskCreatePinnedToCore(
        weather_periodic_task,        // 태스크 함수
        WEATHER_TASK_NAME,         // 태스크 이름
        WEATHER_TASK_STACK_SIZE,   // 스택 크기 (4096 bytes)
        NULL,                      // 태스크 파라미터
        WEATHER_TASK_PRIORITY,     // 태스크 우선순위 (4)
        NULL,                      // 태스크 핸들 (불필요)
        APP_TASK_CORE_ID          // 코어 ID (1 = 애플리케이션 코어)
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create weather task");
        esp_restart();
    } else {
        ESP_LOGI(TAG, "Weather task created successfully on Core %d", APP_TASK_CORE_ID);
    }


    while(true)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    }

}