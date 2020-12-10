/*
 * max7219.c
 *
 *  Created on: May 11, 2019
 *      Author: tabur
 */

#include "max7219.h"
#include "main.h"

#define CS_SET() 	HAL_GPIO_WritePin(CS_MAX7219_GPIO_Port, CS_MAX7219_Pin, GPIO_PIN_RESET)
#define CS_RESET() 	HAL_GPIO_WritePin(CS_MAX7219_GPIO_Port, CS_MAX7219_Pin, GPIO_PIN_SET)

extern SPI_HandleTypeDef hspi1;

static const uint8_t numbers_5x8[][8] = {
    { 0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70 },  //0
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

static const uint8_t numbers_3x5[10][8] = {
    { 0x00, 0x00, 0x00, 0xe0, 0xa0, 0xa0, 0xa0, 0xe0 },  //0
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

static const uint8_t signs[][8] = {
    { 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x00 },  //:
    { 0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c },  //笑脸
    { 0x3c, 0x42, 0xa5, 0x81, 0xbd, 0x81, 0x42, 0x3c },  //标准脸
    {
      0B00000000,
      0B00000000,
      0B00000000,
      0B00000000,
      0B11000000,
      0B00000000,
      0B00000000,
      0B00000000
    },//-
    };

const uint8_t numbers_b[][8] =
    {
        {
            0B01110000,
            0B10001000,
            0B10001000,
            0B10001000,
            0B10001000,
            0B10001000,
            0B10001000,
            0B01110000
        },
        {
            0B00100000,
            0B01100000,
            0B00100000,
            0B00100000,
            0B00100000,
            0B00100000,
            0B00100000,
            0B01110000
        },
        {
            0B01110000,
            0B10001000,
            0B00001000,
            0B00001000,
            0B00010000,
            0B00100000,
            0B01000000,
            0B11111000
        },
        {
            0B01110000,
            0B10001000,
            0B00001000,
            0B00110000,
            0B00001000,
            0B00001000,
            0B10001000,
            0B01110000
        },
        {
            0B00001000,
            0B00011000,
            0B00101000,
            0B01001000,
            0B10001000,
            0B11111000,
            0B00001000,
            0B00001000
        },
        {
            0B11111000,
            0B10000000,
            0B10000000,
            0B11110000,
            0B00001000,
            0B00001000,
            0B10001000,
            0B01110000
        },
        {
            0B01110000,
            0B10001000,
            0B10000000,
            0B11110000,
            0B10001000,
            0B10001000,
            0B10001000,
            0B01110000
        },
        {
            0B11111000,
            0B00001000,
            0B00001000,
            0B00010000,
            0B00100000,
            0B01000000,
            0B01000000,
            0B01000000
        },
        {
            0B01110000,
            0B10001000,
            0B10001000,
            0B01110000,
            0B10001000,
            0B10001000,
            0B10001000,
            0B01110000
        },
        {
            0B01110000,
            0B10001000,
            0B10001000,
            0B10001000,
            0B01111000,
            0B00001000,
            0B10001000,
            0B01110000
        }
    };

static void Max7219_Welcome(void);

static void Max7219_Welcome(void) {
  for (int i = 0; i < 8; i++) {
    CS_SET();
    for (int j = 0; j < 4; j++) {
      Max7219_WriteByte(i + 1, 0x00);
    }
    CS_RESET();
  }

  HAL_Delay(200);

  for (int i = 0; i < 8; i++) {
    CS_SET();
    for (int j = 0; j < 4; j++) {
      Max7219_WriteByte(i + 1, signs[1][i]);
    }
    CS_RESET();
  }

  HAL_Delay(300);
}

void Max7219_Init(void) {
  for (int i = 0; i < LED_NUM; i++) {
    Max7219_SendData(REG_SHUTDOWN, 0x01);
    Max7219_SendData(REG_DECODE_MODE, 0x00);
    Max7219_SendData(REG_INTENSITY, DEFAULT_INTENSIVITY);
    Max7219_SendData(REG_SCAN_LIMIT, 0x07);
    Max7219_SendData(REG_DISPLAY_TEST, 0x00);
  }

  HAL_Delay(1000);

  Max7219_Welcome();
}

void Max7219_SetIntensivity(uint8_t intensivity) {
  Max7219_SendData(REG_INTENSITY, intensivity > 0x0f ? 0x0f : intensivity);
}

void Max7219_SendData(uint8_t addr, uint8_t data) {
  CS_SET();
  Max7219_WriteByte(addr, data);
  CS_RESET();
}

void Max7219_WriteByte(uint8_t addr, uint8_t data) {
  HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
  HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}

void Max7219_ShowNumbers(void) {
  for (int i = 0; i < 8; i++) {
    CS_SET();
    for (int j = 0; j < 4; j++) {
      Max7219_WriteByte(i + 1, numbers_b[j][i]);
    }
    CS_RESET();
  }
}

static uint8_t old_second = 0;
static bool display_point = true;

void Max7219_ShowTime(uint8_t hour, uint8_t minute, uint8_t second, uint32_t tick) {
  const uint8_t hour_1st = hour / 10;
  const uint8_t hour_2nd = hour % 10;

  const uint8_t minute_1st = minute / 10;
  const uint8_t minute_2nd = minute % 10;

  const uint8_t second_1st = second / 10;
  const uint8_t second_2nd = second % 10;

  uint8_t data = 0;
  const uint8_t second_changed = (old_second != second);


  if(!(tick % 10)){
    display_point = !display_point;
  }

  for (uint8_t i = 0; i < 8; i++) {
    CS_SET();
    for (uint8_t j = 0; j < LED_NUM; j++) {
      switch (j) {
        case 0:
          data = numbers_5x8[hour_1st][i] | ((numbers_5x8[hour_2nd][i]) >> 5);
          break;
        case 1:
          data = numbers_5x8[hour_2nd][i] << 3 | ((display_point ? signs[0][i] : 0) >> 3) | ((numbers_5x8[minute_1st][i]) >> 5);
          break;
        case 2:
          data = (numbers_5x8[minute_1st][i] << 3) | (numbers_5x8[minute_2nd][i] >> 2);
          break;
        case 3:
          data = second_changed ? 0 : (numbers_3x5[second_1st][i]) >> 1 | ((numbers_3x5[second_2nd][i]) >> 5);
          break;
      }

      Max7219_WriteByte(i + 1, data);
    }
    CS_RESET();
  }

  old_second = second;
}

void Max7219_ShowDate(uint8_t month, uint8_t day,uint8_t week) {
  const uint8_t month_1st = month / 10;
  const uint8_t month_2nd = month % 10;

  const uint8_t day_1st = day / 10;
  const uint8_t day_2nd = day % 10;

  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
     CS_SET();
     for (uint8_t j = 0; j < LED_NUM; j++) {
       switch (j) {
              case 0:
                data = numbers_5x8[month_1st][i] | ((numbers_5x8[month_2nd][i]) >> 5);
                break;
              case 1:
                data = numbers_5x8[month_2nd][i] << 3 | (signs[3][i] >> 3) | ((numbers_5x8[day_1st][i]) >> 6);
                break;
              case 2:
                data = (numbers_5x8[day_1st][i] << 2) | (numbers_5x8[day_2nd][i] >> 3);
                break;
              case 3:
                data = ((numbers_3x5[week][i]) >> 4);
                break;
            }

            Max7219_WriteByte(i + 1, data);
     }
     CS_RESET();
  }
}
