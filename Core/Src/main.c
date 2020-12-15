/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "adc.h"
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "clock.h"
#include "ds3231.h"
#include "max72xx.h"
#include "ticktimer.h"
#include "usbd_cdc_if.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

enum {
  STATE_SHOW_TIME = 0,
  STATE_TIME_SEC_CHANGED = 1,
  STATE_TIME_SEC_JUMP_UP = 2,
  STATE_TIME_SEC_JUMP_DOWN = 3,
  STATE_SHOW_DATE = 4,
};

typedef struct {
  int state;
  int times;
} State;

static inline void SetState(State *s, uint8_t state, uint16_t times) {
  s->state = state;
  s->times = times;
}

static inline bool TickState(State *s) { return !s->times || --s->times; }

static State s = {0};

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */

int main(void) {
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  Clock_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  //  for (int i = 0; i < 2; i++) {
  //    MAX72XX_SetRowOne(i, 0, 0B01010101);
  //    MAX72XX_SetRowOne(i, 1, 0B10101010);
  //    MAX72XX_SetRowOne(i, 2, 0B01010101);
  //    MAX72XX_SetRowOne(i, 3, 0B10101010);
  //    MAX72XX_SetRowOne(i, 4, 0B01010101);
  //    MAX72XX_SetRowOne(i, 5, 0B10101010);
  //    MAX72XX_SetRowOne(i, 6, 0B01010101);
  //    MAX72XX_SetRowOne(i, 7, 0B10101010);
  //  }
  //    for (int i = 0; i < 4; i++) {
  //      MAX72XX_SetRowOne(i, 0, 0B00000000);
  //      MAX72XX_SetRowOne(i, 1, 0B01111110);
  //      MAX72XX_SetRowOne(i, 2, 0B01000010);
  //      MAX72XX_SetRowOne(i, 3, 0B01011010);
  //      MAX72XX_SetRowOne(i, 4, 0B01011010);
  //      MAX72XX_SetRowOne(i, 5, 0B01000010);
  //      MAX72XX_SetRowOne(i, 6, 0B01111110);
  //      MAX72XX_SetRowOne(i, 7, 0B00000000);
  //    }
  //    for (int i = 0; i < 4; i++) {
  //      MAX72XX_SetRowOne(i, 0, 0B11111111);
  //      MAX72XX_SetRowOne(i, 1, 0B11011011);
  //      MAX72XX_SetRowOne(i, 2, 0B10100101);
  //      MAX72XX_SetRowOne(i, 3, 0B11011011);
  //      MAX72XX_SetRowOne(i, 4, 0B11011011);
  //      MAX72XX_SetRowOne(i, 5, 0B10100101);
  //      MAX72XX_SetRowOne(i, 6, 0B11011011);
  //      MAX72XX_SetRowOne(i, 7, 0B11111111);
  //    }
  //  for (int i = 0; i < 4; i++) {
  //    MAX72XX_SetRowOne(i, 0, 0B00001000);
  //    MAX72XX_SetRowOne(i, 1, 0B00000100);
  //    MAX72XX_SetRowOne(i, 2, 0B00000010);
  //    MAX72XX_SetRowOne(i, 3, 0B11111111);
  //    MAX72XX_SetRowOne(i, 4, 0B00000010);
  //    MAX72XX_SetRowOne(i, 5, 0B00000100);
  //    MAX72XX_SetRowOne(i, 6, 0B00001000);
  //    MAX72XX_SetRowOne(i, 7, 0B00000000);
  //  }
  //  uint64_t image = 0xffe799e799e799ff;
  //  MAX72XX_SetRowOne(3, 0,0x1);
  //  MAX72XX_UpdateAll();
  SetState(&s, STATE_SHOW_TIME, 0);

  static bool time_show_point = true;
  static TickTimer show_date_ticktimer = {
      .autoreload = true, .interval = 59999, .lasttick = 0};
  static TickTimer update_rtc_ticktimer = {
      .autoreload = true, .interval = 49, .lasttick = 0};
  static TickTimer flash_point_ticktimer = {
      .autoreload = true, .interval = 499, .lasttick = 0};
  static TickTimer frame_ticktimer = {
      .autoreload = true, .interval = 99, .lasttick = 0};

  while (1) {
    Clock_TestLedMatrix();


    Clock_ShowTime(1);

    HAL_Delay(1000);
    /*
    const uint32_t tick = HAL_GetTick();

    do {
      if (!TickTimer_IsExpired(&frame_ticktimer, tick)) {
        break;
      }

      if (TickTimer_IsExpired(&show_date_ticktimer, tick)) {
        SetState(&s, STATE_SHOW_DATE, 10);
        break;
      }

      if (s.state != STATE_SHOW_DATE &&
          TickTimer_IsExpired(&update_rtc_ticktimer, tick) &&
          Clock_UpdateRTC()) {
        SetState(&s, STATE_TIME_SEC_CHANGED, 1);
        break;
      }

      switch (s.state) {
        case STATE_SHOW_TIME:
          Clock_ShowTime(time_show_point);
          break;
        case STATE_TIME_SEC_CHANGED:
          Clock_ShowTime(time_show_point);
          if (!TickState(&s)) {
            SetState(&s, STATE_TIME_SEC_JUMP_UP, SECOND_JUMP_TIMES);
          }
          break;
        case STATE_TIME_SEC_JUMP_UP:
          Clock_SecondJumpUp();
          if (!TickState(&s)) {
            SetState(&s, STATE_TIME_SEC_JUMP_DOWN, SECOND_JUMP_TIMES);
          }
          break;
        case STATE_TIME_SEC_JUMP_DOWN:
          Clock_SecondJumpDown();
          if (!TickState(&s)) {
            SetState(&s, STATE_SHOW_TIME, 0);
          }
          break;
        case STATE_SHOW_DATE:
          Clock_ShowDate();
          if (!TickState(&s)) {
            SetState(&s, STATE_SHOW_TIME, 0);
          }
          break;
      }
    } while (false);

    if (TickTimer_IsExpired(&flash_point_ticktimer, tick)) {
      Clock_FlashTimePoint(time_show_point);
      time_show_point = !time_show_point;
    }
*/
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC | RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
