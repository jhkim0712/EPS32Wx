#ifndef SPIFFS_MANAGER_H
#define SPIFFS_MANAGER_H

#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief SPIFFS 초기화
 * @return ESP_OK on success
 */
esp_err_t spiffs_manager_init(void);

/**
 * @brief SPIFFS 해제
 */
void spiffs_manager_deinit(void);

/**
 * @brief 파일 읽기
 * @param path 파일 경로
 * @param buffer 데이터를 저장할 버퍼
 * @param size 읽을 크기
 * @return 실제로 읽은 바이트 수, 오류 시 -1
 */
int spiffs_read_file(const char* path, char* buffer, size_t size);

/**
 * @brief 파일 쓰기
 * @param path 파일 경로
 * @param data 쓸 데이터
 * @param size 데이터 크기
 * @return ESP_OK on success
 */
esp_err_t spiffs_write_file(const char* path, const char* data, size_t size);

/**
 * @brief 파일 존재 여부 확인
 * @param path 파일 경로
 * @return true if exists, false otherwise
 */
bool spiffs_file_exists(const char* path);

/**
 * @brief 파일 크기 얻기
 * @param path 파일 경로
 * @return 파일 크기, 오류 시 -1
 */
long spiffs_get_file_size(const char* path);

/**
 * @brief 파일시스템 정보 출력
 */
void spiffs_show_info(void);

/**
 * @brief Compatibility wrappers (legacy names)
 * These map to spiffs_manager_init/deinit and are kept for convenience.
 */
esp_err_t spiffs_init(void);
void spiffs_deinit(void);

#endif // SPIFFS_MANAGER_H