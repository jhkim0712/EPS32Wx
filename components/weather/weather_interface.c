#include "weather/weather_interface.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "WEATHER_IF";
static weather_provider_t s_provider = WEATHER_PROVIDER_OPENWEATHERMAP;

esp_err_t weather_init(weather_provider_t provider)
{
    s_provider = provider;
    (void)provider;
    return ESP_OK;
}

weather_provider_t weather_get_current_provider(void)
{
    return s_provider;
}

const char* weather_get_provider_name(weather_provider_t provider)
{
    switch (provider) {
    case WEATHER_PROVIDER_OPENWEATHERMAP: return "OpenWeatherMap";
    case WEATHER_PROVIDER_ACCUWEATHER:    return "AccuWeather";
    case WEATHER_PROVIDER_WEATHERAPI:     return "WeatherAPI";
    default: return "Unknown";
    }
}

static void fill_stub(weather_data_t *dst, const char *city)
{
    memset(dst, 0, sizeof(*dst));
    if (city) strncpy(dst->city_name, city, sizeof(dst->city_name)-1);
    strncpy(dst->country, "KR", sizeof(dst->country)-1);
    strncpy(dst->provider, weather_get_provider_name(s_provider), sizeof(dst->provider)-1);
    dst->temperature = 23.5f;
    dst->feels_like = 24.0f;
    dst->temp_min = 20.0f;
    dst->temp_max = 26.0f;
    dst->humidity = 55;
    dst->pressure = 1013;
    dst->visibility = 10000;
    dst->condition_id = 800;
    strncpy(dst->condition_main, "Clear", sizeof(dst->condition_main)-1);
    strncpy(dst->condition_desc, "Clear sky (stub)", sizeof(dst->condition_desc)-1);
    strncpy(dst->icon, "01d", sizeof(dst->icon)-1);
    dst->wind_speed = 1.5f;
    dst->wind_direction = 90;
    dst->cloudiness = 0;
    dst->timestamp = 0;
    dst->sunrise = 0;
    dst->sunset = 0;
    dst->is_valid = true;
    dst->last_update = 0;
}

// Bridge callback to convert provider-specific payload to generic weather_data_t
// Placeholder for future provider-specific async bridge

esp_err_t weather_request_current(weather_provider_t provider, const char *city_name, const char *api_key, weather_callback_t callback)
{
    if (!city_name || !api_key || !callback) return ESP_ERR_INVALID_ARG;
    // Ensure provider init
    if (provider != s_provider) {
        esp_err_t err = weather_init(provider);
        if (err != ESP_OK) return err;
    }

    (void)api_key;
    // For now, return a stub synchronously
    weather_data_t out;
    fill_stub(&out, city_name);
    callback(&out, ESP_OK);
    return ESP_OK;
}

esp_err_t weather_get_current(const char *api_key, const char *city_name, weather_callback_t callback)
{
    return weather_request_current(s_provider, city_name, api_key, callback);
}

esp_err_t weather_get_forecast(const char *api_key, const char *city_name, weather_callback_t callback)
{
    (void)api_key; (void)city_name; (void)callback;
    return ESP_ERR_NOT_SUPPORTED; // Not implemented yet
}

bool weather_validate_data(const weather_data_t *weather_data)
{
    return weather_data && weather_data->is_valid && strlen(weather_data->city_name) > 0;
}

void weather_log_data(const weather_data_t *weather_data)
{
    if (!weather_data) return;
    ESP_LOGI(TAG, "%s: %.1fC, %d%%, %s", weather_data->city_name, weather_data->temperature, weather_data->humidity, weather_data->condition_desc);
}
