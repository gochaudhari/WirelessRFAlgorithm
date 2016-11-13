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
#include <stdlib.h>
#include <Common.h>
#include <string.h>

// General Defs
extern int maxTransmitData;
extern char TransmitBuffer[50];				// 8 bytes of initial sync and then next is the data. Assume 8 + 8
extern char TransmittedData[30];
extern int transmitBufferLength, transmitDataLength;
extern int sizeOfsyncField;
extern int transmitBufferCounter, transmitBitCounter;
extern int bitCount;
extern int dataLengthByte;
extern bool binaryDataFormat, characterDataFormat;

#ifdef EncryptedCommunication
extern bool encryptEntireData;
#endif

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
	uint8_t pinValue;

	pinValue = (TransmitBuffer[transmitBufferCounter] >> transmitBitCounter) & 0x01;
	TransmitPinValue = (pinValue << 6);
	transmitBitCounter--;


	if(transmitBitCounter == -1)
	{
		transmitBufferCounter++;
		transmitBitCounter = 7;
	}
}

#ifdef EncryptedCommunication
// This function does the XOR of the present data to any random digit
void EncryptTransmitSyncField()
{
	uint8_t randomByte = rand();
	int byteCounter = 0, encryptionLength = 0;

	if(encryptEntireData)
	{
		encryptionLength = transmitBufferLength;
	}
	else
	{
		encryptionLength = 32;
	}

	for(byteCounter = 0; byteCounter < encryptionLength; byteCounter++)
	{
		TransmitBuffer[byteCounter] = TransmitBuffer[byteCounter] ^ randomByte;
	}
}
#endif

#ifdef ScramblingAndDescrambling
// This function scrambles data to a given order
void ScrambleData(int scrambleAndDescrambleOrder){

	int counter = 0;
	int bitCounter = 0;
	uint8_t *shiftByFirstPos, *shiftBySecondPos;
	uint8_t *ScrambledData;
	int scramblePos = scrambleAndDescrambleOrder/2;
	scramblePos = scramblePos + 1;

	ScrambledData = (uint8_t *)malloc(sizeof(uint8_t) * transmitDataLength);
	shiftByFirstPos = (uint8_t *)malloc(sizeof(uint8_t) * transmitDataLength);
	shiftBySecondPos = (uint8_t *)malloc(sizeof(uint8_t) * transmitDataLength);

	// Adding all these three data's and then getting the final buffer data

	for(counter = 0; counter < transmitDataLength; counter++)
	{
		for(bitCounter = bitCount; bitCounter>0 ; bitCounter--)
		{
			ShiftRegister(ScrambledData, shiftByFirstPos, transmitDataLength, right, scramblePos);
			ShiftRegister(ScrambledData, shiftBySecondPos, transmitDataLength, right, scrambleAndDescrambleOrder);
			ScrambledData[counter] = ((shiftByFirstPos[counter] ^ shiftBySecondPos[counter]) ^ TransmittedData[counter]);
		}
	}

	for(counter = 0; counter < transmitDataLength; counter++)
	{
		TransmittedData[counter] = ScrambledData[counter];
	}

	free(ScrambledData);
	ScrambledData = NULL;
	free(shiftByFirstPos);
	shiftByFirstPos = NULL;
	free(shiftBySecondPos);
	shiftBySecondPos = NULL;
	//PrintData((uint8_t *)TransmittedData, transmitDataLength, 3);
}
#endif
