# Tong quan nhanh du an `web_esp_pio` (cap nhat moi)

Tai lieu nay tom tat dung trang thai code hien tai trong `src/` va `includes/` de doc nhanh.

## 1) Muc tieu he thong

Firmware ESP32-S3 cung cap:
- Web UI tu LittleFS (`index.html`, `style.css`).
- Quan ly mang: WiFi AP, WiFi STA, Ethernet W5500.
- Dieu khien LED matrix HUB75 (6 panel, layout 3x2).
- Dieu khien relay barrier theo kieu xung nhan-nha.
- Doc 3 kenh beam va 3 nut vat ly tren API status.
- In qua Serial2.
- Speech module (dang tat mac dinh theo build flag).
- OTA update qua URL.

## 2) Cap nhat quan trong da lam

### 2.1 Pin map tap trung
- Da them `includes/pin_config.h` lam noi quan ly pin trung tam.
- Mapping hien tai:
  - Nut vat ly: OPEN `GPIO41`, CLOSE `GPIO42`, STOP `GPIO2`.
  - Relay: OPEN `GPIO38`, CLOSE `GPIO5`, STOP `GPIO1`.
  - Beam: PWM1 `GPIO39`, PWM2 `GPIO40`, A0 `GPIO4`.

### 2.2 Barrier da la 3 relay xung nhan-nha
- `src/device_control.cpp` da doi logic barrier sang pulse:
  - OPEN -> xung relay OPEN.
  - CLOSE -> xung relay CLOSE.
  - STOP -> xung relay STOP.
- Thoi gian xung dang dat `BARRIER_PULSE_MS = 180` ms.
- API barrier nhan `open`, `close`, `stop` (van chap nhan `pause` de tuong thich cu).

### 2.3 Nut vat ly da co loop xu ly
- Da co `deviceControlLoop()` voi debounce (`BTN_DEBOUNCE_MS = 80` ms).
- Edge trigger nhan xuong (HIGH -> LOW), khong can giu nut.
- Da duoc goi trong `loop()` o `src/main.cpp`.

### 2.4 Device status da co du 3 beam + 3 nut
- `GET /api/device/status` tra ve them:
  - `beam_pwm1`, `beam_pwm2`, `beam_a0`
  - `btn_open`, `btn_close`, `btn_stop`
  - `button_open`, `button_close`, `button_stop` (alias cho UI)
  - `btn_open_pressed`, `btn_close_pressed`, `btn_stop_pressed`

### 2.5 WiFi config va static IP
- Co them `GET /api/wifi/config` de web nap lai form SSID/static IP.
- Van giu hardcode test nhung chi la fallback:
  - Neu khong co SSID luu trong file thi moi dung `Phuc An / 88889999`.
  - Neu da co config luu thi khong bi hardcode ghi de.

### 2.6 LED config da luu/nap lai
- `POST /api/led/config` luu vao `/led_config.json`.
- `GET /api/led/config` nap lai cho web.
- Da ho tro `lineSpacing` tuy chinh tu web (`-1` = auto).

### 2.7 Giam log 404 khong can thiet
- Da them route `/chat` tra `204`.
- `handleNotFound()` chi thu serve static fallback khi request la GET va co dang file static.

### 2.8 Speech tat mac dinh, tranh xung dot LED
- `platformio.ini` co `-DDISABLE_SPEECH`.
- `includes/speech.h`: speech chi hoat dong khi `ENABLE_SPEECH` va khong co `DISABLE_SPEECH`.

### 2.9 Tranh xung dot printer voi relay
- `PRINTER_RX_PIN = -1` trong `includes/printer.h` de tranh dung chung `GPIO5` voi relay CLOSE.

## 3) Luong khoi dong va loop

Trong `setup()` (`src/main.cpp`):
1. Khoi tao Serial + LittleFS.
2. Khoi tao LED, hien "BOOT".
3. Khoi tao network.
4. Khoi tao `deviceControlInit()`.
5. Khoi tao printer + speech (speech dang stub neu disable).
6. Khoi tao web server.
7. Hien "READY".

Trong `loop()`:
- `webServerHandleClient()`
- `deviceControlLoop()`
- `trafficLightLoop()`

## 4) API chinh hien tai

- `GET /api/network/status`
- `GET /api/wifi/scan`
- `GET /api/wifi/config`
- `POST /api/wifi/connect`
- `GET /api/lan`
- `POST /api/lan`
- `GET /api/led/config`
- `POST /api/led/config`
- `POST /api/barrier/control`
- `POST /api/traffic-light/control`
- `POST /api/speech/play`
- `POST /api/printer/print`
- `POST /api/ota/update`
- `GET /api/device/status`

## 5) Ghi chu bao tri nhanh

- Barrier panel tren web la 1 panel, ben trong co 3 nut tuong ung 3 relay (open/close/stop).
- Neu can tang giam do rong xung relay, sua `BARRIER_PULSE_MS` trong `src/device_control.cpp`.
- Neu can doi wiring, uu tien sua trong `includes/pin_config.h`.
- Neu upload lai LittleFS, cac file runtime (`/wifi_config.json`, `/led_config.json`) co the bi ghi de.

## 6) Thu tu doc code nhanh

1. `src/main.cpp`
2. `includes/pin_config.h`
3. `src/device_control.cpp`
4. `src/web_server.cpp`
5. `src/network.cpp`
6. `src/led_matrix.cpp`
