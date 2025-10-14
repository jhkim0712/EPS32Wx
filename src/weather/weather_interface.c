#include "weather/weather_interface.h"
#include "weather/openweathermap_api.h"
#include <esp_log.h>
#include <string.h>

static const char *TAG = "WEATHER_INTERFACE";

// 현재 사용 중인 날씨 API 제공자
static weather_provider_t current_provider = WEATHER_PROVIDER_OPENWEATHERMAP;

// 콜백 함수 저장용
static weather_callback_t g_weather_callback = NULL;

// OpenWeatherMap 응답 콜백 (어댑터 함수)
static void openweathermap_response_adapter(const openweathermap_data_t *owm_data, esp_err_t result)
{
    if (g_weather_callback == NULL) {
        return;
    }
    
    if (result != ESP_OK || owm_data == NULL) {
        g_weather_callback(NULL, result);
        return;
    }
    
    // OpenWeatherMap 데이터를 범용 날씨 데이터로 변환
    weather_data_t weather_data = {0};
    
    // 기본 정보
    strncpy(weather_data.city_name, owm_data->city_name, sizeof(weather_data.city_name) - 1);
    strncpy(weather_data.country, owm_data->country, sizeof(weather_data.country) - 1);
    strncpy(weather_data.provider, "OpenWeatherMap", sizeof(weather_data.provider) - 1);
    weather_data.latitude = owm_data->latitude;
    weather_data.longitude = owm_data->longitude;
    
    // 온도 정보
    weather_data.temperature = owm_data->temperature;
    weather_data.feels_like = owm_data->feels_like;
    weather_data.temp_min = owm_data->temp_min;
    weather_data.temp_max = owm_data->temp_max;
    
    // 대기 정보
    weather_data.humidity = owm_data->humidity;
    weather_data.pressure = owm_data->pressure;
    weather_data.visibility = owm_data->visibility;
    
    // 날씨 상태
    weather_data.condition_id = owm_data->condition_id;
    strncpy(weather_data.condition_main, owm_data->condition_main, sizeof(weather_data.condition_main) - 1);
    strncpy(weather_data.condition_desc, owm_data->condition_desc, sizeof(weather_data.condition_desc) - 1);
    strncpy(weather_data.icon, owm_data->icon, sizeof(weather_data.icon) - 1);
    
    // 바람 정보
    weather_data.wind_speed = owm_data->wind_speed;
    weather_data.wind_direction = owm_data->wind_direction;
    
    // 구름 정보
    weather_data.cloudiness = owm_data->cloudiness;
    
    // 시간 정보
    weather_data.timestamp = owm_data->timestamp;
    weather_data.sunrise = owm_data->sunrise;
    weather_data.sunset = owm_data->sunset;
    
    // 시스템 정보
    weather_data.is_valid = owm_data->is_valid;
    weather_data.last_update = owm_data->last_update;
    
    // 콜백 호출
    g_weather_callback(&weather_data, ESP_OK);
}

esp_err_t weather_init(weather_provider_t provider)
{
    ESP_LOGI(TAG, "날씨 인터페이스 초기화: %s", weather_get_provider_name(provider));
    
    current_provider = provider;
    
    switch (provider) {
        case WEATHER_PROVIDER_OPENWEATHERMAP:
            return openweathermap_api_init();
            
        case WEATHER_PROVIDER_ACCUWEATHER:
            // TODO: AccuWeather API 초기화
            ESP_LOGW(TAG, "AccuWeather API는 아직 구현되지 않았습니다");
            return ESP_ERR_NOT_SUPPORTED;
            
        case WEATHER_PROVIDER_WEATHERAPI:
            // TODO: WeatherAPI 초기화
            ESP_LOGW(TAG, "WeatherAPI는 아직 구현되지 않았습니다");
            return ESP_ERR_NOT_SUPPORTED;
            
        default:
            ESP_LOGE(TAG, "지원하지 않는 날씨 API 제공자: %d", provider);
            return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t weather_get_current(const char *api_key, const char *city_name, weather_callback_t callback)
{
    if (api_key == NULL || city_name == NULL || callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    g_weather_callback = callback;
    
    switch (current_provider) {
        case WEATHER_PROVIDER_OPENWEATHERMAP:
            return openweathermap_api_get_current(api_key, city_name, openweathermap_response_adapter);
            
        case WEATHER_PROVIDER_ACCUWEATHER:
            // TODO: AccuWeather API 호출
            ESP_LOGW(TAG, "AccuWeather API는 아직 구현되지 않았습니다");
            return ESP_ERR_NOT_SUPPORTED;
            
        case WEATHER_PROVIDER_WEATHERAPI:
            // TODO: WeatherAPI 호출
            ESP_LOGW(TAG, "WeatherAPI는 아직 구현되지 않았습니다");
            return ESP_ERR_NOT_SUPPORTED;
            
        default:
            ESP_LOGE(TAG, "지원하지 않는 날씨 API 제공자: %d", current_provider);
            return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t weather_get_forecast(const char *api_key, const char *city_name, weather_callback_t callback)
{
    if (api_key == NULL || city_name == NULL || callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    g_weather_callback = callback;
    
    switch (current_provider) {
        case WEATHER_PROVIDER_OPENWEATHERMAP:
            return openweathermap_api_get_forecast(api_key, city_name, openweathermap_response_adapter);
            
        case WEATHER_PROVIDER_ACCUWEATHER:
            // TODO: AccuWeather 예보 API 호출
            ESP_LOGW(TAG, "AccuWeather 예보 API는 아직 구현되지 않았습니다");
            return ESP_ERR_NOT_SUPPORTED;
            
        case WEATHER_PROVIDER_WEATHERAPI:
            // TODO: WeatherAPI 예보 호출
            ESP_LOGW(TAG, "WeatherAPI 예보는 아직 구현되지 않았습니다");
            return ESP_ERR_NOT_SUPPORTED;
            
        default:
            ESP_LOGE(TAG, "지원하지 않는 날씨 API 제공자: %d", current_provider);
            return ESP_ERR_INVALID_ARG;
    }
}

bool weather_validate_data(const weather_data_t *weather_data)
{
    if (weather_data == NULL) {
        return false;
    }
    
    // 기본 유효성 검사
    if (!weather_data->is_valid) {
        return false;
    }
    
    // 온도 범위 검사 (-100°C ~ 60°C)
    if (weather_data->temperature < -100.0f || weather_data->temperature > 60.0f) {
        ESP_LOGW(TAG, "온도 범위 초과: %.1f°C", weather_data->temperature);
        return false;
    }
    
    // 습도 범위 검사 (0% ~ 100%)
    if (weather_data->humidity < 0 || weather_data->humidity > 100) {
        ESP_LOGW(TAG, "습도 범위 초과: %d%%", weather_data->humidity);
        return false;
    }
    
    // 도시명 확인
    if (strlen(weather_data->city_name) == 0) {
        ESP_LOGW(TAG, "도시명이 없습니다");
        return false;
    }
    
    return true;
}

void weather_log_data(const weather_data_t *weather_data)
{
    if (weather_data == NULL) {
        ESP_LOGW(TAG, "날씨 데이터가 NULL입니다");
        return;
    }
    
    ESP_LOGI(TAG, "=== 날씨 정보 (%s) ===", weather_data->provider);
    ESP_LOGI(TAG, "도시: %s, %s (%.6f, %.6f)", weather_data->city_name, weather_data->country,
             weather_data->latitude, weather_data->longitude);
    ESP_LOGI(TAG, "온도: %.1f°C (체감 %.1f°C)", weather_data->temperature, weather_data->feels_like);
    ESP_LOGI(TAG, "최저/최고: %.1f°C / %.1f°C", weather_data->temp_min, weather_data->temp_max);
    ESP_LOGI(TAG, "습도: %d%%, 기압: %dhPa", weather_data->humidity, weather_data->pressure);
    ESP_LOGI(TAG, "날씨: %s (%s)", weather_data->condition_main, weather_data->condition_desc);
    ESP_LOGI(TAG, "바람: %.1fm/s (%.0f°)", weather_data->wind_speed, (float)weather_data->wind_direction);
    ESP_LOGI(TAG, "구름: %d%%, 가시거리: %dm", weather_data->cloudiness, weather_data->visibility);
    ESP_LOGI(TAG, "아이콘: %s", weather_data->icon);
    ESP_LOGI(TAG, "유효성: %s", weather_data->is_valid ? "유효" : "무효");
}

weather_provider_t weather_get_current_provider(void)
{
    return current_provider;
}

const char* weather_get_provider_name(weather_provider_t provider)
{
    switch (provider) {
        case WEATHER_PROVIDER_OPENWEATHERMAP: return "OpenWeatherMap";
        case WEATHER_PROVIDER_ACCUWEATHER: return "AccuWeather";
        case WEATHER_PROVIDER_WEATHERAPI: return "WeatherAPI";
        default: return "Unknown";
    }
}