#pragma once

#include <Arduino.h>

// ══════════════════════════════════════════════════
//  PIN CONFIGURATION — sửa lại nếu board nối dây khác
//  Các pin mặc định đã tránh xung đột với LED Matrix
//  (LED dùng: LAT=7, CLK=15, B2=16, OE=21, ...)
// ══════════════════════════════════════════════════

#ifndef BARRIER_OPEN_PIN
  #define BARRIER_OPEN_PIN   6   // Chân mở barie
#endif
#ifndef BARRIER_CLOSE_PIN
  #define BARRIER_CLOSE_PIN  8   // Chân đóng barie (gốc WeighAll: 7, đổi vì LED LAT)
#endif

#ifndef TRAFFIC_RED_PIN
  #define TRAFFIC_RED_PIN    47  // Chân đèn đỏ (gốc WeighAll: 15, đổi vì LED CLK)
#endif
#ifndef TRAFFIC_YELLOW_PIN
  #define TRAFFIC_YELLOW_PIN 1   // Chân đèn vàng
#endif
#ifndef TRAFFIC_GREEN_PIN
  #define TRAFFIC_GREEN_PIN  42  // Chân đèn xanh (gốc WeighAll: 16, đổi vì LED B2)
#endif

#ifndef BEAM_PIN
  #define BEAM_PIN           2   // Chân lưới hồng ngoại (IR beam)
#endif

// ── Enums ──

enum BarrierState {
  BARRIER_PAUSE = 0,
  BARRIER_OPEN  = 1,
  BARRIER_CLOSE = 2
};

enum TrafficLightState {
  TRAFFIC_OFF       = 0,
  TRAFFIC_GREEN     = 1,
  TRAFFIC_RED       = 2,
  TRAFFIC_YELLOW    = 3,
  TRAFFIC_RED_FLASH = 4
};

// ── API ──

void deviceControlInit();

void barrierControl(BarrierState state);
BarrierState getBarrierState();

void trafficLightControl(TrafficLightState state);
TrafficLightState getTrafficLightState();

// Gọi trong loop() để xử lý chế độ nhấp nháy đèn đỏ
void trafficLightLoop();

int readBeam();
