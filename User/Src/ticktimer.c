/*
 * ticktimer.c
 *
 *  Created on: 2020年12月14日
 *      Author: lindi
 */

#include "ticktimer.h"

bool TickTimer_IsExpired(TickTimer *timer, uint32_t tick) {
  if (tick - timer->lasttick > timer->interval) {
    if (timer->autoreload) {
      timer->lasttick = tick;
    }
    return true;
  }
  return false;
}
