/*
 * ticktimer.h
 *
 *  Created on: 2020年12月14日
 *      Author: lindi
 */

#ifndef INC_TICKTIMER_H_
#define INC_TICKTIMER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct tick_timer {
  uint32_t timeout;
  uint32_t repeat;
  void (*timeout_cb)(void);
  struct tick_timer *next;
} tick_timer;

void timer_init(struct tick_timer *timer, void (*timeout_cb)(void), uint32_t timeout, uint32_t repeat);
int timer_start(struct tick_timer *timer);
void timer_stop(struct tick_timer *timer);
void timer_ticks(uint32_t tick);
void timer_loop(void);

bool TickTimer_IsExpired(tick_timer *timer, uint32_t tick);

#endif /* INC_TICKTIMER_H_ */
