/*
 * delay.h
 *
 *  Created on: Dec 10, 2020
 *      Author: linhao
 */

#ifndef DELAY_H_
#define DELAY_H_

 #include "stm32f1xx_hal.h"

void delay_us(uint32_t us)
{
    uint32_t delay = (HAL_RCC_GetHCLKFreq() / 4000000 * us);
    while (delay--)
  {
    ;
  }
}


#endif /* DELAY_H_ */
