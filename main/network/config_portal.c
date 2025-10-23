#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"

#include "common/constants.h"
#include "common/types.h"
#include "storage/nvs_manager.h"
#include "storage/spiffs_manager.h"
#include "network/config_portal.h"
#include "network/wifi_manager.h"
#include "cJSON.h"

static const char *TAG = LOG_TAG_CONFIG_PORTAL;

static httpd_handle_t s_server = NULL;
static config_complete_cb_t s_complete_cb = NULL;

static esp_err_t serve_file(httpd_req_t *req, const char *path, const char *content_type)
{
    // Ensure SPIFFS mounted
    if (spiffs_manager_init() != ESP_OK) return ESP_FAIL;

    long fsize = spiffs_get_file_size(path);
    if (fsize <= 0) {
        ESP_LOGW(TAG, "File not found: %s", path);
        return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
    }

    char *buf = (char*)malloc((size_t)fsize);
    if (!buf) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");

    int r = spiffs_read_file(path, buf, (size_t)fsize);
    if (r <= 0) {
        free(buf);
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Read error");
    }

    httpd_resp_set_type(req, content_type);
    esp_err_t ret = httpd_resp_send(req, buf, r);
    free(buf);
    return ret;
}

static esp_err_t root_get_handler(httpd_req_t *req)
{
    return serve_file(req, "/index.html", "text/html");
}

static void set_json_type(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
}

static esp_err_t api_scan_get_handler(httpd_req_t *req)
{
    // Perform WiFi scan and return JSON list
    esp_err_t err = wifi_manager_scan();
    if (err != ESP_OK) {
        set_json_type(req);
        return httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"scan failed\"}");
    }

    wifi_ap_record_t ap_info[20];
    uint16_t count = wifi_manager_get_scan_results(ap_info, 20);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", true);
    cJSON *arr = cJSON_AddArrayToObject(root, "networks");
    for (uint16_t i = 0; i < count; ++i) {
        cJSON *n = cJSON_CreateObject();
        cJSON_AddStringToObject(n, "ssid", (const char*)ap_info[i].ssid);
        cJSON_AddNumberToObject(n, "rssi", ap_info[i].rssi);
        cJSON_AddNumberToObject(n, "authmode", ap_info[i].authmode);
        cJSON_AddItemToArray(arr, n);
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json) {
        set_json_type(req);
        return httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"oom\"}");
    }

    set_json_type(req);
    esp_err_t ret = httpd_resp_sendstr(req, json);
    cJSON_free(json);
    return ret;
}

static esp_err_t api_status_get_handler(httpd_req_t *req)
{
    app_config_t cfg;
    (void)nvs_manager_load_config(&cfg);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddStringToObject(root, "cityName", cfg.city_name);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json) {
        set_json_type(req);
        return httpd_resp_sendstr(req, "{\"success\":false}");
    }

    set_json_type(req);
    esp_err_t ret = httpd_resp_sendstr(req, json);
    cJSON_free(json);
    return ret;
}

static esp_err_t api_config_options_handler(httpd_req_t *req)
{
    // CORS preflight for JSON POST
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t api_config_post_handler(httpd_req_t *req)
{
    // Expect JSON body: { ssid, password, apiKey, cityName }
    int total = req->content_len;
    if (total <= 0 || total > 1024) {
        set_json_type(req);
        return httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"invalid body\"}");
    }

    char *buf = (char*)malloc((size_t)total + 1);
    if (!buf) {
        set_json_type(req);
        return httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"oom\"}");
    }

    int recvd = 0;
    while (recvd < total) {
        int r = httpd_req_recv(req, buf + recvd, total - recvd);
        if (r <= 0) {
            if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
            free(buf);
            set_json_type(req);
            return httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"recv error\"}");
        }
        recvd += r;
    }
    buf[recvd] = '\0';

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        set_json_type(req);
        return httpd_resp_sendstr(req, "{\"success\":false,\"message\":\"invalid json\"}");
    }

    const cJSON *j_ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    const cJSON *j_pass = cJSON_GetObjectItemCaseSensitive(root, "password");
    const cJSON *j_api  = cJSON_GetObjectItemCaseSensitive(root, "apiKey");
    const cJSON *j_city = cJSON_GetObjectItemCaseSensitive(root, "cityName");

    app_config_t cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.first_boot = false;
    cfg.update_interval = DEFAULT_UPDATE_INTERVAL_SEC;

    if (cJSON_IsString(j_ssid) && j_ssid->valuestring) {
        strncpy(cfg.wifi.ssid, j_ssid->valuestring, sizeof(cfg.wifi.ssid) - 1);
    }
    if (cJSON_IsString(j_pass) && j_pass->valuestring) {
        strncpy(cfg.wifi.password, j_pass->valuestring, sizeof(cfg.wifi.password) - 1);
    }
    if (cJSON_IsString(j_api) && j_api->valuestring) {
        strncpy(cfg.api_key, j_api->valuestring, sizeof(cfg.api_key) - 1);
    }
    if (cJSON_IsString(j_city) && j_city->valuestring) {
        strncpy(cfg.city_name, j_city->valuestring, sizeof(cfg.city_name) - 1);
    }

    cfg.wifi.configured = (cfg.wifi.ssid[0] != '\0');

    // Persist
    (void)nvs_manager_save_wifi_config(cfg.wifi.ssid, cfg.wifi.password);
    (void)nvs_manager_save_api_key(cfg.api_key);
    (void)nvs_manager_save_city_name(cfg.city_name);
    (void)nvs_manager_set_first_boot(false);

    if (s_complete_cb) s_complete_cb(&cfg);

    cJSON_Delete(root);

    set_json_type(req);
    return httpd_resp_sendstr(req, "{\"success\":true,\"message\":\"Configuration saved\"}");
}

static esp_err_t post_config_handler(httpd_req_t *req)
{
    // Expect urlencoded form: ssid=...&password=...&api_key=...&city_name=...
    char content[512];
    int total = req->content_len;
    int recv_len = 0;
    while (recv_len < total) {
        int cur = httpd_req_recv(req, content + recv_len, MIN(total - recv_len, (int)sizeof(content) - 1 - recv_len));
        if (cur <= 0) {
            if (cur == HTTPD_SOCK_ERR_TIMEOUT) continue;
            return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "recv error");
        }
        recv_len += cur;
    }
    content[recv_len] = '\0';

    // Very simple parsing (not robust URL decoding)
    app_config_t cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.first_boot = false;
    cfg.update_interval = DEFAULT_UPDATE_INTERVAL_SEC;

    // Helper to extract key=value pairs
    const char *find_kv = content;
    while (find_kv && *find_kv) {
        const char *eq = strchr(find_kv, '=');
        if (!eq) break;
        const char *amp = strchr(eq + 1, '&');
        size_t klen = (size_t)(eq - find_kv);
        size_t vlen = amp ? (size_t)(amp - (eq + 1)) : strlen(eq + 1);
        char key[32]; char val[256];
        if (klen < sizeof(key)) { memcpy(key, find_kv, klen); key[klen] = '\0'; }
        else { key[0] = '\0'; }
        size_t copy = vlen < sizeof(val)-1 ? vlen : sizeof(val)-1;
        memcpy(val, eq + 1, copy); val[copy] = '\0';

        if (strcmp(key, "ssid") == 0) {
            strncpy(cfg.wifi.ssid, val, sizeof(cfg.wifi.ssid)-1);
        } else if (strcmp(key, "password") == 0) {
            strncpy(cfg.wifi.password, val, sizeof(cfg.wifi.password)-1);
        } else if (strcmp(key, "api_key") == 0) {
            strncpy(cfg.api_key, val, sizeof(cfg.api_key)-1);
        } else if (strcmp(key, "city_name") == 0) {
            strncpy(cfg.city_name, val, sizeof(cfg.city_name)-1);
        }

        if (!amp) break;
        find_kv = amp + 1;
    }
    cfg.wifi.configured = (cfg.wifi.ssid[0] != '\0');

    // Save
    (void)nvs_manager_save_wifi_config(cfg.wifi.ssid, cfg.wifi.password);
    (void)nvs_manager_save_api_key(cfg.api_key);
    (void)nvs_manager_save_city_name(cfg.city_name);
    (void)nvs_manager_set_first_boot(false);

    if (s_complete_cb) s_complete_cb(&cfg);

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}

static httpd_handle_t start_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = CONFIG_SERVER_PORT;
    config.lru_purge_enable = true;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t post_cfg = { .uri = "/config", .method = HTTP_POST, .handler = post_config_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &post_cfg);

        // API endpoints used by data/index.html
        httpd_uri_t api_scan = { .uri = "/api/scan", .method = HTTP_GET, .handler = api_scan_get_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &api_scan);

        httpd_uri_t api_status = { .uri = "/api/status", .method = HTTP_GET, .handler = api_status_get_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &api_status);

        httpd_uri_t api_cfg_opt = { .uri = "/api/config", .method = HTTP_OPTIONS, .handler = api_config_options_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &api_cfg_opt);

        httpd_uri_t api_cfg = { .uri = "/api/config", .method = HTTP_POST, .handler = api_config_post_handler, .user_ctx = NULL };
        httpd_register_uri_handler(server, &api_cfg);

        ESP_LOGI(TAG, "Config portal started on port %d", config.server_port);
        return server;
    }
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return NULL;
}

esp_err_t config_portal_start(void)
{
    if (s_server) {
        ESP_LOGI(TAG, "Config portal already running");
        return ESP_OK;
    }
    s_server = start_server();
    return s_server ? ESP_OK : ESP_FAIL;
}

esp_err_t config_portal_stop(void)
{
    if (!s_server) return ESP_OK;
    httpd_stop(s_server);
    s_server = NULL;
    ESP_LOGI(TAG, "Config portal stopped");
    return ESP_OK;
}

bool config_portal_is_running(void)
{
    return s_server != NULL;
}

void config_portal_set_callback(config_complete_cb_t callback)
{
    s_complete_cb = callback;
}
