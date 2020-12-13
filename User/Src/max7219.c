/*
 * max7219.c
 *
 *  Created on: May 11, 2019
 *      Author: tabur
 */

#include "max7219.h"
#include <string.h>
#include "main.h"
#include <stdio.h>

#define CS_SET() 	HAL_GPIO_WritePin(CS_MAX7219_GPIO_Port, CS_MAX7219_Pin, GPIO_PIN_RESET)
#define CS_RESET() 	HAL_GPIO_WritePin(CS_MAX7219_GPIO_Port, CS_MAX7219_Pin, GPIO_PIN_SET)

extern SPI_HandleTypeDef hspi1;
//extern TIM_HandleTypeDef htim2;

static uint8_t volatile frame_data_changed = false;

static uint8_t frame_data[FRAME_DATA_SIZE] = { 0 };

static void Max7219_Welcome(void);
static void Max7219_SendData(uint8_t addr, uint8_t data);
static void Max7219_WriteByte(uint8_t addr, uint8_t data);

static void Max7219_Welcome(void) {
    static uint8_t welcome_data[] = {
        0B10101010,
        0B01010101,
        0B10101010,
        0B01010101,
        0B10101010,
        0B01010101,
        0B10101010,
        0B01010101
    };

    for (int i = 0; i < 8; i++) {
        CS_SET();
        for (int j = 0; j < 4; j++) {
            Max7219_WriteByte(i + 1, 0xff);
        }
        CS_RESET();
    }

    HAL_Delay(500);

    for (int i = 0; i < 8; i++) {
        CS_SET();
        for (int j = 0; j < 4; j++) {
            Max7219_WriteByte(i + 1, 0x00);
        }
        CS_RESET();
    }

    HAL_Delay(500);

    for (int i = 0; i < 8; i++) {
        CS_SET();
        for (int j = 0; j < 4; j++) {
            Max7219_WriteByte(i + 1, welcome_data[i]);
        }
        CS_RESET();
    }

    HAL_Delay(1000);
}

void Max7219_Init(void) {
    for (int i = 0; i < LED_UNIT_NUM; i++) {
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

static void Max7219_SendData(uint8_t addr, uint8_t data) {
    CS_SET();
    Max7219_WriteByte(addr, data);
    CS_RESET();
}

static void Max7219_WriteByte(uint8_t addr, uint8_t data) {
    HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}

void Max7219_SetData(const uint8_t *data, uint32_t len) {
    if (!data) {
        return;
    }

    if (len != 8 * LED_UNIT_NUM) {
        return;
    }

    memcpy(frame_data, data, len);
    frame_data_changed = true;
}

void Max7219_Render(void) {
    if (!frame_data_changed) {
        return;
    }

    for (uint8_t i = 0; i < 8; i++) {
        CS_SET();
        for (uint8_t j = 0; j < LED_UNIT_NUM; j++) {
            Max7219_WriteByte(i + 1, *(frame_data + i * LED_UNIT_NUM + j));
        }
        CS_RESET();
    }

    frame_data_changed = false;
}

void Max7219_RenderData(const uint8_t *data, uint32_t len) {
    if (!data) {
        return;
    }

    Max7219_SetData(data, len);
    Max7219_Render();
}

//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
//    if (htim == (&htim2)) {
//        Max7219_Render();
//    }
//}
