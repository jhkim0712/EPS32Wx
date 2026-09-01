# 🌤️ ESP32Wx

WT32-SC01-PLUS(ESP32-S3, 3.5" 터치 LCD) 기반의 스마트 데스크 디스플레이 프로젝트입니다.
날씨/시계 화면, WiFi 설정 포털, SD 카드 디지털 액자, GitHub 릴리스 기반 OTA를 ESP-IDF 표준
컴포넌트 구조 위에서 제공합니다. GeekMagic SmallTV류 제품의 UX(시계·날씨·사진 슬라이드쇼
화면 전환 + WiFi/System/Pictures 탭 웹 설정)를 참고해 온디바이스 터치 화면과 웹 포털 양쪽에
적용했습니다.

## 📋 목차

- [프로젝트 개요](#-프로젝트-개요)
- [주요 기능](#-주요-기능)
- [프로젝트 구조](#-프로젝트-구조)
- [하드웨어 요구사항](#-하드웨어-요구사항)
- [소프트웨어 요구사항](#-소프트웨어-요구사항)
- [설치 및 빌드](#-설치-및-빌드)
- [사용 방법](#-사용-방법)
- [설정 포털 사용법](#-설정-포털-사용법)
- [메모리 파티션](#-메모리-파티션)
- [디버깅](#-디버깅)
- [향후 계획](#-향후-계획)
- [라이선스](#-라이선스)

## 🎯 프로젝트 개요

WT32-SC01-PLUS(ESP32-S3, 320×480 ST7796 터치 LCD)를 사용하는 데스크 디스플레이 프로젝트입니다.
ESP-IDF 표준 컴포넌트 구조 위에 온디바이스 UI, 웹 설정 포털, SD 카드 디지털 액자, GitHub OTA를
갖췄습니다.

### 주요 특징
- 🖥️ **온디바이스 UI**: Clock/Weather/Gallery/Settings 4개 화면을 스와이프+하단 탭바로 전환
- 📱 **웹 설정 포털**: WiFi/System/Pictures 탭 구조, AP 모드뿐 아니라 정상 운영 중에도 접속 가능
- 🖼️ **SD 카드 디지털 액자**: 외장 SPI microSD 모듈에서 jpg/png/gif 슬라이드쇼
- 🔄 **GitHub 매니페스트 기반 OTA**: 특정 저장소에 종속되지 않는 범용 업데이트 메커니즘 + 롤백
- 🔧 **ESP-IDF 표준 컴포넌트 구조**: 기능별로 완전히 독립된 컴포넌트, 순환 의존성 없음
- 💾 **영구 저장소**: NVS(설정)·SPIFFS(웹 파일)·SD카드(사진) 조합
- 🕹️ **UART 콘솔**: 기본 콘솔 UART 위 `esp_console` REPL로 실시간 디버그 명령

## ✨ 주요 기능

### 1. 🖥️ 온디바이스 UI (`components/ui`)
- Clock: 큰 시계 + 날짜 + 날씨 요약 한 줄
- Weather: 온도/습도/날씨상태 카드
- Gallery: SD카드 `/photos`의 jpg/png/gif 슬라이드쇼 (자동 전환 + 탭으로 수동 이동)
- Settings: 밝기(LEDC PWM)/화면 회전(런타임)/WiFi 재설정/야간 자동 감광/OTA 확인·설치/SD 상태
- 스와이프 제스처 또는 하단 4아이콘 내비게이션 바로 화면 전환

### 2. 🌐 웹 설정 포털 (`components/webui`)
- **WiFi** 탭: 실시간 스캔, 신호 강도/보안 표시, 저장 시 자동 재시작
- **System** 탭: 밝기/회전/타임존/야간감광/OTA 매니페스트 URL 설정 및 즉시 반영
- **Pictures** 탭: 슬라이드쇼 on/off·간격, SD카드 `/photos` 파일 목록 조회
- AP 설정 모드뿐 아니라 STA로 정상 연결된 뒤에도 계속 접속 가능

### 3. 🖼️ SD 카드 디지털 액자 (`components/sd_storage`)
- 외장 SPI microSD 모듈(SDSPI+FATFS), 보드에 슬롯이 없어 GPIO 38/39/40/41에 배선
- jpg/jpeg/png/gif 자동 인식, LVGL GIF/JPEG/PNG 디코더 + PSRAM 백엔드 힙

### 4. 🔄 GitHub 매니페스트 기반 OTA (`components/ota_updater`)
- `{version, url, size, sha256}` 형식의 매니페스트 JSON — 특정 저장소 하드코딩 없음
- `esp_https_ota` 스트리밍 다운로드 + 진행률, 부팅 시 롤백 헬스체크, 수동 롤백 지원

### 5. 📶 WiFi 관리 (`components/wifi_net`)
- **STA 모드**: 저장된 WiFi 네트워크 자동 연결
- **AP 모드**: 설정용 핫스팟 생성 (SSID: `ESP32_Wx_Config`)
- 연결 상태 모니터링, 스캔 기능

### 6. 🔧 NVS 설정 관리 (`components/nvs_storage`)
- WiFi 자격 증명, API 키, 도시명, 밝기/회전/타임존/야간감광/슬라이드쇼/OTA 매니페스트 URL 등
  전체 설정을 NVS에 저장

### 7. 🕹️ UART 디버그 콘솔 (`components/uart_console`)
- 기본 콘솔 UART(=`idf.py monitor` 포트)에 `esp_console` REPL 연결
- `help`/`restart`/`heap`/`version`/`wifi`/`sd [ls]`/`brightness [0-100]`/`ota [check|status]`

## 📁 프로젝트 구조

이 프로젝트는 ESP-IDF 표준 컴포넌트 형식을 따릅니다. `main/`은 각 컴포넌트를 초기화 순서대로
호출하는 얇은 오케스트레이터일 뿐이고, 실제 기능은 모두 `components/` 아래 독립 컴포넌트로 분리되어
있습니다 (각 컴포넌트는 자체 `CMakeLists.txt`/`include/` 를 가지며 `idf_component_register()`로 등록됩니다).

```
ESP32Wx/
├── 📄 CMakeLists.txt          # ESP-IDF 프로젝트 루트 설정 (EXTRA_COMPONENT_DIRS=components)
├── 📄 partitions.csv          # 메모리 파티션 테이블 (16MB Flash, A/B OTA)
├── 📄 README.md              # 프로젝트 문서 (이 파일)
├── 📂 docs/                   # 문서 및 이미지
│   └── 📂 images/
│       ├── 📂 hardware/       # 하드웨어 연결 이미지
│       └── 📂 ui/            # UI 스크린샷
├── 📂 data/                   # 웹 설정 포털 정적 파일 (SPIFFS로 이미지화)
│   ├── 📄 index.html
│   ├── 📂 css/
│   └── 📂 js/
├── 📂 components/             # ESP-IDF 컴포넌트들
│   ├── 📂 cjson/              # cJSON 라이브러리
│   ├── 📂 lvgl/               # LVGL 8.3.11 그래픽 라이브러리
│   ├── 📂 app_config/         # 공유 상수/설정/타입 (헤더 전용)
│   ├── 📂 sys_util/           # 공용 유틸리티 (지연 재부팅 등)
│   ├── 📂 nvs_storage/        # NVS 설정 저장소
│   ├── 📂 spiffs_storage/     # SPIFFS 파일시스템 (웹 파일)
│   ├── 📂 sd_storage/         # SD 카드 (SDSPI+FATFS, 디지털 액자용)
│   ├── 📂 display/            # LCD/LVGL 드라이버 + 백라이트 PWM
│   ├── 📂 ui/                 # 온디바이스 화면 관리자 + 화면들
│   ├── 📂 wifi_net/           # WiFi STA/AP 관리
│   ├── 📂 webui/              # 웹 설정 포털 (HTTP 서버 + API)
│   ├── 📂 weather/            # 날씨 데이터 처리
│   ├── 📂 ota_updater/        # GitHub 매니페스트 기반 OTA
│   └── 📂 uart_console/       # 기본 콘솔 UART(UART0) 위의 esp_console REPL 디버그 명령
└── 📂 main/                   # 얇은 오케스트레이터 (ESP-IDF 표준)
    ├── 📄 main.c             # app_main(): 각 컴포넌트 초기화/배선
    └── 📄 CMakeLists.txt     # main 컴포넌트 빌드 설정 (PRIV_REQUIRES로 전체 컴포넌트 연결)
```

## 🔧 하드웨어 요구사항

### 🎯 **개발 보드: WT32-SC01-PLUS**
- **SoC**: WT32-S3-WROVER-N16R2 (ESP32-S3 기반)
- **Flash**: 16MB (QIO 모드)
- **PSRAM**: 2MB (QIO 모드)
- **디스플레이**: 3.5" TFT LCD 320×480 (ST7796 컨트롤러)
- **터치**: I2C 터치 패널 (FT6236 컨트롤러)
- **WiFi**: IEEE 802.11 b/g/n (2.4 GHz)

### WT32-SC01-PLUS 핀 매핑 (기본값)
- Board 제조사: https://en.wireless-tag.com/product-item-26.html

핀 매핑은 `components/app_config/include/common/constants.h`에서 수정할 수 있습니다.

### 💾 SD 카드 (선택, 디지털 액자용)
WT32-SC01-PLUS 보드 자체에는 SD 슬롯이 없습니다. 외장 SPI microSD 브레이크아웃 모듈을 아래
핀에 배선하세요 (배선하지 않아도 나머지 기능은 정상 동작하며, 갤러리 화면만 비활성화됩니다):

| 모듈 핀 | GPIO |
|---|---|
| MISO | 38 |
| MOSI | 40 |
| SCK  | 39 |
| CS   | 41 |

카드에 FAT32로 포맷 후 `/photos` 폴더를 만들고 jpg/png/gif 파일을 넣으세요.

### 📋 **최소 시스템 요구사항**
- **Flash 메모리**: 최소 8MB (권장 16MB)
- **PSRAM**: 필수 (LVGL 힙/버퍼 및 사진·GIF 디코딩에 사용, `sdkconfig.defaults`에서 활성화됨)
- **WiFi 안테나**: 내장 또는 외장

## 💻 소프트웨어 요구사항

- **ESP-IDF Framework** 5.5.x (프로젝트는 5.5.1 기준으로 개발됨)
- **VS Code** + **ESP-IDF Extension** (또는 CLI)
- **Python** >= 3.8 (ESP-IDF용)
- **Git** (소스 코드 관리용)

### 🔧 **ESP-IDF 컴포넌트 의존성**
컴포넌트별 정확한 `REQUIRES`/`PRIV_REQUIRES`는 각 `components/*/CMakeLists.txt`를 참고하세요.
프로젝트 전체가 사용하는 주요 ESP-IDF 컴포넌트:
- `nvs_flash` - 설정 저장 (WiFi 자격증명, API 키, System/Pictures/OTA 설정 등)
- `esp_wifi` / `esp_netif` / `esp_event` - WiFi STA/AP
- `esp_http_server` / `esp_http_client` / `esp-tls` / `mbedtls` - 웹 포털, OTA 매니페스트/펌웨어 다운로드
- `spiffs` - 웹 파일 시스템 (`data/`)
- `fatfs` / `sdmmc` - SD 카드(FAT32) 마운트
- `cjson` (자체 벤더링) - JSON 파싱
- `driver` / `esp_lcd` - LCD/터치/LEDC 백라이트/SPI
- `lvgl` (자체 벤더링, v8.3.11) - UI 그래픽 라이브러리
- `app_update` / `esp_https_ota` - OTA 업데이트
- `console` - UART 디버그 REPL

## 🚀 설치 및 빌드

### 1. ESP-IDF 환경 준비
```bash
# ESP-IDF 설치 (Windows)
# VS Code에서 ESP-IDF Extension 설치 권장
# 또는 ESP-IDF 공식 설치 가이드 참조
```

### 2. 저장소 클론
```bash
git clone https://github.com/your-username/esp32-weather-station.git
cd ESP32Wx
```

### 3. ESP-IDF로 빌드 (Windows PowerShell)
```powershell
# ESP-IDF 환경 활성화 이후 실행 (ESP-IDF PowerShell 또는 export.ps1)
idf.py set-target esp32s3
idf.py build

# 플래시 및 모니터링
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

### 4. sdkconfig / PSRAM
`sdkconfig`는 저장소에 커밋되어 있어 바로 빌드 가능하지만, `sdkconfig.defaults`에도 동일한
핵심 설정(PSRAM Quad 모드, 16MB Flash, 커스텀 파티션 테이블, OTA 롤백)이 정리되어 있습니다.
`sdkconfig`를 지우고 새로 구성할 경우 이 값들이 시드로 사용됩니다.

### 5. LVGL 포팅 요약 (v8.3.11)

- LVGL 코어 초기화 후 esp_timer로 5ms tick, 10ms 주기 핸들러 태스크(Core 1)
- 디스플레이: 커스텀 MIPI DCS 명령으로 ST7796 제어, i80(8080) 8bit 병렬 버스
- 백라이트: LEDC PWM (`components/display/backlight.c`), 0~100% 밝기 조절
- 회전: `display_set_rotation()`으로 런타임 변경 가능 (컴파일 타임 상수 아님)
- 터치: FT6236 I2C 단일 터치 좌표 읽기
- 스레드 안전: 다른 태스크에서 LVGL API 사용 시 `lvgl_lock(timeout_ms)`/`lvgl_unlock()` 호출
- 힙: PSRAM 백엔드 커스텀 할당자 (`LV_MEM_CUSTOM`), GIF/JPEG/PNG 디코더 활성화

### 6. 파티션 설정
- **16MB Flash**: `partitions.csv`에서 커스텀 파티션 테이블 사용
- **SPIFFS**: 웹 파일용 ~4.75MB 할당
- **OTA**: `ota_0`/`ota_1` 1.5MB씩 A/B 파티션 (`otadata` 포함) — 무선 업데이트 + 롤백 지원

## 📱 사용 방법

### 첫 설정 (최초 1회)

1. **디바이스 부팅**
   - ESP32를 전원에 연결
   - 첫 부팅 시 자동으로 AP 모드로 전환

2. **WiFi 연결**
   - 스마트폰/PC에서 **"ESP32_Wx_Config"** 네트워크 연결 (기본 오픈 네트워크, 비밀번호 없음)

3. **설정 포털 접속**
   - 브라우저에서 `http://192.168.4.1` 접속
   - 설정 페이지가 자동으로 로드됨

4. **설정 입력**
   - WiFi 네트워크 선택 또는 직접 입력
   - WiFi 비밀번호 입력
   - OpenWeatherMap API 키 입력
   - 도시명 입력 (예: Seoul, Tokyo, New York)

5. **설정 완료**
   - "설정 저장" 버튼 클릭
   - 디바이스가 (응답 전송 후) 약 2초 뒤 자동으로 재시작
   - 설정된 WiFi로 자동 연결

### 정상 운영

- 디바이스가 설정된 WiFi에 자동 연결되면 화면이 Clock 화면으로 전환됨
- 화면을 스와이프하거나 하단 탭바로 Clock/Weather/Gallery/Settings 전환
- 웹 포털은 STA 연결 후에도 계속 접속 가능 (`http://<디바이스 IP>`) — System/Pictures 탭 등 설정 변경
- WiFi 연결이 실패하면 자동으로 AP 설정 모드로 전환
- ⚠️ 날씨 데이터는 아직 실제 API 연동이 없는 상태(스텁 데이터)입니다 — [향후 계획](#-향후-계획) 참고

## 🌐 설정 포털 사용법

웹 포털은 WiFi / System / Pictures 3개 탭으로 구성됩니다 (`data/index.html` + `css/`+`js/`).

### WiFi 탭
1. **WiFi 스캔** — 버튼 클릭 시 주변 네트워크 목록, 신호 강도, 보안 상태 표시. 클릭하면 SSID 자동 입력
2. **수동 입력** — 숨겨진 네트워크는 SSID 직접 입력, 비밀번호는 항상 수동 입력
3. **API 설정** — [OpenWeatherMap](https://openweathermap.org/api) 무료 API 키 (선택, 대소문자 구분)
4. **도시명** — 영문 도시명 (예: Seoul, Busan,KR)

### System 탭
- 밝기 슬라이더, 화면 회전(0/90/180/270°), 타임존(POSIX TZ 문자열, 예: `KST-9`)
- 야간 자동 감광: 시작/종료 시각 + 상태 표시
- 펌웨어: 현재 버전, OTA 매니페스트 URL 저장, "업데이트 확인"/"지금 업데이트" + 진행률 표시

### Pictures 탭
- 슬라이드쇼 사용 여부 + 전환 간격(초)
- SD카드 `/photos` 파일 목록 (읽기 전용 — 업로드는 SD카드를 PC에 연결해 직접 채우는 방식)

### API 엔드포인트

| 엔드포인트 | 메소드 | 설명 |
|-----------|--------|------|
| `/` | GET | 설정 포털 메인 페이지 |
| `/api/scan` | GET | WiFi 네트워크 스캔 |
| `/api/config` | POST | WiFi/API키/도시명 저장 (기존 하위 호환) |
| `/api/status` | GET | WiFi 상태/IP/밝기/SD 마운트/펌웨어 버전 |
| `/api/system` | GET/POST | 밝기/회전/타임존/야간감광/OTA 매니페스트 URL 조회·저장(즉시 반영) |
| `/api/gallery/list` | GET | SD `/photos` 파일 목록 |
| `/api/gallery/config` | POST | 슬라이드쇼 사용 여부/간격 저장 |
| `/api/ota/check` | GET | 매니페스트 확인, 새 버전 유무 |
| `/api/ota/start` | POST | OTA 업데이트 시작 (비동기) |
| `/api/ota/status` | GET | OTA 진행 상태/퍼센트 폴링 |

### API 응답 예제

#### WiFi 스캔 (`/api/scan`)
```json
{
  "success": true,
  "networks": [
    { "ssid": "MyWiFi", "rssi": -45, "authmode": 3 }
  ]
}
```

#### System 설정 조회 (`/api/system`)
```json
{
  "success": true,
  "brightness": 80,
  "rotation": 270,
  "timezone": "KST-9",
  "nightDimEnabled": false,
  "nightDimStart": 22,
  "nightDimEnd": 7,
  "otaManifestUrl": "",
  "firmwareVersion": "1.0.0"
}
```

## 📊 메모리 파티션

`partitions.csv` 기준 (16MB Flash, A/B OTA):

| 파티션 | 타입 | 오프셋 | 크기 | 용도 |
|--------|------|--------|------|------|
| nvs | data | 0x9000 | 24KB | 설정 저장소 |
| phy_init | data | 0xf000 | 4KB | RF 보정 데이터 |
| otadata | data | 0x10000 | 8KB | 현재 부팅 슬롯 정보 |
| ota_0 | app | 0x20000 | 1.5MB | 애플리케이션 슬롯 A |
| ota_1 | app | 0x1A0000 | 1.5MB | 애플리케이션 슬롯 B (OTA 대상) |
| spiffs | data | 0x320000 | ~4.75MB | 웹 파일(`data/`) |

## 🔍 디버깅

### 시리얼 모니터 / UART 콘솔
```powershell
# 실시간 로그 확인 (ESP-IDF)
idf.py -p COM3 monitor
```
로그와 같은 UART(UART0)에 `esp_console` REPL이 붙어 있어, `idf.py monitor` 화면에서 바로
명령을 입력할 수 있습니다:

| 명령 | 기능 |
|---|---|
| `help` | 전체 명령 목록 |
| `restart` | 재부팅 |
| `heap` | free heap (전체/internal/PSRAM) |
| `version` | 펌웨어/IDF/칩 버전 |
| `wifi` | WiFi 연결 상태 |
| `sd [ls]` | SD 마운트 상태, `sd ls`로 `/photos` 목록 |
| `brightness [0-100]` | 밝기 조회/설정 |
| `ota [check\|status]` | OTA 업데이트 확인/진행 상태 |

### 주요 로그 태그
- `MAIN` - main.c 오케스트레이터
- `WIFI_MANAGER` - WiFi 연결 상태
- `CONFIG_PORTAL` - 웹 서버 동작
- `NVS_MANAGER` - 설정 저장/로드
- `SPIFFS_MGR` / `SD_STORAGE` - 파일 시스템
- `DISPLAY` / `UI_APP` - 디스플레이/UI
- `OTA_MANAGER` - OTA 업데이트
- `SYS_UTIL` - 지연 재부팅 등 공용 유틸
- `UART_CONSOLE` - 디버그 콘솔

### 문제 해결

1. **WiFi 연결 실패**
   - 시리얼 모니터에서 `WIFI_MANAGER` 로그 확인
   - 설정을 다시 입력하거나 디바이스 리셋

2. **웹 페이지 로드 실패**
   - AP 모드 IP 주소 확인 (기본: 192.168.4.1)
   - 방화벽 설정 확인

3. **설정 저장 실패**
   - NVS 파티션 상태 확인
   - 플래시 메모리 오류 가능성 점검

## 🔮 향후 계획

### ✅ 완료
- [x] ESP-IDF 표준 컴포넌트 구조로 재편 (12개 컴포넌트, 순환 의존성 제거)
- [x] LCD/터치 디스플레이 드라이버 (ST7796, i80, FT6236), LEDC 백라이트 PWM
- [x] 온디바이스 UI (Clock/Weather/Gallery/Settings, 스와이프+탭바 내비게이션)
- [x] 웹 설정 포털 WiFi/System/Pictures 탭 재구성
- [x] SD 카드 디지털 액자 (SDSPI+FATFS, jpg/png/gif 슬라이드쇼)
- [x] GitHub 매니페스트 기반 OTA (스트리밍 다운로드, 진행률, 롤백)
- [x] 야간 자동 감광 스케줄러, 화면 회전 런타임 API
- [x] UART 디버그 콘솔 (`esp_console` REPL)

### 남은 과제: 날씨 API 연동
`weather_interface.c`는 아직 실제 API 호출 없이 고정된 스텁 데이터(23.5°C 등)를 반환합니다.
- [ ] `openweathermap_api.c` 구현 (헤더만 존재, cJSON은 이미 벤더링됨)
- [ ] HTTP 클라이언트 기반 실제 요청 + JSON 파싱
- [ ] 오류 처리 및 재시도 로직
- [ ] 날씨 아이콘 그래픽 렌더링 (현재는 텍스트만)

### 남은 과제: 고급 기능
- [ ] 다국어 폰트 (한글 등 — `USE_FONT_KOREAN` 플래그는 있으나 폰트 파일 미포함)
- [ ] 날씨 예보 (5일간), 알림/경고 시스템
- [ ] 데이터 로깅 및 히스토리
- [ ] MQTT 연동, 센서 데이터 수집
- [ ] 다중 도시 지원, 웹 포털 기본 인증(UI는 있으나 서버측 미검증 경로 재점검 필요)
- [ ] SD 카드 사진 업로드(현재는 PC로 직접 채우는 방식만 지원)

## 🤝 기여하기

1. 이 저장소를 Fork
2. 새로운 기능 브랜치 생성 (`git checkout -b feature/AmazingFeature`)
3. 변경사항을 커밋 (`git commit -m 'Add some AmazingFeature'`)
4. 브랜치에 푸시 (`git push origin feature/AmazingFeature`)
5. Pull Request 생성

## 📝 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 📞 연락처

- **프로젝트 링크**: [https://github.com/your-username/esp32-weather-station](https://github.com/your-username/esp32-weather-station)
- **이슈 트래커**: [https://github.com/your-username/esp32-weather-station/issues](https://github.com/your-username/esp32-weather-station/issues)

## 🙏 감사의 말

- [ESP-IDF](https://github.com/espressif/esp-idf) - Espressif Systems
- [OpenWeatherMap](https://openweathermap.org/) - 날씨 API 서비스
- [cJSON](https://github.com/DaveGamble/cJSON) - JSON 파싱 라이브러리

---

⭐ 이 프로젝트가 유용하다면 별표를 눌러주세요!