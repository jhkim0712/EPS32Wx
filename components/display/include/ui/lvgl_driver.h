#ifndef LVGL_DRIVER_H
#define LVGL_DRIVER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize LVGL core and prepare for UI usage.
// This minimal port only calls lv_init(); a full port can later
// register display and input drivers.
esp_err_t lvgl_driver_init(void);

// Acquire/release LVGL draw mutex to perform UI updates from other tasks.
// Returns true if lock acquired within timeout_ms.
bool lvgl_lock(uint32_t timeout_ms);
void lvgl_unlock(void);

// Change display rotation at runtime (0/90/180/270 only). Re-sends the MADCTL
// command and updates the LVGL display driver's resolution + redraws.
// Must be called after lvgl_driver_init(). Returns ESP_ERR_INVALID_ARG for any
// other angle, ESP_ERR_INVALID_STATE if the driver isn't initialized yet.
esp_err_t display_set_rotation(int rotation_deg);

#ifdef __cplusplus
}
#endif

#endif // LVGL_DRIVER_H
