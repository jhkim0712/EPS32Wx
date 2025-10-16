#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <lwip/err.h>
#include <lwip/sys.h>

#include "wifi_manager.h"
#include "config_portal.h"
#include "nvs_manager.h"
#include "../common/constants.h"

static const char *TAG = LOG_TAG_WIFI;

static EventGroupHandle_t wifi_event_group;
static esp_netif_t* sta_netif = NULL;
static esp_netif_t* ap_netif = NULL;
static wifi_state_t current_state = WIFI_STATE_DISCONNECTED;
static esp_event_handler_t user_event_handler = NULL;
static void* user_event_arg = NULL;

// Event bits
const int WIFI_CONNECTED_BIT = BIT0;
const int WIFI_FAIL_BIT = BIT1;
const int WIFI_AP_START_BIT = BIT2;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, 
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi STA started");
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        current_state = WIFI_STATE_DISCONNECTED;
        xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        ESP_LOGI(TAG, "WiFi disconnected");
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        current_state = WIFI_STATE_CONNECTED;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Station %02x:%02x:%02x:%02x:%02x:%02x join, AID=%d", 
                 event->mac[0], event->mac[1], event->mac[2], 
                 event->mac[3], event->mac[4], event->mac[5], event->aid);
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "Station %02x:%02x:%02x:%02x:%02x:%02x leave, AID=%d", 
                 event->mac[0], event->mac[1], event->mac[2], 
                 event->mac[3], event->mac[4], event->mac[5], event->aid);
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        current_state = WIFI_STATE_AP_MODE;
        xEventGroupSetBits(wifi_event_group, WIFI_AP_START_BIT);
        ESP_LOGI(TAG, "WiFi AP started");
    }
    
    // 사용자 이벤트 핸들러 호출
    if (user_event_handler) {
        user_event_handler(user_event_arg, event_base, event_id, event_data);
    }
}

esp_err_t wifi_manager_init(void)
{
    wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    
    ESP_LOGI(TAG, "WiFi manager initialized");
    return ESP_OK;
}

esp_err_t wifi_manager_connect_sta(const char* ssid, const char* password)
{
    if (!ssid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // STA 네트워크 인터페이스 생성
    if (!sta_netif) {
        sta_netif = esp_netif_create_default_wifi_sta();
    }
    
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    current_state = WIFI_STATE_CONNECTING;
    
    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", ssid);
    
    // 연결 대기
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE, pdFALSE, 
                                          pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT));
    
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to WiFi successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to connect to WiFi");
        current_state = WIFI_STATE_ERROR;
        return ESP_FAIL;
    }
}

esp_err_t wifi_manager_start_ap(void)
{
    // AP 네트워크 인터페이스 생성
    if (!ap_netif) {
        ap_netif = esp_netif_create_default_wifi_ap();
    }
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = DEFAULT_AP_SSID,
            .ssid_len = strlen(DEFAULT_AP_SSID),
            .channel = AP_CHANNEL,
            .password = DEFAULT_AP_PASSWORD,
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    
    if (strlen(DEFAULT_AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi AP started. SSID: %s, Password: %s", DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
    
    // AP 시작 대기
    xEventGroupWaitBits(wifi_event_group, WIFI_AP_START_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    
    return ESP_OK;
}

esp_err_t wifi_manager_disconnect(void)
{
    esp_err_t ret = esp_wifi_stop();
    if (ret == ESP_OK) {
        current_state = WIFI_STATE_DISCONNECTED;
        ESP_LOGI(TAG, "WiFi disconnected");
    }
    return ret;
}

wifi_state_t wifi_manager_get_state(void)
{
    return current_state;
}

bool wifi_manager_is_connected(void)
{
    return (current_state == WIFI_STATE_CONNECTED);
}

bool wifi_manager_is_ap_mode(void)
{
    return (current_state == WIFI_STATE_AP_MODE);
}

esp_err_t wifi_manager_get_info(char* ssid, char* ip_addr)
{
    if (!ssid || !ip_addr) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (current_state == WIFI_STATE_CONNECTED) {
        wifi_config_t wifi_config;
        esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
        strcpy(ssid, (char*)wifi_config.sta.ssid);
        
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(sta_netif, &ip_info);
        sprintf(ip_addr, IPSTR, IP2STR(&ip_info.ip));
        
        return ESP_OK;
    } else if (current_state == WIFI_STATE_AP_MODE) {
        strcpy(ssid, DEFAULT_AP_SSID);
        
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(ap_netif, &ip_info);
        sprintf(ip_addr, IPSTR, IP2STR(&ip_info.ip));
        
        return ESP_OK;
    }
    
    return ESP_FAIL;
}

esp_err_t wifi_manager_scan(void)
{
    return esp_wifi_scan_start(NULL, true);
}

uint16_t wifi_manager_get_scan_results(wifi_ap_record_t* ap_info, uint16_t max_ap)
{
    if (!ap_info) {
        return 0;
    }
    
    uint16_t ap_count = max_ap;
    esp_err_t ret = esp_wifi_scan_get_ap_records(&ap_count, ap_info);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Found %d access points", ap_count);
        return ap_count;
    } else {
        ESP_LOGE(TAG, "Failed to get scan results");
        return 0;
    }
}

esp_err_t wifi_manager_set_event_handler(esp_event_handler_t handler, void* arg)
{
    user_event_handler = handler;
    user_event_arg = arg;
    return ESP_OK;
}

esp_err_t wifi_manager_setup_network(void)
{
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "Starting network setup");
    
    wifi_config_t wifi_config = {0};
    if (nvs_get_wifi_config(&wifi_config) == ESP_OK) {
        ESP_LOGI(TAG, "Attempting to connect to saved WiFi: %s", wifi_config.sta.ssid);
        
        ret = wifi_manager_connect_sta((char*)wifi_config.sta.ssid, (char*)wifi_config.sta.password);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "WiFi connection successful");
            return ESP_OK;
        } else {
            ESP_LOGW(TAG, "WiFi connection failed, switching to AP mode");
        }
    } else {
        ESP_LOGI(TAG, "No saved WiFi configuration found. Starting AP mode");
    }
    
    ESP_LOGI(TAG, "Starting AP mode");
    ret = wifi_manager_start_ap();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AP mode startup failed");
        return ret;
    }
    
    ret = config_portal_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Configuration portal startup failed");
        return ret;
    }
    
    ESP_LOGI(TAG, "Configuration portal started");
    ESP_LOGI(TAG, "Connect to WiFi '%s' and open http://192.168.4.1", DEFAULT_AP_SSID);
    
    return ESP_OK;
}