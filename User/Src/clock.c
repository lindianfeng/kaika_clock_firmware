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

const uint8_t signs[][8] = {
    { 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x00 },  //:
    { 0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c },  //笑脸
    { 0x3c, 0x42, 0xa5, 0x81, 0xbd, 0x81, 0x42, 0x3c },
    { 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x06, 0x0c, 0x18, 0x30, 0x18, 0x0c, 0x06 } };

static void led_display_time(uint8_t hour, uint8_t minute, uint8_t second,
                             bool b) {
  const uint8_t hour_1st = hour / 10;
  const uint8_t hour_2nd = hour % 10;

  const uint8_t minute_1st = minute / 10;
  const uint8_t minute_2nd = minute % 10;

  const uint8_t second_1st = second / 10;
  const uint8_t second_2nd = second % 10;

  MAX72XX_ClearAll();
  MAX72XX_ControlAll(UPDATE, OFF);

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
          data = numbers_5x8[month_1st][row] | ((numbers_5x8[month_2nd][row]) << 5);
          break;
        case 1:
          data = numbers_5x8[month_2nd][row] >> 3 | signs[3][row] << 3 | numbers_5x8[day_1st][row] << 7;
          break;
        case 2:
          data = (numbers_5x8[day_1st][row] >> 1) | (numbers_5x8[day_2nd][row] << 4);
          break;
        case 3:
          data = (numbers_5x8[day_2nd][row] >> 4) | ((numbers_3x5[day_of_week == 1 ? 7 : day_of_week - 1][row]) << 5);
          break;
      }
      MAX72XX_SetRowOne(dev, row, data);
    }
  }

  MAX72XX_ControlAll(UPDATE, ON);
  MAX72XX_UpdateAll();
}

void Clock_TestLedMatrix() {

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

  HAL_Delay(200);

  MAX72XX_ClearAll();
  MAX72XX_SetPixelColumn(0, 0xff);
  for (int i = 0; i < 31; i++) {
    MAX72XX_TransformAll(TSL);
    HAL_Delay(50);
  }

  MAX72XX_ClearAll();
  MAX72XX_SetRowAll(0, 0xff);
  for (int i = 0; i < 7; i++) {
    MAX72XX_TransformAll(TSD);
    HAL_Delay(50);
  }

  HAL_Delay(200);
}
#define POINT_COL_NUM 11
void Clock_FlashTimePoint(bool b) {
  MAX72XX_SetPoint(1, POINT_COL_NUM, b);
  MAX72XX_SetPoint(2, POINT_COL_NUM, b);
  MAX72XX_SetPoint(5, POINT_COL_NUM, b);
  MAX72XX_SetPoint(6, POINT_COL_NUM, b);
  MAX72XX_UpdateAll();
}

bool Clock_UpdateRTC(void) {
  static uint8_t old_sec = 0;
  static uint32_t last_tick = 0;
  DS3231_GetTime(&rtc);

  if (HAL_GetTick() - last_tick > 999) {
    rtc.Sec++;
    last_tick = HAL_GetTick();
  }

  if (rtc.Sec > 59) {
    rtc.Min++;
    rtc.Sec = 0;
  }

  if (rtc.Min > 59) {
    rtc.Hour++;
    rtc.Min = 0;
  }

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

void Clock_Init(void) {
  DS3231_Init();
  MAX72XX_Init();
  MAX72XX_ShowWelcome();
}

void Clock_ShowTime(bool b) {
  led_display_time(rtc.Hour, rtc.Min, rtc.Sec, b);
}

void Clock_ShowDate(void) {
  led_display_date(rtc.Month, rtc.Day, rtc.DaysOfWeek);
}

void Clock_SecondJumpUp(void) {
  MAX72XX_TransformOne(3, TSU);
}

void Clock_SecondJumpDown(void) {
  MAX72XX_TransformOne(3, TSD);
}

#define SECOND_MOVE_ROWS 2
#define DISPLAY_DATE_TICMKS 20
