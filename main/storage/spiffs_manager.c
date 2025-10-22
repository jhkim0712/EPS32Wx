#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_spiffs.h"

#include "common/constants.h"
#include "storage/spiffs_manager.h"

static const char *TAG = LOG_TAG_SPIFFS;
static const char *SPIFFS_LABEL = "spiffs";     // must match partitions.csv and CMake label
static const char *SPIFFS_BASE  = "/spiffs";   // mount point
static bool s_spiffs_mounted = false;

// Normalize path: ensure it starts with "/spiffs/" for VFS
static void spiffs_normalize_path(const char *path, char *out, size_t out_len)
{
    if (!path || !out || out_len == 0) return;
    out[0] = '\0';

    if (strncmp(path, SPIFFS_BASE, strlen(SPIFFS_BASE)) == 0) {
        // already absolute within /spiffs
        strncpy(out, path, out_len - 1);
        out[out_len - 1] = '\0';
        return;
    }

    if (path[0] == '/') {
        // relative to mount point
        snprintf(out, out_len, "%s%s", SPIFFS_BASE, path);
    } else {
        snprintf(out, out_len, "%s/%s", SPIFFS_BASE, path);
    }
}

esp_err_t spiffs_manager_init(void)
{
    if (s_spiffs_mounted) {
        ESP_LOGI(TAG, "SPIFFS already mounted at %s", SPIFFS_BASE);
        return ESP_OK;
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path = SPIFFS_BASE,
        .partition_label = SPIFFS_LABEL,
        .max_files = 8,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(SPIFFS_LABEL, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted: total=%u bytes, used=%u bytes", (unsigned)total, (unsigned)used);
    } else {
        ESP_LOGW(TAG, "Failed to get SPIFFS info: %s", esp_err_to_name(ret));
    }

    s_spiffs_mounted = true;
    return ESP_OK;
}

void spiffs_manager_deinit(void)
{
    if (!s_spiffs_mounted) return;
    esp_vfs_spiffs_unregister(SPIFFS_LABEL);
    s_spiffs_mounted = false;
    ESP_LOGI(TAG, "SPIFFS unmounted");
}

int spiffs_read_file(const char* path, char* buffer, size_t size)
{
    if (!path || !buffer || size == 0) return -1;
    if (!s_spiffs_mounted) {
        if (spiffs_manager_init() != ESP_OK) return -1;
    }

    char fullpath[256];
    spiffs_normalize_path(path, fullpath, sizeof(fullpath));

    FILE *f = fopen(fullpath, "rb");
    if (!f) {
        ESP_LOGW(TAG, "File not found: %s", fullpath);
        return -1;
    }
    size_t read_bytes = fread(buffer, 1, size, f);
    fclose(f);
    return (int)read_bytes;
}

esp_err_t spiffs_write_file(const char* path, const char* data, size_t size)
{
    if (!path || (!data && size > 0)) return ESP_ERR_INVALID_ARG;
    if (!s_spiffs_mounted) {
        esp_err_t r = spiffs_manager_init();
        if (r != ESP_OK) return r;
    }

    char fullpath[256];
    spiffs_normalize_path(path, fullpath, sizeof(fullpath));

    FILE *f = fopen(fullpath, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open for write: %s", fullpath);
        return ESP_FAIL;
    }
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    if (written != size) {
        ESP_LOGE(TAG, "Short write: %u/%u bytes to %s", (unsigned)written, (unsigned)size, fullpath);
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool spiffs_file_exists(const char* path)
{
    if (!path) return false;
    if (!s_spiffs_mounted) {
        if (spiffs_manager_init() != ESP_OK) return false;
    }

    char fullpath[256];
    spiffs_normalize_path(path, fullpath, sizeof(fullpath));
    struct stat st;
    return stat(fullpath, &st) == 0;
}

long spiffs_get_file_size(const char* path)
{
    if (!path) return -1;
    if (!s_spiffs_mounted) {
        if (spiffs_manager_init() != ESP_OK) return -1;
    }

    char fullpath[256];
    spiffs_normalize_path(path, fullpath, sizeof(fullpath));
    struct stat st;
    if (stat(fullpath, &st) == 0) {
        return (long)st.st_size;
    }
    return -1;
}

void spiffs_show_info(void)
{
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info(SPIFFS_LABEL, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS info: total=%u, used=%u, free=%u",
                 (unsigned)total, (unsigned)used, (unsigned)(total - used));
    } else {
        ESP_LOGW(TAG, "esp_spiffs_info failed: %s", esp_err_to_name(ret));
    }
}

// Optional compatibility wrappers (legacy names)
esp_err_t spiffs_init(void)  { return spiffs_manager_init(); }
void spiffs_deinit(void)     { spiffs_manager_deinit(); }
