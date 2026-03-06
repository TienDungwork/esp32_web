#ifndef __SOFT_TIMER_H__
#define __SOFT_TIMER_H__

#include <stdint.h>
#include <stdbool.h>

#define MAX_SOFT_TIMERS 10

typedef void (*timer_callback_t)(void);

void soft_timer_init(void);
int soft_timer_start(uint32_t period_ms, bool repeat, timer_callback_t cb);
void soft_timer_stop(int timer_id);
void soft_timer_reset(int timer_id);
void soft_timer_update(void);

#endif
