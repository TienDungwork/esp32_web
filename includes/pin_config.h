#pragma once

#include <Arduino.h>

// =========================================================
// Centralized pin mapping for hardware I/O
// Edit this file when wiring changes.
// =========================================================

// ---------------------------------------------------------
// Physical buttons (active LOW, use INPUT_PULLUP)
// ---------------------------------------------------------
#ifndef BTN_OPEN_PIN
  #define BTN_OPEN_PIN 41
#endif

#ifndef BTN_CLOSE_PIN
  #define BTN_CLOSE_PIN 42
#endif

#ifndef BTN_STOP_PIN
  #define BTN_STOP_PIN 2
#endif

// ---------------------------------------------------------
// Relay outputs
// ---------------------------------------------------------
#ifndef RELAY_OPEN_PIN
  #define RELAY_OPEN_PIN 38  // D1
#endif

#ifndef RELAY_CLOSE_PIN
  #define RELAY_CLOSE_PIN 5   // A3
#endif

#ifndef RELAY_STOP_PIN
  #define RELAY_STOP_PIN 1    // Reserved stop relay
#endif

// ---------------------------------------------------------
// Beam sensor wiring
// ---------------------------------------------------------
#ifndef BEAM_PWM1_IN_PIN
  #define BEAM_PWM1_IN_PIN 39   // PWM1 signal into ESP
#endif

#ifndef BEAM_PWM2_OUT_PIN
  #define BEAM_PWM2_OUT_PIN 40  // PWM2 signal into ESP
#endif

#ifndef BEAM_A0_PIN
  #define BEAM_A0_PIN 4         // A0 signal into ESP
#endif

// ---------------------------------------------------------
// Traffic light shares the same 3 relay IO as barrier
// RED    -> RELAY_OPEN_PIN
// YELLOW -> RELAY_STOP_PIN
// GREEN  -> RELAY_CLOSE_PIN
// ---------------------------------------------------------
#ifndef TRAFFIC_RED_PIN
  #define TRAFFIC_RED_PIN RELAY_OPEN_PIN
#endif

#ifndef TRAFFIC_YELLOW_PIN
  #define TRAFFIC_YELLOW_PIN RELAY_STOP_PIN
#endif

#ifndef TRAFFIC_GREEN_PIN
  #define TRAFFIC_GREEN_PIN RELAY_CLOSE_PIN
#endif
