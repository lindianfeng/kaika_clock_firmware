/*
 * dht11.h
 *
 *  Created on: Jan 2, 2021
 *      Author: lindi
 */

#ifndef INC_DHT11_H_
#define INC_DHT11_H_

#include "main.h"

#define DHT11_OUT_1       HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_SET)
#define DHT11_OUT_0       HAL_GPIO_WritePin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin, GPIO_PIN_RESET)

#define DHT11_IN          HAL_GPIO_ReadPin(DHT11_PIN_GPIO_Port, DHT11_PIN_Pin)

typedef struct
{
  uint8_t humi_int;       // 湿度的整数部分
  uint8_t humi_deci;      // 湿度的小数部分
  uint8_t temp_int;       // 温度的整数部分
  uint8_t temp_deci;      // 温度的小数部分
  uint8_t check_sum;      // 校验和

} DHT11_Data_TypeDef;



#endif /* INC_DHT11_H_ */
