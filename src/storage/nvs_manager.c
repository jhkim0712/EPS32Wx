#include "storage/nvs_manager.h"
#include "common/config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "NVS_MANAGER";

esp_err_t nvs_manager_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "NVS initialized successfully");
    return ESP_OK;
}

esp_err_t nvs_manager_load_config(app_config_t* config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "No saved config found, using defaults");
        // 기본값 설정
        memset(config, 0, sizeof(app_config_t));
        config->first_boot = true;
        config->update_interval = UPDATE_INTERVAL_MS;
        return ESP_OK;
    }
    
    size_t required_size;
    
    // WiFi SSID 로드
    required_size = sizeof(config->wifi.ssid);
    ret = nvs_get_str(nvs_handle, NVS_KEY_WIFI_SSID, config->wifi.ssid, &required_size);
    if (ret != ESP_OK) {
        config->wifi.ssid[0] = '\0';
    }
    
    // WiFi 패스워드 로드
    required_size = sizeof(config->wifi.password);
    ret = nvs_get_str(nvs_handle, NVS_KEY_WIFI_PASS, config->wifi.password, &required_size);
    if (ret != ESP_OK) {
        config->wifi.password[0] = '\0';
    }
    
    // API 키 로드
    required_size = sizeof(config->api_key);
    ret = nvs_get_str(nvs_handle, NVS_KEY_API_KEY, config->api_key, &required_size);
    if (ret != ESP_OK) {
        config->api_key[0] = '\0';
    }
    
    // 도시명 로드
    required_size = sizeof(config->city_name);
    ret = nvs_get_str(nvs_handle, NVS_KEY_CITY_NAME, config->city_name, &required_size);
    if (ret != ESP_OK) {
        strcpy(config->city_name, "Seoul");
    }
    
    // 첫 부팅 플래그 로드
    uint8_t first_boot = 1;
    ret = nvs_get_u8(nvs_handle, NVS_KEY_FIRST_BOOT, &first_boot);
    config->first_boot = (first_boot != 0);
    
    // WiFi 설정 여부 확인
    config->wifi.configured = (strlen(config->wifi.ssid) > 0);
    config->update_interval = UPDATE_INTERVAL_MS;
    
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "Config loaded - WiFi: %s, City: %s, First boot: %d", 
             config->wifi.ssid, config->city_name, config->first_boot);
    
    return ESP_OK;
}

esp_err_t nvs_manager_save_config(const app_config_t* config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 모든 설정 저장
    nvs_set_str(nvs_handle, NVS_KEY_WIFI_SSID, config->wifi.ssid);
    nvs_set_str(nvs_handle, NVS_KEY_WIFI_PASS, config->wifi.password);
    nvs_set_str(nvs_handle, NVS_KEY_API_KEY, config->api_key);
    nvs_set_str(nvs_handle, NVS_KEY_CITY_NAME, config->city_name);
    nvs_set_u8(nvs_handle, NVS_KEY_FIRST_BOOT, config->first_boot ? 1 : 0);
    
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Configuration saved successfully");
    } else {
        ESP_LOGE(TAG, "Failed to save configuration: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t nvs_manager_save_wifi_config(const char* ssid, const char* password)
{
    if (!ssid || !password) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    nvs_set_str(nvs_handle, NVS_KEY_WIFI_SSID, ssid);
    nvs_set_str(nvs_handle, NVS_KEY_WIFI_PASS, password);
    
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "WiFi config saved: %s", ssid);
    return ret;
}

esp_err_t nvs_manager_save_api_key(const char* api_key)
{
    if (!api_key) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    nvs_set_str(nvs_handle, NVS_KEY_API_KEY, api_key);
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "API key saved");
    return ret;
}

esp_err_t nvs_manager_save_city_name(const char* city_name)
{
    if (!city_name) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    nvs_set_str(nvs_handle, NVS_KEY_CITY_NAME, city_name);
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "City name saved: %s", city_name);
    return ret;
}

esp_err_t nvs_manager_set_first_boot(bool first_boot)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    nvs_set_u8(nvs_handle, NVS_KEY_FIRST_BOOT, first_boot ? 1 : 0);
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    ESP_LOGI(TAG, "First boot flag set to: %d", first_boot);
    return ret;
}

esp_err_t nvs_manager_erase_all(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = nvs_erase_all(nvs_handle);
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
        ESP_LOGI(TAG, "All NVS data erased");
    } else {
        ESP_LOGE(TAG, "Failed to erase NVS data: %s", esp_err_to_name(ret));
    }
    
    nvs_close(nvs_handle);
    return ret;
}