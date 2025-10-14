#include "ota/ota_manager.h"
#include "common/config.h"
#include <esp_log.h>
#include <esp_app_format.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <string.h>

static const char *TAG = "OTA_MANAGER";

// OTA 상태 변수
static ota_info_t g_ota_info = {0};
static esp_ota_handle_t g_ota_handle = 0;
static const esp_partition_t *g_update_partition = NULL;

esp_err_t ota_manager_init(void)
{
    ESP_LOGI(TAG, "OTA Manager 초기화");
    
    // 현재 실행 중인 파티션 정보 얻기
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    if (running_partition == NULL) {
        ESP_LOGE(TAG, "실행 중인 파티션을 찾을 수 없습니다");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "현재 실행 파티션: %s, offset: 0x%lx, size: 0x%lx",
             running_partition->label, running_partition->address, running_partition->size);
    
    // 업데이트용 파티션 찾기
    g_update_partition = esp_ota_get_next_update_partition(NULL);
    if (g_update_partition == NULL) {
        ESP_LOGE(TAG, "업데이트 파티션을 찾을 수 없습니다");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "업데이트 파티션: %s, offset: 0x%lx, size: 0x%lx",
             g_update_partition->label, g_update_partition->address, g_update_partition->size);
    
    // 현재 버전 정보 설정
    strncpy(g_ota_info.current_version, FIRMWARE_VERSION, sizeof(g_ota_info.current_version) - 1);
    g_ota_info.status = OTA_STATUS_IDLE;
    g_ota_info.progress_percent = 0;
    
    return ESP_OK;
}

esp_err_t ota_manager_get_info(ota_info_t *info)
{
    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(info, &g_ota_info, sizeof(ota_info_t));
    return ESP_OK;
}

esp_err_t ota_manager_check_update(const char *update_url)
{
    if (update_url == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "업데이트 확인: %s", update_url);
    
    // TODO: HTTP 클라이언트를 사용하여 서버에서 버전 정보 확인
    // 현재는 기본 구현만 제공
    
    strncpy(g_ota_info.update_url, update_url, sizeof(g_ota_info.update_url) - 1);
    
    return ESP_OK;
}

esp_err_t ota_manager_start_update(const char *update_url)
{
    if (update_url == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "OTA 업데이트 시작: %s", update_url);
    
    if (g_update_partition == NULL) {
        ESP_LOGE(TAG, "업데이트 파티션이 초기화되지 않았습니다");
        return ESP_FAIL;
    }
    
    g_ota_info.status = OTA_STATUS_DOWNLOADING;
    g_ota_info.progress_percent = 0;
    
    // OTA 핸들 시작
    esp_err_t err = esp_ota_begin(g_update_partition, OTA_SIZE_UNKNOWN, &g_ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin 실패: %s", esp_err_to_name(err));
        g_ota_info.status = OTA_STATUS_FAILED;
        return err;
    }
    
    ESP_LOGI(TAG, "OTA 다운로드 시작됨");
    
    // TODO: HTTP 클라이언트를 사용하여 실제 펌웨어 다운로드 및 설치
    // 현재는 기본 구조만 제공
    
    return ESP_OK;
}

ota_status_t ota_manager_get_status(void)
{
    return g_ota_info.status;
}

int ota_manager_get_progress(void)
{
    return g_ota_info.progress_percent;
}

esp_err_t ota_manager_reboot(void)
{
    ESP_LOGI(TAG, "시스템 재부팅");
    esp_restart();
    return ESP_OK;
}

esp_err_t ota_manager_rollback(void)
{
    ESP_LOGI(TAG, "OTA 롤백 시작");
    
    const esp_partition_t *last_invalid_app = esp_ota_get_last_invalid_partition();
    if (last_invalid_app != NULL) {
        ESP_LOGI(TAG, "마지막 무효 파티션: %s", last_invalid_app->label);
        
        // 이전 파티션으로 설정
        esp_err_t err = esp_ota_set_boot_partition(last_invalid_app);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "부트 파티션 설정 실패: %s", esp_err_to_name(err));
            return err;
        }
        
        g_ota_info.status = OTA_STATUS_ROLLBACK;
        ESP_LOGI(TAG, "롤백 완료, 재부팅 필요");
        
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "롤백할 파티션이 없습니다");
        return ESP_FAIL;
    }
}