/*
 * ticktimer.c
 *
 *  Created on: 2020年12月14日
 *      Author: lindi
 */

#include "ticktimer.h"

bool TickTimer_IsExpired(tick_timer *timer, uint32_t tick) {
  if (tick - timer->lasttick > timer->interval) {
    if (timer->autoreload) {
      timer->lasttick = tick;
    }
    return true;
  }
  return false;
}

void TickTimer_Startup(tick_timer *timer, uint32_t tick, uint32_t interval) {
  timer->lasttick = tick;
  timer->interval = interval;
}

void TickTimer_Cleanup(tick_timer *timer) {
  timer->lasttick = 0;
  timer->interval = 0;
}
