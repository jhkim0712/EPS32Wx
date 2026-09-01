// 기본 UART0(idf.py monitor/flash와 같은 포트) 위의 esp_console REPL.
// 로그(ESP_LOGx)도 같은 포트로 나가므로 monitor 화면에 로그와 명령 출력이 함께
// 섞여 보이는 게 정상이다 — 별도 디버그 UART가 아니라 지금 보고 있는 그 터미널에서
// 바로 명령을 입력할 수 있게 하는 것이 목적.
#include "console/uart_console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"

#include "common/config.h"
#include "common/constants.h"
#include "display/backlight.h"
#include "network/wifi_manager.h"
#include "ota/ota_manager.h"
#include "storage/sd_card_manager.h"
#include "util/system_restart.h"

static const char *TAG = LOG_TAG_CONSOLE;
static bool s_started = false;

// ---- restart ----
static int cmd_restart(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Restarting...\n");
    fflush(stdout);
    system_restart_delayed(300); // 프롬프트 출력이 UART로 나갈 시간을 잠깐 준다
    return 0;
}

// ---- heap ----
static int cmd_heap(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Free heap (total) : %lu bytes\n", (unsigned long)esp_get_free_heap_size());
    printf("Free heap (internal): %u bytes\n", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("Free heap (PSRAM)  : %u bytes\n", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    return 0;
}

// ---- version ----
static int cmd_version(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("Firmware : %s\n", FIRMWARE_VERSION);
    printf("ESP-IDF  : %s\n", esp_get_idf_version());
    printf("Chip     : %s\n", CONFIG_IDF_TARGET);
    return 0;
}

// ---- wifi ----
static int cmd_wifi(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if (wifi_manager_is_connected())
    {
        char ssid[33] = {0}, ip[32] = {0};
        wifi_manager_get_info(ssid, ip);
        printf("STA connected: %s (%s)\n", ssid, ip);
    }
    else if (wifi_manager_is_ap_mode())
    {
        printf("AP mode: SSID=%s\n", DEFAULT_AP_SSID);
    }
    else
    {
        printf("Not connected (state=%d)\n", (int)wifi_manager_get_state());
    }
    return 0;
}

// ---- sd ----
static int cmd_sd(int argc, char **argv)
{
    if (!sd_card_is_mounted())
    {
        printf("SD card: not mounted\n");
        return 0;
    }

    uint64_t total = 0, freeb = 0;
    sd_card_get_info(&total, &freeb);
    printf("SD card: mounted, %llu/%llu MB free\n",
           (unsigned long long)(freeb / (1024 * 1024)), (unsigned long long)(total / (1024 * 1024)));

    if (argc > 1 && strcmp(argv[1], "ls") == 0)
    {
        sd_file_entry_t entries[16];
        int count = sd_card_list_dir(SD_PHOTOS_SUBDIR, entries, 16);
        for (int i = 0; i < count; ++i)
        {
            printf("  %-40s %8u bytes\n", entries[i].name, (unsigned)entries[i].size);
        }
        printf("(%d file(s) shown, max 16)\n", count);
    }
    else
    {
        printf("(use \"sd ls\" to list %s)\n", SD_PHOTOS_SUBDIR);
    }
    return 0;
}

// ---- brightness [0-100] ----
static int cmd_brightness(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Current brightness: %u%%\n", backlight_get_percent());
        return 0;
    }
    int pct = atoi(argv[1]);
    if (pct < 0 || pct > 100)
    {
        printf("Usage: brightness [0-100]\n");
        return 1;
    }
    backlight_set_percent((uint8_t)pct);
    printf("Brightness set to %d%%\n", pct);
    return 0;
}

// ---- ota check|status ----
static int cmd_ota(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "status") == 0)
    {
        ota_info_t info;
        ota_manager_get_info(&info);
        printf("current=%s available=%s status=%d progress=%d%%\n",
               info.current_version, info.available_version, (int)info.status, info.progress_percent);
        return 0;
    }
    if (strcmp(argv[1], "check") == 0)
    {
        esp_err_t err = ota_manager_check_update(NULL);
        if (err == ESP_OK)
        {
            ota_info_t info;
            ota_manager_get_info(&info);
            printf("Update available: %s (current %s)\n", info.available_version, info.current_version);
        }
        else if (err == ESP_ERR_NOT_FOUND)
        {
            printf("Already up to date\n");
        }
        else if (err == ESP_ERR_INVALID_STATE)
        {
            printf("No OTA manifest URL configured (set it in the web System tab)\n");
        }
        else
        {
            printf("Check failed: %s\n", esp_err_to_name(err));
        }
        return 0;
    }
    printf("Usage: ota [check|status]\n");
    return 1;
}

static void register_commands(void)
{
    esp_console_register_help_command();

    const esp_console_cmd_t cmds[] = {
        {.command = "restart",                  .help = "Restart the device", .hint = NULL, .func = &cmd_restart},
        {.command = "heap",                     .help = "Show free heap (internal/PSRAM)", .hint = NULL, .func = &cmd_heap},
        {.command = "version",                  .help = "Show firmware/IDF version", .hint = NULL, .func = &cmd_version},
        {.command = "wifi",                     .help = "Show WiFi connection status", .hint = NULL, .func = &cmd_wifi},
        {.command = "sd",                       .help = "Show SD card status (\"sd ls\" to list photos)", .hint = "[ls]", .func = &cmd_sd},
        {.command = "brightness",               .help = "Get/set display brightness", .hint = "[0-100]", .func = &cmd_brightness},
        {.command = "ota",                      .help = "Check for / show OTA update status", .hint = "[check|status]", .func = &cmd_ota},
    };
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); ++i)
    {
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmds[i]));
    }
}

esp_err_t uart_console_start(void)
{
    if (s_started)
    {
        return ESP_OK;
    }

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "eps32wx>";
    repl_config.max_cmdline_length = 256;

    // 채널/핀을 건드리지 않고 기본값(sdkconfig의 CONFIG_ESP_CONSOLE_UART_NUM, 보통 UART0)을
    // 그대로 사용 — idf.py monitor로 보고 있는 바로 그 포트에 REPL이 붙는다.
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    esp_err_t err = esp_console_new_repl_uart(&uart_config, &repl_config, &repl);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_console_new_repl_uart failed: %s", esp_err_to_name(err));
        return err;
    }

    register_commands();

    err = esp_console_start_repl(repl);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_console_start_repl failed: %s", esp_err_to_name(err));
        return err;
    }

    s_started = true;
    ESP_LOGI(TAG, "Debug console REPL started on the default console UART");
    return ESP_OK;
}
