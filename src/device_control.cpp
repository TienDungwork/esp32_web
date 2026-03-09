#include "device_control.h"

// ── State tracking ──
static BarrierState       currentBarrierState  = BARRIER_PAUSE;
static TrafficLightState  currentTrafficState  = TRAFFIC_OFF;
static bool               trafficFlashRedOn    = false;
static unsigned long      trafficFlashLastMs   = 0;
static const unsigned long FLASH_INTERVAL_MS   = 500;

static bool               lastBtnOpenState     = HIGH;
static bool               lastBtnCloseState    = HIGH;
static bool               lastBtnStopState     = HIGH;
static unsigned long      lastBtnOpenMs        = 0;
static unsigned long      lastBtnCloseMs       = 0;
static unsigned long      lastBtnStopMs        = 0;
static const unsigned long BTN_DEBOUNCE_MS     = 80;

static bool               barrierPulseActive   = false;
static int                barrierPulsePin      = -1;
static unsigned long      barrierPulseStartMs  = 0;
static const unsigned long BARRIER_PULSE_MS    = 180;

static void setAllBarrierRelaysLow() {
  digitalWrite(BARRIER_OPEN_PIN,  LOW);
  digitalWrite(BARRIER_CLOSE_PIN, LOW);
  digitalWrite(RELAY_STOP_PIN,    LOW);
}

static void triggerBarrierPulse(int relayPin) {
  setAllBarrierRelaysLow();
  digitalWrite(relayPin, HIGH);
  barrierPulseActive = true;
  barrierPulsePin = relayPin;
  barrierPulseStartMs = millis();
}

// ══════════════════════════════════════════════════
//  Init
// ══════════════════════════════════════════════════
void deviceControlInit() {
  // Barrier pins
  pinMode(BARRIER_OPEN_PIN,  OUTPUT);
  pinMode(BARRIER_CLOSE_PIN, OUTPUT);
  pinMode(RELAY_STOP_PIN,    OUTPUT);
  setAllBarrierRelaysLow();

  // Physical buttons (momentary buttons with pull-up)
  pinMode(BTN_OPEN_PIN,  INPUT_PULLUP);
  pinMode(BTN_CLOSE_PIN, INPUT_PULLUP);
  pinMode(BTN_STOP_PIN,  INPUT_PULLUP);

  // Traffic light pins
  pinMode(TRAFFIC_RED_PIN,    OUTPUT);
  pinMode(TRAFFIC_YELLOW_PIN, OUTPUT);
  pinMode(TRAFFIC_GREEN_PIN,  OUTPUT);
  digitalWrite(TRAFFIC_RED_PIN,    LOW);
  digitalWrite(TRAFFIC_YELLOW_PIN, LOW);
  digitalWrite(TRAFFIC_GREEN_PIN,  LOW);

  // Beam sensor pins
  pinMode(BEAM_PWM1_IN_PIN,  INPUT_PULLUP);
  pinMode(BEAM_PWM2_OUT_PIN, INPUT_PULLUP);
  pinMode(BEAM_A0_PIN,       INPUT_PULLUP);

  Serial.println("[DevCtrl] GPIO initialized");
  Serial.printf("  Barrier relays: OPEN=%d CLOSE=%d STOP=%d\n",
                BARRIER_OPEN_PIN, BARRIER_CLOSE_PIN, RELAY_STOP_PIN);
  Serial.printf("  Buttons: OPEN=%d CLOSE=%d STOP=%d\n",
                BTN_OPEN_PIN, BTN_CLOSE_PIN, BTN_STOP_PIN);
  Serial.printf("  Traffic: R=%d Y=%d G=%d\n", TRAFFIC_RED_PIN, TRAFFIC_YELLOW_PIN, TRAFFIC_GREEN_PIN);
  Serial.printf("  Beam: PWM1_IN=%d PWM2_OUT=%d A0=%d\n",
                BEAM_PWM1_IN_PIN, BEAM_PWM2_OUT_PIN, BEAM_A0_PIN);
}

void deviceControlLoop() {
  const unsigned long now = millis();

  if (barrierPulseActive && (now - barrierPulseStartMs) >= BARRIER_PULSE_MS) {
    if (barrierPulsePin >= 0) {
      digitalWrite(barrierPulsePin, LOW);
    }
    barrierPulseActive = false;
    barrierPulsePin = -1;
    // Barrier is momentary: after pulse completes, return to idle state.
    currentBarrierState = BARRIER_PAUSE;
  }

  const int btnOpenNow = digitalRead(BTN_OPEN_PIN);
  if (lastBtnOpenState == HIGH && btnOpenNow == LOW && (now - lastBtnOpenMs) >= BTN_DEBOUNCE_MS) {
    barrierControl(BARRIER_OPEN);
    lastBtnOpenMs = now;
    Serial.println("[DevCtrl] Button OPEN pressed");
  }
  lastBtnOpenState = btnOpenNow;

  const int btnCloseNow = digitalRead(BTN_CLOSE_PIN);
  if (lastBtnCloseState == HIGH && btnCloseNow == LOW && (now - lastBtnCloseMs) >= BTN_DEBOUNCE_MS) {
    barrierControl(BARRIER_CLOSE);
    lastBtnCloseMs = now;
    Serial.println("[DevCtrl] Button CLOSE pressed");
  }
  lastBtnCloseState = btnCloseNow;

  const int btnStopNow = digitalRead(BTN_STOP_PIN);
  if (lastBtnStopState == HIGH && btnStopNow == LOW && (now - lastBtnStopMs) >= BTN_DEBOUNCE_MS) {
    barrierControl(BARRIER_PAUSE);
    lastBtnStopMs = now;
    Serial.println("[DevCtrl] Button STOP pressed");
  }
  lastBtnStopState = btnStopNow;
}

// ══════════════════════════════════════════════════
//  Barrier
// ══════════════════════════════════════════════════
void barrierControl(BarrierState state) {
  currentBarrierState = state;
  switch (state) {
    case BARRIER_PAUSE:
      triggerBarrierPulse(RELAY_STOP_PIN);
      Serial.println("[DevCtrl] Barrier: STOP pulse");
      break;
    case BARRIER_OPEN:
      triggerBarrierPulse(BARRIER_OPEN_PIN);
      Serial.println("[DevCtrl] Barrier: OPEN pulse");
      break;
    case BARRIER_CLOSE:
      triggerBarrierPulse(BARRIER_CLOSE_PIN);
      Serial.println("[DevCtrl] Barrier: CLOSE pulse");
      break;
  }
}

void barrierDeactivate() {
  if (barrierPulseActive && barrierPulsePin >= 0) {
    digitalWrite(barrierPulsePin, LOW);
  }
  barrierPulseActive = false;
  barrierPulsePin = -1;
  setAllBarrierRelaysLow();
  currentBarrierState = BARRIER_PAUSE;
  Serial.println("[DevCtrl] Barrier deactivated");
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

void trafficLightDeactivate() {
  trafficLightControl(TRAFFIC_OFF);
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
  return readBeamPwm1();
}

int readBeamPwm1() {
  return digitalRead(BEAM_PWM1_IN_PIN);
}

int readBeamPwm2() {
  return digitalRead(BEAM_PWM2_OUT_PIN);
}

int readBeamA0() {
  return digitalRead(BEAM_A0_PIN);
}

int readButtonOpen() {
  return digitalRead(BTN_OPEN_PIN);
}

int readButtonClose() {
  return digitalRead(BTN_CLOSE_PIN);
}

int readButtonStop() {
  return digitalRead(BTN_STOP_PIN);
}
