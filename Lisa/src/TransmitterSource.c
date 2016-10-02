/*
 * TransmitterSource.c
 *
 *  Created on: Sep 20, 2016
 *      Author: gocha
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <TransmitterSource.h>
#include <AllDefs.h>

// General Defs
extern char transmitBuffer[50];				// 8 bytes of initial sync and then next is the data. Assume 8 + 8
extern char transmitData[30];
extern int transmitBufferLength, transmitDataLength;
extern int sizeOfsyncField;

// This function creates the initial sync stream (Size : 64 bytes)
void CreateSyncStream()
{
	int dataCounter;
	uint8_t firstRepeatPattern = 0x5;
	uint8_t secondRepeatPattern = 0xA;

	for(dataCounter = 0; dataCounter < sizeOfsyncField; dataCounter++)
	{
		// Append the repeating pattern 0x5x
		if(dataCounter < 16)
		{
			transmitBuffer[dataCounter] = ((firstRepeatPattern << 4) & 0xF0) | (dataCounter & 0x0F);
			transmitBufferLength++;
		}
		// Append the repeating pattern 0xAx
		else
		{
			transmitBuffer[dataCounter] = ((secondRepeatPattern << 4) & 0xF0) | (dataCounter & 0x0F);
			transmitBufferLength++;
		}
	}
}

void AppendUserData(char *transmitDataAppend)
{
	int counter;

	for(counter = 0; counter < transmitDataLength; counter++)
	{
		transmitBuffer[transmitBufferLength] = (transmitDataAppend[counter] & 0xFF);
		transmitBufferLength++;
	}
}

// This function transmits the data to the wire
void TransmitData()
{
	int counter = 0, bitCount = 8, bitCounter = 0;
	uint8_t pinValue;

	for(counter = 0; counter < transmitBufferLength; counter++)
	{
		for(bitCounter = bitCount-1; bitCounter >= 0; bitCounter--)
		{
			pinValue = (transmitBuffer[counter] >> bitCounter) & 0x01;
			TransmitPinValue = (pinValue << 6);
		}
	}
}
