/*
 * utils.h
 *
 *  Created on: Dec 11, 2020
 *      Author: linhao
 */

#ifndef UTILS_H_
#define UTILS_H_

#define BIT_SET(a,b) ((a) |= ((uint8_t)1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~((uint8_t)1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= ((uint8_t)1<<(b)))
#define BIT_CHECK(a,b) (!!((a) & ((uint8_t)1<<(b))))

/* x=target variable, y=mask */
#define BITMASK_SET(x,y) ((x) |= (y))
#define BITMASK_CLEAR(x,y) ((x) &= (~(y)))
#define BITMASK_FLIP(x,y) ((x) ^= (y))
#define BITMASK_CHECK_ALL(x,y) (!(~(x) & (y)))
#define BITMASK_CHECK_ANY(x,y) ((x) & (y))


#endif /* UTILS_H_ */
