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
extern char TransmitBuffer[50];				// 8 bytes of initial sync and then next is the data. Assume 8 + 8
extern char TransmittedData[30];
extern int transmitBufferLength, transmitDataLength;
extern int sizeOfsyncField;
extern int transmitBufferCounter, transmitBitCounter;

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
			TransmitBuffer[dataCounter] = ((firstRepeatPattern << 4) & 0xF0) | (dataCounter & 0x0F);
			transmitBufferLength++;
		}
		// Append the repeating pattern 0xAx
		else
		{
			TransmitBuffer[dataCounter] = ((secondRepeatPattern << 4) & 0xF0) | (dataCounter & 0x0F);
			transmitBufferLength++;
		}
	}
}

void AppendUserData(char *transmitDataAppend)
{
	int counter;

	for(counter = 0; counter < transmitDataLength; counter++)
	{
		TransmitBuffer[transmitBufferLength] = (transmitDataAppend[counter] & 0xFF);
		transmitBufferLength++;
	}
}

// This function transmits the data to the wire
void TransmitData()
{
	//int counter = 0, bitCount = 8, bitCounter = 0;
	//int transmitBufferCounter, transmitBitCounter;

	uint8_t pinValue;

	if(transmitBitCounter >= 0)
	{
		pinValue = (TransmitBuffer[transmitBufferCounter] >> transmitBitCounter) & 0x01;
		TransmitPinValue = (pinValue << 6);
	}
	transmitBitCounter--;


	if(transmitBitCounter == 0)
	{
		transmitBufferCounter++;
		transmitBitCounter = 7;
	}

	/*for(counter = 0; counter < transmitBufferLength; counter++)s
	{
		for(bitCounter = bitCount-1; bitCounter >= 0; bitCounter--)
		{
			pinValue = (TransmitBuffer[counter] >> bitCounter) & 0x01;
			TransmitPinValue = (pinValue << 6);

		}
	}
*/
}
