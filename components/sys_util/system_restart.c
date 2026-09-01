#include "util/system_restart.h"

#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "common/constants.h"

static const char *TAG = LOG_TAG_SYS_UTIL;

typedef struct {
    uint32_t delay_ms;
} restart_task_arg_t;

static void restart_task(void *arg)
{
    restart_task_arg_t *a = (restart_task_arg_t *)arg;
    uint32_t delay_ms = a->delay_ms;
    vPortFree(a);

    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    ESP_LOGI(TAG, "Restarting now...");
    esp_restart();
}

void system_restart_delayed(uint32_t delay_ms)
{
    restart_task_arg_t *arg = (restart_task_arg_t *)pvPortMalloc(sizeof(restart_task_arg_t));
    if (!arg)
    {
        ESP_LOGE(TAG, "Failed to allocate restart task arg, restarting immediately");
        esp_restart();
        return;
    }
    arg->delay_ms = delay_ms;

    ESP_LOGI(TAG, "Scheduling restart in %lu ms", (unsigned long)delay_ms);
    if (xTaskCreate(restart_task, "sys_restart", 2048, arg, 5, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create restart task, restarting immediately");
        vPortFree(arg);
        esp_restart();
    }
}
