# Tong quan nhanh du an `web_esp_pio` (src + includes)

Tai lieu nay duoc viet de doc nhanh va nam ngay cau truc code trong `src/` va `includes/`.

## 1) Muc tieu he thong

Firmware ESP32-S3 cung cap:
- Web UI tu LittleFS (`index.html`, `style.css`, ...).
- Quan ly ket noi mang: WiFi AP, WiFi STA, Ethernet W5500.
- Dieu khien LED matrix HUB75 (6 panel, layout 3x2).
- Dieu khien thiet bi ngoai vi: barie, den giao thong, IR beam.
- In qua Serial2 (printer ESC/POS-like).
- Phat am thanh MP3 tu SD card qua I2S (tuy chon compile).
- OTA update qua URL.

## 2) Kien truc module

Cap file chinh:
- `src/main.cpp`: Entry point, khoi tao tat ca module.
- `src/web_server.cpp` + `includes/web_server.h`: HTTP server + API handler.
- `src/network.cpp` + `includes/network.h`: Quan ly AP/STA/Ethernet + config file mang.
- `src/led_matrix.cpp` + `includes/led_matrix.h`: Khoi tao HUB75 va render text.
- `src/device_control.cpp` + `includes/device_control.h`: GPIO cho barie/den/beam.
- `src/printer.cpp` + `includes/printer.h`: Lenh in qua UART2.
- `src/speech.cpp` + `includes/speech.h`: SD + I2S audio (chi khi `ENABLE_SPEECH`).

Phu thuoc tong quan:
- `main` -> `network`, `led_matrix`, `device_control`, `printer`, `speech`, `web_server`
- `web_server` -> goi ham o cac module con de xu ly API
- `network` + `LittleFS` -> doc/ghi file config JSON

## 3) Luong khoi dong (`setup`) va vong lap (`loop`)

Trong `setup()` (`src/main.cpp`):
1. Tat brownout detect.
2. Mo serial debug 115200.
3. Mount LittleFS va log file co san.
4. Khoi tao LED matrix, hien "BOOT".
5. Khoi tao mang (`networkInit`).
6. Khoi tao GPIO device control.
7. Khoi tao printer (Serial2).
8. Khoi tao speech (stub neu khong bat macro).
9. Khoi tao web server va dang ky endpoint.
10. Hien "READY" tren LED.

Trong `loop()`:
- `webServerHandleClient()` de phuc vu HTTP request.
- `trafficLightLoop()` de xu ly che do nhap nhay den do (`TRAFFIC_RED_FLASH`).

## 4) Tom tat tung module

### 4.1 `device_control`

Chuc nang:
- Dieu khien barie qua 2 chan: `BARRIER_OPEN_PIN`, `BARRIER_CLOSE_PIN`.
- Dieu khien den giao thong 3 mau + che do `red_flash`.
- Doc trang thai IR beam (`readBeam`).

State noi bo:
- `currentBarrierState`
- `currentTrafficState`
- Bien timer blink do (`trafficFlashLastMs`) voi chu ky 500ms.

Ghi chu:
- Default pin duoc doi de tranh xung dot voi LED matrix pin.
- `BEAM_PIN` dung `INPUT_PULLUP`.

### 4.2 `led_matrix`

Chuc nang:
- Cau hinh HUB75 I2S DMA.
- Tao bang mau 565 co ban (`LED_COLOR_*`).
- Hien thi text giua (`ledMatrixShowCenterText`).
- Hien thi toi da 5 dong text (`ledMatrixShowMultiLine`).

Diem ky thuat:
- Dung `VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>` cho layout 3 hang x 2 cot.
- `PANEL_ROTATION` dang dat = 2 (xoay 180 do).
- `ledMatrixParseColor` nhan hex `#RRGGBB` -> RGB565.
- `boardCount` duoc nhan tu API nhung hien tai chua duoc dung thuc te (`(void)boardCount`).
- Multi-line hien dang can trai (co margin), khong can giua.

### 4.3 `network`

Chuc nang:
- Load/save WiFi config (`/wifi_config.json`).
- Load/save LAN config (`/lan_config.json`).
- Khoi tao AP.
- Thu Ethernet W5500.
- Thu WiFi STA tu config.

Hanh vi hien tai (`networkInit`):
1. `startAp()` luon bat AP (`WIFI_AP_STA`).
2. `startEthernet()` thu ket noi W5500.
3. `startWifiStaFromConfig()` thu vao WiFi da luu.

Luu y quan trong:
- Co doan test hardcode SSID/password trong `loadWiFiConfig()`:
  - `wifi_ssid = "Phuc An"`
  - `wifi_password = "88889999"`
- Neu ca Ethernet va WiFi deu ket noi thanh cong, `currentNetworkMode` se bi mode cuoi cung ghi de (thuong la `wifi_sta` vi goi sau).

### 4.4 `printer`

Chuc nang:
- Khoi tao `Serial2` voi pin TX/RX macro.
- Cac lenh text co ban: align, bold, underline, size, newline.
- In barcode 1D.
- In QR code.
- In bitmap (dot-line style).
- Ham demo xu ly tieng Viet (`printer_print_vietnamese`) dang mapping rat don gian.

Luu y:
- Cac command code mang tinh tuy chinh theo dong may in dang dung.
- Xu ly UTF-8 tieng Viet chua day du (chi demo).

### 4.5 `speech`

Compile-time:
- Chi build logic that khi co `-DENABLE_SPEECH`.
- Neu khong bat macro, `speechInit()` va `playTracks()` la stub (khong lam gi).

Chuc nang khi bat:
- Khoi tao SD card qua HSPI.
- Cau hinh I2S pinout + volume.
- `playTracks("1,2,3")` hoac `"1-2-3"` -> lan luot phat `/1.mp3`, `/2.mp3`, `/3.mp3`.

Luu y:
- `playTracks` la blocking (doi phat xong tung file).
- Pin I2S mac dinh dang trung voi pin LED (`LAT/CLK/B2`), can doi neu dung dong thoi LED + speech.

### 4.6 `web_server`

Vai tro:
- Tao `WebServer server(80)`.
- Serve static file tu LittleFS.
- Khai bao toan bo API cho web app.

Dang ky endpoint:
- `GET /api/network/status`
- `GET /api/wifi/scan`
- `POST /api/wifi/connect`
- `GET /api/lan`
- `POST /api/lan`
- `POST /api/led/config`
- `POST /api/barrier/control`
- `POST /api/traffic-light/control`
- `POST /api/speech/play`
- `POST /api/printer/print`
- `POST /api/ota/update`
- `GET /api/device/status`

Pattern xu ly chung:
- Kiem tra body JSON (`server.arg("plain")`).
- Parse bang `ArduinoJson`.
- Validate tham so.
- Goi ham module tuong ung.
- Tra JSON response.

Luu y:
- `handleSpeechPlay()` tra response truoc roi moi goi `playTracks` (vi blocking).
- `handleWifiConnect()` cung co khoang cho doi ket noi toi da ~15s trong handler.
- CORS da bat `Access-Control-Allow-Origin: *` cho cac API.

## 5) Luong du lieu thuc te (from request -> hardware)

1. Browser goi API vao ESP32 (`web_server`).
2. `web_server` parse JSON va chuyen lenh:
- den `device_control` de bat/tat barie-den.
- den `led_matrix` de render noi dung.
- den `printer` de in.
- den `speech` de phat file MP3.
- den `network` de luu/cap nhat config mang.
3. Ket qua trang thai duoc tra lai qua JSON (`success`, `state`, `ip`, ...).

## 6) Cac file config runtime quan trong

File tren LittleFS duoc code dung:
- `/wifi_config.json`
- `/lan_config.json`
- `/index.html`
- `/style.css`

Neu thieu static file, `main.cpp` se log can upload lai LittleFS (`pio run -t uploadfs`).

## 7) Danh sach rui ro / can nho khi bao tri

- Co thong tin WiFi test hardcode trong `network.cpp` (nen bo khi dua production).
- Xung dot pin giua `speech` va `led_matrix` neu khong doi pin.
- Mot so API co do tre do blocking (speech play, wifi connect).
- `boardCount` o LED config dang chua duoc su dung thuc su.
- Xu ly tieng Viet cua printer chua hoan chinh.

## 8) Goi y doc code sieu nhanh lan sau

Thu tu doc de nam nhanh:
1. `src/main.cpp`
2. `src/web_server.cpp`
3. `src/network.cpp`
4. `src/device_control.cpp`
5. `src/led_matrix.cpp`
6. `src/printer.cpp`
7. `src/speech.cpp`

Neu can sua theo chuc nang:
- Sua API: vao `src/web_server.cpp`
- Sua pin/IO: vao `includes/device_control.h`, `includes/led_matrix.h`, `includes/printer.h`, `includes/speech.h`
- Sua logic mang: vao `src/network.cpp`
- Sua layout hien thi LED: vao `src/led_matrix.cpp`
