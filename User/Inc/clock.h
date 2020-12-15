/*
 * clock.h
 *
 *  Created on: Dec 10, 2020
 *      Author: linhao
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <stdint.h>
#include <stdbool.h>

#define SECOND_JUMP_TIMES  2
void Clock_Init(void);
bool Clock_UpdateRTC(void);
void Clock_ShowTime(bool b);
void Clock_ShowDate(void);
void Clock_FlashTimePoint(bool b);
void Clock_SecondJumpUp(void);
void Clock_SecondJumpDown(void);
void Clock_TestLedMatrix();

#endif /* CLOCK_H_ */
