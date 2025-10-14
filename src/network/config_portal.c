#include "network/config_portal.h"
#include "storage/nvs_manager.h"
#include "storage/spiffs_manager.h"
#include "network/wifi_manager.h"
#include "ota/ota_manager.h"
#include "common/config.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include <string.h>

static const char* TAG = "CONFIG_PORTAL";

static httpd_handle_t server = NULL;
static config_complete_cb_t config_callback = NULL;

// 임베디드 HTML 내용 (간단한 버전)
static const char index_html[] = 
"<!DOCTYPE html>"
"<html lang=\"ko\">"
"<head>"
"<meta charset=\"UTF-8\">"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
"<title>ESP32 Weather Station Config</title>"
"<style>"
"body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f5f5f5; }"
".container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
"h1 { color: #333; text-align: center; margin-bottom: 30px; }"
".form-group { margin-bottom: 20px; }"
"label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }"
"input[type=\"text\"], input[type=\"password\"] { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; font-size: 16px; }"
"button { background-color: #007bff; color: white; padding: 12px 30px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; width: 100%; margin-top: 10px; }"
"button:hover { background-color: #0056b3; }"
".scan-btn { background-color: #28a745; margin-bottom: 10px; }"
".scan-btn:hover { background-color: #218838; }"
"#wifiList { margin-top: 10px; border: 1px solid #ddd; border-radius: 5px; max-height: 200px; overflow-y: auto; }"
".wifi-item { padding: 10px; border-bottom: 1px solid #eee; cursor: pointer; }"
".wifi-item:hover { background-color: #f8f9fa; }"
".signal-strength { float: right; color: #666; }"
".status { padding: 10px; margin: 10px 0; border-radius: 5px; }"
".status.success { background-color: #d4edda; color: #155724; border: 1px solid #c3e6cb; }"
".status.error { background-color: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }"
".loading { text-align: center; color: #666; padding: 20px; }"
"</style>"
"</head>"
"<body>"
"<div class=\"container\">"
"<h1>ESP32 Weather Station</h1>"
"<div id=\"status\"></div>"
"<form id=\"configForm\">"
"<div class=\"form-group\">"
"<label for=\"ssid\">WiFi Network:</label>"
"<button type=\"button\" class=\"scan-btn\" onclick=\"scanWiFi()\">Scan WiFi</button>"
"<input type=\"text\" id=\"ssid\" name=\"ssid\" placeholder=\"Enter WiFi SSID\" required>"
"<div id=\"wifiList\" style=\"display: none;\"></div>"
"</div>"
"<div class=\"form-group\">"
"<label for=\"password\">WiFi Password:</label>"
"<input type=\"password\" id=\"password\" name=\"password\" placeholder=\"Enter WiFi Password\">"
"</div>"
"<div class=\"form-group\">"
"<label for=\"apiKey\">Weather API Key:</label>"
"<input type=\"text\" id=\"apiKey\" name=\"apiKey\" placeholder=\"OpenWeatherMap API Key\" required>"
"<small>Get free API key from: <a href=\"https://openweathermap.org/api\" target=\"_blank\">OpenWeatherMap</a></small>"
"</div>"
"<div class=\"form-group\">"
"<label for=\"cityName\">City Name:</label>"
"<input type=\"text\" id=\"cityName\" name=\"cityName\" placeholder=\"e.g. Seoul, Tokyo, New York\" value=\"Seoul\" required>"
"</div>"
"<button type=\"submit\">Save Configuration</button>"
"</form>"
"</div>"
"<script>"
"let wifiNetworks = [];"
"function showStatus(message, type) {"
"const statusDiv = document.getElementById('status');"
"statusDiv.innerHTML = '<div class=\"status ' + type + '\">' + message + '</div>';"
"setTimeout(() => { statusDiv.innerHTML = ''; }, 5000);"
"}"
"function scanWiFi() {"
"const wifiList = document.getElementById('wifiList');"
"wifiList.style.display = 'block';"
"wifiList.innerHTML = '<div class=\"loading\">Scanning...</div>';"
"fetch('/api/scan')"
".then(response => response.json())"
".then(data => {"
"if (data.success) {"
"wifiNetworks = data.networks;"
"displayWiFiNetworks(data.networks);"
"} else {"
"wifiList.innerHTML = '<div class=\"loading\">Scan failed</div>';"
"}"
"})"
".catch(error => {"
"console.error('Error:', error);"
"wifiList.innerHTML = '<div class=\"loading\">Scan error</div>';"
"});"
"}"
"function displayWiFiNetworks(networks) {"
"const wifiList = document.getElementById('wifiList');"
"if (networks.length === 0) {"
"wifiList.innerHTML = '<div class=\"loading\">No networks found</div>';"
"return;"
"}"
"let html = '';"
"networks.forEach(network => {"
"const signalStrength = network.rssi > -50 ? 'Very Strong' : network.rssi > -60 ? 'Strong' : network.rssi > -70 ? 'Good' : 'Weak';"
"const security = network.authmode > 0 ? 'Secured' : 'Open';"
"html += '<div class=\"wifi-item\" onclick=\"selectWiFi(\\'' + network.ssid + '\\')\">' + security + ' ' + network.ssid + '<span class=\"signal-strength\">' + signalStrength + '</span></div>';"
"});"
"wifiList.innerHTML = html;"
"}"
"function selectWiFi(ssid) {"
"document.getElementById('ssid').value = ssid;"
"document.getElementById('wifiList').style.display = 'none';"
"}"
"document.getElementById('configForm').addEventListener('submit', function(e) {"
"e.preventDefault();"
"const formData = new FormData(this);"
"const config = {"
"ssid: formData.get('ssid'),"
"password: formData.get('password'),"
"apiKey: formData.get('apiKey'),"
"cityName: formData.get('cityName')"
"};"
"const submitBtn = this.querySelector('button[type=\"submit\"]');"
"submitBtn.disabled = true;"
"submitBtn.textContent = 'Saving...';"
"fetch('/api/config', {"
"method: 'POST',"
"headers: { 'Content-Type': 'application/json' },"
"body: JSON.stringify(config)"
"})"
".then(response => response.json())"
".then(data => {"
"if (data.success) {"
"showStatus('Configuration saved! Restarting...', 'success');"
"setTimeout(() => { window.location.href = '/'; }, 3000);"
"} else {"
"showStatus('Failed to save: ' + (data.message || 'Unknown error'), 'error');"
"submitBtn.disabled = false;"
"submitBtn.textContent = 'Save Configuration';"
"}"
"})"
".catch(error => {"
"console.error('Error:', error);"
"showStatus('Configuration save error.', 'error');"
"submitBtn.disabled = false;"
"submitBtn.textContent = 'Save Configuration';"
"});"
"});"
"</script>"
"</body>"
"</html>";

// HTTP 핸들러들
static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, strlen(index_html));
    
    return ESP_OK;
}

static esp_err_t scan_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "WiFi scan request received");
    
    // WiFi 스캔 시작
    esp_err_t ret = wifi_manager_scan();
    if (ret != ESP_OK) {
        const char* resp = "{\"success\":false,\"message\":\"Scan failed\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_OK;
    }
    
    // 스캔 결과 대기 (최대 10초)
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // 스캔 결과 가져오기
    wifi_ap_record_t ap_info[20];
    uint16_t ap_count = wifi_manager_get_scan_results(ap_info, 20);
    
    // JSON 응답 생성
    cJSON *json = cJSON_CreateObject();
    cJSON *success = cJSON_CreateTrue();
    cJSON *networks = cJSON_CreateArray();
    
    cJSON_AddItemToObject(json, "success", success);
    cJSON_AddItemToObject(json, "networks", networks);
    
    for (int i = 0; i < ap_count; i++) {
        cJSON *network = cJSON_CreateObject();
        cJSON_AddStringToObject(network, "ssid", (char*)ap_info[i].ssid);
        cJSON_AddNumberToObject(network, "rssi", ap_info[i].rssi);
        cJSON_AddNumberToObject(network, "authmode", ap_info[i].authmode);
        cJSON_AddItemToArray(networks, network);
    }
    
    char *json_string = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

static esp_err_t config_handler(httpd_req_t *req)
{
    char buf[1024];
    int ret, remaining = req->content_len;
    
    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long");
        return ESP_FAIL;
    }
    
    // POST 데이터 읽기
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';
    
    ESP_LOGI(TAG, "Received config: %s", buf);
    
    // JSON 파싱
    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        const char* resp = "{\"success\":false,\"message\":\"Invalid JSON\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_OK;
    }
    
    // 설정 값 추출
    cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON *password = cJSON_GetObjectItem(json, "password");
    cJSON *api_key = cJSON_GetObjectItem(json, "apiKey");
    cJSON *city_name = cJSON_GetObjectItem(json, "cityName");
    
    if (!cJSON_IsString(ssid) || !cJSON_IsString(api_key) || !cJSON_IsString(city_name)) {
        const char* resp = "{\"success\":false,\"message\":\"Missing required fields\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, strlen(resp));
        cJSON_Delete(json);
        return ESP_OK;
    }
    
    // 설정 저장
    app_config_t config = {0};
    strncpy(config.wifi.ssid, ssid->valuestring, sizeof(config.wifi.ssid) - 1);
    if (cJSON_IsString(password)) {
        strncpy(config.wifi.password, password->valuestring, sizeof(config.wifi.password) - 1);
    }
    strncpy(config.api_key, api_key->valuestring, sizeof(config.api_key) - 1);
    strncpy(config.city_name, city_name->valuestring, sizeof(config.city_name) - 1);
    config.wifi.configured = true;
    config.first_boot = false;
    config.update_interval = UPDATE_INTERVAL_MS;
    
    esp_err_t save_ret = nvs_manager_save_config(&config);
    
    // 응답 전송
    const char* resp;
    if (save_ret == ESP_OK) {
        resp = "{\"success\":true,\"message\":\"Configuration saved\"}";
        ESP_LOGI(TAG, "Configuration saved successfully");
        
        // 콜백 호출
        if (config_callback) {
            config_callback(&config);
        }
    } else {
        resp = "{\"success\":false,\"message\":\"Failed to save configuration\"}";
        ESP_LOGE(TAG, "Failed to save configuration");
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, strlen(resp));
    
    cJSON_Delete(json);
    
    return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req)
{
    // 현재 상태 정보 반환
    app_config_t config;
    nvs_manager_load_config(&config);
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "cityName", config.city_name);
    cJSON_AddBoolToObject(json, "configured", config.wifi.configured);
    cJSON_AddBoolToObject(json, "firstBoot", config.first_boot);
    
    char *json_string = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, strlen(json_string));
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

esp_err_t config_portal_start(void)
{
    if (server != NULL) {
        ESP_LOGW(TAG, "Server already running");
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = CONFIG_SERVER_PORT;
    config.max_uri_handlers = 8;
    config.max_resp_headers = 8;
    
    ESP_LOGI(TAG, "Starting HTTP server on port: %d", config.server_port);
    
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Error starting server!");
        return ESP_FAIL;
    }
    
    // URI 핸들러 등록
    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &index_uri);
    
    httpd_uri_t scan_uri = {
        .uri       = "/api/scan",
        .method    = HTTP_GET,
        .handler   = scan_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &scan_uri);
    
    httpd_uri_t config_uri = {
        .uri       = "/api/config",
        .method    = HTTP_POST,
        .handler   = config_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &config_uri);
    
    httpd_uri_t status_uri = {
        .uri       = "/api/status",
        .method    = HTTP_GET,
        .handler   = status_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &status_uri);
    
    ESP_LOGI(TAG, "Config portal started successfully");
    ESP_LOGI(TAG, "Open http://192.168.4.1 in your browser to configure");
    
    return ESP_OK;
}

esp_err_t config_portal_stop(void)
{
    if (server == NULL) {
        return ESP_OK;
    }
    
    esp_err_t ret = httpd_stop(server);
    server = NULL;
    
    ESP_LOGI(TAG, "Config portal stopped");
    return ret;
}

bool config_portal_is_running(void)
{
    return (server != NULL);
}

void config_portal_set_callback(config_complete_cb_t callback)
{
    config_callback = callback;
}