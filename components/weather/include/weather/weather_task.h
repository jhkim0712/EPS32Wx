#ifndef WEATHER_TASK_H
#define WEATHER_TASK_H

#include "esp_err.h"
#include "weather/weather_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize weather task
 * 
 * Initializes the weather task system and sets up callbacks
 */
esp_err_t weather_task_init(void);

/**
 * @brief Request current weather data
 * 
 * Loads API key and city name from NVS and requests current weather data.
 * Results will be handled by internal callback and UI will be updated.
 */
void weather_task_request_current(void);

/**
 * @brief Start periodic weather updates
 * 
 * Starts a task that periodically requests weather data based on configuration
 * 
 * @param update_interval_sec Update interval in seconds
 */
esp_err_t weather_task_start_periodic(uint32_t update_interval_sec);

/**
 * @brief Stop periodic weather updates
 */
void weather_task_stop_periodic(void);

/**
 * @brief Check if weather task is running
 * 
 * @return true if periodic weather updates are active, false otherwise
 */
bool weather_task_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_TASK_H