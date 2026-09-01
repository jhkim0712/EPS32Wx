#ifndef WEATHER_INTERFACE_H
#define WEATHER_INTERFACE_H

#include <esp_err.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 범용 날씨 데이터 구조체 (모든 API에서 공통으로 사용)
typedef struct {
    // 기본 정보
    char city_name[32];
    char country[8];
    char provider[16];           // API 제공자 (OpenWeatherMap, AccuWeather 등)
    double latitude;             // 위도
    double longitude;            // 경도
    
    // 온도 정보 (섭씨)
    float temperature;
    float feels_like;
    float temp_min;
    float temp_max;
    
    // 대기 정보
    int humidity;                // 습도 (%)
    int pressure;                // 기압 (hPa)
    int visibility;              // 가시거리 (m)
    
    // 날씨 상태
    int condition_id;            // 날씨 ID (각 API마다 다름)
    char condition_main[32];     // 주요 날씨
    char condition_desc[64];     // 상세 설명
    char icon[16];               // 아이콘 ID
    
    // 바람 정보
    float wind_speed;            // 풍속 (m/s)
    int wind_direction;          // 풍향 (도)
    
    // 구름 정보
    int cloudiness;              // 구름량 (%)
    
    // 시간 정보
    int64_t timestamp;           // 데이터 시간
    int64_t sunrise;             // 일출 시간
    int64_t sunset;              // 일몰 시간
    
    // 시스템 정보
    bool is_valid;
    int64_t last_update;
} weather_data_t;

// 날씨 API 제공자 열거형
typedef enum {
    WEATHER_PROVIDER_OPENWEATHERMAP = 0,
    WEATHER_PROVIDER_ACCUWEATHER,
    WEATHER_PROVIDER_WEATHERAPI,
    WEATHER_PROVIDER_MAX
} weather_provider_t;

// 날씨 응답 콜백 함수 타입
typedef void (*weather_callback_t)(const weather_data_t *weather_data, esp_err_t result);

/**
 * @brief 날씨 인터페이스 초기화
 * @param provider 사용할 날씨 API 제공자
 * @return ESP_OK on success
 */
esp_err_t weather_init(weather_provider_t provider);

/**
 * @brief 현재 날씨 요청 (제공자 지정)
 * @param provider 사용할 날씨 API 제공자
 * @param city_name 도시명
 * @param api_key API 키
 * @param callback 결과 콜백
 * @return ESP_OK on success, error otherwise
 */
esp_err_t weather_request_current(weather_provider_t provider, const char *city_name, const char *api_key, weather_callback_t callback);

/**
 * @brief 현재 날씨 정보 요청 (범용 인터페이스)
 * @param api_key API 키
 * @param city_name 도시명
 * @param callback 결과 콜백 함수
 * @return ESP_OK on success
 */
esp_err_t weather_get_current(const char *api_key, const char *city_name, weather_callback_t callback);

/**
 * @brief 예보 정보 요청 (범용 인터페이스)
 * @param api_key API 키
 * @param city_name 도시명
 * @param callback 결과 콜백 함수
 * @return ESP_OK on success
 */
esp_err_t weather_get_forecast(const char *api_key, const char *city_name, weather_callback_t callback);

/**
 * @brief 날씨 데이터 유효성 검사
 * @param weather_data 검사할 날씨 데이터
 * @return true if valid, false otherwise
 */
bool weather_validate_data(const weather_data_t *weather_data);

/**
 * @brief 날씨 데이터 로그 출력
 * @param weather_data 출력할 날씨 데이터
 */
void weather_log_data(const weather_data_t *weather_data);

/**
 * @brief 현재 사용 중인 날씨 API 제공자 반환
 * @return 현재 제공자
 */
weather_provider_t weather_get_current_provider(void);

/**
 * @brief 제공자명 문자열 반환
 * @param provider 제공자 열거형
 * @return 제공자명 문자열
 */
const char* weather_get_provider_name(weather_provider_t provider);

#ifdef __cplusplus
}
#endif

#endif // WEATHER_INTERFACE_H