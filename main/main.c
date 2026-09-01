#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
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
#include "esp_heap_caps.h"
#include "esp_netif_sntp.h"

#include "common/constants.h"
#include "common/config.h"
#include "common/types.h"
#include "storage/nvs_manager.h"
#include "storage/spiffs_manager.h"
#include "storage/sd_card_manager.h"
#include "network/wifi_manager.h"
#include "network/config_portal.h"
#include "weather/weather_task.h"
#include "ui/lvgl_driver.h"
#include "ui/ui_app.h"
#include "display/backlight.h"
#include "ota/ota_manager.h"
#include "util/system_restart.h"
#include "console/uart_console.h"

// Forward declaration (redundant with header, but avoids implicit declaration if include resolution lags)
esp_err_t ui_app_start(void);

static const char *TAG = LOG_TAG_MAIN;

static esp_err_t device_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Starting device initialization");

    // 다른 서브시스템보다 먼저 띄워 부팅 초반 문제도 콘솔에서 디버깅할 수 있게 한다.
    // idf.py monitor와 같은 UART를 공유하므로 로그와 명령 입출력이 한 화면에 섞여 보인다.
    uart_console_start();

    ESP_LOGI(TAG, "PSRAM free: %u bytes", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

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

    // SD 카드는 선택 장치(외장 microSD 모듈이 배선되어 있지 않을 수 있음) —
    // 실패해도 앱 전체를 막지 않고 경고만 남긴다 (디지털 액자 기능만 비활성화됨).
    ESP_LOGI(TAG, "Initializing SD card...");
    ret = sd_card_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD card not available: %s (photo gallery disabled)", esp_err_to_name(ret));
    } else {
        uint64_t total = 0, free_b = 0;
        if (sd_card_get_info(&total, &free_b) == ESP_OK) {
            ESP_LOGI(TAG, "SD card: %llu MB total, %llu MB free",
                     (unsigned long long)(total / (1024 * 1024)), (unsigned long long)(free_b / (1024 * 1024)));
        }
    }

    // 저장된 설정(밝기/회전/타임존/슬라이드쇼 등)을 한 번 읽어, TZ 환경변수와
    // UI 레이어(백라이트/회전/갤러리)에 적용한다.
    app_config_t cfg;
    (void)nvs_manager_load_config(&cfg);
    if (cfg.timezone_posix[0] != '\0') {
        setenv("TZ", cfg.timezone_posix, 1);
        tzset();
    }

    ESP_LOGI(TAG, "Initializing LVGL driver...");
    ret = lvgl_driver_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LVGL driver initialization failed: %s (display limited)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "LVGL driver initialization completed");
        ui_app_start();
        ui_apply_config(&cfg);
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

// 설정 포털(웹 UI)에서 새 설정 저장이 완료되면 호출된다. WiFi/시스템 설정이
// 바뀌었으므로 새 설정을 적용하기 위해 잠시 후 재부팅한다. HTTP 응답이
// 클라이언트에 먼저 전달될 시간을 벌기 위해 즉시가 아니라 지연 재부팅한다.
static void on_config_saved(const app_config_t *config)
{
    (void)config;
    ESP_LOGI(TAG, "Configuration saved via web portal, scheduling restart");
    system_restart_delayed(2000);
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

    // 설정 포털(웹 UI)은 AP 모드든 STA 모드든 항상 접근 가능해야 한다 —
    // 정상 운영 중에도 System/Pictures 탭 등 설정 화면에 접속할 수 있어야 하므로
    // wifi_manager_setup_network()의 결과와 무관하게 항상 기동한다.
    config_portal_set_callback(on_config_saved);
    ret = config_portal_start();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Config portal failed to start: %s (web settings unavailable)", esp_err_to_name(ret));
    }

    // If WiFi is not connected (likely AP/config mode), show AP info on display
    if (wifi_manager_is_ap_mode() == true) {
        ESP_LOGI(TAG, "Displaying configuration portal info on UI");
        char ssid[33] = {0};
        char ip[32] = {0};
        if (wifi_manager_get_info(ssid, ip) == ESP_OK) {
            char url[64];
            snprintf(url, sizeof(url), "http://%s", ip[0] ? ip : DEFAULT_AP_IP_ADDR);
            ui_show_config_portal_info(ssid, DEFAULT_AP_PASSWORD, url);
        } else {
            ui_show_config_portal_info(DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD, DEFAULT_AP_IP_ADDR);
        }
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
    // Start weather task only when WiFi is connected
    if (wifi_manager_is_connected())
    {
        weather_task_start_periodic(DEFAULT_UPDATE_INTERVAL_SEC);

        ESP_LOGI(TAG, "Starting SNTP time sync...");
        esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        esp_netif_sntp_init(&sntp_config);

        // OTA 매니저 초기화 — 롤백 헬스체크(새 펌웨어가 PENDING_VERIFY 상태면 여기서
        // valid로 확정)를 겸하므로, 반드시 STA 연결 성공 후에만 호출해야 한다.
        ota_manager_init();
        app_config_t cfg;
        if (nvs_manager_load_config(&cfg) == ESP_OK && cfg.ota_auto_check && cfg.ota_manifest_url[0] != '\0') {
            if (ota_manager_check_update(NULL) == ESP_OK) {
                ESP_LOGI(TAG, "OTA update available (auto-check) — visit the web System tab or on-device Settings to install");
            }
        }
    }
    else
    {
        ESP_LOGI(TAG, "WiFi not connected; skipping weather task until configured.");
    }

    // 야간 자동 감광: 1분 주기로 현재 시각과 설정된 시간대를 비교해 밝기를 전환한다.
    static bool s_night_dim_active = false;
    int loop_count = 0;

    while(true)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());

        // debug
        ESP_LOGI(TAG, "wifi_manager_get_state(): %d\r\n", wifi_manager_get_state());

        if (++loop_count >= 6) { // ~60초마다
            loop_count = 0;

            app_config_t cur;
            if (nvs_manager_load_config(&cur) == ESP_OK && cur.night_dim_enabled) {
                time_t now = time(NULL);
                struct tm tm_now;
                localtime_r(&now, &tm_now);
                int h = tm_now.tm_hour;

                bool in_range = (cur.night_dim_start_hour <= cur.night_dim_end_hour)
                    ? (h >= cur.night_dim_start_hour && h < cur.night_dim_end_hour)
                    : (h >= cur.night_dim_start_hour || h < cur.night_dim_end_hour); // 자정을 넘는 구간

                if (in_range && !s_night_dim_active) {
                    backlight_fade_to(cur.night_dim_brightness_percent, 3000);
                    s_night_dim_active = true;
                } else if (!in_range && s_night_dim_active) {
                    backlight_fade_to(cur.brightness_percent, 3000);
                    s_night_dim_active = false;
                }
            }
        }
    }

}
