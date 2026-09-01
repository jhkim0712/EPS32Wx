#ifndef CONSTANTS_H
#define CONSTANTS_H

// =============================================================================
// NVS 키 정의
// =============================================================================

// NVS 네임스페이스
#define NVS_NAMESPACE          "weather_cfg"

// WiFi 관련 NVS 키
#define NVS_KEY_WIFI_SSID      "wifi_ssid"
#define NVS_KEY_WIFI_PASS      "wifi_pass"
#define NVS_KEY_WIFI_CONFIGURED "wifi_configured"

// API 및 날씨 관련 NVS 키
#define NVS_KEY_API_KEY        "api_key"
#define NVS_KEY_CITY_NAME      "city_name"
#define NVS_WEATHER_API_KEY    NVS_KEY_API_KEY        // 호환성 유지
#define NVS_WEATHER_CITY       NVS_KEY_CITY_NAME      // 호환성 유지

// 시스템 설정 NVS 키
#define NVS_KEY_FIRST_BOOT     "first_boot"
#define NVS_KEY_UPDATE_INTERVAL "update_interval"

// System 탭 / 온디바이스 설정 관련 NVS 키 (15자 이내, NVS 키 길이 제한)
#define NVS_KEY_BRIGHTNESS       "brightness"
#define NVS_KEY_ROTATION         "rotation"
#define NVS_KEY_TIMEZONE         "tz"
#define NVS_KEY_NIGHT_DIM_EN     "nd_en"
#define NVS_KEY_NIGHT_DIM_START  "nd_start"
#define NVS_KEY_NIGHT_DIM_END    "nd_end"
#define NVS_KEY_NIGHT_DIM_BRIGHT "nd_bright"
#define NVS_KEY_WEB_AUTH_EN      "web_auth_en"
#define NVS_KEY_WEB_AUTH_USER    "web_user"
#define NVS_KEY_WEB_AUTH_PASS    "web_pass"
#define NVS_KEY_SLIDESHOW_EN     "slide_en"
#define NVS_KEY_SLIDESHOW_INT    "slide_int"
#define NVS_KEY_OTA_MANIFEST_URL "ota_manifest"
#define NVS_KEY_OTA_AUTO_CHECK   "ota_auto"

// 기본값
#define DEFAULT_TIMEZONE_POSIX          "UTC0"
#define DEFAULT_NIGHT_DIM_START_HOUR    22
#define DEFAULT_NIGHT_DIM_END_HOUR      7
#define DEFAULT_NIGHT_DIM_BRIGHTNESS    20
#define DEFAULT_SLIDESHOW_INTERVAL_SEC  5

// =============================================================================
// WiFi 설정 상수
// =============================================================================

// WiFi 기본 설정
#define WIFI_SSID_MAX_LEN       32
#define WIFI_PASS_MAX_LEN       64
#define WIFI_CONNECT_TIMEOUT    10000  // 10초

// AP 모드 설정 (WiFi 설정 포털용)
#define DEFAULT_AP_SSID         "ESP32_Wx_Config"
#define DEFAULT_AP_PASSWORD     ""
#define AP_CHANNEL              1
#define AP_MAX_CONN             4
#define DEFAULT_AP_IP_ADDR      "192.168.4.1"
#define DEFAULT_AP_NETMASK      "255.255.255.0"

// =============================================================================
// 날씨 API 설정 상수
// =============================================================================

// API 키 및 도시명 길이 제한
#define API_KEY_MAX_LEN         64
#define CITY_NAME_MAX_LEN       32

// 업데이트 간격 (밀리초)
#define UPDATE_INTERVAL_MS      600000  // 10분
#define DEFAULT_UPDATE_INTERVAL_SEC 300  // 5분

// OpenWeatherMap API URLs
#define OPENWEATHERMAP_CURRENT_API_URL "http://api.openweathermap.org/data/2.5/weather"
#define OPENWEATHERMAP_FORECAST_API_URL "http://api.openweathermap.org/data/2.5/forecast"
#define OPENWEATHERMAP_GEOCODING_API_URL "http://api.openweathermap.org/geo/1.0/direct"
#define OPENWEATHERMAP_API_TIMEOUT_MS 5000  // 5초

// =============================================================================
// 웹 서버 설정 상수
// =============================================================================

#define CONFIG_SERVER_PORT      80
#define WEB_SERVER_MAX_CONNECTIONS 4

// =============================================================================
// 시스템 설정 상수
// =============================================================================

// 태스크 스택 크기 (power of two, bytes 단위)
#define WEATHER_TASK_STACK_SIZE 4096
#define UI_TASK_STACK_SIZE      4096

// 태스크 우선순위
#define WEATHER_TASK_PRIORITY   4
#define UI_TASK_PRIORITY        3

// 태스크 이름
#define WEATHER_TASK_NAME       "weather_task"
#define UI_TASK_NAME            "ui_task"

// CPU 코어 설정
#define APP_TASK_CORE_ID       1   // 애플리케이션용 코어 (코어1)
#define SYSTEM_CORE_ID          0   // 시스템용 코어 (코어0)

// 시스템 타임아웃
#define SYSTEM_RESTART_DELAY_MS 5000
#define CONFIG_PORTAL_TIMEOUT_MS 300000  // 5분

// =============================================================================
// 하드웨어 설정 상수
// =============================================================================

// 디스플레이 설정
#define DISPLAY_WIDTH           320
#define DISPLAY_HEIGHT          480
#define DISPLAY_ROTATION        270  // 0, 90, 180, 270 도

// LCD 좌우/상하 미러 옵션 (필요 시 조정)
// 좌우가 바뀌어 보이면 LCD_MIRROR_X를 1로 설정합니다.
// 상하가 바뀌어 보이면 LCD_MIRROR_Y를 1로 설정합니다.
#ifndef LCD_MIRROR_X
#define LCD_MIRROR_X            1   // 좌우 반전 ON (영상 좌우 반전 이슈 대응)
#endif
#ifndef LCD_MIRROR_Y
#define LCD_MIRROR_Y            0   // 상하 반전
#endif

// LCD 패널 드라이버 선택 (ESP-IDF 제공 드라이버 기준)
// ST7796 심볼이 없는 IDF 버전이 있어 기본 ILI9488로 설정
#define LCD_PANEL_USE_ST7796     1
#define LCD_PANEL_USE_ILI9488    0

// WT32-SC01-PLUS (ESP32-S3) LCD 인터페이스 핀 매핑
// - 패널 컨트롤러: ST7796
// - 버스 타입: 8bit MCU (8080, i80)
// 데이터시트 표(첨부) 기준으로 정의

// 백라이트 및 리셋
#define LCD_BL_PIN              45   // BL_PWM, Active High
#define LCD_BL_ACTIVE_HIGH      1
#define DEFAULT_BRIGHTNESS_PERCENT 80
#define LCD_RST_PIN             4    // LCD reset, TP reset과 멀티플렉스

// i80(8080) 제어 신호
#define LCD_RS_PIN              0    // RS (D/C)
#define LCD_WR_PIN              47   // WR (Write clock)
#define LCD_TE_PIN              48   // TE (Tearing effect / frame sync)

// i80(8080) 데이터 버스 (DB0..DB7)
#define LCD_DB0_PIN             9
#define LCD_DB1_PIN             46
#define LCD_DB2_PIN             3
#define LCD_DB3_PIN             8
#define LCD_DB4_PIN             18
#define LCD_DB5_PIN             17
#define LCD_DB6_PIN             16
#define LCD_DB7_PIN             15

// 레거시(SPI) 핀 정의 - 현재 보드에서는 사용하지 않음. 유지하여 기존 코드 빌드 영향 최소화.
#define LCD_SPI_MOSI_PIN        11
#define LCD_SPI_SCLK_PIN        12
#define LCD_SPI_CS_PIN          10
#define LCD_SPI_DC_PIN          13
#define LCD_PIXEL_CLOCK_HZ      40000000  // 40MHz (SPI 사용 시)

// LVGL 렌더 버퍼 설정 (라인 수)
// PSRAM 활성화 전에는 40(내부 DMA RAM에 맞는 크기)이었으나, PSRAM 사용 시
// 320*120*2B = 76.8KB로 확대. 갤러리 슬라이드쇼 성능을 보고 더 키울 수 있음.
#define LVGL_BUFFER_LINES       120
#define LVGL_TICK_PERIOD_MS      5
#define LVGL_TASK_PERIOD_MS      10
#define LVGL_TASK_STACK_SIZE     4096
#define LVGL_TASK_PRIORITY       5
#define LVGL_TASK_CORE_ID        1

// 터치 스크린 설정
#define TOUCH_I2C_SDA_PIN       6
#define TOUCH_I2C_SCL_PIN       5
#define TOUCH_I2C_ADDR          0x38
#define TOUCH_I2C_CLK_HZ        400000
#define TOUCH_I2C_PORT          0  // I2C_NUM_0
#define TOUCH_INT_PIN           7
#define TOUCH_RST_PIN           4


// SD카드 설정 (외장 SPI microSD 모듈 — 보드 자체에는 SD 슬롯이 없음)
// LCD는 i80(병렬) 버스를 쓰므로 SPI2_HOST는 비어 있지만, 이름이 헷갈리는
// DISPLAY_SPI_HOST(레거시/미사용) 대신 SD 전용 SPI 호스트를 명시적으로 지정한다.
#define SD_MOUNT_POINT          "/sdcard"
#define SD_PHOTOS_SUBDIR        "/photos"
#define SD_SPI_HOST             SPI3_HOST
#define SD_SPI_MISO_PIN         38
#define SD_SPI_MOSI_PIN         40
#define SD_SPI_SCK_PIN          39
#define SD_SPI_CS_PIN           41
#define SD_SPI_MAX_FILES        5
#define SD_SPI_MAX_TRANSFER_SZ  4000

// UART for Debug
#define UART_DEV_TX_PIN         43
#define UART_DEV_RX_PIN         44

//Extended IO
#define EXT_IO_01_PIN           10
#define EXT_IO_02_PIN           11
#define EXT_IO_03_PIN           12
#define EXT_IO_04_PIN           13
#define EXT_IO_05_PIN           14
#define EXT_IO_06_PIN           21


// =============================================================================
// 로그 태그 상수
// =============================================================================

#define LOG_TAG_MAIN            "MAIN"
#define LOG_TAG_WIFI            "WIFI_MANAGER"
#define LOG_TAG_WEATHER         "WEATHER_TASK"
#define LOG_TAG_UI              "UI_APP"
#define LOG_TAG_NVS             "NVS_MANAGER"
#define LOG_TAG_CONFIG_PORTAL   "CONFIG_PORTAL"
#define LOG_TAG_OTA             "OTA_MANAGER"
#define LOG_TAG_SPIFFS          "SPIFFS_MANAGER"
#define LOG_TAG_SYS_UTIL        "SYS_UTIL"
#define LOG_TAG_SD              "SD_STORAGE"
#define LOG_TAG_DISPLAY         "DISPLAY"
#define LOG_TAG_CONSOLE         "UART_CONSOLE"

// =============================================================================
// 폰트 설정 (한글 표시용)
// =============================================================================
// 한글 표시를 위해 커스텀 폰트를 사용할지 여부 (0: 사용 안 함, 1: 사용)
// 1로 설정하고, ui/fonts 디렉토리에 lv_font_conv로 생성한 한글 폰트 C 파일을 추가하세요.
#ifndef USE_FONT_KOREAN
#define USE_FONT_KOREAN         0
#endif

#endif // CONSTANTS_H