#include "weather/weather_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "common/constants.h"
#include "common/config.h"
#include "storage/nvs_manager.h"
#include "network/wifi_manager.h"
#include "weather/weather_interface.h"
#include "ui/ui_app.h"


static const char *TAG = LOG_TAG_WEATHER;

static TaskHandle_t weather_task_handle = NULL;
static uint32_t update_interval = 300; // Default 5 minutes
static bool task_running = false;

static void weather_response_callback(const weather_data_t *weather_data, esp_err_t result)
{
    if (result == ESP_OK && weather_validate_data(weather_data)) {
        ESP_LOGI(TAG, "Weather data updated successfully for %s", weather_data->city_name);
        weather_log_data(weather_data);
        
        // Update UI with new weather data
        ui_update_weather_info(weather_data->temperature, weather_data->humidity, weather_data->condition_desc);
        
    } else {
        ESP_LOGE(TAG, "Weather request failed: %s", esp_err_to_name(result));
    }
}

void weather_task_request_current(void)
{
    char api_key[128] = {0};
    char city_name[64] = {0};

    // Check if WiFi is connected
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected. Cannot request weather data.");
        return;
    }
    
    // Load configuration from NVS
    if (nvs_get_string(NVS_WEATHER_API_KEY, api_key, sizeof(api_key)) == ESP_OK &&
        nvs_get_string(NVS_WEATHER_CITY, city_name, sizeof(city_name)) == ESP_OK) {
        
        ESP_LOGI(TAG, "Requesting weather data for: %s", city_name);
        esp_err_t ret = weather_request_current(WEATHER_PROVIDER_OPENWEATHERMAP, 
                                              city_name, api_key, weather_response_callback);
        
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initiate weather request: %s", esp_err_to_name(ret));
        }
        
    } else {
        ESP_LOGW(TAG, "Weather API settings not configured. Use config portal.");
    }
}

static void weather_periodic_task(void *pvParameters)
{
    uint32_t interval_sec = *((uint32_t*)pvParameters);
    
    ESP_LOGI(TAG, "Weather task started with %lu second interval", interval_sec);
    
    // Initial delay before first request
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Make initial weather request
    weather_task_request_current();
    
    while (task_running) {
        // Wait for the specified interval
        vTaskDelay(pdMS_TO_TICKS(interval_sec * 1000));
        
        // Make periodic weather request
        if (task_running) {
            ESP_LOGI(TAG, "Periodic weather data update");
            weather_task_request_current();
        }
    }
    
    ESP_LOGI(TAG, "Weather task terminated");
    weather_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t weather_task_init(void)
{
    ESP_LOGI(TAG, "Initializing weather task system");
    
    
    ESP_LOGI(TAG, "Weather task system initialized successfully");
    return ESP_OK;
}

esp_err_t weather_task_start_periodic(uint32_t update_interval_sec)
{
    if (task_running) {
        ESP_LOGW(TAG, "Weather task already running. Stopping current task first.");
        weather_task_stop_periodic();
        vTaskDelay(pdMS_TO_TICKS(100)); // Brief delay to ensure cleanup
    }
    
    esp_err_t ret = weather_task_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize weather task system: %s", esp_err_to_name(ret));
        return ret;
    }
    
    update_interval = update_interval_sec;
    task_running = true;
    
    BaseType_t result = xTaskCreate(
        weather_periodic_task,
        "weather_task",
        4096,
        &update_interval,
        5,  // Priority
        &weather_task_handle
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create weather task");
        task_running = false;
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Periodic weather task started (interval: %lu seconds)", update_interval_sec);
    return ESP_OK;
}

void weather_task_stop_periodic(void)
{
    if (weather_task_handle != NULL) {
        ESP_LOGI(TAG, "Stopping periodic weather task");
        task_running = false;
        
        // Wait for task to finish
        while (weather_task_handle != NULL) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        
        ESP_LOGI(TAG, "Periodic weather task stopped");
    }
}

bool weather_task_is_running(void)
{
    return task_running && (weather_task_handle != NULL);
}