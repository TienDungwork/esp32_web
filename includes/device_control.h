#pragma once

#include <Arduino.h>
#include "pin_config.h"

// ══════════════════════════════════════════════════
//  Barrier aliases from centralized pin map
// ══════════════════════════════════════════════════

#ifndef BARRIER_OPEN_PIN
  #define BARRIER_OPEN_PIN   RELAY_OPEN_PIN
#endif
#ifndef BARRIER_CLOSE_PIN
  #define BARRIER_CLOSE_PIN  RELAY_CLOSE_PIN
#endif

#ifndef BEAM_PIN
  #define BEAM_PIN           BEAM_PWM1_IN_PIN
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
