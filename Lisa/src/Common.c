/*
 * Common.c
 *
 *  Created on: Nov 5, 2016
 *      Author: Gaurao Chaudhari
 */

#include <Common.h>
#include <AllDefs.h>

#if defined(Transmit) || defined(Receive)
void PrintData(uint8_t *buffer, int length, int characterPosition)
{
	int counter;
	//	int totalLengthOfData = characterPosition + buffer[characterPosition + 1] + 1;
	printf("\n");
	for(counter = 0; counter < length; counter++)
	{
		if(characterPosition == 0)
		{
			printf("%x", buffer[counter]);
		}

		if(counter < characterPosition)
		{
			printf("%x", buffer[counter]);
		}
		else if((counter > characterPosition) && (counter < length))
		{
			printf("%c", buffer[counter]);
		}
	}
}
#endif

// For maximum 127 shiftCount
void ShiftRegister(uint8_t *ShiftBuffer, uint8_t *ShiftedBuffer, int lengthOfBuffer, ShiftDirection shiftDirection, int shiftCount)
{
	int counter = 0;
	uint8_t copyHandle = 0, pasteHandle = 0;
	uint8_t localCharHandle = 0;
	int8_t setZeroCount = 0;

	// Setting the zero indexes for shift more than 7 bits
	if(shiftCount/8 > 0)
	{
		setZeroCount = shiftCount/8;
		shiftCount = shiftCount % 8;				// Setting the actual shift count now which is between 0 and 7
	}
	else
	{
		setZeroCount = 0;
	}

	for(counter = 0; counter < lengthOfBuffer; counter++)
	{
		if(shiftDirection == right)
		{
			if((setZeroCount > 0) && counter < setZeroCount)
			{
				ShiftedBuffer[counter] = 0;
			}
			else
			{
				localCharHandle = ShiftBuffer[counter - setZeroCount];
				copyHandle = 0;
				copyHandle = localCharHandle << (8 - shiftCount);				// 8 for number of bits in a byte
				localCharHandle = (localCharHandle >> shiftCount);
				localCharHandle = localCharHandle | pasteHandle;
				pasteHandle = copyHandle;
				ShiftedBuffer[counter] = localCharHandle;
			}
		}
		else
		{
			// To change this. Not yet complete.
			/*			copyHandle = ShiftBuffer[counter] << (8 - shiftCount);				// 8 for number of bits in a byte
			ShiftBuffer[counter] = (ShiftBuffer[counter]  >> shiftCount);
			ShiftBuffer[counter] |= (pasteHandle << (8 - shiftCount));
			pasteHandle = copyHandle;*/
		}
	}
}
