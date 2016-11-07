/*
 * Common.h
 *
 *  Created on: Nov 5, 2016
 *      Author: gocha
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

typedef enum
{
	left,
	right
} ShiftDirection;

void PrintData(uint8_t *buffer, int length, int characterPosition);
void ShiftRegister(uint8_t *ShiftBuffer, uint8_t *ShiftedBuffer, int lengthOfBuffer, ShiftDirection shiftDirection, int shiftCount);

#endif /* COMMON_H_ */