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
    if (!out_handle)
        return ESP_ERR_INVALID_ARG;
    return nvs_open(NVS_NAMESPACE, mode, out_handle);
}

// Helper: get string with safe copy into buffer
static esp_err_t nvs_read_string(const char *key, char *out, size_t out_len)
{
    if (!key || !out || out_len == 0)
        return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READONLY);
    if (err != ESP_OK)
        return err;

    size_t required = 0;
    err = nvs_get_str(h, key, NULL, &required);
    if (err == ESP_OK && required > 0)
    {
        // required includes trailing null
        if (required > out_len)
        {
            // read into temp then truncate
            char *tmp = (char *)malloc(required);
            if (!tmp)
            {
                nvs_close(h);
                return ESP_ERR_NO_MEM;
            }
            esp_err_t err2 = nvs_get_str(h, key, tmp, &required);
            if (err2 == ESP_OK)
            {
                // truncate safely
                strncpy(out, tmp, out_len - 1);
                out[out_len - 1] = '\0';
            }
            free(tmp);
            nvs_close(h);
            return err2;
        }
        else
        {
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
    if (err == ESP_OK)
        nvs_close(h);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "NVS namespace '%s' ready", NVS_NAMESPACE);
    }
    return err;
}

esp_err_t nvs_manager_load_config(app_config_t *config)
{
    if (!config)
        return ESP_ERR_INVALID_ARG;
    memset(config, 0, sizeof(*config));

    // Defaults
    config->first_boot = true;
    config->update_interval = DEFAULT_UPDATE_INTERVAL_SEC;
    config->brightness_percent = DEFAULT_BRIGHTNESS_PERCENT;
    config->display_rotation_deg = DISPLAY_ROTATION;
    strncpy(config->timezone_posix, DEFAULT_TIMEZONE_POSIX, sizeof(config->timezone_posix) - 1);
    config->night_dim_enabled = false;
    config->night_dim_start_hour = DEFAULT_NIGHT_DIM_START_HOUR;
    config->night_dim_end_hour = DEFAULT_NIGHT_DIM_END_HOUR;
    config->night_dim_brightness_percent = DEFAULT_NIGHT_DIM_BRIGHTNESS;
    config->web_auth_enabled = false;
    config->slideshow_enabled = false;
    config->slideshow_interval_sec = DEFAULT_SLIDESHOW_INTERVAL_SEC;
    config->ota_auto_check = false;

    // 문자열 값들 (각자 자체적으로 핸들을 열고 닫는 안전한 헬퍼)
    (void)nvs_read_string(NVS_KEY_WIFI_SSID, config->wifi.ssid, sizeof(config->wifi.ssid));
    (void)nvs_read_string(NVS_KEY_WIFI_PASS, config->wifi.password, sizeof(config->wifi.password));
    (void)nvs_read_string(NVS_KEY_API_KEY, config->api_key, sizeof(config->api_key));
    (void)nvs_read_string(NVS_KEY_CITY_NAME, config->city_name, sizeof(config->city_name));
    (void)nvs_read_string(NVS_KEY_TIMEZONE, config->timezone_posix, sizeof(config->timezone_posix));
    (void)nvs_read_string(NVS_KEY_WEB_AUTH_USER, config->web_auth_user, sizeof(config->web_auth_user));
    (void)nvs_read_string(NVS_KEY_WEB_AUTH_PASS, config->web_auth_pass, sizeof(config->web_auth_pass));
    (void)nvs_read_string(NVS_KEY_OTA_MANIFEST_URL, config->ota_manifest_url, sizeof(config->ota_manifest_url));

    // 숫자/불리언 값들은 핸들 하나로 한 번에 읽는다
    nvs_handle_t h;
    if (nvs_open_ns(&h, NVS_READONLY) == ESP_OK)
    {
        uint8_t u8v;
        uint16_t u16v;
        uint32_t u32v;

        if (nvs_get_u8(h, NVS_KEY_WIFI_CONFIGURED, &u8v) == ESP_OK)
        {
            config->wifi.configured = (u8v != 0);
        }
        else
        {
            config->wifi.configured = (config->wifi.ssid[0] != '\0'); // SSID 존재로 추론
        }

        if (nvs_get_u8(h, NVS_KEY_FIRST_BOOT, &u8v) == ESP_OK)
            config->first_boot = (u8v != 0);
        if (nvs_get_u32(h, NVS_KEY_UPDATE_INTERVAL, &u32v) == ESP_OK && u32v > 0)
            config->update_interval = u32v;

        if (nvs_get_u8(h, NVS_KEY_BRIGHTNESS, &u8v) == ESP_OK)
            config->brightness_percent = u8v;
        if (nvs_get_u16(h, NVS_KEY_ROTATION, &u16v) == ESP_OK)
            config->display_rotation_deg = u16v;
        if (nvs_get_u8(h, NVS_KEY_NIGHT_DIM_EN, &u8v) == ESP_OK)
            config->night_dim_enabled = (u8v != 0);
        if (nvs_get_u8(h, NVS_KEY_NIGHT_DIM_START, &u8v) == ESP_OK)
            config->night_dim_start_hour = u8v;
        if (nvs_get_u8(h, NVS_KEY_NIGHT_DIM_END, &u8v) == ESP_OK)
            config->night_dim_end_hour = u8v;
        if (nvs_get_u8(h, NVS_KEY_NIGHT_DIM_BRIGHT, &u8v) == ESP_OK)
            config->night_dim_brightness_percent = u8v;
        if (nvs_get_u8(h, NVS_KEY_WEB_AUTH_EN, &u8v) == ESP_OK)
            config->web_auth_enabled = (u8v != 0);
        if (nvs_get_u8(h, NVS_KEY_SLIDESHOW_EN, &u8v) == ESP_OK)
            config->slideshow_enabled = (u8v != 0);
        if (nvs_get_u16(h, NVS_KEY_SLIDESHOW_INT, &u16v) == ESP_OK && u16v > 0)
            config->slideshow_interval_sec = u16v;
        if (nvs_get_u8(h, NVS_KEY_OTA_AUTO_CHECK, &u8v) == ESP_OK)
            config->ota_auto_check = (u8v != 0);

        nvs_close(h);
    }
    else
    {
        // 네임스페이스가 아직 없음(최초 부팅) — SSID 유무로만 추론
        config->wifi.configured = (config->wifi.ssid[0] != '\0');
    }

    return ESP_OK;
}

esp_err_t nvs_manager_save_config(const app_config_t *config)
{
    if (!config)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;

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

    // System 탭 / 온디바이스 설정
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_BRIGHTNESS, config->brightness_percent));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u16(h, NVS_KEY_ROTATION, config->display_rotation_deg));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_TIMEZONE, config->timezone_posix));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_NIGHT_DIM_EN, config->night_dim_enabled ? 1 : 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_NIGHT_DIM_START, config->night_dim_start_hour));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_NIGHT_DIM_END, config->night_dim_end_hour));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_NIGHT_DIM_BRIGHT, config->night_dim_brightness_percent));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_WEB_AUTH_EN, config->web_auth_enabled ? 1 : 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WEB_AUTH_USER, config->web_auth_user));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WEB_AUTH_PASS, config->web_auth_pass));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_SLIDESHOW_EN, config->slideshow_enabled ? 1 : 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u16(h, NVS_KEY_SLIDESHOW_INT, config->slideshow_interval_sec));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_OTA_MANIFEST_URL, config->ota_manifest_url));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_OTA_AUTO_CHECK, config->ota_auto_check ? 1 : 0));

    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_brightness(uint8_t percent)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_BRIGHTNESS, percent));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_rotation(uint16_t degrees)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u16(h, NVS_KEY_ROTATION, degrees));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_timezone(const char *timezone_posix)
{
    if (!timezone_posix)
        return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_TIMEZONE, timezone_posix));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_night_dim(bool enabled, uint8_t start_hour, uint8_t end_hour, uint8_t dim_brightness_percent)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_NIGHT_DIM_EN, enabled ? 1 : 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_NIGHT_DIM_START, start_hour));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_NIGHT_DIM_END, end_hour));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_NIGHT_DIM_BRIGHT, dim_brightness_percent));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_slideshow(bool enabled, uint16_t interval_sec)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_SLIDESHOW_EN, enabled ? 1 : 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u16(h, NVS_KEY_SLIDESHOW_INT, interval_sec));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_ota_manifest_url(const char *url)
{
    if (!url)
        return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_OTA_MANIFEST_URL, url));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_ota_auto_check(bool enabled)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_OTA_AUTO_CHECK, enabled ? 1 : 0));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_web_auth(bool enabled, const char *user, const char *pass)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_WEB_AUTH_EN, enabled ? 1 : 0));
    if (user)
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WEB_AUTH_USER, user));
    if (pass)
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WEB_AUTH_PASS, pass));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_wifi_config(const char *ssid, const char *password)
{
    if (!ssid)
        return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;

    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WIFI_SSID, ssid));
    if (password)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WIFI_PASS, password));
    }
    else
    {
        // store empty password
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_WIFI_PASS, ""));
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_WIFI_CONFIGURED, 1));

    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_api_key(const char *api_key)
{
    if (!api_key)
        return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_API_KEY, api_key));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_save_city_name(const char *city_name)
{
    if (!city_name)
        return ESP_ERR_INVALID_ARG;
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(h, NVS_KEY_CITY_NAME, city_name));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_set_first_boot(bool first_boot)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(h, NVS_KEY_FIRST_BOOT, first_boot ? 1 : 0));
    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_clear_wifi_config(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;

    // 키가 아직 없을 수 있으므로 ESP_ERR_NVS_NOT_FOUND는 무시한다
    esp_err_t e1 = nvs_erase_key(h, NVS_KEY_WIFI_SSID);
    esp_err_t e2 = nvs_erase_key(h, NVS_KEY_WIFI_PASS);
    esp_err_t e3 = nvs_erase_key(h, NVS_KEY_WIFI_CONFIGURED);
    (void)e1;
    (void)e2;
    (void)e3;

    err = nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t nvs_manager_erase_all(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open_ns(&h, NVS_READWRITE);
    if (err != ESP_OK)
        return err;
    err = nvs_erase_all(h);
    if (err == ESP_OK)
        err = nvs_commit(h);
    nvs_close(h);
    return err;
}

// ---- Convenience helpers used by other modules ----

// Expose a simple getter to align with existing call sites
esp_err_t nvs_get_string(const char *key, char *out, size_t out_len)
{
    return nvs_read_string(key, out, out_len);
}

esp_err_t nvs_get_wifi_config(wifi_config_t *wifi_config)
{
    if (!wifi_config)
        return ESP_ERR_INVALID_ARG;
    memset(wifi_config, 0, sizeof(*wifi_config));

    char ssid[WIFI_SSID_MAX_LEN + 1] = {0};
    char pass[WIFI_PASS_MAX_LEN + 1] = {0};

    esp_err_t err_ssid = nvs_read_string(NVS_KEY_WIFI_SSID, ssid, sizeof(ssid));
    esp_err_t err_pass = nvs_read_string(NVS_KEY_WIFI_PASS, pass, sizeof(pass));

    if (err_ssid != ESP_OK)
    {
        return err_ssid; // SSID is required
    }

    // Copy into wifi_config_t
    strncpy((char *)wifi_config->sta.ssid, ssid, sizeof(wifi_config->sta.ssid) - 1);
    if (err_pass == ESP_OK)
    {
        strncpy((char *)wifi_config->sta.password, pass, sizeof(wifi_config->sta.password) - 1);
    }
    else
    {
        wifi_config->sta.password[0] = '\0';
    }

    return ESP_OK;
}
