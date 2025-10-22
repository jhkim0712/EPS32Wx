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
