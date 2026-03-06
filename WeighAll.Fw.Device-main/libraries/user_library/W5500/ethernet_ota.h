#ifndef ETHERNET_OTA_H
#define ETHERNET_OTA_H

#include <Arduino.h>
#include <Ethernet.h>   // dùng EthernetClient
#include <Update.h>
#include "driver_config.h"

// Hàm chính để gọi OTA update 1 file .bin
// - host: IP hoặc domain server (ví dụ "192.168.1.123")
// - port: port HTTP server (ví dụ 80)
// - path: đường dẫn file firmware (ví dụ "/firmware.bin")
// - expectedMD5: chuỗi 32 ký tự hex nếu muốn kiểm MD5 (tùy chọn, có thể nullptr)
//
// Hàm sẽ:
//  1. Kết nối đến server
//  2. Gửi HTTP GET
//  3. Đọc header, lấy Content-Length hoặc Transfer-Encoding
//  4. Stream nội dung về flash qua Update.write()
//  5. Gọi Update.end(), rồi ESP.restart() nếu thành công
//
// Trả về true nếu update thành công (thực tế sẽ restart),
// false nếu lỗi.
bool http_ota_update_1file(const char* host,
                           uint16_t port,
                           const char* path,
                           const char* expectedMD5 = nullptr);

#endif // ETHERNET_OTA_H
