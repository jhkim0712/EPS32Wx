#ifndef SYSTEM_RESTART_H
#define SYSTEM_RESTART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 지정한 시간(ms) 뒤에 esp_restart()를 호출하는 백그라운드 태스크를 생성한다.
 *
 * HTTP 응답 등 호출자가 먼저 마쳐야 하는 작업이 끝날 시간을 벌기 위해 사용한다.
 * 태스크 생성에 실패하면 즉시 esp_restart()를 호출한다.
 *
 * @param delay_ms 재부팅까지 대기할 시간 (밀리초)
 */
void system_restart_delayed(uint32_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_RESTART_H
