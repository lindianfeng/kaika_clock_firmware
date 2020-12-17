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
#include "i2c.h"
#include "spi.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "clock.h"
#include "ds3231.h"
#include "max72xx.h"
#include "ticktimer.h"
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
/* USER CODE BEGIN 0 */
#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
 set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

//int PUTCHAR_PROTOTYPE(int ch)
//{
//  /* Place your implementation of fputc here */
//  /* e.g. write a character to the USART1 and Loop until the end of transmission */
//  HAL_UART_Transmit(&huart1, (uint8_t*) &ch, 1, 0xFFFF);
//  return ch;
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

static State clock_states[3] = {
    {
        .fired = false,
        .state = STATE_CLOCK_TIME,
        .timeout = 10,
        .repeat = -1,
        .starttick = 0,
        .callback = Clock_ShowTime
    },
    {
        .fired = false,
        .state = STATE_CLOCK_DATE,
        .timeout = 2000,
        .repeat = 1,
        .starttick = 0,
        .callback = Clock_ShowDate
    },
    {
        .fired = false,
        .state = STATE_CLOCK_TEMP,
        .timeout = 0,
        .repeat = 0,
        .starttick = 0,
        .callback = 0
    }
};

static State time_states[4] = {
    {
        .fired = false,
        .state = STATE_TIME_SHOW,
        .timeout = 10,
        .repeat = -1,
        .starttick = 0,
        .callback = Clock_ShowTime
    },
    {
        .fired = false,
        .state = STATE_TIME_SEC_CHANGED,
        .timeout = 0,
        .repeat = 0,
        .starttick = 0,
        .callback = NULL
    },
    {
        .fired = false,
        .state = STATE_TIME_SEC_JUMP_UP,
        .timeout = 100,
        .repeat = 2,
        .starttick = 0,
        .callback = Clock_SecondJumpUp
    },
    {
        .fired = false,
        .state = STATE_TIME_SEC_JUMP_DOWN,
        .timeout = 100,
        .repeat = 2,
        .starttick = 0,
        .callback = Clock_SecondJumpDown
    }
};

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
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
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

  printf("Clock Init Completed!\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  static tick_timer show_date_ticktimer = { .autoreload = true, .interval = 59999, .lasttick = 0 };
  static tick_timer update_rtc_ticktimer = { .autoreload = true, .interval = 49, .lasttick = 0 };
  static tick_timer flash_point_ticktimer = { .autoreload = true, .interval = 499, .lasttick = 0 };
  static tick_timer reader_ticktimer = { .autoreload = true, .interval = 10, .lasttick = 0 };

  clock_s = clock_states[0];
  time_s = time_states[0];

  while (1)
  {
    const uint32_t tick = HAL_GetTick();

    if (TickTimer_IsExpired(&show_date_ticktimer, tick)) {
      ChangeClockState(STATE_CLOCK_DATE, tick);
    }

    switch (clock_s.state) {
      case STATE_CLOCK_TIME: {
        if (TickTimer_IsExpired(&update_rtc_ticktimer, tick) && Clock_UpdateRTC()) {
          ChangeTimeState(STATE_TIME_SEC_CHANGED, tick);
        }

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

    if (TickTimer_IsExpired(&flash_point_ticktimer, tick)) {
      Clock_ToggleSecPoint();
    }

    if (TickTimer_IsExpired(&reader_ticktimer, tick)) {
      Clock_UpdateDiplay();
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
