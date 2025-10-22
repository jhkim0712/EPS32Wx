# 🌤️ ESP32 Weather Station

ESP32 기반의 모듈화된 날씨 스테이션 프로젝트입니다. WiFi 설정 포털, NVS 저장소, SPIFFS 파일 시스템을 포함한 완전한 IoT 솔루션을 제공합니다.

## 📋 목차

- [프로젝트 개요](#-프로젝트-개요)
- [주요 기능](#-주요-기능)
- [프로젝트 구조](#-프로젝트-구조)
- [하드웨어 요구사항](#-하드웨어-요구사항)
- [소프트웨어 요구사항](#-소프트웨어-요구사항)
- [설치 및 빌드](#-설치-및-빌드)
- [사용 방법](#-사용-방법)
- [설정 포털 사용법](#-설정-포털-사용법)
- [API 문서](#-api-문서)
- [향후 계획](#-향후-계획)
- [라이선스](#-라이선스)

## 🎯 프로젝트 개요

이 프로젝트는 ESP32-S3-Box를 사용하여 실시간 날씨 정보를 표시하는 스마트 날씨 스테이션입니다. 모듈화된 아키텍처로 설계되어 확장성과 유지보수성을 극대화했습니다.

### 주요 특징
- 📱 **반응형 웹 설정 포털**: 스마트폰으로 쉬운 초기 설정
- 🔧 **모듈화된 구조**: 각 기능별로 독립적인 모듈
- 💾 **영구 저장소**: NVS와 SPIFFS를 활용한 설정 및 파일 저장
- 🔄 **자동 복구**: WiFi 연결 실패 시 자동 설정 모드 전환
- 🎨 **아름다운 UI**: 직관적이고 사용하기 쉬운 웹 인터페이스

## ✨ 주요 기능

### 1. 🔧 NVS 설정 관리
- WiFi 자격 증명 안전 저장
- OpenWeatherMap API 키 관리
- 도시명 및 기타 설정 저장
- 첫 부팅 플래그 관리

### 2. 📁 SPIFFS 파일 시스템
- 웹 파일 저장 및 서빙
- 로그 파일 관리
- 설정 파일 백업/복구

### 3. 📶 WiFi 관리
- **STA 모드**: 저장된 WiFi 네트워크 자동 연결
- **AP 모드**: 설정용 핫스팟 생성 (SSID: ESP32_Weather_Config)
- 연결 상태 모니터링 및 자동 재연결
- WiFi 네트워크 스캔 기능

### 4. 🌐 웹 설정 포털
- 반응형 HTML5 웹 인터페이스
- 실시간 WiFi 네트워크 스캔
- 신호 강도 및 보안 상태 표시
- 폼 유효성 검사 및 오류 처리
- 설정 완료 후 자동 재시작

## 📁 프로젝트 구조

```
ESP32Wx/
├── 📄 CMakeLists.txt          # ESP-IDF 프로젝트 루트 설정
├── 📄 partitions.csv          # 메모리 파티션 테이블 (16MB Flash)
├── 📄 README.md              # 프로젝트 문서 (이 파일)
├── � sdkconfig.esp32s3       # ESP32-S3 SDK 설정
├── �📂 docs/                   # 문서 및 이미지
│   └── 📂 images/
│       ├── 📂 hardware/       # 하드웨어 연결 이미지
│       └── 📂 ui/            # UI 스크린샷
├── 📂 data/                   # 웹 파일 저장소
│   └── 📄 index.html         # 설정 포털 웹페이지
├── 📂 components/             # ESP-IDF 커스텀 컴포넌트
│   └── 📂 lvgl/              # LVGL 8.3.11 그래픽 라이브러리
│       ├── 📄 CMakeLists.txt # LVGL 컴포넌트 빌드 설정
│       ├── 📄 lv_conf.h      # LVGL 설정 (권장: 16bpp, FreeRTOS OSAL)
│       └── 📂 lvgl/          # LVGL 소스 코드 (v8.3.11)
└── 📂 main/                   # 메인 애플리케이션 (ESP-IDF 표준)
    ├── 📄 main.c             # 메인 애플리케이션 엔트리 포인트
    ├── 📄 CMakeLists.txt     # 메인 컴포넌트 빌드 설정
    ├── 📂 include/            # 헤더 파일 디렉토리 (ESP-IDF 표준)
    │   ├── 📂 common/
    │   │   ├── 📄 config.h       # 전역 설정 상수
    │   │   └── 📄 types.h        # 공통 데이터 구조체
    │   ├── 📂 network/
    │   │   ├── 📄 wifi_manager.h    # WiFi 관리 인터페이스
    │   │   └── 📄 config_portal.h   # 웹 포털 인터페이스
    │   ├── 📂 storage/
    │   │   ├── 📄 nvs_manager.h     # NVS 저장소 인터페이스
    │   │   └── 📄 spiffs_manager.h  # SPIFFS 파일시스템 인터페이스
    │   ├── 📂 ui/
    │   │   ├── 📄 lvgl_driver.h     # LVGL 디스플레이 드라이버
    │   │   └── � ui_app.h         # UI 애플리케이션 인터페이스
    │   ├── 📂 weather/
    │   │   ├── 📄 weather_interface.h    # 날씨 API 인터페이스
    │   │   └── 📄 openweathermap_api.h   # OpenWeatherMap API
    │   └── 📂 ota/
    │       └── 📄 ota_manager.h        # OTA 업데이트 인터페이스
    ├── 📂 network/
    │   ├── 📄 wifi_manager.c    # WiFi 연결 및 관리
    │   └── 📄 config_portal.c   # HTTP 웹 서버 및 API
    ├── 📂 storage/
    │   ├── 📄 nvs_manager.c     # NVS 읽기/쓰기 구현
    │   └── 📄 spiffs_manager.c  # SPIFFS 파일 관리
    ├── 📂 ui/                # LVGL UI 구현
    │   ├── 📄 lvgl_driver.c  # 디스플레이 드라이버 (ST7796)
    │   └── 📄 ui_app.c       # UI 애플리케이션
    ├── 📂 weather/           # 날씨 API 처리
    │   ├── 📄 weather_interface.c    # 날씨 데이터 처리
    │   └── 📄 openweathermap_api.c   # OpenWeatherMap API 구현
    └── 📂 ota/              # OTA 업데이트 관리
        └── 📄 ota_manager.c     # OTA 업데이트 구현
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



핀 매핑은 `main/include/common/constants.h`에서 수정할 수 있습니다.

### 📋 **최소 시스템 요구사항**
- **Flash 메모리**: 최소 8MB (권장 16MB)
- **RAM**: 최소 512KB (PSRAM 포함)
- **WiFi 안테나**: 내장 또는 외장

## 💻 소프트웨어 요구사항

- **ESP-IDF Framework** >= 5.0 (권장 5.1+)
- **VS Code** + **ESP-IDF Extension**
- **Python** >= 3.8 (ESP-IDF용)
- **Git** (소스 코드 관리용)

### 🔧 **ESP-IDF 컴포넌트 의존성**
- `nvs_flash` - 설정 저장 (WiFi 자격증명, API 키)
- `esp_wifi` - WiFi STA/AP 기능
- `esp_netif` - 네트워크 인터페이스 관리
- `esp_http_server` - 웹 설정 포털
- `esp_http_client` - HTTP API 호출
- `esp_tls` - HTTPS 보안 통신
- `spiffs` - 웹 파일 시스템
- `json` - JSON 파싱 (cJSON)
- `esp_lcd` - 디스플레이 드라이버
- `app_update` - OTA 업데이트
- `mbedtls` - TLS/SSL 암호화

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

### 4. LVGL 컴포넌트 확인
LVGL v8.3.11은 `components/lvgl/` 아래에 있고, CMake로 자동 빌드됩니다.

### 6. LVGL 포팅 요약 (v8.3.11)

- LVGL 코어 초기화 후 esp_timer로 5ms tick, 10ms 주기 핸들러 태스크(Core 1)
- 디스플레이: esp_lcd ST7796 패널 사용 (i80 8bit 예정), 백라이트/리셋 GPIO 제어
- 터치: FT6236 I2C 단일 터치 좌표 읽기
- 스레드 안전: 다른 태스크에서 LVGL API 사용 시 `lvgl_lock(timeout_ms)`/`lvgl_unlock()` 호출

UI 예제는 온도/습도/상태 레이블 3개를 생성하고, 날씨 태스크에서 값을 갱신합니다.

### 5. 파티션 설정
- **16MB Flash**: `partitions.csv`에서 자동 설정
- **SPIFFS**: 웹 파일용 2MB 할당
- **OTA**: A/B 파티션으로 무선 업데이트 지원

## 📱 사용 방법

### 첫 설정 (최초 1회)

1. **디바이스 부팅**
   - ESP32를 전원에 연결
   - 첫 부팅 시 자동으로 AP 모드로 전환

2. **WiFi 연결**
   - 스마트폰/PC에서 **"ESP32_Weather_Config"** 네트워크 연결
   - 비밀번호: `12345678`

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
   - 디바이스가 자동으로 재시작
   - 설정된 WiFi로 자동 연결

### 정상 운영

- 디바이스가 설정된 WiFi에 자동 연결
- 주기적으로 날씨 데이터 업데이트 (향후 구현)
- WiFi 연결이 실패하면 자동으로 설정 모드로 전환

## 🌐 설정 포털 사용법

### 메인 기능

1. **WiFi 스캔**
   - "WiFi 스캔" 버튼 클릭
   - 주변 네트워크 목록 표시
   - 신호 강도 및 보안 상태 확인
   - 네트워크 선택 시 자동으로 SSID 입력

2. **수동 입력**
   - 숨겨진 네트워크의 경우 직접 SSID 입력
   - 비밀번호는 항상 수동 입력 필요

3. **API 설정**
   - [OpenWeatherMap](https://openweathermap.org/api)에서 무료 API 키 발급
   - API 키를 정확히 입력 (대소문자 구분)

4. **도시명 입력**
   - 영문 도시명 사용 (예: Seoul, Busan, Incheon)
   - 국가 코드 포함 가능 (예: Seoul,KR)

### API 엔드포인트

| 엔드포인트 | 메소드 | 설명 |
|-----------|--------|------|
| `/` | GET | 메인 설정 페이지 |
| `/api/scan` | GET | WiFi 네트워크 스캔 |
| `/api/config` | POST | 설정 저장 |
| `/api/status` | GET | 현재 상태 조회 |

### API 응답 예제

#### WiFi 스캔 (`/api/scan`)
```json
{
  "success": true,
  "networks": [
    {
      "ssid": "MyWiFi",
      "rssi": -45,
      "authmode": 3
    }
  ]
}
```

#### 설정 저장 (`/api/config`)
```json
{
  "success": true,
  "message": "Configuration saved"
}
```

## 📊 메모리 파티션

| 파티션 | 타입 | 크기 | 용도 |
|--------|------|------|------|
| nvs | data | 24KB | 설정 저장소 |
| phy_init | data | 4KB | RF 보정 데이터 |
| factory | app | 2MB | 메인 애플리케이션 |
| spiffs | data | 2MB | 웹 파일 및 로그 |

## 🔍 디버깅

### 시리얼 모니터 출력
```powershell
# 실시간 로그 확인 (ESP-IDF)
idf.py -p COM3 monitor
```

### 주요 로그 태그
- `MAIN` - 메인 애플리케이션
- `WIFI_MANAGER` - WiFi 연결 상태
- `CONFIG_PORTAL` - 웹 서버 동작
- `NVS_MANAGER` - 설정 저장/로드
- `SPIFFS_MGR` - 파일 시스템

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

### Phase 1: 날씨 API 연동
- [ ] HTTP 클라이언트 모듈 구현
- [ ] OpenWeatherMap API 연동
- [ ] JSON 응답 파싱 및 데이터 구조화
- [ ] 오류 처리 및 재시도 로직

### Phase 2: 디스플레이 구현
- [ ] LCD/OLED 디스플레이 드라이버
- [ ] 날씨 아이콘 및 그래픽 렌더링
- [ ] 다국어 지원 (한글, 영어)
- [ ] 사용자 인터페이스 개선

### Phase 3: 고급 기능
- [ ] 날씨 예보 (5일간)
- [ ] 알림 및 경고 시스템
- [ ] 데이터 로깅 및 히스토리
- [ ] OTA (Over-The-Air) 업데이트
- [ ] MQTT 연동
- [ ] 센서 데이터 수집 (온도, 습도)

### Phase 4: 확장성
- [ ] 다중 도시 지원
- [ ] 커스터마이저블 위젯
- [ ] 모바일 앱 연동
- [ ] 클라우드 백엔드 연결

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