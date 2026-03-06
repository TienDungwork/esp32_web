#ifndef __DRIVER_CONFIG_H__
#define __DRIVER_CONFIG_H__


#include <Arduino.h>
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include "Ethernet.h"
#include "utility/w5100.h"
#include <SPI.h>
#include <HTTPClient.h>
#include "Wire.h"
#include <SD.h>
#include <FS.h>
#include <WiFi.h>
#include "Audio.h"
//#include "Material_16Bit_222x480px.h"


#include "pin_config.h"  // dùng cho esp32_w5500


#include "W5500/ethernet_tcp.h"
#include "W5500/ethernet_ota.h"
#include "JSON/jsmn.h"
#include "API/api.h"

//#if DEVICE_HAS_DISPLAY
#include "UART/uart.h"
#include "WIFI/wifi_ap.h"
#include "TFT/tft_display.h"
#include "TFT/touch.h"
//#endif

#include "GPIO/gpio.h"
#include "TIMER/soft_timer.h"
#include "PRINTER/printer.h"
#include "SPEECH/speech.h"

// ========== CẤU HÌNH SERIAL DEBUG ==========
//#define USE_SERIAL0   0  // 1 = dùng Serial0 (UART0), 0 = dùng Serial (USB CDC)

#if USE_SERIAL0
  #define DEBUG_PORT Serial0
#else
  #define DEBUG_PORT Serial
#endif

// Gọi trong setup() để khởi tạo debug UART
inline void InitDebugSerial(uint32_t baud = 115200) {
#if USE_SERIAL0
  DEBUG_PORT.begin(baud);
#else
  DEBUG_PORT.begin(baud);
  while (!DEBUG_PORT);  // USB CDC cần đợi host mở cổng
#endif
}

// ✅ In không xuống dòng (giống Serial.print)
template <typename T>
inline void PrintDebug(const T& val) {
  DEBUG_PORT.print(val);
}

// ✅ In có xuống dòng (giống Serial.println)
template <typename T>
inline void PrintDebugLn(const T& val) {
  DEBUG_PORT.println(val);
}

// ✅ In xuống dòng rỗng
inline void PrintDebugLn() {
  DEBUG_PORT.println();
}

// ✅ PrintDebugLn với tm struct
inline void PrintDebugLn(struct tm *info, const char *format = nullptr) {
  DEBUG_PORT.println(info, format);
}

// ✅ PrintDebugf giống Serial.printf
inline void PrintDebugf(const char* fmt, ...) {
  char loc_buf[64];     // buffer cục bộ
  char* temp = loc_buf; // con trỏ tới buffer
  va_list args;
  va_start(args, fmt);

  // Copy args để đo độ dài
  va_list args_copy;
  va_copy(args_copy, args);
  int len = vsnprintf(temp, sizeof(loc_buf), fmt, args_copy);
  va_end(args_copy);

  if (len < 0) {
    va_end(args);
    return;
  }

  // Nếu chuỗi dài hơn buffer cục bộ thì cấp phát động
  if (len >= (int)sizeof(loc_buf)) {
    temp = (char*)malloc(len + 1);
    if (temp == NULL) {
      va_end(args);
      return;
    }
    vsnprintf(temp, len + 1, fmt, args);
  }

  va_end(args);
  DEBUG_PORT.write((const uint8_t*)temp, len);

  if (temp != loc_buf) {
    free(temp);
  }
}

#endif // __DRIVER_CONFIG_H__