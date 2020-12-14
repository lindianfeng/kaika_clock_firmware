/*
 * clock.c
 *
 *  Created on: Dec 10, 2020
 *      Author: linhao
 */

#include "clock.h"
#include <string.h>
#include <stdio.h>

#include "max72xx.h"
#include "ds3231.h"
#include "utils.h"
#include "stm32f1xx_hal.h"

extern RTC_Data rtc;

const uint8_t numbers_5x8[][8] = { { 0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70 },  //0
    { 0x20, 0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70 },  //1
    { 0x70, 0x88, 0x08, 0x08, 0x10, 0x20, 0x40, 0xf8 },  //2
    { 0x70, 0x88, 0x08, 0x30, 0x08, 0x08, 0x88, 0x70 },  //3
    { 0x08, 0x18, 0x28, 0x48, 0x88, 0xf8, 0x08, 0x08 },  //4
    { 0xf8, 0x80, 0x80, 0xf0, 0x08, 0x08, 0x88, 0x70 },  //5
    { 0x70, 0x88, 0x80, 0xf0, 0x88, 0x88, 0x88, 0x70 },  //6
    { 0xf8, 0x08, 0x08, 0x10, 0x20, 0x40, 0x40, 0x40 },  //7
    { 0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x88, 0x70 },  //8
    { 0x70, 0x88, 0x88, 0x88, 0x78, 0x08, 0x88, 0x70 }   //9
};

const uint8_t numbers_3x5[10][8] = { { 0x00, 0x00, 0x00, 0xe0, 0xa0, 0xa0, 0xa0, 0xe0 },  //0
    { 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x40 },  //1
    { 0x00, 0x00, 0x00, 0xe0, 0x20, 0xe0, 0x80, 0xe0 },  //2
    { 0x00, 0x00, 0x00, 0xe0, 0x20, 0xe0, 0x20, 0xe0 },  //3
    { 0x00, 0x00, 0x00, 0xa0, 0xa0, 0xe0, 0x20, 0x20 },  //4
    { 0x00, 0x00, 0x00, 0xe0, 0x80, 0xe0, 0x20, 0xe0 },  //5
    { 0x00, 0x00, 0x00, 0xe0, 0x80, 0xe0, 0xa0, 0xe0 },  //6
    { 0x00, 0x00, 0x00, 0xe0, 0x20, 0x20, 0x20, 0x20 },  //7
    { 0x00, 0x00, 0x00, 0xe0, 0xa0, 0xe0, 0xa0, 0xe0 },  //8
    { 0x00, 0x00, 0x00, 0xe0, 0xa0, 0xe0, 0x20, 0xe0 }   //9
};

const uint8_t signs[][8] = { { 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x00 },  //:
    { 0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c },  //笑脸
    { 0x3c, 0x42, 0xa5, 0x81, 0xbd, 0x81, 0x42, 0x3c }};

static void led_display_time(uint8_t hour, uint8_t minute, uint8_t second) {
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
      uint8_t data = 0;
      switch (dev) {
        case 0:
          data = numbers_5x8[hour_1st][row] | ((numbers_5x8[hour_2nd][row]) >> 5);
          break;
        case 1:
          data = numbers_5x8[hour_2nd][row] << 3 | (signs[0][row] >> 3) | ((numbers_5x8[minute_1st][row]) >> 5);
          break;
        case 2:
          data = (numbers_5x8[minute_1st][row] << 3) | (numbers_5x8[minute_2nd][row] >> 2);
          break;
        case 3:
          data = (numbers_3x5[second_1st][row]) >> 1 | ((numbers_3x5[second_2nd][row]) >> 5);
          break;
      }

      MAX72XX_SetRowOne(dev, row, data);
    }
  }
	MAX72XX_ControlAll(UPDATE, ON);
  MAX72XX_UpdateAll();
}

static void led_display_date(uint8_t month, uint8_t day_of_month, uint8_t dayofweek) {
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
          data = numbers_5x8[month_1st][row] | ((numbers_5x8[month_2nd][row]) >> 5);
          break;
        case 1:
          data = numbers_5x8[month_2nd][row] << 3 | (signs[0][row] >> 3 | numbers_5x8[day_1st][row] >> 7);
          break;
        case 2:
          data = (numbers_5x8[day_1st][row] << 1) | (numbers_5x8[day_2nd][row] >> 4);
          break;
        case 3:
          data = (numbers_5x8[day_2nd][row] << 4) | ((numbers_3x5[day_of_week == 1 ? 7 : day_of_week - 1][row]) >> 5);
          break;
      }
      MAX72XX_SetRowOne(dev, row, data);
    }
  }
	
  MAX72XX_ControlAll(UPDATE, ON);
  MAX72XX_UpdateAll();
}

//static void modify_frame_data(uint8_t *data, uint8_t row, uint8_t col, bool bit) {
//  uint8_t led_unit = col / 8;
//  uint8_t led_unit_col = col % 8;
//
//  uint8_t *led_unit_row_data = data + row * LED_UNIT_NUM + led_unit;
//
//  uint8_t bit_no = 7 - led_unit_col;
//
//  if (bit) {
//    bitSet(*led_unit_row_data, bit_no);
//  } else {
//    bitClear(*led_unit_row_data, bit_no);
//  }
//}

//static uint8_t* frame_data_row_to_col(uint8_t *row_frame_data) {
//    static uint8_t col_frame_data[FRAME_DATA_SIZE] = { 0 };
//
//    for (int unit = 0; unit < LED_UNIT_NUM; ++unit) {
//        for (int bit = 0; bit < 8; ++bit) {
//            for (int row = 0; row < 8; ++row) {
//                const uint8_t unit_row_byte = *(row_frame_data + row * LED_UNIT_NUM + unit);
//                uint8_t *col_data = &col_frame_data[row * unit];
//                //BIT_CHECK(unit_row_byte,bit) ? BIT_Set(col_data, );
//            }
//        }
//    }
//
//    return col_frame_data;
//}

//static void shift_down_time_sec_data(uint8_t *time_frame_data) {
//  const uint8_t old = *(time_frame_data + 7 * LED_UNIT_NUM + 3);
//  for (int8_t i = 6; i >= 0; i--) {
//    uint8_t *data = time_frame_data + i * LED_UNIT_NUM + 3;
//    *(time_frame_data + (i + 1) * LED_UNIT_NUM + 3) = *data;
//  }
//
//  *(time_frame_data + 0 * LED_UNIT_NUM + 3) = old;
//}
//
//static void shift_up_time_sec_data(uint8_t *time_frame_data) {
//  const uint8_t old = *(time_frame_data + 0 * LED_UNIT_NUM + 3);
//
//  for (uint8_t i = 0; i < 7; i++) {
//    uint8_t *data = time_frame_data + i * LED_UNIT_NUM + 3;
//    *data = *(time_frame_data + (i + 1) * LED_UNIT_NUM + 3);
//  }
//
//  *(time_frame_data + 7 * LED_UNIT_NUM + 3) = old;
//}

//static uint8_t* merge_sec_data(uint8_t *time_frame_data_from, uint8_t from_begin_row, uint8_t from_len, uint8_t *time_frame_data_to, uint8_t to_begin_row, uint8_t to_len) {
//    if ((one_len + two_len) > 8) {
//        return NULL;
//    }
//
//    for (int8_t i = 0; i < one_len; i++) {
//        const uint8_t row_data = *(time_frame_data_from + (one_begin_row + i) * LED_UNIT_NUM + 3);
//        merge_data[i] = *row_data;
//    }
//
//    for (int8_t i = 0; i < two_len; i++) {
//        const uint8_t row_data = *(time_frame_data_two + (two_begin_row + i) * LED_UNIT_NUM + 3);
//        merge_data[i + one_len + 1] = *row_data;
//    }
//
//    return merge_data;
//}

//static void modify_time_data_point(uint8_t *frame_data, bool display_point) {
//  modify_frame_data(frame_data, 1, 11, display_point);
//  modify_frame_data(frame_data, 2, 11, display_point);
//  modify_frame_data(frame_data, 5, 11, display_point);
//  modify_frame_data(frame_data, 6, 11, display_point);
//}

void Clock_Init(void) {
  DS3231_Init();
  MAX72XX_Init();
}

void Clock_ShowTime(void) {
  led_display_time(rtc.Hour, rtc.Min, rtc.Sec);
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
