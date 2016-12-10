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

extern int generatorMatrix[8][12];
extern int transposeMatrix[12][4];

#ifdef EncryptedCommunication
extern bool encryptEntireData;
#endif

// This function creates the initial sync stream (Size : 64 bytes)
void CreateSyncStream()
{
	int dataCounter;
	uint8_t firstRepeatPattern = 0x5;
	uint8_t secondRepeatPattern = 0xA;
	transmitBufferLength = 0;

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

#ifdef LinearBlockCoding
// Considering fixed value of n and k
void EncodeUsingLinearBlockCoding()
{
	int nVal = 12, kVal = 8;
	uint8_t LBCEncodedBytes[45];				// Size is 45 since the max size of input data is 30 bytes. Should be 1.5 times of input data
	int nCount, kCount, byteCount, encodedFirstByteCount, encodedSecondByteCount, middleVarSum;

	for(byteCount = 0; byteCount < (transmitDataLength + transmitDataLength/2); byteCount++)
	{
		LBCEncodedBytes[byteCount] = 0;
	}

	for(byteCount = 0; byteCount < transmitDataLength; byteCount++)
	{
		encodedFirstByteCount = byteCount + (byteCount/2);
		encodedSecondByteCount = byteCount + (byteCount/2) + 1;

		// Encoding using generator matrix
		for(nCount = 0; nCount < nVal; nCount++)
		{
			middleVarSum = 0;
			for(kCount = 0; kCount < kVal; kCount++)
			{
				// Performing matrix multiplication of input and Generator matrix bits
				middleVarSum = middleVarSum ^ (((TransmittedData[byteCount] >> (7 - kCount)) & 0x01) * generatorMatrix[kCount][nCount]);
			}

			if(byteCount % 2 == 0)
			{
				if(nCount >= 0 && nCount < 8)
				{
					LBCEncodedBytes[encodedFirstByteCount] |= middleVarSum << (7 - nCount);
				}
				else
				{
					LBCEncodedBytes[encodedSecondByteCount] |= middleVarSum << (15 - nCount);
				}
			}
			else
			{
				if(nCount >= 0 && nCount < 4)
				{
					LBCEncodedBytes[encodedFirstByteCount] |= middleVarSum << (3 - nCount);
				}
				else
				{
					LBCEncodedBytes[encodedSecondByteCount] |= middleVarSum << (11 - nCount);
				}
			}
		}
	}
	transmitDataLength = encodedSecondByteCount + 1;
	// Finally, dumping all the data in Transmitted data again
	for(byteCount = 0; byteCount < transmitDataLength; byteCount++)
	{
		TransmittedData[byteCount] = LBCEncodedBytes[byteCount];
	}
}
#endif
