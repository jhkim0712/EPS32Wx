#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_server.h"

#include "common/config.h"
#include "common/constants.h"
#include "common/types.h"
#include "storage/nvs_manager.h"
#include "storage/spiffs_manager.h"
#include "storage/sd_card_manager.h"
#include "network/config_portal.h"
#include "network/wifi_manager.h"
#include "display/backlight.h"
#include "ui/lvgl_driver.h"
#include "ota/ota_manager.h"
#include "cJSON.h"

static const char *TAG = LOG_TAG_CONFIG_PORTAL;

static httpd_handle_t s_server = NULL;
static config_complete_cb_t s_complete_cb = NULL;

// =============================================================================
// 정적 파일 서빙 (data/ -> SPIFFS) — 확장자로 Content-Type을 추론하는 와일드카드 핸들러.
// 새 정적 파일(css/js/이미지)을 추가할 때마다 핸들러를 등록하지 않아도 되게 한다.
// =============================================================================

static esp_err_t serve_file(httpd_req_t *req, const char *path, const char *content_type)
{
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

static const char *guess_content_type(const char *uri)
{
    const char *dot = strrchr(uri, '.');
    if (!dot) return "text/plain";
    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".css") == 0) return "text/css";
    if (strcmp(dot, ".js") == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".ico") == 0) return "image/x-icon";
    if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
    return "text/plain";
}

// GET "/*" — 루트는 index.html, 그 외는 요청 경로 그대로 SPIFFS에서 찾는다.
static esp_err_t static_file_get_handler(httpd_req_t *req)
{
    char path[64];
    if (strcmp(req->uri, "/") == 0) {
        snprintf(path, sizeof(path), "/index.html");
    } else {
        // 쿼리스트링 제거
        const char *q = strchr(req->uri, '?');
        size_t len = q ? (size_t)(q - req->uri) : strlen(req->uri);
        if (len >= sizeof(path)) len = sizeof(path) - 1;
        memcpy(path, req->uri, len);
        path[len] = '\0';
    }
    return serve_file(req, path, guess_content_type(path));
}

static void set_json_type(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
}

// 요청 바디를 malloc된 버퍼에 통째로 읽어온다 (max_len 초과 시 NULL).
// 성공 시 호출자가 free() 해야 한다.
static char *read_body(httpd_req_t *req, int max_len)
{
    int total = req->content_len;
    if (total <= 0 || total > max_len) return NULL;

    char *buf = (char *)malloc((size_t)total + 1);
    if (!buf) return NULL;

    int recvd = 0;
    while (recvd < total) {
        int r = httpd_req_recv(req, buf + recvd, total - recvd);
        if (r <= 0) {
            if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
            free(buf);
            return NULL;
        }
        recvd += r;
    }
    buf[recvd] = '\0';
    return buf;
}

static esp_err_t send_simple(httpd_req_t *req, bool success, const char *message)
{
    set_json_type(req);
    char buf[192];
    snprintf(buf, sizeof(buf), "{\"success\":%s,\"message\":\"%s\"}", success ? "true" : "false", message ? message : "");
    return httpd_resp_sendstr(req, buf);
}

// =============================================================================
// WiFi 스캔 / 설정 (기존 동작 유지, 하위 호환)
// =============================================================================

static esp_err_t api_scan_get_handler(httpd_req_t *req)
{
    esp_err_t err = wifi_manager_scan();
    if (err != ESP_OK) {
        return send_simple(req, false, "scan failed");
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
    if (!json) return send_simple(req, false, "oom");

    set_json_type(req);
    esp_err_t ret = httpd_resp_sendstr(req, json);
    cJSON_free(json);
    return ret;
}

// GET /api/status — WiFi/밝기/SD/버전 등 대시보드 요약 정보
static esp_err_t api_status_get_handler(httpd_req_t *req)
{
    app_config_t cfg;
    (void)nvs_manager_load_config(&cfg);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddStringToObject(root, "cityName", cfg.city_name); // 하위 호환 유지

    bool connected = wifi_manager_is_connected();
    bool ap_mode = wifi_manager_is_ap_mode();
    cJSON_AddBoolToObject(root, "wifiConnected", connected);
    cJSON_AddBoolToObject(root, "apMode", ap_mode);
    if (connected) {
        char ssid[33] = {0}, ip[32] = {0};
        wifi_manager_get_info(ssid, ip);
        cJSON_AddStringToObject(root, "ssid", ssid);
        cJSON_AddStringToObject(root, "ip", ip);
    }

    cJSON_AddNumberToObject(root, "brightness", cfg.brightness_percent);
    cJSON_AddBoolToObject(root, "sdMounted", sd_card_is_mounted());
    cJSON_AddStringToObject(root, "firmwareVersion", FIRMWARE_VERSION);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return send_simple(req, false, "oom");

    set_json_type(req);
    esp_err_t ret = httpd_resp_sendstr(req, json);
    cJSON_free(json);
    return ret;
}

static esp_err_t cors_options_handler(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t api_config_post_handler(httpd_req_t *req)
{
    char *buf = read_body(req, 1024);
    if (!buf) return send_simple(req, false, "invalid body");

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return send_simple(req, false, "invalid json");

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

    (void)nvs_manager_save_wifi_config(cfg.wifi.ssid, cfg.wifi.password);
    (void)nvs_manager_save_api_key(cfg.api_key);
    (void)nvs_manager_save_city_name(cfg.city_name);
    (void)nvs_manager_set_first_boot(false);

    if (s_complete_cb) s_complete_cb(&cfg);

    cJSON_Delete(root);
    return send_simple(req, true, "Configuration saved");
}

static esp_err_t post_config_handler(httpd_req_t *req)
{
    // 레거시 urlencoded form 지원 (ssid=...&password=...&api_key=...&city_name=...)
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

    app_config_t cfg; memset(&cfg, 0, sizeof(cfg));
    cfg.first_boot = false;
    cfg.update_interval = DEFAULT_UPDATE_INTERVAL_SEC;

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

    (void)nvs_manager_save_wifi_config(cfg.wifi.ssid, cfg.wifi.password);
    (void)nvs_manager_save_api_key(cfg.api_key);
    (void)nvs_manager_save_city_name(cfg.city_name);
    (void)nvs_manager_set_first_boot(false);

    if (s_complete_cb) s_complete_cb(&cfg);

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}

// =============================================================================
// System 탭: 밝기/회전/타임존/야간감광/OTA 매니페스트 URL — 조회 + 저장(즉시 반영)
// =============================================================================

static esp_err_t api_system_get_handler(httpd_req_t *req)
{
    app_config_t cfg;
    (void)nvs_manager_load_config(&cfg);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddNumberToObject(root, "brightness", cfg.brightness_percent);
    cJSON_AddNumberToObject(root, "rotation", cfg.display_rotation_deg);
    cJSON_AddStringToObject(root, "timezone", cfg.timezone_posix);
    cJSON_AddBoolToObject(root, "nightDimEnabled", cfg.night_dim_enabled);
    cJSON_AddNumberToObject(root, "nightDimStart", cfg.night_dim_start_hour);
    cJSON_AddNumberToObject(root, "nightDimEnd", cfg.night_dim_end_hour);
    cJSON_AddNumberToObject(root, "nightDimBrightness", cfg.night_dim_brightness_percent);
    cJSON_AddBoolToObject(root, "webAuthEnabled", cfg.web_auth_enabled);
    cJSON_AddStringToObject(root, "otaManifestUrl", cfg.ota_manifest_url);
    cJSON_AddBoolToObject(root, "otaAutoCheck", cfg.ota_auto_check);
    cJSON_AddStringToObject(root, "firmwareVersion", FIRMWARE_VERSION);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return send_simple(req, false, "oom");

    set_json_type(req);
    esp_err_t ret = httpd_resp_sendstr(req, json);
    cJSON_free(json);
    return ret;
}

static esp_err_t api_system_post_handler(httpd_req_t *req)
{
    char *buf = read_body(req, 1024);
    if (!buf) return send_simple(req, false, "invalid body");

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return send_simple(req, false, "invalid json");

    // 전달된 필드만 적용 (부분 업데이트)
    const cJSON *j;

    if ((j = cJSON_GetObjectItemCaseSensitive(root, "brightness")) && cJSON_IsNumber(j)) {
        uint8_t pct = (uint8_t)(j->valuedouble < 0 ? 0 : (j->valuedouble > 100 ? 100 : j->valuedouble));
        backlight_set_percent(pct);
        nvs_manager_save_brightness(pct);
    }
    if ((j = cJSON_GetObjectItemCaseSensitive(root, "rotation")) && cJSON_IsNumber(j)) {
        int deg = (int)j->valuedouble;
        if (display_set_rotation(deg) == ESP_OK) {
            nvs_manager_save_rotation((uint16_t)deg);
        }
    }
    if ((j = cJSON_GetObjectItemCaseSensitive(root, "timezone")) && cJSON_IsString(j)) {
        nvs_manager_save_timezone(j->valuestring);
    }
    if ((j = cJSON_GetObjectItemCaseSensitive(root, "otaManifestUrl")) && cJSON_IsString(j)) {
        nvs_manager_save_ota_manifest_url(j->valuestring);
    }
    if ((j = cJSON_GetObjectItemCaseSensitive(root, "otaAutoCheck")) && cJSON_IsBool(j)) {
        nvs_manager_save_ota_auto_check(cJSON_IsTrue(j));
    }
    if ((j = cJSON_GetObjectItemCaseSensitive(root, "webAuthEnabled")) && cJSON_IsBool(j)) {
        const cJSON *ju = cJSON_GetObjectItemCaseSensitive(root, "webAuthUser");
        const cJSON *jp = cJSON_GetObjectItemCaseSensitive(root, "webAuthPass");
        nvs_manager_save_web_auth(cJSON_IsTrue(j),
                                   (ju && cJSON_IsString(ju)) ? ju->valuestring : NULL,
                                   (jp && cJSON_IsString(jp)) ? jp->valuestring : NULL);
    }

    // 야간 감광 4개 필드는 하나의 NVS 호출로 묶여 있으므로, 하나라도 오면 현재값 기준으로 병합 저장
    const cJSON *j_nd_en = cJSON_GetObjectItemCaseSensitive(root, "nightDimEnabled");
    const cJSON *j_nd_start = cJSON_GetObjectItemCaseSensitive(root, "nightDimStart");
    const cJSON *j_nd_end = cJSON_GetObjectItemCaseSensitive(root, "nightDimEnd");
    const cJSON *j_nd_bright = cJSON_GetObjectItemCaseSensitive(root, "nightDimBrightness");
    if (j_nd_en || j_nd_start || j_nd_end || j_nd_bright) {
        app_config_t cur;
        nvs_manager_load_config(&cur);
        bool en = j_nd_en ? cJSON_IsTrue(j_nd_en) : cur.night_dim_enabled;
        uint8_t start_h = (j_nd_start && cJSON_IsNumber(j_nd_start)) ? (uint8_t)j_nd_start->valuedouble : cur.night_dim_start_hour;
        uint8_t end_h = (j_nd_end && cJSON_IsNumber(j_nd_end)) ? (uint8_t)j_nd_end->valuedouble : cur.night_dim_end_hour;
        uint8_t bright = (j_nd_bright && cJSON_IsNumber(j_nd_bright)) ? (uint8_t)j_nd_bright->valuedouble : cur.night_dim_brightness_percent;
        nvs_manager_save_night_dim(en, start_h, end_h, bright);
    }

    cJSON_Delete(root);
    return send_simple(req, true, "System settings saved");
}

// =============================================================================
// Pictures 탭: SD 카드 사진 목록 + 슬라이드쇼 설정
// =============================================================================

static esp_err_t api_gallery_list_get_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddBoolToObject(root, "sdMounted", sd_card_is_mounted());
    cJSON *arr = cJSON_AddArrayToObject(root, "files");

    if (sd_card_is_mounted()) {
        sd_file_entry_t entries[32];
        int count = sd_card_list_dir(SD_PHOTOS_SUBDIR, entries, 32);
        for (int i = 0; i < count; ++i) {
            cJSON *f = cJSON_CreateObject();
            cJSON_AddStringToObject(f, "name", entries[i].name);
            cJSON_AddNumberToObject(f, "size", (double)entries[i].size);
            cJSON_AddItemToArray(arr, f);
        }
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return send_simple(req, false, "oom");

    set_json_type(req);
    esp_err_t ret = httpd_resp_sendstr(req, json);
    cJSON_free(json);
    return ret;
}

static esp_err_t api_gallery_config_post_handler(httpd_req_t *req)
{
    char *buf = read_body(req, 256);
    if (!buf) return send_simple(req, false, "invalid body");

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return send_simple(req, false, "invalid json");

    app_config_t cur;
    nvs_manager_load_config(&cur);

    const cJSON *j_en = cJSON_GetObjectItemCaseSensitive(root, "enabled");
    const cJSON *j_interval = cJSON_GetObjectItemCaseSensitive(root, "intervalSec");

    bool en = (j_en && cJSON_IsBool(j_en)) ? cJSON_IsTrue(j_en) : cur.slideshow_enabled;
    uint16_t interval = (j_interval && cJSON_IsNumber(j_interval)) ? (uint16_t)j_interval->valuedouble : cur.slideshow_interval_sec;
    if (interval == 0) interval = 1;

    nvs_manager_save_slideshow(en, interval);

    cJSON_Delete(root);
    return send_simple(req, true, "Gallery settings saved");
}

// =============================================================================
// OTA: 확인 / 시작 / 진행 상태
// =============================================================================

static esp_err_t api_ota_check_get_handler(httpd_req_t *req)
{
    esp_err_t err = ota_manager_check_update(NULL);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", true);

    ota_info_t info;
    ota_manager_get_info(&info);
    cJSON_AddStringToObject(root, "currentVersion", info.current_version);

    if (err == ESP_OK) {
        cJSON_AddBoolToObject(root, "available", true);
        cJSON_AddStringToObject(root, "availableVersion", info.available_version);
    } else if (err == ESP_ERR_NOT_FOUND) {
        cJSON_AddBoolToObject(root, "available", false);
    } else if (err == ESP_ERR_INVALID_STATE) {
        cJSON_AddBoolToObject(root, "available", false);
        cJSON_AddStringToObject(root, "error", "no_manifest_url");
    } else {
        cJSON_AddBoolToObject(root, "available", false);
        cJSON_AddStringToObject(root, "error", "check_failed");
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return send_simple(req, false, "oom");

    set_json_type(req);
    esp_err_t ret = httpd_resp_sendstr(req, json);
    cJSON_free(json);
    return ret;
}

static esp_err_t api_ota_start_post_handler(httpd_req_t *req)
{
    esp_err_t err = ota_manager_start_update(NULL); // check_update()가 채운 펌웨어 URL 사용
    if (err != ESP_OK) {
        return send_simple(req, false, "OTA start failed (run check first?)");
    }
    return send_simple(req, true, "OTA update started");
}

static esp_err_t api_ota_status_get_handler(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "success", true);
    cJSON_AddNumberToObject(root, "status", (int)ota_manager_get_status());
    cJSON_AddNumberToObject(root, "progress", ota_manager_get_progress());

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return send_simple(req, false, "oom");

    set_json_type(req);
    esp_err_t ret = httpd_resp_sendstr(req, json);
    cJSON_free(json);
    return ret;
}

// =============================================================================
// 서버 시작/중지
// =============================================================================

static httpd_handle_t start_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = CONFIG_SERVER_PORT;
    config.lru_purge_enable = true;
    config.max_uri_handlers = 20; // 기본값(8)보다 핸들러가 많으므로 늘려준다
    config.uri_match_fn = httpd_uri_match_wildcard; // "/*" 와일드카드 라우팅에 필요

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }

    // 순서 중요: httpd는 등록된 순서대로 URI를 매칭하므로, 구체적인 경로("/api/...")를
    // 와일드카드("/*") 정적 파일 핸들러보다 먼저 등록해야 한다.
    static const httpd_uri_t uris[] = {
        { .uri = "/config",              .method = HTTP_POST,    .handler = post_config_handler },
        { .uri = "/api/scan",            .method = HTTP_GET,     .handler = api_scan_get_handler },
        { .uri = "/api/status",          .method = HTTP_GET,     .handler = api_status_get_handler },
        { .uri = "/api/config",          .method = HTTP_OPTIONS, .handler = cors_options_handler },
        { .uri = "/api/config",          .method = HTTP_POST,    .handler = api_config_post_handler },
        { .uri = "/api/system",          .method = HTTP_GET,     .handler = api_system_get_handler },
        { .uri = "/api/system",          .method = HTTP_OPTIONS, .handler = cors_options_handler },
        { .uri = "/api/system",          .method = HTTP_POST,    .handler = api_system_post_handler },
        { .uri = "/api/gallery/list",    .method = HTTP_GET,     .handler = api_gallery_list_get_handler },
        { .uri = "/api/gallery/config",  .method = HTTP_OPTIONS, .handler = cors_options_handler },
        { .uri = "/api/gallery/config",  .method = HTTP_POST,    .handler = api_gallery_config_post_handler },
        { .uri = "/api/ota/check",       .method = HTTP_GET,     .handler = api_ota_check_get_handler },
        { .uri = "/api/ota/start",       .method = HTTP_OPTIONS, .handler = cors_options_handler },
        { .uri = "/api/ota/start",       .method = HTTP_POST,    .handler = api_ota_start_post_handler },
        { .uri = "/api/ota/status",      .method = HTTP_GET,     .handler = api_ota_status_get_handler },
        { .uri = "/*",                   .method = HTTP_GET,     .handler = static_file_get_handler },
    };

    for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); ++i) {
        httpd_register_uri_handler(server, &uris[i]);
    }

    ESP_LOGI(TAG, "Config portal started on port %d", config.server_port);
    return server;
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
