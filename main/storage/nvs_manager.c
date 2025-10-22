#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi_types.h"

#include "common/constants.h"
#include "common/types.h"
#include "storage/nvs_manager.h"

static const char *TAG = LOG_TAG_NVS;

// Helper: open namespace
static esp_err_t nvs_open_ns(nvs_handle_t *out_handle, nvs_open_mode_t mode)
{
    if (!out_handle) return ESP_ERR_INVALID_ARG;
    return nvs_open(NVS_NAMESPACE, mode, out_handle);
}

// Helper: get string with safe copy into buffer
static esp_err_t nvs_read_string(const char *key, char *out, size_t out_len)
{
    if (!key || !out || out_len == 0) return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READONLY);
    if (err != ESP_OK) return err;

    size_t required = 0;
    err = nvs_get_str(h, key, NULL, &required);
    if (err == ESP_OK && required > 0) {
        // required includes trailing null
        if (required > out_len) {
            // read into temp then truncate
            char *tmp = (char*)malloc(required);
            if (!tmp) { nvs_close(h); return ESP_ERR_NO_MEM; }
            esp_err_t err2 = nvs_get_str(h, key, tmp, &required);
            if (err2 == ESP_OK) {
                // truncate safely
                strncpy(out, tmp, out_len - 1);
                out[out_len - 1] = '\0';
            }
            free(tmp);
            nvs_close(h);
            return err2;
        } else {
            err = nvs_get_str(h, key, out, &required);
            nvs_close(h);
            return err;
        }
    }

    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_init(void)
{
    // NVS is usually initialized in device_init via nvs_flash_init().
    // This function can be used to ensure namespace accessibility.
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err == ESP_OK) nvs_close(h);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NVS namespace '%s' ready", NVS_NAMESPACE);
    }
    return err;
}

esp_err_t nvs_manager_load_config(app_config_t* config)
{
    if (!config) return ESP_ERR_INVALID_ARG;
    memset(config, 0, sizeof(*config));

    // Defaults
    config->first_boot = true;
    config->update_interval = DEFAULT_UPDATE_INTERVAL_SEC;

    // Read WiFi SSID
    (void)nvs_read_string(NVS_KEY_WIFI_SSID, config->wifi.ssid, sizeof(config->wifi.ssid));
    // Read WiFi password
    (void)nvs_read_string(NVS_KEY_WIFI_PASS, config->wifi.password, sizeof(config->wifi.password));

    // WiFi configured flag
    nvs_handle_t h;
    if (nvs_open_ns(&h, NVS_READONLY) == ESP_OK) {
        uint8_t configured = 0;
        if (nvs_get_u8(h, NVS_KEY_WIFI_CONFIGURED, &configured) == ESP_OK) {
            config->wifi.configured = (configured != 0);
        } else {
            // If SSID present, infer configured
            config->wifi.configured = (config->wifi.ssid[0] != '\0');
        }

    // API key and city (use safe helper; nvs_get_str expects size_t* as 4th arg)
    (void)nvs_read_string(NVS_KEY_API_KEY, config->api_key, sizeof(config->api_key));
    (void)nvs_read_string(NVS_KEY_CITY_NAME, config->city_name, sizeof(config->city_name));

        // first_boot
        uint8_t first_boot = 1;
        if (nvs_get_u8(h, NVS_KEY_FIRST_BOOT, &first_boot) == ESP_OK) {
            config->first_boot = (first_boot != 0);
        }

        // update_interval (seconds)
        uint32_t interval = 0;
        if (nvs_get_u32(h, NVS_KEY_UPDATE_INTERVAL, &interval) == ESP_OK && interval > 0) {
            config->update_interval = interval;
        }

        nvs_close(h);
    }

    return ESP_OK;
}

esp_err_t nvs_manager_save_config(const app_config_t* config)
{
    if (!config) return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK) return err;

    // Write WiFi settings
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WIFI_SSID, config->wifi.ssid));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WIFI_PASS, config->wifi.password));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_WIFI_CONFIGURED, config->wifi.configured ? 1 : 0));

    // Write API and city
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_API_KEY, config->api_key));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_CITY_NAME, config->city_name));

    // first boot and update interval
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_FIRST_BOOT, config->first_boot ? 1 : 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u32(h, NVS_KEY_UPDATE_INTERVAL, config->update_interval));

    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_wifi_config(const char* ssid, const char* password)
{
    if (!ssid) return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK) return err;

    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WIFI_SSID, ssid));
    if (password) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WIFI_PASS, password));
    } else {
        // store empty password
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WIFI_PASS, ""));
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_WIFI_CONFIGURED, 1));

    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_api_key(const char* api_key)
{
    if (!api_key) return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK) return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_API_KEY, api_key));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_city_name(const char* city_name)
{
    if (!city_name) return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK) return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_CITY_NAME, city_name));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_set_first_boot(bool first_boot)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK) return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_FIRST_BOOT, first_boot ? 1 : 0));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_erase_all(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK) return err;
    err = nvs_erase_all(h);
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

// ---- Convenience helpers used by other modules ----

// Expose a simple getter to align with existing call sites
esp_err_t nvs_get_string(const char* key, char* out, size_t out_len)
{
    return nvs_read_string(key, out, out_len);
}

esp_err_t nvs_get_wifi_config(wifi_config_t* wifi_config)
{
    if (!wifi_config) return ESP_ERR_INVALID_ARG;
    memset(wifi_config, 0, sizeof(*wifi_config));

    char ssid[WIFI_SSID_MAX_LEN + 1] = {0};
    char pass[WIFI_PASS_MAX_LEN + 1] = {0};

    esp_err_t err_ssid = nvs_read_string(NVS_KEY_WIFI_SSID, ssid, sizeof(ssid));
    esp_err_t err_pass = nvs_read_string(NVS_KEY_WIFI_PASS, pass, sizeof(pass));

    if (err_ssid != ESP_OK) {
        return err_ssid; // SSID is required
    }

    // Copy into wifi_config_t
    strncpy((char*)wifi_config->sta.ssid, ssid, sizeof(wifi_config->sta.ssid) - 1);
    if (err_pass == ESP_OK) {
        strncpy((char*)wifi_config->sta.password, pass, sizeof(wifi_config->sta.password) - 1);
    } else {
        wifi_config->sta.password[0] = '\0';
    }

    return ESP_OK;
}
