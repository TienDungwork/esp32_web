#include "soft_timer.h"
#include <Arduino.h>  // Cần cho millis() trên ESP32

typedef struct {
    bool is_running;
    bool is_repeat;
    uint32_t period_ms;
    uint32_t last_tick;
    timer_callback_t callback;
} SoftTimer;

static SoftTimer timers[MAX_SOFT_TIMERS];

void soft_timer_init(void) {
    for (int i = 0; i < MAX_SOFT_TIMERS; i++) {
        timers[i].is_running = false;
        timers[i].callback = NULL;
    }
}

int soft_timer_start(uint32_t period_ms, bool repeat, timer_callback_t cb) {
    for (int i = 0; i < MAX_SOFT_TIMERS; i++) {
        if (!timers[i].is_running) {
            timers[i].period_ms = period_ms;
            timers[i].is_repeat = repeat;
            timers[i].callback = cb;
            timers[i].last_tick = millis();
            timers[i].is_running = true;
            return i;
        }
    }
    return -1;  // No available timer slot
}

void soft_timer_stop(int timer_id) {
    if (timer_id >= 0 && timer_id < MAX_SOFT_TIMERS) {
        timers[timer_id].is_running = false;
    }
}

void soft_timer_reset(int timer_id) {
    if (timer_id >= 0 && timer_id < MAX_SOFT_TIMERS) {
        timers[timer_id].last_tick = millis();
    }
}

void soft_timer_update(void) {
    uint32_t now = millis();
    for (int i = 0; i < MAX_SOFT_TIMERS; i++) {
        if (timers[i].is_running) {
            if ((now - timers[i].last_tick) >= timers[i].period_ms) {
                timers[i].last_tick = now;
                if (timers[i].callback) {
                    timers[i].callback();
                }
                if (!timers[i].is_repeat) {
                    timers[i].is_running = false;
                }
            }
        }
    }
}
