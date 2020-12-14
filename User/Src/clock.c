/*
 * clock.c
 *
 *  Created on: Dec 10, 2020
 *      Author: linhao
 */

#include "clock.h"

#include <stdio.h>
#include <string.h>

#include "ds3231.h"
#include "max72xx.h"
#include "stm32f1xx_hal.h"
#include "utils.h"

extern RTC_Data rtc;

const uint8_t numbers_5x8[][8] = { { 0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70 },  // 0
    { 0x20, 0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70 },  // 1
    { 0x70, 0x88, 0x08, 0x08, 0x10, 0x20, 0x40, 0xf8 },  // 2
    { 0x70, 0x88, 0x08, 0x30, 0x08, 0x08, 0x88, 0x70 },  // 3
    { 0x08, 0x18, 0x28, 0x48, 0x88, 0xf8, 0x08, 0x08 },  // 4
    { 0xf8, 0x80, 0x80, 0xf0, 0x08, 0x08, 0x88, 0x70 },  // 5
    { 0x70, 0x88, 0x80, 0xf0, 0x88, 0x88, 0x88, 0x70 },  // 6
    { 0xf8, 0x08, 0x08, 0x10, 0x20, 0x40, 0x40, 0x40 },  // 7
    { 0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x88, 0x70 },  // 8
    { 0x70, 0x88, 0x88, 0x88, 0x78, 0x08, 0x88, 0x70 }   // 9
};

const uint8_t numbers_3x5[10][8] = { { 0x00, 0x00, 0x00, 0xe0, 0xa0, 0xa0, 0xa0, 0xe0 },  // 0
    { 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x40 },  // 1
    { 0x00, 0x00, 0x00, 0xe0, 0x20, 0xe0, 0x80, 0xe0 },  // 2
    { 0x00, 0x00, 0x00, 0xe0, 0x20, 0xe0, 0x20, 0xe0 },  // 3
    { 0x00, 0x00, 0x00, 0xa0, 0xa0, 0xe0, 0x20, 0x20 },  // 4
    { 0x00, 0x00, 0x00, 0xe0, 0x80, 0xe0, 0x20, 0xe0 },  // 5
    { 0x00, 0x00, 0x00, 0xe0, 0x80, 0xe0, 0xa0, 0xe0 },  // 6
    { 0x00, 0x00, 0x00, 0xe0, 0x20, 0x20, 0x20, 0x20 },  // 7
    { 0x00, 0x00, 0x00, 0xe0, 0xa0, 0xe0, 0xa0, 0xe0 },  // 8
    { 0x00, 0x00, 0x00, 0xe0, 0xa0, 0xe0, 0x20, 0xe0 }   // 9
};

const uint8_t signs[][8] = { { 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x00 },  //:
    { 0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c },  //笑脸
    { 0x3c, 0x42, 0xa5, 0x81, 0xbd, 0x81, 0x42, 0x3c }, { 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00 } };

const uint64_t num[] = { 0x1c08080808080c08, 0x3e0408102020221c, 0x1c2220201820221c, 0x20203e2224283020, 0x1c2220201e02023e, 0x1c2222221e02221c, 0x040404081020203e, 0x1c2222221c22221c, 0x1c22203c2222221c, 0x1c2222222222221c };
static void led_display_time(uint8_t hour, uint8_t minute, uint8_t second, bool b) {
  const uint8_t hour_1st = hour / 10;
  const uint8_t hour_2nd = hour % 10;

  const uint8_t minute_1st = minute / 10;
  const uint8_t minute_2nd = minute % 10;

  const uint8_t second_1st = second / 10;
  const uint8_t second_2nd = second % 10;

  MAX7219_ClearAll();
  MAX7219_ControlAll(UPDATE, OFF);
  for (uint8_t row = 0; row < 8; row++) {
    for (uint8_t dev = 0; dev < MAX_DEVICES; dev++) {
      uint8_t data = 0;
      switch (dev) {
      case 0:
        data = ((num[hour_1st] >> row * 8) & 0xFF) | (((num[hour_2nd] >> row * 8) & 0xFF)>> 5);
        break;
      case 1:
        data = ((num[hour_2nd] >> row * 8) & 0xFF) << 3 | (b ? signs[0][row] >> 3 : 0) | ((numbers_5x8[minute_1st][row]) >> 5);
        break;
      case 2:
        data = (numbers_5x8[minute_1st][row] << 3) | (numbers_5x8[minute_2nd][row] >> 2);
        break;
      case 3:
        data = (numbers_3x5[second_1st][row]) >> 1 | ((numbers_3x5[second_2nd][row]) >> 5);
        break;
      }

      MAX7219_SetRowOne(dev, row, data);
    }
  }

  MAX7219_UpdateAll();
  MAX7219_ControlAll(UPDATE, ON);
}

static void led_display_date(uint8_t month, uint8_t day_of_month, uint8_t dayofweek) {
  const uint8_t month_1st = month / 10;
  const uint8_t month_2nd = month % 10;

  const uint8_t day_1st = day_of_month / 10;
  const uint8_t day_2nd = day_of_month % 10;
  const uint8_t day_of_week = dayofweek;

  MAX7219_ControlAll(UPDATE, OFF);
  MAX7219_ClearAll();

  for (uint8_t row = 0; row < 8; row++) {
    for (uint8_t dev = 0; dev < MAX_DEVICES; dev++) {
      uint8_t data = 0;
      switch (dev) {
      case 0:
        data = numbers_5x8[month_1st][row] | ((numbers_5x8[month_2nd][row]) << 5);
        break;
      case 1:
        data = numbers_5x8[month_2nd][row] << 3 | (signs[3][row] >> 3 | numbers_5x8[day_1st][row] << 7);
        break;
      case 2:
        data = (numbers_5x8[day_1st][row] >> 1) | (numbers_5x8[day_2nd][row] << 4);
        break;
      case 3:
        data = (numbers_5x8[day_2nd][row] >> 4) | ((numbers_3x5[day_of_week == 1 ? 7 : day_of_week - 1][row]) << 5);
        break;
      }
      MAX7219_SetRowOne(dev, row, data);
    }
  }

  MAX7219_ControlAll(UPDATE, ON);
  MAX7219_UpdateAll();
}

void Clock_FlashTimePoint(bool b) {
  MAX7219_SetPoint(1, 12, b);
  MAX7219_SetPoint(2, 12, b);
  MAX7219_SetPoint(5, 12, b);
  MAX7219_SetPoint(6, 12, b);
  MAX7219_UpdateAll();
}

bool Clock_UpdateRTC(void) {
  static uint8_t old_sec = 0;

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

static void show_welcome(void) {
  MAX7219_ClearAll();
  for (uint8_t row = 0; row < 8; row++) {
    MAX7219_SetRowAll(row, 0xff);
  }

  HAL_Delay(500);

  MAX7219_ClearAll();

  MAX7219_SetPixelColumn(0, 0xff);

  for (int i = 0; i < 31; i++) {
    MAX7219_TransformAll(TSL);
    HAL_Delay(50);
  }

  MAX7219_ClearAll();

  MAX7219_UpdateAll();
}

void Clock_Init(void) {
  DS3231_Init();
  MAX7219_Init();
  //show_welcome();
}

void Clock_ShowTime(bool b) {
  led_display_time(rtc.Hour, rtc.Min, rtc.Sec, b);
}

void Clock_ShowDate(void) {
  led_display_date(rtc.Month, rtc.Day, rtc.DaysOfWeek);
}

void Clock_SecondJumpUp(void) {
  MAX7219_TransformOne(3, TSU);
}

void Clock_SecondJumpDown(void) {
  MAX7219_TransformOne(3, TSD);
}

#define SECOND_MOVE_ROWS 2
#define DISPLAY_DATE_TICMKS 20
