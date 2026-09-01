// WT32-SC01-PLUS 보드 전용 하드웨어 배선/핀 정의.
//
// constants.h와 분리한 이유: 이 파일은 "이 보드는 어떻게 배선되어 있는가"만 다루고,
// constants.h는 NVS 키/타임아웃/태스크 설정 같은 소프트웨어 로직 상수를 다룬다.
// 다른 보드로 포팅하거나 핀을 바꿀 때는 이 파일만 보면 된다.
//
// Board 제조사: https://en.wireless-tag.com/product-item-26.html
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

// =============================================================================
// 디스플레이 패널
// =============================================================================

#define DISPLAY_WIDTH           320
#define DISPLAY_HEIGHT          480
#define DISPLAY_ROTATION        270  // 0, 90, 180, 270 도 (런타임 변경은 display_set_rotation() 참고)

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

// =============================================================================
// 터치 스크린 (FT6236, I2C)
// =============================================================================

#define TOUCH_I2C_SDA_PIN       6
#define TOUCH_I2C_SCL_PIN       5
#define TOUCH_I2C_ADDR          0x38
#define TOUCH_I2C_CLK_HZ        400000
#define TOUCH_I2C_PORT          0  // I2C_NUM_0
#define TOUCH_INT_PIN           7
#define TOUCH_RST_PIN           4

// =============================================================================
// SD 카드 (외장 SPI microSD 모듈 — 보드 자체에는 SD 슬롯이 없음)
// =============================================================================
// LCD는 i80(병렬) 버스를 쓰므로 SPI2_HOST는 비어 있지만, 이름이 헷갈리는
// DISPLAY_SPI_HOST(레거시/미사용) 대신 SD 전용 SPI 호스트를 명시적으로 지정한다.
#define SD_SPI_HOST             SPI3_HOST
#define SD_SPI_MISO_PIN         38
#define SD_SPI_MOSI_PIN         40
#define SD_SPI_SCK_PIN          39
#define SD_SPI_CS_PIN           41

// =============================================================================
// 디버그 UART (현재 미사용 — UART 콘솔은 기본 콘솔 UART/UART0을 그대로 사용함.
// 별도 채널이 필요해지면 재사용할 수 있도록 예약만 해 둔다.)
// =============================================================================
#define UART_DEV_TX_PIN         43
#define UART_DEV_RX_PIN         44

// =============================================================================
// 확장 IO
// =============================================================================
#define EXT_IO_01_PIN           10
#define EXT_IO_02_PIN           11
#define EXT_IO_03_PIN           12
#define EXT_IO_04_PIN           13
#define EXT_IO_05_PIN           14
#define EXT_IO_06_PIN           21

#endif // BOARD_PINS_H
