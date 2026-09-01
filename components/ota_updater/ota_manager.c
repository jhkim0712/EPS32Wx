// GitHub 릴리스(또는 임의의 HTTPS 호스트)에 올려둔 매니페스트 JSON을 기반으로 하는
// 범용 OTA 업데이트 구현. 특정 저장소를 하드코딩하지 않고, app_config_t.ota_manifest_url
// (NVS, 웹 System 탭에서 설정)만 참조한다.
//
// 매니페스트 스키마 예시:
// {
//   "version": "1.2.0",
//   "min_version": "1.0.0",
//   "url": "https://github.com/<user>/<repo>/releases/download/v1.2.0/firmware.bin",
//   "size": 1468928,
//   "sha256": "...",
//   "notes": "Bug fixes"
// }

#include "ota/ota_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "cJSON.h"

#include "common/config.h"
#include "common/constants.h"
#include "common/types.h"
#include "storage/nvs_manager.h"
#include "util/system_restart.h"

static const char *TAG = LOG_TAG_OTA;

static ota_info_t s_info = {0};
static bool s_inited = false;

// --- 버전 비교: "a.b.c" 형식, a==b==c 시 0, a>b면 양수, a<b면 음수 ---
static int version_compare(const char *v1, const char *v2)
{
    int a_maj = 0, a_min = 0, a_patch = 0;
    int b_maj = 0, b_min = 0, b_patch = 0;
    sscanf(v1 ? v1 : "", "%d.%d.%d", &a_maj, &a_min, &a_patch);
    sscanf(v2 ? v2 : "", "%d.%d.%d", &b_maj, &b_min, &b_patch);

    if (a_maj != b_maj) return a_maj - b_maj;
    if (a_min != b_min) return a_min - b_min;
    return a_patch - b_patch;
}

// --- 부팅 직후 롤백 헬스체크: 새 OTA 이미지가 PENDING_VERIFY 상태면 유효 처리 ---
static void run_rollback_health_check(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) != ESP_OK) {
        return;
    }

    if (state == ESP_OTA_IMG_PENDING_VERIFY) {
        // 여기 도달했다는 것 자체가 부트로더의 "롤백 활성화" 상태에서 앱이 정상
        // 기동해 이 코드를 실행 중이라는 뜻이다. 호출자(main.c)는 WiFi STA 연결
        // 성공 후에만 ota_manager_init()을 부르므로, 네트워크까지 살아있음이
        // 곧 헬스체크 통과 조건이다.
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "New firmware marked valid (rollback cancelled)");
        } else {
            ESP_LOGW(TAG, "esp_ota_mark_app_valid_cancel_rollback failed: %s", esp_err_to_name(err));
        }
    }
}

esp_err_t ota_manager_init(void)
{
    if (s_inited) return ESP_OK;

    memset(&s_info, 0, sizeof(s_info));
    strncpy(s_info.current_version, FIRMWARE_VERSION, sizeof(s_info.current_version) - 1);
    s_info.status = OTA_STATUS_IDLE;

    run_rollback_health_check();

    s_inited = true;
    ESP_LOGI(TAG, "OTA manager initialized (current version: %s)", s_info.current_version);
    return ESP_OK;
}

esp_err_t ota_manager_get_info(ota_info_t *info)
{
    if (!info) return ESP_ERR_INVALID_ARG;
    *info = s_info;
    return ESP_OK;
}

ota_status_t ota_manager_get_status(void)
{
    return s_info.status;
}

int ota_manager_get_progress(void)
{
    return s_info.progress_percent;
}

esp_err_t ota_manager_check_update(const char *update_url)
{
    char manifest_url[sizeof(((app_config_t *)0)->ota_manifest_url)] = {0};

    if (update_url && update_url[0] != '\0') {
        strncpy(manifest_url, update_url, sizeof(manifest_url) - 1);
    } else {
        app_config_t cfg;
        if (nvs_manager_load_config(&cfg) != ESP_OK || cfg.ota_manifest_url[0] == '\0') {
            ESP_LOGW(TAG, "No OTA manifest URL configured");
            return ESP_ERR_INVALID_STATE;
        }
        strncpy(manifest_url, cfg.ota_manifest_url, sizeof(manifest_url) - 1);
    }

    ESP_LOGI(TAG, "Checking for updates: %s", manifest_url);

    esp_http_client_config_t http_cfg = {
        .url = manifest_url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = OTA_TIMEOUT_MS,
    };
    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
    if (!client) return ESP_FAIL;

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open manifest URL: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }
    esp_http_client_fetch_headers(client);

    // 매니페스트는 작은 JSON이라고 가정 (넉넉히 4KB까지)
    char *buf = (char *)malloc(4096);
    if (!buf) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return ESP_ERR_NO_MEM;
    }
    int total_read = 0;
    int r;
    while ((r = esp_http_client_read(client, buf + total_read, 4095 - total_read)) > 0) {
        total_read += r;
        if (total_read >= 4095) break;
    }
    buf[total_read] = '\0';
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (total_read <= 0) {
        ESP_LOGW(TAG, "Manifest response empty or read failed");
        free(buf);
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        ESP_LOGW(TAG, "Manifest is not valid JSON");
        return ESP_ERR_INVALID_RESPONSE;
    }

    const cJSON *j_version = cJSON_GetObjectItemCaseSensitive(root, "version");
    const cJSON *j_url = cJSON_GetObjectItemCaseSensitive(root, "url");
    const cJSON *j_size = cJSON_GetObjectItemCaseSensitive(root, "size");

    if (!cJSON_IsString(j_version) || !cJSON_IsString(j_url)) {
        ESP_LOGW(TAG, "Manifest missing required 'version'/'url' fields");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }

    strncpy(s_info.available_version, j_version->valuestring, sizeof(s_info.available_version) - 1);
    strncpy(s_info.update_url, j_url->valuestring, sizeof(s_info.update_url) - 1);
    s_info.firmware_size = cJSON_IsNumber(j_size) ? (size_t)j_size->valuedouble : 0;

    bool newer = version_compare(s_info.available_version, s_info.current_version) > 0;
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Manifest: current=%s available=%s -> %s",
             s_info.current_version, s_info.available_version, newer ? "UPDATE AVAILABLE" : "up to date");

    return newer ? ESP_OK : ESP_ERR_NOT_FOUND;
}

static void ota_task(void *arg)
{
    char fw_url[sizeof(s_info.update_url)];
    strncpy(fw_url, (const char *)arg, sizeof(fw_url) - 1);
    free(arg);

    s_info.status = OTA_STATUS_DOWNLOADING;
    s_info.progress_percent = 0;

    esp_http_client_config_t http_cfg = {
        .url = fw_url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = true,
        .timeout_ms = OTA_TIMEOUT_MS,
    };
    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_https_ota_handle_t handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_begin failed: %s", esp_err_to_name(err));
        s_info.status = OTA_STATUS_FAILED;
        vTaskDelete(NULL);
        return;
    }

    int total_size = esp_https_ota_get_image_size(handle);

    while (1) {
        err = esp_https_ota_perform(handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) break;

        int read = esp_https_ota_get_image_len_read(handle);
        s_info.progress_percent = (total_size > 0) ? (int)((int64_t)read * 100 / total_size) : 0;
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_perform failed: %s", esp_err_to_name(err));
        esp_https_ota_abort(handle);
        s_info.status = OTA_STATUS_FAILED;
        vTaskDelete(NULL);
        return;
    }

    if (!esp_https_ota_is_complete_data_received(handle)) {
        ESP_LOGE(TAG, "OTA data not fully received");
        esp_https_ota_abort(handle);
        s_info.status = OTA_STATUS_FAILED;
        vTaskDelete(NULL);
        return;
    }

    s_info.status = OTA_STATUS_INSTALLING;
    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_finish failed: %s", esp_err_to_name(err));
        s_info.status = OTA_STATUS_FAILED;
        vTaskDelete(NULL);
        return;
    }

    s_info.status = OTA_STATUS_SUCCESS;
    s_info.progress_percent = 100;
    ESP_LOGI(TAG, "OTA update successful, restarting...");
    system_restart_delayed(2000);

    vTaskDelete(NULL);
}

esp_err_t ota_manager_start_update(const char *update_url)
{
    const char *url = (update_url && update_url[0] != '\0') ? update_url : s_info.update_url;
    if (!url || url[0] == '\0') {
        ESP_LOGW(TAG, "No firmware URL to update from (run ota_manager_check_update() first)");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_info.status == OTA_STATUS_DOWNLOADING || s_info.status == OTA_STATUS_INSTALLING) {
        return ESP_ERR_INVALID_STATE; // 이미 진행 중
    }

    char *url_copy = strdup(url);
    if (!url_copy) return ESP_ERR_NO_MEM;

    if (xTaskCreate(ota_task, "ota_update", 8192, url_copy, 5, NULL) != pdPASS) {
        free(url_copy);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t ota_manager_reboot(void)
{
    system_restart_delayed(500);
    return ESP_OK;
}

esp_err_t ota_manager_rollback(void)
{
    ESP_LOGW(TAG, "Manual rollback requested — reverting to previous firmware and restarting");
    return esp_ota_mark_app_invalid_rollback_and_reboot();
}
