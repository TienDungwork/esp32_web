#pragma once

// ══════════════════════════════════════════════════
//  Speech Module — Phát MP3 từ SD card qua I2S
//  Bật bằng cách thêm -DENABLE_SPEECH vào build_flags
//  và thêm lib "schreibfaul1/ESP32-audioI2S" vào lib_deps
// ══════════════════════════════════════════════════

#ifdef ENABLE_SPEECH

#include <Arduino.h>

// ── Pin SD card (SPI) ──
#ifndef SD_CS_PIN
  #define SD_CS_PIN   39
#endif
#ifndef SD_MOSI_PIN
  #define SD_MOSI_PIN 20
#endif
#ifndef SD_MISO_PIN
  #define SD_MISO_PIN 37
#endif
#ifndef SD_SCK_PIN
  #define SD_SCK_PIN  36
#endif

// ── Pin I2S (MAX98357 amplifier) ──
#ifndef I2S_DOUT_PIN
  #define I2S_DOUT_PIN 7    // ⚠️ Xung đột LED LAT — đổi nếu dùng LED
#endif
#ifndef I2S_BCLK_PIN
  #define I2S_BCLK_PIN 15   // ⚠️ Xung đột LED CLK — đổi nếu dùng LED
#endif
#ifndef I2S_LRC_PIN
  #define I2S_LRC_PIN  16   // ⚠️ Xung đột LED B2  — đổi nếu dùng LED
#endif

// ── API ──
void speechInit();
void playTracks(String cmd);

#else

// Stubs khi ENABLE_SPEECH chưa được bật
#include <Arduino.h>
inline void speechInit() {}
inline void playTracks(String) {}

#endif  // ENABLE_SPEECH
