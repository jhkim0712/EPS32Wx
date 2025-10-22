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

#ifdef __cplusplus
}
#endif

#endif // LVGL_DRIVER_H
