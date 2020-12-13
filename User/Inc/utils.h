/*
 * utils.h
 *
 *  Created on: Dec 11, 2020
 *      Author: linhao
 */

#ifndef UTILS_H_
#define UTILS_H_

#define MAX_DEBUG 0   ///< Enable or disable (default) debugging output from the MD_MAX72xx library

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))


#endif /* UTILS_H_ */
