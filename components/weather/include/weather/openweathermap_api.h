#ifndef OPENWEATHERMAP_API_H
#define OPENWEATHERMAP_API_H

#include <esp_err.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 날씨 상태 코드 (OpenWeatherMap API 기준)
typedef enum {
    WEATHER_CLEAR = 800,           // 맑음
    WEATHER_FEW_CLOUDS = 801,      // 구름 조금
    WEATHER_SCATTERED_CLOUDS = 802, // 구름 많음
    WEATHER_BROKEN_CLOUDS = 803,   // 흐림
    WEATHER_OVERCAST_CLOUDS = 804, // 완전 흐림
    WEATHER_LIGHT_RAIN = 500,      // 가벼운 비
    WEATHER_MODERATE_RAIN = 501,   // 보통 비
    WEATHER_HEAVY_RAIN = 502,      // 강한 비
    WEATHER_LIGHT_SNOW = 600,      // 가벼운 눈
    WEATHER_SNOW = 601,            // 눈
    WEATHER_HEAVY_SNOW = 602,      // 폭설
    WEATHER_MIST = 701,            // 안개
    WEATHER_THUNDERSTORM = 200     // 뇌우
} weather_condition_t;

// 지오코딩 데이터 구조체
typedef struct {
    char city_name[32];
    char country[8];
    char state[32];          // 주/도
    double latitude;         // 위도
    double longitude;        // 경도
    bool is_valid;
} geocoding_data_t;

// 날씨 데이터 구조체 (OpenWeatherMap 전용)
typedef struct {
    // 기본 정보
    char city_name[32];
    char country[8];
    double latitude;         // 위도
    double longitude;        // 경도
    
    // 온도 정보 (섭씨)
    float temperature;        // 현재 온도
    float feels_like;        // 체감 온도
    float temp_min;          // 최저 온도
    float temp_max;          // 최고 온도
    
    // 대기 정보
    int humidity;            // 습도 (%)
    int pressure;            // 기압 (hPa)
    int visibility;          // 가시거리 (m)
    
    // 날씨 상태
    weather_condition_t condition_id;  // 날씨 ID
    char condition_main[32];           // 주요 날씨 (Rain, Snow, Clear 등)
    char condition_desc[64];           // 상세 설명
    char icon[8];                      // 아이콘 ID (01d, 02n 등)
    
    // 바람 정보
    float wind_speed;        // 풍속 (m/s)
    int wind_direction;      // 풍향 (도)
    
    // 구름 정보
    int cloudiness;          // 구름량 (%)
    
    // 시간 정보
    int64_t timestamp;       // 데이터 시간 (Unix timestamp)
    int64_t sunrise;         // 일출 시간
    int64_t sunset;          // 일몰 시간
    
    // 시스템 정보
    bool is_valid;           // 데이터 유효성
    int64_t last_update;     // 마지막 업데이트 시간
} openweathermap_data_t;

// HTTP 응답 콜백 함수 타입
typedef void (*openweathermap_response_cb_t)(const openweathermap_data_t *weather_data, esp_err_t result);
typedef void (*geocoding_response_cb_t)(const geocoding_data_t *geo_data, esp_err_t result);

/**
 * @brief OpenWeatherMap API 모듈 초기화
 * @return ESP_OK on success
 */
esp_err_t openweathermap_api_init(void);

/**
 * @brief OpenWeatherMap API에서 현재 날씨 정보 요청
 * @param api_key API 키
 * @param city_name 도시명 (예: "Seoul", "Tokyo,JP")
 * @param callback 결과를 받을 콜백 함수
 * @return ESP_OK on success
 */
esp_err_t openweathermap_api_get_current(const char *api_key, const char *city_name, openweathermap_response_cb_t callback);

/**
 * @brief OpenWeatherMap JSON 응답 파싱하여 날씨 데이터 구조체로 변환
 * @param json_str OpenWeatherMap API JSON 응답 문자열
 * @param weather_data 파싱된 데이터를 저장할 구조체
 * @return ESP_OK on success, ESP_FAIL on parsing error
 */
esp_err_t openweathermap_api_parse_response(const char *json_str, openweathermap_data_t *weather_data);

/**
 * @brief OpenWeatherMap 5일 예보 정보 요청
 * @param api_key API 키
 * @param city_name 도시명
 * @param callback 결과를 받을 콜백 함수
 * @return ESP_OK on success
 */
esp_err_t openweathermap_api_get_forecast(const char *api_key, const char *city_name, openweathermap_response_cb_t callback);

/**
 * @brief 날씨 상태 코드를 한국어 설명으로 변환
 * @param condition_id 날씨 상태 코드
 * @return 한국어 날씨 설명 문자열
 */
const char* openweathermap_get_condition_desc_ko(weather_condition_t condition_id);

/**
 * @brief 풍향을 한국어로 변환 (예: 북, 북동, 동 등)
 * @param degrees 풍향 각도 (0-360)
 * @return 한국어 풍향 문자열
 */
const char* openweathermap_get_wind_direction_ko(int degrees);

/**
 * @brief OpenWeatherMap 아이콘 ID를 아이콘 설명으로 변환
 * @param icon_id 아이콘 ID (예: "01d", "02n")
 * @return 아이콘 설명 문자열
 */
const char* openweathermap_get_icon_desc(const char *icon_id);

/**
 * @brief Kelvin을 Celsius로 변환
 * @param kelvin 켈빈 온도
 * @return 섭씨 온도
 */
float openweathermap_kelvin_to_celsius(float kelvin);

/**
 * @brief OpenWeatherMap 날씨 데이터 유효성 검사
 * @param weather_data 검사할 날씨 데이터
 * @return true if valid, false otherwise
 */
bool openweathermap_validate_data(const openweathermap_data_t *weather_data);

/**
 * @brief OpenWeatherMap 날씨 데이터를 로그로 출력 (디버깅용)
 * @param weather_data 출력할 날씨 데이터
 */
void openweathermap_log_data(const openweathermap_data_t *weather_data);

/**
 * @brief 도시명으로 지오코딩하여 위도/경도 얻기
 * @param api_key API 키
 * @param city_name 도시명 (예: "Seoul", "Seoul,KR")
 * @param callback 결과를 받을 콜백 함수
 * @return ESP_OK on success
 */
esp_err_t openweathermap_geocoding(const char *api_key, const char *city_name, geocoding_response_cb_t callback);

/**
 * @brief 위도/경도로 직접 날씨 정보 요청
 * @param api_key API 키
 * @param latitude 위도
 * @param longitude 경도
 * @param callback 결과를 받을 콜백 함수
 * @return ESP_OK on success
 */
esp_err_t openweathermap_api_get_current_by_coords(const char *api_key, double latitude, double longitude, 
                                                   openweathermap_response_cb_t callback);

/**
 * @brief 지오코딩 JSON 응답 파싱
 * @param json_str 지오코딩 API JSON 응답 문자열
 * @param geo_data 파싱된 데이터를 저장할 구조체
 * @return ESP_OK on success, ESP_FAIL on parsing error
 */
esp_err_t openweathermap_parse_geocoding_response(const char *json_str, geocoding_data_t *geo_data);

/**
 * @brief OpenWeatherMap API 요청 URL 생성 (위도/경도 기반)
 * @param base_url 기본 URL
 * @param latitude 위도
 * @param longitude 경도
 * @param api_key API 키
 * @param url_buffer 생성된 URL을 저장할 버퍼
 * @param buffer_size 버퍼 크기
 * @return ESP_OK on success
 */
esp_err_t openweathermap_build_url_coords(const char *base_url, double latitude, double longitude, 
                                          const char *api_key, char *url_buffer, size_t buffer_size);

/**
 * @brief 지오코딩 URL 생성
 * @param city_name 도시명
 * @param api_key API 키
 * @param url_buffer 생성된 URL을 저장할 버퍼
 * @param buffer_size 버퍼 크기
 * @return ESP_OK on success
 */
esp_err_t openweathermap_build_geocoding_url(const char *city_name, const char *api_key, 
                                              char *url_buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // OPENWEATHERMAP_API_H