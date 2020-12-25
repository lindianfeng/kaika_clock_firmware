/*
 * clock.c
 *
 *  Created on: Dec 10, 2020
 *      Author: linhao
 */

#include <stdio.h>
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "usb_device.h"
#include "gpio.h"
#include "ds3231.h"
#include "max72xx.h"
#include "ticktimer.h"

extern RTC_Data rtc;
static bool need_flash_point = false;

const uint8_t numbers_5x8[][8] = {
    { 0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e },
    { 0x04, 0x06, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e },
    { 0x0e, 0x11, 0x10, 0x10, 0x08, 0x04, 0x02, 0x1f },
    { 0x0e, 0x11, 0x10, 0x0c, 0x10, 0x10, 0x11, 0x0e },
    { 0x10, 0x18, 0x14, 0x12, 0x11, 0x1f, 0x10, 0x10 },
    { 0x1f, 0x01, 0x01, 0x0f, 0x10, 0x10, 0x11, 0x0e },
    { 0x0e, 0x11, 0x01, 0x0f, 0x11, 0x11, 0x11, 0x0e },
    { 0x1f, 0x10, 0x10, 0x08, 0x04, 0x02, 0x02, 0x02 },
    { 0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x11, 0x0e },
    { 0x0e, 0x11, 0x11, 0x11, 0x1e, 0x10, 0x11, 0x0e },
    { 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x07, 0x04, 0x07, 0x01, 0x07 },
    { 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01 },
    { 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00 } };

const uint8_t numbers_5x8_col_data[][8] = {
    { 0x7e, 0x81, 0x81, 0x81, 0x7e, 0x00, 0x00, 0x00 },
    { 0x00, 0x41, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00 },
    { 0x41, 0x83, 0x85, 0x89, 0x71, 0x00, 0x00, 0x00 },
    { 0x42, 0x81, 0x91, 0x91, 0x6e, 0x00, 0x00, 0x00 },
    { 0x0c, 0x14, 0x24, 0x44, 0xff, 0x00, 0x00, 0x00 },
    { 0xf2, 0x91, 0x91, 0x91, 0x8e, 0x00, 0x00, 0x00 },
    { 0x7e, 0x91, 0x91, 0x91, 0x4e, 0x00, 0x00, 0x00 },
    { 0x80, 0x87, 0x88, 0x90, 0xe0, 0x00, 0x00, 0x00 },
    { 0x6e, 0x91, 0x91, 0x91, 0x6e, 0x00, 0x00, 0x00 },
    { 0x72, 0x89, 0x89, 0x89, 0x7e, 0x00, 0x00, 0x00 },
    { 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x17, 0x15, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x41, 0x63, 0x36, 0x1c, 0x08, 0x00, 0x00 }
};

const uint8_t numbers_3x5[10][8] = {
    { 0x00, 0x00, 0x00, 0x07, 0x05, 0x05, 0x05, 0x07 },
    { 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02 },
    { 0x00, 0x00, 0x00, 0x07, 0x04, 0x07, 0x01, 0x07 },
    { 0x00, 0x00, 0x00, 0x07, 0x04, 0x07, 0x04, 0x07 },
    { 0x00, 0x00, 0x00, 0x05, 0x05, 0x07, 0x04, 0x04 },
    { 0x00, 0x00, 0x00, 0x07, 0x01, 0x07, 0x04, 0x07 },
    { 0x00, 0x00, 0x00, 0x07, 0x01, 0x07, 0x05, 0x07 },
    { 0x00, 0x00, 0x00, 0x07, 0x04, 0x04, 0x04, 0x04 },
    { 0x00, 0x00, 0x00, 0x07, 0x05, 0x07, 0x05, 0x07 },
    { 0x00, 0x00, 0x00, 0x07, 0x05, 0x07, 0x04, 0x07 } };

const uint8_t numbers_3x5_col_data[][8] = {
    { 0x1f, 0x11, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x17, 0x15, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x15, 0x15, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x1c, 0x04, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x1d, 0x15, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x1f, 0x15, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x10, 0x10, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x1f, 0x15, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x1d, 0x15, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

const uint8_t signs[][8] = {
    { 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x00 },  //:
    { 0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c },  //笑脸
    { 0x3c, 0x42, 0xa5, 0x81, 0xbd, 0x81, 0x42, 0x3c },
    { 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x06, 0x0c, 0x18, 0x30, 0x18, 0x0c, 0x06 } };

//uint8_t* OneDevDataRow2Col(const uint8_t *row_data) {
//  static uint8_t col_data[8] = { 0 };
//
//  if (!row_data) {
//    return NULL;
//  }
//
//  for (int i = 0; i < 8; ++i) {
//    for (int j = 0; j < 8; ++j) {
//      //int reverseRowByte = bitReverse(rowBytes[j]);
//      bitWrite(col_data[i], 7 - j, bitRead(*(row_data +j), 7 - i));
//    }
//  }
//  return col_data;
//}

static void led_display_time(uint8_t hour, uint8_t minute, uint8_t second,
bool b) {
  const uint8_t hour_1st = hour / 10;
  const uint8_t hour_2nd = hour % 10;

  const uint8_t minute_1st = minute / 10;
  const uint8_t minute_2nd = minute % 10;

  const uint8_t second_1st = second / 10;
  const uint8_t second_2nd = second % 10;

  MAX72XX_ControlAll(UPDATE, OFF);
  MAX72XX_ClearAll();
  for (uint8_t row = 0; row < 8; row++) {
    for (uint8_t dev = 0; dev < MAX_DEVICES; dev++) {
      uint8_t data = numbers_5x8[0][row];
      switch (dev) {
        case 0:
          data = (numbers_5x8[hour_1st][row] | numbers_5x8[hour_2nd][row] << 5);
          break;
        case 1:
          data = numbers_5x8[hour_2nd][row] >> 3 |
              (b ? signs[0][row] >> 4 : 0) |
              ((numbers_5x8[minute_1st][row]) << 5);
          break;
        case 2:
          data = (numbers_5x8[minute_1st][row] >> 3) |
              (numbers_5x8[minute_2nd][row] << 2);
          break;
        case 3:
          data = (numbers_3x5[second_1st][row]) << 1 |
              ((numbers_3x5[second_2nd][row]) << 5);
          break;
      }

      MAX72XX_SetRowOne(dev, row, data);
    }
  }
  MAX72XX_UpdateAll();
  MAX72XX_ControlAll(UPDATE, ON);
}

static void led_display_date(uint8_t month, uint8_t day_of_month,
                             uint8_t dayofweek) {
  const uint8_t month_1st = month / 10;
  const uint8_t month_2nd = month % 10;

  const uint8_t day_1st = day_of_month / 10;
  const uint8_t day_2nd = day_of_month % 10;
  const uint8_t day_of_week = dayofweek;

  MAX72XX_ControlAll(UPDATE, OFF);
  MAX72XX_ClearAll();

  for (uint8_t row = 0; row < 8; row++) {
    for (uint8_t dev = 0; dev < MAX_DEVICES; dev++) {
      uint8_t data = 0;
      switch (dev) {
        case 0:
          data = numbers_5x8[month_1st][row] << 2 | numbers_5x8[month_2nd][row] << 7;
          break;
        case 1:
          data = numbers_5x8[month_2nd][row] >> 1 | signs[3][row] << 5;
          break;
        case 2:
          data = numbers_5x8[day_1st][row] << 1 | numbers_5x8[day_2nd][row] << 6;
          break;
        case 3:
          data = numbers_5x8[day_2nd][row] >> 2 | (numbers_3x5[day_of_week == 1 ? 7 : day_of_week - 1][row]) << 5;
          break;

      }
      MAX72XX_SetRowOne(dev, row, data);
    }
  }

  MAX72XX_ControlAll(UPDATE, ON);
  MAX72XX_UpdateAll();
}

static void Clock_TestLedMatrix() {

  MAX72XX_ClearAll();
  for (int i = 0; i < 32; i++) {
    MAX72XX_SetPixelColumn(i, 0xff);
    HAL_Delay(50);
    MAX72XX_SetPixelColumn(i, 0x00);
    HAL_Delay(50);
  }

  HAL_Delay(200);

  MAX72XX_ClearAll();
  for (int i = 0; i < 8; i++) {
    MAX72XX_SetRowAll(i, 0xff);
    HAL_Delay(50);
    MAX72XX_SetRowAll(i, 0x00);
    HAL_Delay(50);
  }

//  HAL_Delay(200);
//
//  MAX72XX_ClearAll();
//  MAX72XX_SetPixelColumn(0, 0xff);
//  for (int i = 0; i < 31; i++) {
//    MAX72XX_TransformAll(TSL);
//    HAL_Delay(50);
//  }
//
//  MAX72XX_ClearAll();
//  MAX72XX_SetRowAll(0, 0xff);
//  for (int i = 0; i < 7; i++) {
//    MAX72XX_TransformAll(TSD);
//    HAL_Delay(50);
//  }

  HAL_Delay(200);
}
#define POINT_COL_NUM 11
static void Clock_FlashTimePoint() {
  MAX72XX_SetPoint(1, POINT_COL_NUM, need_flash_point);
  MAX72XX_SetPoint(2, POINT_COL_NUM, need_flash_point);
  MAX72XX_SetPoint(5, POINT_COL_NUM, need_flash_point);
  MAX72XX_SetPoint(6, POINT_COL_NUM, need_flash_point);
  MAX72XX_UpdateAll();
}

static bool Clock_UpdateRTC(void) {
  static uint8_t old_sec = 0;
//  static uint32_t last_tick = 0;
  DS3231_GetTime(&rtc);

//  if (HAL_GetTick() - last_tick > 999) {
//    rtc.Sec++;
//    last_tick = HAL_GetTick();
//  }
//
//  if (rtc.Sec > 59) {
//    rtc.Min++;
//    rtc.Sec = 0;
//  }
//
//  if (rtc.Min > 59) {
//    rtc.Hour++;
//    rtc.Min = 0;
//  }

  if (rtc.Sec != old_sec) {
    old_sec = rtc.Sec;
    return true;
  }

  return false;
}

static void MAX72XX_ShowWelcome(void) {
  MAX72XX_ClearAll();
  for (uint8_t row = 0; row < 8; row++) {
    MAX72XX_SetRowAll(row, 0xff);
  }

  HAL_Delay(500);

  MAX72XX_ClearAll();

  Clock_TestLedMatrix();

  MAX72XX_ClearAll();

  MAX72XX_UpdateAll();
}

static void Clock_Init(void) {
  DS3231_Init();
  MAX72XX_Init();
  MAX72XX_ShowWelcome();
}

static void Clock_ToggleSecPoint() {
  need_flash_point = !need_flash_point;
}

static void Clock_ShowTime() {
  led_display_time(rtc.Hour, rtc.Min, rtc.Sec, need_flash_point);
}

static void Clock_ShowDate(void) {
  led_display_date(rtc.Month, rtc.Day, rtc.DaysOfWeek);
}

static void Clock_SecondJumpUp(void) {
  Clock_FlashTimePoint();
  MAX72XX_TransformOne(3, TSU);
}

static void Clock_SecondJumpDown(void) {
  Clock_FlashTimePoint();
  MAX72XX_TransformOne(3, TSD);
}

static void Clock_UpdateDiplay() {
  MAX72XX_UpdateAll();
}

//static void Clock_ClearAll() {
//  MAX72XX_ClearAll();
//}


enum {
  STATE_CLOCK_TIME = 0,
  STATE_CLOCK_DATE = 1,
  STATE_CLOCK_TEMP = 2,
};

enum {
  STATE_TIME_SHOW = 0,
  STATE_TIME_SEC_CHANGED = 1,
  STATE_TIME_SEC_JUMP_UP = 2,
  STATE_TIME_SEC_JUMP_DOWN = 3,
};

typedef struct {
  bool fired;
  int16_t state;
  int32_t timeout;
  int16_t repeat;
  uint32_t starttick;
  void (*callback)(void);
} State;

static State clock_states[3] = { { .fired = false, .state = STATE_CLOCK_TIME, .timeout = 10, .repeat = -1, .starttick = 0, .callback = Clock_ShowTime }, { .fired = false, .state = STATE_CLOCK_DATE, .timeout = 2000, .repeat = 1, .starttick =
    0, .callback = Clock_ShowDate }, { .fired = false, .state = STATE_CLOCK_TEMP, .timeout = 0, .repeat = 0, .starttick = 0, .callback = 0 } };

static State time_states[4] = { { .fired = false, .state = STATE_TIME_SHOW, .timeout = 10, .repeat = -1, .starttick = 0, .callback = Clock_ShowTime }, {
    .fired = false, .state = STATE_TIME_SEC_CHANGED, .timeout = 0, .repeat = 0, .starttick = 0, .callback = NULL }, { .fired = false, .state = STATE_TIME_SEC_JUMP_UP, .timeout = 100, .repeat = 2, .starttick = 0, .callback =
    Clock_SecondJumpUp }, { .fired = false, .state = STATE_TIME_SEC_JUMP_DOWN, .timeout = 100, .repeat = 2, .starttick = 0, .callback = Clock_SecondJumpDown } };

static State clock_s = { 0 };
static State time_s = { 0 };

static inline void ChangeClockState(int clock_state_n, uint32_t tick) {
  printf("ChangeClockState:%d.\r\n", clock_state_n);
  clock_s = clock_states[clock_state_n];
}

static inline void ChangeTimeState(int time_state_n, uint32_t tick) {
  printf("ChangeTimeState:%d.\r\n", time_state_n);
  time_s = time_states[time_state_n];
}

static inline bool TickState(State *s, uint32_t tick) {
  if (!s->timeout) {
    printf("1.TickState:%d,timeout:%ld,repeat:%d,fired:%d.\r\n", s->state, s->timeout, s->repeat, s->fired);
    return true;
  }

  if (!s->fired) {
    if (s->callback) {
      s->callback();
    }

    s->fired = true;
    s->starttick = tick;

    if (-1 != s->repeat) {
      --s->repeat;
    }
    printf("2.TickState:%d,timeout:%ld,repeat:%d,fired:%d.\r\n", s->state, s->timeout, s->repeat, s->fired);
  } else {
    if ((tick - s->starttick) > s->timeout) {
      if (0 == s->repeat) {
        printf("3.TickState:%d,timeout:%ld,repeat:%d,fired:%d.\r\n", s->state, s->timeout, s->repeat, s->fired);
        return true;
      } else {
        if (s->callback) {
          s->callback();
        }

        if (-1 != s->repeat) {
          --s->repeat;
        }

        printf("4.TickState:%d,timeout:%ld,repeat:%d,fired:%d.\r\n", s->state, s->timeout, s->repeat, s->fired);
      }

      s->starttick = tick;
    } else {
      printf("5.TickState:%d,timeout:%ld,repeat:%d,fired:%d.\r\n", s->state, s->timeout, s->repeat, s->fired);
    }
  }

  return false;
}

static void on_show_date_ticktimer_timeout() {
  ChangeClockState(STATE_CLOCK_DATE, HAL_GetTick());
}

static void on_update_rtc_ticktimer_timeout() {
  if (Clock_UpdateRTC()) {
    ChangeTimeState(STATE_TIME_SEC_CHANGED, HAL_GetTick());
  }
}

static void on_flash_point_ticktimer_timeout() {
  Clock_ToggleSecPoint();
}

static void on_reader_ticktimer_timeout() {
  Clock_UpdateDiplay();
}

extern void SystemClock_Config(void);

int main(void)
{
  HAL_Init();

  SystemClock_Config();


  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  Clock_Init();

  printf("Clock Init Completed!\r\n");

  static tick_timer show_date_ticktimer = { .timeout_cb = on_show_date_ticktimer_timeout, .timeout = 59999, .repeat = 59999 };
  static tick_timer update_rtc_ticktimer = { .timeout_cb = on_update_rtc_ticktimer_timeout, .timeout = 199, .repeat = 199 };
  static tick_timer flash_point_ticktimer = { .timeout_cb = on_flash_point_ticktimer_timeout, .timeout = 499, .repeat = 499 };
  static tick_timer reader_ticktimer = { .timeout_cb = on_reader_ticktimer_timeout, .timeout = 99, .repeat = 99 };

  timer_start(&show_date_ticktimer);
  timer_start(&update_rtc_ticktimer);
  timer_start(&flash_point_ticktimer);
  timer_start(&reader_ticktimer);

  clock_s = clock_states[0];
  time_s = time_states[0];

  while (1)
  {
    const uint32_t tick = HAL_GetTick();

    timer_ticks(tick);

    switch (clock_s.state) {
      case STATE_CLOCK_TIME: {
        switch (time_s.state) {
          case STATE_TIME_SHOW:
            TickState(&time_s, tick);
            break;
          case STATE_TIME_SEC_CHANGED:
            if (TickState(&time_s, tick)) {
              ChangeTimeState(STATE_TIME_SEC_JUMP_UP, tick);
            }
            break;
          case STATE_TIME_SEC_JUMP_UP:
            if (TickState(&time_s, tick)) {
              ChangeTimeState(STATE_TIME_SEC_JUMP_DOWN, tick);
            }
            break;
          case STATE_TIME_SEC_JUMP_DOWN:
            if (TickState(&time_s, tick)) {
              ChangeTimeState(STATE_TIME_SHOW, tick);
            }
            break;
        }

        break;
      }
      case STATE_CLOCK_DATE:
        if (TickState(&clock_s, tick)) {
          ChangeClockState(STATE_CLOCK_TIME, tick);
          ChangeTimeState(STATE_TIME_SHOW, tick);
          Clock_ShowTime();
        }
        break;
    }

    timer_loop();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}





#define SECOND_MOVE_ROWS 2
#define DISPLAY_DATE_TICMKS 20
