#include "storage/sd_card_manager.h"

#include <dirent.h>
#include <string.h>
#include <strings.h> // strcasecmp
#include <sys/stat.h>

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "common/constants.h"

static const char *TAG = LOG_TAG_SD;

static sdmmc_card_t *s_card = NULL;
static bool s_mounted = false;
static bool s_spi_bus_initialized = false;

esp_err_t sd_card_init(void)
{
    if (s_mounted) {
        return ESP_OK;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false, // 사용자의 사진이 든 카드를 임의로 포맷하지 않는다
        .max_files = SD_SPI_MAX_FILES,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;

    if (!s_spi_bus_initialized) {
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = SD_SPI_MOSI_PIN,
            .miso_io_num = SD_SPI_MISO_PIN,
            .sclk_io_num = SD_SPI_SCK_PIN,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = SD_SPI_MAX_TRANSFER_SZ,
        };
        esp_err_t err = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
            return err;
        }
        s_spi_bus_initialized = true;
    }

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_SPI_CS_PIN;
    slot_cfg.host_id = host.slot;

    esp_err_t ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_cfg, &mount_cfg, &s_card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGW(TAG, "Failed to mount SD card filesystem (card present but unreadable/unformatted?)");
        } else {
            ESP_LOGW(TAG, "Failed to initialize SD card (%s) - is a card inserted and wired to "
                          "MISO=%d MOSI=%d SCK=%d CS=%d?",
                     esp_err_to_name(ret), SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_SCK_PIN, SD_SPI_CS_PIN);
        }
        return ret;
    }

    s_mounted = true;
    ESP_LOGI(TAG, "SD card mounted at %s", SD_MOUNT_POINT);
#if CONFIG_LOG_DEFAULT_LEVEL >= 3
    sdmmc_card_print_info(stdout, s_card);
#endif
    return ESP_OK;
}

void sd_card_deinit(void)
{
    if (!s_mounted) return;

    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
    s_card = NULL;
    s_mounted = false;

    if (s_spi_bus_initialized) {
        spi_bus_free(SD_SPI_HOST);
        s_spi_bus_initialized = false;
    }
    ESP_LOGI(TAG, "SD card unmounted");
}

bool sd_card_is_mounted(void)
{
    return s_mounted;
}

esp_err_t sd_card_get_info(uint64_t *total_bytes, uint64_t *free_bytes)
{
    if (!s_mounted) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!total_bytes || !free_bytes) {
        return ESP_ERR_INVALID_ARG;
    }

    FATFS *fs;
    DWORD free_clusters;
    // FATFS 드라이브 번호는 esp_vfs_fat_sdspi_mount이 내부적으로 "0:" 하나만 쓰므로 고정 문자열 사용
    FRESULT res = f_getfree("0:", &free_clusters, &fs);
    if (res != FR_OK) {
        ESP_LOGW(TAG, "f_getfree failed: %d", (int)res);
        return ESP_FAIL;
    }

    uint64_t total_sectors = (uint64_t)(fs->n_fatent - 2) * fs->csize;
    uint64_t free_sectors = (uint64_t)free_clusters * fs->csize;

    *total_bytes = total_sectors * fs->ssize;
    *free_bytes = free_sectors * fs->ssize;
    return ESP_OK;
}

static bool has_image_extension(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (!dot) return false;
    static const char *exts[] = { ".jpg", ".jpeg", ".png", ".gif" };
    for (size_t i = 0; i < sizeof(exts) / sizeof(exts[0]); ++i) {
        if (strcasecmp(dot, exts[i]) == 0) return true;
    }
    return false;
}

int sd_card_list_dir(const char *rel_dir, sd_file_entry_t *out, int max_entries)
{
    if (!s_mounted || !rel_dir || !out || max_entries <= 0) {
        return 0;
    }

    char full_dir[96];
    snprintf(full_dir, sizeof(full_dir), "%s%s", SD_MOUNT_POINT, rel_dir);

    DIR *dir = opendir(full_dir);
    if (!dir) {
        ESP_LOGW(TAG, "Directory not found: %s", full_dir);
        return 0;
    }

    int count = 0;
    struct dirent *entry;
    while (count < max_entries && (entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) continue;
        if (!has_image_extension(entry->d_name)) continue;

        sd_file_entry_t *e = &out[count];
        strncpy(e->name, entry->d_name, sizeof(e->name) - 1);
        e->name[sizeof(e->name) - 1] = '\0';
        snprintf(e->full_path, sizeof(e->full_path), "%s/%s", full_dir, entry->d_name);

        struct stat st;
        e->size = (stat(e->full_path, &st) == 0) ? (size_t)st.st_size : 0;

        count++;
    }
    closedir(dir);

    ESP_LOGI(TAG, "Listed %d image file(s) in %s", count, full_dir);
    return count;
}
