#include "storage/spiffs_manager.h"
#include "common/config.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

static const char* TAG = "SPIFFS_MGR";
static bool spiffs_mounted = false;

esp_err_t spiffs_manager_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }
    
    spiffs_mounted = true;
    ESP_LOGI(TAG, "SPIFFS mounted successfully");
    
    spiffs_show_info();
    return ESP_OK;
}

void spiffs_manager_deinit(void)
{
    if (spiffs_mounted) {
        esp_vfs_spiffs_unregister("spiffs");
        spiffs_mounted = false;
        ESP_LOGI(TAG, "SPIFFS unmounted");
    }
}

int spiffs_read_file(const char* path, char* buffer, size_t size)
{
    if (!path || !buffer || !spiffs_mounted) {
        return -1;
    }
    
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "/spiffs%s", path);
    
    FILE* file = fopen(full_path, "r");
    if (!file) {
        ESP_LOGW(TAG, "Failed to open file for reading: %s", full_path);
        return -1;
    }
    
    int bytes_read = fread(buffer, 1, size - 1, file);
    fclose(file);
    
    if (bytes_read >= 0) {
        buffer[bytes_read] = '\0';  // null terminate
    }
    
    return bytes_read;
}

esp_err_t spiffs_write_file(const char* path, const char* data, size_t size)
{
    if (!path || !data || !spiffs_mounted) {
        return ESP_ERR_INVALID_ARG;
    }
    
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "/spiffs%s", path);
    
    FILE* file = fopen(full_path, "w");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", full_path);
        return ESP_FAIL;
    }
    
    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    
    if (written != size) {
        ESP_LOGE(TAG, "Failed to write complete data to file: %s", full_path);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "File written successfully: %s (%zu bytes)", full_path, written);
    return ESP_OK;
}

bool spiffs_file_exists(const char* path)
{
    if (!path || !spiffs_mounted) {
        return false;
    }
    
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "/spiffs%s", path);
    
    struct stat st;
    return (stat(full_path, &st) == 0);
}

long spiffs_get_file_size(const char* path)
{
    if (!path || !spiffs_mounted) {
        return -1;
    }
    
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "/spiffs%s", path);
    
    struct stat st;
    if (stat(full_path, &st) == 0) {
        return st.st_size;
    }
    
    return -1;
}

void spiffs_show_info(void)
{
    if (!spiffs_mounted) {
        ESP_LOGW(TAG, "SPIFFS not mounted");
        return;
    }
    
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info("spiffs", &total, &used);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Partition size: total: %zu, used: %zu", total, used);
    } else {
        ESP_LOGE(TAG, "Failed to get partition information");
    }
}