/*
 * key.h
 *
 *  Created on: Dec 10, 2020
 *      Author: linhao
 */

#ifndef KEY_H_
#define KEY_H_

#include "main.h"
#include <stdbool.h>

bool KEY1_StateRead(void)
{
  /* 读取此时按键值并判断是否是被按下状态，如果是被按下状态进入函数内 */
  if (!HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin))
      {
    /* 延时一小段时间，消除抖动 */
    HAL_Delay(10);
    /* 延时时间后再来判断按键状态，如果还是按下状态说明按键确实被按下 */
    if (!HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin))
        {
      /* 等待按键弹开才退出按键扫描函数 */
      while (!HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin))
        ;
      /* 按键扫描完毕，确定按键被按下，返回按键被按下状态 */
      return true;
    }
  }
  /* 按键没被按下，返回没被按下状态 */
  return false;
}

#endif /* KEY_H_ */
