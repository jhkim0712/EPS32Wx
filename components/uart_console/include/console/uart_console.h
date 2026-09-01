#ifndef UART_CONSOLE_H
#define UART_CONSOLE_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 기본 콘솔 UART(idf.py monitor와 같은 포트, sdkconfig의
 *        CONFIG_ESP_CONSOLE_UART_NUM — 보통 UART0)에 esp_console REPL을 붙여 시작한다.
 *        로그(ESP_LOGx) 출력과 같은 포트를 공유하므로 monitor 화면에 로그와 명령 입출력이
 *        섞여 보이는 것이 정상이다. 내부적으로 백그라운드 태스크에서 동작하므로 호출 즉시
 *        반환한다. 여러 번 호출해도 안전하다(이미 시작됐으면 ESP_OK).
 *
 *        기본 제공 명령: help, restart, heap, version, wifi, sd, brightness, ota
 * @return ESP_OK on success
 */
esp_err_t uart_console_start(void);

#ifdef __cplusplus
}
#endif

#endif // UART_CONSOLE_H
