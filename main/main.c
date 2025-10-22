#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"
#ifndef __has_include
#define __has_include(x) 0
#endif
#if __has_include("esp_flash.h")
#include "esp_flash.h"
#define HAS_ESP_FLASH 1
#else
#include "esp_spi_flash.h"
#define HAS_ESP_FLASH 0
#endif
#include "esp_timer.h"

#include "common/constants.h"
#include "common/config.h"
#include "common/types.h"
#include "storage/nvs_manager.h"
#include "storage/spiffs_manager.h"
#include "network/wifi_manager.h"
#include "network/config_portal.h"
#include "weather/weather_task.h"
#include "ui/lvgl_driver.h"
#include "weather/weather_task.h"
#include "ui/ui_app.h"

// Forward declaration (redundant with header, but avoids implicit declaration if include resolution lags)
esp_err_t ui_app_start(void);

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
    
    #if HAS_ESP_FLASH
    uint32_t flash_size;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        ESP_LOGI(TAG, "  Flash: %lu MB", flash_size / (1024 * 1024));
    }
    #else
    ESP_LOGI(TAG, "  Flash: %lu MB", spi_flash_get_chip_size() / (1024 * 1024));
    #endif
    ESP_LOGI(TAG, "  Free heap: %lu bytes", esp_get_free_heap_size());
    

    // Start main task on application core (Core 1)
   
    weather_task_start_periodic(DEFAULT_UPDATE_INTERVAL_SEC);


    while(true)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    }

}