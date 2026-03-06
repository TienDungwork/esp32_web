#include "device_control.h"

// ── State tracking ──
static BarrierState       currentBarrierState  = BARRIER_PAUSE;
static TrafficLightState  currentTrafficState  = TRAFFIC_OFF;
static bool               trafficFlashRedOn    = false;
static unsigned long      trafficFlashLastMs   = 0;
static const unsigned long FLASH_INTERVAL_MS   = 500;

// ══════════════════════════════════════════════════
//  Init
// ══════════════════════════════════════════════════
void deviceControlInit() {
  // Barrier pins
  pinMode(BARRIER_OPEN_PIN,  OUTPUT);
  pinMode(BARRIER_CLOSE_PIN, OUTPUT);
  digitalWrite(BARRIER_OPEN_PIN,  LOW);
  digitalWrite(BARRIER_CLOSE_PIN, LOW);

  // Traffic light pins
  pinMode(TRAFFIC_RED_PIN,    OUTPUT);
  pinMode(TRAFFIC_YELLOW_PIN, OUTPUT);
  pinMode(TRAFFIC_GREEN_PIN,  OUTPUT);
  digitalWrite(TRAFFIC_RED_PIN,    LOW);
  digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
  digitalWrite(TRAFFIC_GREEN_PIN,  LOW);

  // IR beam (input pull-up)
  pinMode(BEAM_PIN, INPUT_PULLUP);

  Serial.println("[DevCtrl] GPIO initialized");
  Serial.printf("  Barrier: OPEN=%d CLOSE=%d\n", BARRIER_OPEN_PIN, BARRIER_CLOSE_PIN);
  Serial.printf("  Traffic: R=%d Y=%d G=%d\n", TRAFFIC_RED_PIN, TRAFFIC_YELLOW_PIN, TRAFFIC_GREEN_PIN);
  Serial.printf("  Beam: %d\n", BEAM_PIN);
}

// ══════════════════════════════════════════════════
//  Barrier
// ══════════════════════════════════════════════════
void barrierControl(BarrierState state) {
  currentBarrierState = state;
  switch (state) {
    case BARRIER_PAUSE:
      digitalWrite(BARRIER_OPEN_PIN,  LOW);
      digitalWrite(BARRIER_CLOSE_PIN, LOW);
      Serial.println("[DevCtrl] Barrier: PAUSE");
      break;
    case BARRIER_OPEN:
      digitalWrite(BARRIER_OPEN_PIN,  HIGH);
      digitalWrite(BARRIER_CLOSE_PIN, LOW);
      Serial.println("[DevCtrl] Barrier: OPEN");
      break;
    case BARRIER_CLOSE:
      digitalWrite(BARRIER_OPEN_PIN,  LOW);
      digitalWrite(BARRIER_CLOSE_PIN, HIGH);
      Serial.println("[DevCtrl] Barrier: CLOSE");
      break;
  }
}

BarrierState getBarrierState() {
  return currentBarrierState;
}

// ══════════════════════════════════════════════════
//  Traffic Light
// ══════════════════════════════════════════════════
void trafficLightControl(TrafficLightState state) {
  currentTrafficState = state;

  // Tắt flash timer khi chuyển sang mode khác
  if (state != TRAFFIC_RED_FLASH) {
    trafficFlashRedOn = false;
  }

  switch (state) {
    case TRAFFIC_OFF:
      digitalWrite(TRAFFIC_RED_PIN,    LOW);
      digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
      digitalWrite(TRAFFIC_GREEN_PIN,  LOW);
      Serial.println("[DevCtrl] Traffic: OFF");
      break;
    case TRAFFIC_GREEN:
      digitalWrite(TRAFFIC_RED_PIN,    LOW);
      digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
      digitalWrite(TRAFFIC_GREEN_PIN,  HIGH);
      Serial.println("[DevCtrl] Traffic: GREEN");
      break;
    case TRAFFIC_RED:
      digitalWrite(TRAFFIC_RED_PIN,    HIGH);
      digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
      digitalWrite(TRAFFIC_GREEN_PIN,  LOW);
      Serial.println("[DevCtrl] Traffic: RED");
      break;
    case TRAFFIC_YELLOW:
      digitalWrite(TRAFFIC_RED_PIN,    LOW);
      digitalWrite(TRAFFIC_YELLOW_PIN, HIGH);
      digitalWrite(TRAFFIC_GREEN_PIN,  LOW);
      Serial.println("[DevCtrl] Traffic: YELLOW");
      break;
    case TRAFFIC_RED_FLASH:
      // Xử lý blink trong trafficLightLoop()
      trafficFlashLastMs = millis();
      trafficFlashRedOn  = true;
      digitalWrite(TRAFFIC_RED_PIN,    HIGH);
      digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
      digitalWrite(TRAFFIC_GREEN_PIN,  LOW);
      Serial.println("[DevCtrl] Traffic: RED_FLASH");
      break;
  }
}

TrafficLightState getTrafficLightState() {
  return currentTrafficState;
}

// Gọi trong loop() — chỉ làm gì khi đang ở chế độ nhấp nháy
void trafficLightLoop() {
  if (currentTrafficState != TRAFFIC_RED_FLASH) return;

  unsigned long now = millis();
  if (now - trafficFlashLastMs >= FLASH_INTERVAL_MS) {
    trafficFlashRedOn  = !trafficFlashRedOn;
    trafficFlashLastMs = now;
    digitalWrite(TRAFFIC_RED_PIN, trafficFlashRedOn ? HIGH : LOW);
  }
}

// ══════════════════════════════════════════════════
//  IR Beam
// ══════════════════════════════════════════════════
int readBeam() {
  return digitalRead(BEAM_PIN);
}
