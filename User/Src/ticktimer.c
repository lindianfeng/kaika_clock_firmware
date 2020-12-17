/*
 * ticktimer.c
 *
 *  Created on: 2020年12月14日
 *      Author: lindi
 */

#include "ticktimer.h"

static struct tick_timer *head_timer = 0;
static uint32_t cur_ticks = 0;

void timer_init(struct tick_timer *timer, void (*timeout_cb)(void), uint32_t timeout, uint32_t repeat)
{
  timer->timeout = cur_ticks + timeout;
  timer->repeat = repeat;
  timer->timeout_cb = timeout_cb;

}

int timer_start(struct tick_timer *timer) {
  struct tick_timer *target = head_timer;
  while (target) {
    if (target == timer)
      return -1;  //already exist.
    target = target->next;
  }

  timer->next = head_timer;
  head_timer = timer;

  return 0;
}

void timer_stop(struct tick_timer *timer)
{
  struct tick_timer **curr;
  for (curr = &head_timer; *curr;) {
    struct tick_timer *entry = *curr;
    if (entry == timer) {
      *curr = entry->next;
    } else
      curr = &entry->next;
  }
}

void timer_ticks(uint32_t tick) {
  cur_ticks = tick;
}

void timer_loop(void) {
  struct tick_timer *target;
  for (target = head_timer; target; target = target->next) {
    if (cur_ticks >= target->timeout) {
      if (target->repeat == 0) {
        timer_stop(target);
      } else {
        target->timeout = cur_ticks + target->repeat;
      }
      target->timeout_cb();
    }
  }
}
