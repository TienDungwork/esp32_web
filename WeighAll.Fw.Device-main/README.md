
# ESP-W5500 Project

## 📌 Mô tả

Đây là dự án sử dụng **ESP32-S3** với module **W5500 Ethernet** để kết nối với server thông qua giao thức **T-Connect Pro**. Dự án được tổ chức theo chuẩn PlatformIO với cấu trúc rõ ràng và có thể mở rộng, dễ dàng phát triển thêm các tính năng khác.

Toàn bộ logic chính của chương trình nằm trong thư mục `myProject/esp_w5500`. Các chức năng được config thành từng thư viện riêng trong `user_library` để dễ bảo trì và tái sử dụng.

---

## ⚙️ Cấu hình PlatformIO (`platformio.ini`)

```ini
[env]
platform = espressif32 @6.5.0
board = esp32s3_flash_16MB
framework = arduino
upload_port = COM22
monitor_port = COM22
monitor_speed = 115200
upload_speed = 921600
board_upload.flash_size = 16MB
board_build.memory_type = qio_opi
board_build.partitions = default_16MB.csv

build_flags = 
    -Wall
    -Wextra
    -D CORE_DEBUG_LEVEL=1
    -D BOARD_HAS_PSRAM
    -D ARDUINO_USB_MODE=1
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -D ARDUINO_RUNNING_CORE=1
    -D ARDUINO_EVENT_RUNNING_CORE=1

[platformio]
boards_dir = ./boards
lib_dir = ./libraries
src_dir = myProject/${platformio.default_envs}
default_envs = esp_w5500
```

---

## 📁 Cấu trúc thư mục

```
.
├── libraries/                  # Các thư viện mở rộng sử dụng
│   ├── Arduino_DriveBus
│   ├── Arduino_GFX
│   ├── Ethernet
│   ├── FastLED
│   └── RadioLib
│
├── user_library/               # Thư viện người dùng tự xây dựng
│   ├── API/                    # Xử lý API server
│   ├── GPIO/                   # Điều khiển GPIO
│   ├── JSON/                   # Xử lý chuỗi JSON
│   ├── TFT/                    # Hiển thị TFT
│   ├── TIMER/                  # Bộ đếm thời gian
│   ├── UART/                   # Giao tiếp UART
│   └── W5500/                  # Giao tiếp Ethernet qua W5500
│       ├── driver_config.h
│       ├── pin_config.h
│       └── Material_16Bit_222x480px.h
│
├── myProject/
│   └── esp_w5500/
│       └── esp_w5500.ino       # File chính của dự án
│
└── platformio.ini              # File cấu hình PlatformIO
```

---

## 🧩 Các thư viện sử dụng

- **Arduino_DriveBus** `v1.1.12`
- **Arduino_GFX** `v1.4.6`
- **Ethernet** `v2.0.0`
- **FastLED** `v3.6.0`
- **RadioLib** `v7.1.2`

---

## 🚀 Hướng dẫn sử dụng

1. Cài đặt PlatformIO trong VS Code.
2. Mở thư mục dự án.
3. Kiểm tra `platformio.ini` để chắc chắn đúng cổng `COM` và bo mạch.
4. Biên dịch và nạp chương trình.

---

