/*
 * delay.h
 *
 *  Created on: Dec 10, 2020
 *      Author: linhao
 */

#ifndef DELAY_H_
#define DELAY_H_

#include "stm32f1xx_hal.h"

extern TIM_HandleTypeDef htim4;

void delay_us(uint16_t us)
{
  __HAL_TIM_SET_COUNTER(&htim4, 0);

  HAL_TIM_Base_Start(&htim4);

  while (__HAL_TIM_GET_COUNTER(&htim4) != us);

  HAL_TIM_Base_Stop(&htim4);
}

#endif /* DELAY_H_ */
