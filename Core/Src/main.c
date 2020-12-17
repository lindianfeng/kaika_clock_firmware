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
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
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

int __io_putchar(int ch)
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART1 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart1, (uint8_t*) &ch, 1, 0xFFFF);
  return ch;
}

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
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  Clock_Init();

  printf("Clock Init Completed!\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  static tick_timer show_date_ticktimer;
  static tick_timer update_rtc_ticktimer;
  static tick_timer flash_point_ticktimer;
  static tick_timer reader_ticktimer;

  timer_init(&show_date_ticktimer, on_show_date_ticktimer_timeout, 59999, 59999);
  timer_init(&update_rtc_ticktimer, on_update_rtc_ticktimer_timeout, 499, 499);
  timer_init(&flash_point_ticktimer, on_flash_point_ticktimer_timeout, 499, 499);
  timer_init(&reader_ticktimer, on_reader_ticktimer_timeout, 99, 99);

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

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
  RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
      {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
      | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
      {
    Error_Handler();
  }
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
      {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
      {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
      {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CS_MAX7219_GPIO_Port, CS_MAX7219_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LD4_Pin | LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : CS_MAX7219_Pin */
  GPIO_InitStruct.Pin = CS_MAX7219_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_MAX7219_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin */
  GPIO_InitStruct.Pin = LD4_Pin | LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

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
