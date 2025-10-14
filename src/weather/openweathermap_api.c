#include "weather/openweathermap_api.h"
#include "common/config.h"
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_tls.h>
#include <cJSON.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "OPENWEATHERMAP_API";

// HTTP 응답 버퍼
#define HTTP_RESPONSE_BUFFER_SIZE   4096
static char http_response_buffer[HTTP_RESPONSE_BUFFER_SIZE];
static int http_response_len = 0;

// 콜백 함수 저장용
static openweathermap_response_cb_t g_response_callback = NULL;
static geocoding_response_cb_t g_geocoding_callback = NULL;

// 현재 사용 중인 API 키 (지오코딩 -> 날씨 요청 연계용)
static char g_current_api_key[API_KEY_MAX_LEN] = {0};

esp_err_t openweathermap_api_init(void)
{
    ESP_LOGI(TAG, "OpenWeatherMap API 모듈 초기화");
    
    // HTTP 응답 버퍼 초기화
    memset(http_response_buffer, 0, HTTP_RESPONSE_BUFFER_SIZE);
    http_response_len = 0;
    
    return ESP_OK;
}

// HTTP 이벤트 핸들러
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP 오류 발생");
            break;
            
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP 연결됨");
            break;
            
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP 헤더 전송됨");
            break;
            
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP 헤더 수신: %s: %s", evt->header_key, evt->header_value);
            break;
            
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP 데이터 수신, 길이=%d", evt->data_len);
            
            // 응답 데이터를 버퍼에 추가
            if (http_response_len + evt->data_len < HTTP_RESPONSE_BUFFER_SIZE - 1) {
                memcpy(http_response_buffer + http_response_len, evt->data, evt->data_len);
                http_response_len += evt->data_len;
                http_response_buffer[http_response_len] = '\0';
            } else {
                ESP_LOGW(TAG, "HTTP 응답 버퍼 초과");
            }
            break;
            
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP 요청 완료, 총 길이=%d", http_response_len);
            break;
            
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP 연결 해제됨");
            break;
            
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t openweathermap_build_url_coords(const char *base_url, double latitude, double longitude, 
                                          const char *api_key, char *url_buffer, size_t buffer_size)
{
    if (base_url == NULL || api_key == NULL || url_buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 위도/경도 기반 URL 생성
    int ret = snprintf(url_buffer, buffer_size, "%s?lat=%.6f&lon=%.6f&appid=%s&units=metric&lang=kr", 
                       base_url, latitude, longitude, api_key);
    
    if (ret >= buffer_size) {
        ESP_LOGE(TAG, "URL 버퍼 크기 초과");
        return ESP_ERR_NO_MEM;
    }
    
    return ESP_OK;
}

esp_err_t openweathermap_build_geocoding_url(const char *city_name, const char *api_key, 
                                              char *url_buffer, size_t buffer_size)
{
    if (city_name == NULL || api_key == NULL || url_buffer == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 지오코딩 URL 생성
    int ret = snprintf(url_buffer, buffer_size, "%s?q=%s&limit=1&appid=%s", 
                       OPENWEATHERMAP_GEOCODING_API_URL, city_name, api_key);
    
    if (ret >= buffer_size) {
        ESP_LOGE(TAG, "지오코딩 URL 버퍼 크기 초과");
        return ESP_ERR_NO_MEM;
    }
    
    return ESP_OK;
}

// 지오코딩 완료 후 날씨 정보 요청하는 내부 콜백
static void geocoding_complete_callback(const geocoding_data_t *geo_data, esp_err_t result)
{
    if (result != ESP_OK || geo_data == NULL || !geo_data->is_valid) {
        ESP_LOGE(TAG, "지오코딩 실패");
        if (g_response_callback) {
            g_response_callback(NULL, ESP_FAIL);
        }
        return;
    }
    
    ESP_LOGI(TAG, "지오코딩 성공: %s (%.6f, %.6f)", 
             geo_data->city_name, geo_data->latitude, geo_data->longitude);
    
    // 위도/경도로 날씨 정보 요청
    esp_err_t weather_err = openweathermap_api_get_current_by_coords(
        g_current_api_key, geo_data->latitude, geo_data->longitude, g_response_callback);
    
    if (weather_err != ESP_OK) {
        ESP_LOGE(TAG, "날씨 정보 요청 실패");
        if (g_response_callback) {
            g_response_callback(NULL, weather_err);
        }
    }
}

esp_err_t openweathermap_api_get_current(const char *api_key, const char *city_name, openweathermap_response_cb_t callback)
{
    if (api_key == NULL || city_name == NULL || callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // API 키 저장 (지오코딩 완료 후 날씨 요청에 사용)
    strncpy(g_current_api_key, api_key, sizeof(g_current_api_key) - 1);
    g_current_api_key[sizeof(g_current_api_key) - 1] = '\0';
    
    // 콜백 함수 저장
    g_response_callback = callback;
    
    ESP_LOGI(TAG, "OpenWeatherMap API 요청 시작: %s", city_name);
    
    // 먼저 지오코딩으로 위도/경도 얻기
    return openweathermap_geocoding(api_key, city_name, geocoding_complete_callback);
}

esp_err_t openweathermap_api_get_current_by_coords(const char *api_key, double latitude, double longitude, 
                                                   openweathermap_response_cb_t callback)
{
    if (api_key == NULL || callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 콜백 함수 저장
    g_response_callback = callback;
    
    // API URL 생성
    char url[512];
    esp_err_t url_err = openweathermap_build_url_coords(OPENWEATHERMAP_CURRENT_API_URL, latitude, longitude, api_key, url, sizeof(url));
    if (url_err != ESP_OK) {
        ESP_LOGE(TAG, "URL 생성 실패");
        return url_err;
    }
    
    ESP_LOGI(TAG, "OpenWeatherMap API 요청 (위도/경도): %.6f, %.6f", latitude, longitude);
    ESP_LOGD(TAG, "요청 URL: %s", url);
    
    // HTTP 응답 버퍼 초기화
    memset(http_response_buffer, 0, HTTP_RESPONSE_BUFFER_SIZE);
    http_response_len = 0;
    
    // HTTP 클라이언트 설정
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = OPENWEATHERMAP_API_TIMEOUT_MS,
        .buffer_size = 1024,
        .buffer_size_tx = 1024
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP 클라이언트 초기화 실패");
        return ESP_FAIL;
    }
    
    // HTTP GET 요청 수행
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        
        ESP_LOGI(TAG, "HTTP 응답: 상태=%d, 길이=%d", status_code, content_length);
        
        if (status_code == 200) {
            // JSON 파싱 및 콜백 호출
            openweathermap_data_t weather_data = {0};
            esp_err_t parse_result = openweathermap_api_parse_response(http_response_buffer, &weather_data);
            
            if (g_response_callback) {
                g_response_callback(&weather_data, parse_result);
            }
            
            err = parse_result;
        } else {
            ESP_LOGE(TAG, "HTTP 오류 응답: %d", status_code);
            if (g_response_callback) {
                g_response_callback(NULL, ESP_FAIL);
            }
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP 요청 실패: %s", esp_err_to_name(err));
        if (g_response_callback) {
            g_response_callback(NULL, err);
        }
    }
    
    esp_http_client_cleanup(client);
    return err;
}

esp_err_t openweathermap_geocoding(const char *api_key, const char *city_name, geocoding_response_cb_t callback)
{
    if (api_key == NULL || city_name == NULL || callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 콜백 함수 저장
    g_geocoding_callback = callback;
    
    // 지오코딩 API URL 생성
    char url[512];
    esp_err_t url_err = openweathermap_build_geocoding_url(city_name, api_key, url, sizeof(url));
    if (url_err != ESP_OK) {
        ESP_LOGE(TAG, "지오코딩 URL 생성 실패");
        return url_err;
    }
    
    ESP_LOGI(TAG, "지오코딩 API 요청: %s", city_name);
    ESP_LOGD(TAG, "지오코딩 URL: %s", url);
    
    // HTTP 응답 버퍼 초기화
    memset(http_response_buffer, 0, HTTP_RESPONSE_BUFFER_SIZE);
    http_response_len = 0;
    
    // HTTP 클라이언트 설정
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = OPENWEATHERMAP_API_TIMEOUT_MS,
        .buffer_size = 1024,
        .buffer_size_tx = 1024
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "지오코딩 HTTP 클라이언트 초기화 실패");
        return ESP_FAIL;
    }
    
    // HTTP GET 요청 수행
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        
        ESP_LOGI(TAG, "지오코딩 HTTP 응답: 상태=%d, 길이=%d", status_code, content_length);
        
        if (status_code == 200) {
            // JSON 파싱 및 콜백 호출
            geocoding_data_t geo_data = {0};
            esp_err_t parse_result = openweathermap_parse_geocoding_response(http_response_buffer, &geo_data);
            
            if (g_geocoding_callback) {
                g_geocoding_callback(&geo_data, parse_result);
            }
            
            err = parse_result;
        } else {
            ESP_LOGE(TAG, "지오코딩 HTTP 오류 응답: %d", status_code);
            if (g_geocoding_callback) {
                g_geocoding_callback(NULL, ESP_FAIL);
            }
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "지오코딩 HTTP 요청 실패: %s", esp_err_to_name(err));
        if (g_geocoding_callback) {
            g_geocoding_callback(NULL, err);
        }
    }
    
    esp_http_client_cleanup(client);
    return err;
}

esp_err_t openweathermap_api_get_forecast(const char *api_key, const char *city_name, openweathermap_response_cb_t callback)
{
    if (api_key == NULL || city_name == NULL || callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 5일 예보 API URL
    char forecast_url[512];
    snprintf(forecast_url, sizeof(forecast_url), 
             "http://api.openweathermap.org/data/2.5/forecast?q=%s&appid=%s&units=metric&lang=kr",
             city_name, api_key);
    
    ESP_LOGI(TAG, "OpenWeatherMap 5일 예보 요청: %s", city_name);
    
    // TODO: 5일 예보 파싱 로직 구현
    // 현재는 기본 구조만 제공
    
    return ESP_OK;
}

esp_err_t openweathermap_parse_geocoding_response(const char *json_str, geocoding_data_t *geo_data)
{
    if (json_str == NULL || geo_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "지오코딩 JSON 파싱 시작");
    ESP_LOGD(TAG, "지오코딩 JSON: %s", json_str);
    
    // JSON 파싱 시작
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        ESP_LOGE(TAG, "지오코딩 JSON 파싱 실패");
        return ESP_FAIL;
    }
    
    // 배열인지 확인 (지오코딩 API는 배열을 반환)
    if (!cJSON_IsArray(json)) {
        ESP_LOGE(TAG, "지오코딩 응답이 배열이 아닙니다");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 배열 크기 확인
    int array_size = cJSON_GetArraySize(json);
    if (array_size == 0) {
        ESP_LOGE(TAG, "지오코딩 결과가 없습니다");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 첫 번째 결과 선택
    cJSON *first_item = cJSON_GetArrayItem(json, 0);
    if (!cJSON_IsObject(first_item)) {
        ESP_LOGE(TAG, "지오코딩 첫 번째 항목이 객체가 아닙니다");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 기본값으로 초기화
    memset(geo_data, 0, sizeof(geocoding_data_t));
    
    // 도시명 파싱
    cJSON *name = cJSON_GetObjectItem(first_item, "name");
    if (cJSON_IsString(name)) {
        strncpy(geo_data->city_name, name->valuestring, sizeof(geo_data->city_name) - 1);
    }
    
    // 국가 코드 파싱
    cJSON *country = cJSON_GetObjectItem(first_item, "country");
    if (cJSON_IsString(country)) {
        strncpy(geo_data->country, country->valuestring, sizeof(geo_data->country) - 1);
    }
    
    // 주/도 파싱 (선택사항)
    cJSON *state = cJSON_GetObjectItem(first_item, "state");
    if (cJSON_IsString(state)) {
        strncpy(geo_data->state, state->valuestring, sizeof(geo_data->state) - 1);
    }
    
    // 위도 파싱
    cJSON *lat = cJSON_GetObjectItem(first_item, "lat");
    if (cJSON_IsNumber(lat)) {
        geo_data->latitude = lat->valuedouble;
    } else {
        ESP_LOGE(TAG, "위도 정보가 없습니다");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 경도 파싱
    cJSON *lon = cJSON_GetObjectItem(first_item, "lon");
    if (cJSON_IsNumber(lon)) {
        geo_data->longitude = lon->valuedouble;
    } else {
        ESP_LOGE(TAG, "경도 정보가 없습니다");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 유효성 설정
    geo_data->is_valid = true;
    
    cJSON_Delete(json);
    
    ESP_LOGI(TAG, "지오코딩 파싱 완료: %s, %s (%.6f, %.6f)", 
             geo_data->city_name, geo_data->country, geo_data->latitude, geo_data->longitude);
    
    return ESP_OK;
}

esp_err_t openweathermap_api_parse_response(const char *json_str, openweathermap_data_t *weather_data)
{
    if (json_str == NULL || weather_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "OpenWeatherMap JSON 파싱 시작");
    ESP_LOGD(TAG, "JSON 데이터: %s", json_str);
    
    // JSON 파싱 시작
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        ESP_LOGE(TAG, "JSON 파싱 실패");
        return ESP_FAIL;
    }
    
    // 에러 응답 확인
    cJSON *cod = cJSON_GetObjectItem(json, "cod");
    if (cJSON_IsNumber(cod) && cod->valueint != 200) {
        cJSON *message = cJSON_GetObjectItem(json, "message");
        const char *error_msg = cJSON_IsString(message) ? message->valuestring : "Unknown error";
        ESP_LOGE(TAG, "OpenWeatherMap API 오류 (코드: %d): %s", cod->valueint, error_msg);
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 기본값으로 초기화
    memset(weather_data, 0, sizeof(openweathermap_data_t));
    
    // 현재 시간 설정
    struct timeval tv;
    gettimeofday(&tv, NULL);
    weather_data->last_update = tv.tv_sec;
    
    // 도시명과 국가 코드 파싱
    cJSON *name = cJSON_GetObjectItem(json, "name");
    if (cJSON_IsString(name)) {
        strncpy(weather_data->city_name, name->valuestring, sizeof(weather_data->city_name) - 1);
    }
    
    // 좌표 정보 파싱
    cJSON *coord = cJSON_GetObjectItem(json, "coord");
    if (cJSON_IsObject(coord)) {
        cJSON *lat = cJSON_GetObjectItem(coord, "lat");
        if (cJSON_IsNumber(lat)) {
            weather_data->latitude = lat->valuedouble;
        }
        
        cJSON *lon = cJSON_GetObjectItem(coord, "lon");
        if (cJSON_IsNumber(lon)) {
            weather_data->longitude = lon->valuedouble;
        }
    }
    
    cJSON *sys = cJSON_GetObjectItem(json, "sys");
    if (cJSON_IsObject(sys)) {
        cJSON *country = cJSON_GetObjectItem(sys, "country");
        if (cJSON_IsString(country)) {
            strncpy(weather_data->country, country->valuestring, sizeof(weather_data->country) - 1);
        }
        
        cJSON *sunrise = cJSON_GetObjectItem(sys, "sunrise");
        if (cJSON_IsNumber(sunrise)) {
            weather_data->sunrise = sunrise->valueint;
        }
        
        cJSON *sunset = cJSON_GetObjectItem(sys, "sunset");
        if (cJSON_IsNumber(sunset)) {
            weather_data->sunset = sunset->valueint;
        }
    }
    
    // 온도 정보 파싱
    cJSON *main = cJSON_GetObjectItem(json, "main");
    if (cJSON_IsObject(main)) {
        cJSON *temp = cJSON_GetObjectItem(main, "temp");
        if (cJSON_IsNumber(temp)) {
            weather_data->temperature = temp->valuedouble;
        }
        
        cJSON *feels_like = cJSON_GetObjectItem(main, "feels_like");
        if (cJSON_IsNumber(feels_like)) {
            weather_data->feels_like = feels_like->valuedouble;
        }
        
        cJSON *temp_min = cJSON_GetObjectItem(main, "temp_min");
        if (cJSON_IsNumber(temp_min)) {
            weather_data->temp_min = temp_min->valuedouble;
        }
        
        cJSON *temp_max = cJSON_GetObjectItem(main, "temp_max");
        if (cJSON_IsNumber(temp_max)) {
            weather_data->temp_max = temp_max->valuedouble;
        }
        
        cJSON *humidity = cJSON_GetObjectItem(main, "humidity");
        if (cJSON_IsNumber(humidity)) {
            weather_data->humidity = humidity->valueint;
        }
        
        cJSON *pressure = cJSON_GetObjectItem(main, "pressure");
        if (cJSON_IsNumber(pressure)) {
            weather_data->pressure = pressure->valueint;
        }
    }
    
    // 날씨 상태 파싱
    cJSON *weather = cJSON_GetObjectItem(json, "weather");
    if (cJSON_IsArray(weather) && cJSON_GetArraySize(weather) > 0) {
        cJSON *weather_item = cJSON_GetArrayItem(weather, 0);
        if (cJSON_IsObject(weather_item)) {
            cJSON *id = cJSON_GetObjectItem(weather_item, "id");
            if (cJSON_IsNumber(id)) {
                weather_data->condition_id = id->valueint;
            }
            
            cJSON *main_weather = cJSON_GetObjectItem(weather_item, "main");
            if (cJSON_IsString(main_weather)) {
                strncpy(weather_data->condition_main, main_weather->valuestring, 
                        sizeof(weather_data->condition_main) - 1);
            }
            
            cJSON *description = cJSON_GetObjectItem(weather_item, "description");
            if (cJSON_IsString(description)) {
                strncpy(weather_data->condition_desc, description->valuestring, 
                        sizeof(weather_data->condition_desc) - 1);
            }
            
            cJSON *icon = cJSON_GetObjectItem(weather_item, "icon");
            if (cJSON_IsString(icon)) {
                strncpy(weather_data->icon, icon->valuestring, sizeof(weather_data->icon) - 1);
            }
        }
    }
    
    // 바람 정보 파싱
    cJSON *wind = cJSON_GetObjectItem(json, "wind");
    if (cJSON_IsObject(wind)) {
        cJSON *speed = cJSON_GetObjectItem(wind, "speed");
        if (cJSON_IsNumber(speed)) {
            weather_data->wind_speed = speed->valuedouble;
        }
        
        cJSON *deg = cJSON_GetObjectItem(wind, "deg");
        if (cJSON_IsNumber(deg)) {
            weather_data->wind_direction = deg->valueint;
        }
    }
    
    // 구름 정보 파싱
    cJSON *clouds = cJSON_GetObjectItem(json, "clouds");
    if (cJSON_IsObject(clouds)) {
        cJSON *all = cJSON_GetObjectItem(clouds, "all");
        if (cJSON_IsNumber(all)) {
            weather_data->cloudiness = all->valueint;
        }
    }
    
    // 가시거리 파싱
    cJSON *visibility = cJSON_GetObjectItem(json, "visibility");
    if (cJSON_IsNumber(visibility)) {
        weather_data->visibility = visibility->valueint;
    }
    
    // 타임스탬프 파싱
    cJSON *dt = cJSON_GetObjectItem(json, "dt");
    if (cJSON_IsNumber(dt)) {
        weather_data->timestamp = dt->valueint;
    }
    
    // 데이터 유효성 설정
    weather_data->is_valid = true;
    
    cJSON_Delete(json);
    
    ESP_LOGI(TAG, "OpenWeatherMap JSON 파싱 완료: %s, %.1f°C", 
             weather_data->city_name, weather_data->temperature);
    
    return ESP_OK;
}

const char* openweathermap_get_condition_desc_ko(weather_condition_t condition_id)
{
    switch (condition_id) {
        case WEATHER_CLEAR: return "맑음";
        case WEATHER_FEW_CLOUDS: return "구름 조금";
        case WEATHER_SCATTERED_CLOUDS: return "구름 많음";
        case WEATHER_BROKEN_CLOUDS: return "흐림";
        case WEATHER_OVERCAST_CLOUDS: return "완전 흐림";
        case WEATHER_LIGHT_RAIN: return "가벼운 비";
        case WEATHER_MODERATE_RAIN: return "보통 비";
        case WEATHER_HEAVY_RAIN: return "강한 비";
        case WEATHER_LIGHT_SNOW: return "가벼운 눈";
        case WEATHER_SNOW: return "눈";
        case WEATHER_HEAVY_SNOW: return "폭설";
        case WEATHER_MIST: return "안개";
        case WEATHER_THUNDERSTORM: return "뇌우";
        default:
            if (condition_id >= 200 && condition_id < 300) return "뇌우";
            else if (condition_id >= 300 && condition_id < 400) return "이슬비";
            else if (condition_id >= 500 && condition_id < 600) return "비";
            else if (condition_id >= 600 && condition_id < 700) return "눈";
            else if (condition_id >= 700 && condition_id < 800) return "안개/연무";
            else if (condition_id >= 801 && condition_id <= 804) return "구름";
            else return "알 수 없음";
    }
}

const char* openweathermap_get_wind_direction_ko(int degrees)
{
    if (degrees >= 0 && degrees <= 22) return "북";
    else if (degrees <= 67) return "북동";
    else if (degrees <= 112) return "동";
    else if (degrees <= 157) return "남동";
    else if (degrees <= 202) return "남";
    else if (degrees <= 247) return "남서";
    else if (degrees <= 292) return "서";
    else if (degrees <= 337) return "북서";
    else return "북";
}

const char* openweathermap_get_icon_desc(const char *icon_id)
{
    if (icon_id == NULL) return "알 수 없음";
    
    if (strncmp(icon_id, "01d", 3) == 0) return "맑음(낮)";
    else if (strncmp(icon_id, "01n", 3) == 0) return "맑음(밤)";
    else if (strncmp(icon_id, "02d", 3) == 0) return "구름 조금(낮)";
    else if (strncmp(icon_id, "02n", 3) == 0) return "구름 조금(밤)";
    else if (strncmp(icon_id, "03", 2) == 0) return "구름 많음";
    else if (strncmp(icon_id, "04", 2) == 0) return "흐림";
    else if (strncmp(icon_id, "09", 2) == 0) return "소나기";
    else if (strncmp(icon_id, "10d", 3) == 0) return "비(낮)";
    else if (strncmp(icon_id, "10n", 3) == 0) return "비(밤)";
    else if (strncmp(icon_id, "11", 2) == 0) return "뇌우";
    else if (strncmp(icon_id, "13", 2) == 0) return "눈";
    else if (strncmp(icon_id, "50", 2) == 0) return "안개";
    else return "알 수 없음";
}

float openweathermap_kelvin_to_celsius(float kelvin)
{
    return kelvin - 273.15f;
}

bool openweathermap_validate_data(const openweathermap_data_t *weather_data)
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

void openweathermap_log_data(const openweathermap_data_t *weather_data)
{
    if (weather_data == NULL) {
        ESP_LOGW(TAG, "날씨 데이터가 NULL입니다");
        return;
    }
    
    ESP_LOGI(TAG, "=== OpenWeatherMap 날씨 정보 ===");
    ESP_LOGI(TAG, "도시: %s, %s (%.6f, %.6f)", weather_data->city_name, weather_data->country, 
             weather_data->latitude, weather_data->longitude);
    ESP_LOGI(TAG, "온도: %.1f°C (체감 %.1f°C)", weather_data->temperature, weather_data->feels_like);
    ESP_LOGI(TAG, "최저/최고: %.1f°C / %.1f°C", weather_data->temp_min, weather_data->temp_max);
    ESP_LOGI(TAG, "습도: %d%%, 기압: %dhPa", weather_data->humidity, weather_data->pressure);
    ESP_LOGI(TAG, "날씨: %s (%s)", weather_data->condition_main, weather_data->condition_desc);
    ESP_LOGI(TAG, "바람: %.1fm/s %s", weather_data->wind_speed, 
             openweathermap_get_wind_direction_ko(weather_data->wind_direction));
    ESP_LOGI(TAG, "구름: %d%%, 가시거리: %dm", weather_data->cloudiness, weather_data->visibility);
    ESP_LOGI(TAG, "아이콘: %s (%s)", weather_data->icon, openweathermap_get_icon_desc(weather_data->icon));
    ESP_LOGI(TAG, "유효성: %s", weather_data->is_valid ? "유효" : "무효");
}