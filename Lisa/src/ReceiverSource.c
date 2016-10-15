/*
 * ReceiverSource.c
 *
 *  Created on: Sep 20, 2016
 *      Author: Gaurao Chaudhari
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <AllDefs.h>
#include <ReceiverSource.h>
#include <stdint.h>
#include <string.h>

extern char ReceiveBuffer[1024];
extern uint8_t Buffer[1024];
extern int receiveBufferLength, receiveDataLength;
extern bool bitReceived, dataReceived;
extern int receiverBufferCounter, bitCount, receiverBitCounter;
extern char TransmitBuffer[50]; // 8 bytes of initial sync and then next is the data. Assume 8 + 8
int sync_field_count;

// This function receives the data sent from the transmitter
// Algorithm
// 1) This would just listen to the transmitter i.e a while loop would call this function
// 2)
// This function behaves like a loop function
void ReceiveData()
{
	static uint8_t pinValue = 0;

	if((ReceivePinValue >> 7) & 0x01)
	{
		pinValue |= (0x01 << receiverBitCounter);
	}
	else
	{
		pinValue |= (0x00 << receiverBitCounter);
	}
	receiverBitCounter--;

	if(receiverBitCounter == -1)
	{
		ReceiveBuffer[receiverBufferCounter] = pinValue;
		receiverBufferCounter++;
		receiverBitCounter = 7;
		pinValue = 0x00;
	}
}

int * FindMessage()
{
	int dataCounter = 0;
	char dataByte;
	int no_of_sync_bytes = 32;
	bool startOfDataString = false;
	int error_count = 0;
	static int retValues[2] = {0, 0};
	int receiveBufferLength = 1024;
	char Kernel[32]=
	{       0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
			0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf
	};

	for(dataCounter = 0; dataCounter < receiveBufferLength; dataCounter++)
	{
		dataByte = Buffer[dataCounter];

		if(sync_field_count < no_of_sync_bytes)
		{
			if(Kernel[dataCounter] == dataByte)
			{
				sync_field_count++;
			}
			else
			{
				error_count++;
				sync_field_count++;
			}
		}
		else if(startOfDataString == false)
		{
			retValues[1] = dataCounter;
			sync_field_count = 0;

			if(error_count < 10)
			{
				printf("error count %d\n", error_count);
				startOfDataString = true;
			}
			retValues[0] = startOfDataString;
			return retValues;
		}
	}
}

void LISAProcessingReceivedData()
{
	int numberCounter = 0, fullBitsCounter = 0, byteCounter = 0, checkCounter = 0, countForFourBits = 0;
	bool syncFieldDetected = false, firstHalfDetected = false, secondHalfDetected = false, ignoreFourBits = false;
	char dataByte, nextByte, detectedByte;
	int bitCounter = 0;

	strncpy(ReceiveBuffer, TransmitBuffer, 50);

	dataByte = ReceiveBuffer[0];
	if(!syncFieldDetected)
	{
		printf("\n\nAfter LISA Implementation\n");
		while(1)
		{
			if(fullBitsCounter == 8)
			{
				dataByte = ReceiveBuffer[byteCounter];
				fullBitsCounter = 0;
				checkCounter = 0;
				byteCounter++;
			}

			if(ignoreFourBits)
			{
				countForFourBits++;
			}

			if(countForFourBits == 4)
			{
				ignoreFourBits = false;
			}

			if(!ignoreFourBits) {
				if(!firstHalfDetected)
				{
					secondHalfDetected = false;
					if(((dataByte >> 4)& 0xF) == 0x5)
					{
						detectedByte = ((dataByte >> 4)& 0xF);
						printf("%x", detectedByte);
						// first half detected
						firstHalfDetected = true;
						// Search for the next
						ignoreFourBits = true;
						countForFourBits = 0;
					}

					if(((dataByte >> 4)& 0xF) == 0xA)
					{
						detectedByte = ((dataByte >> 4)& 0xF);
						printf("%x", detectedByte);
						// first half detected
						firstHalfDetected = true;
						// Search for the next
						ignoreFourBits = true;
						countForFourBits = 0;
					}
				}
				else
				{
					firstHalfDetected = false;
					if(((dataByte >> 4)& 0xF) == numberCounter)
					{
						detectedByte = ((dataByte >> 4)& 0xF);
						printf("%x", detectedByte);
						secondHalfDetected = true;
						ignoreFourBits = true;
						numberCounter++;
						countForFourBits = 0;
					}

					if(numberCounter == 16)
					{
						numberCounter = 0;
					}
				}
			}

			if(checkCounter < 4)
			{
				dataByte = dataByte << 1;
				fullBitsCounter++;
				checkCounter++;
			}

			if(checkCounter == 4 && fullBitsCounter != 8)
			{
				if(byteCounter < receiveBufferLength)
				{
					nextByte = ReceiveBuffer[byteCounter+1];
					dataByte |= (nextByte >> 4);
					checkCounter = 0;
				}
				else
				{
					nextByte = 0;
					checkCounter = 0;
				}
			}
		}
		//		dataByte = (ReceiveBuffer << 1);
	}
}

int ProcessLISAOnReceivedData()
{
	int mainByteCount = 0, internalBufferCount = 0, firstBitIndex = 0, secondBitIndex = 0;
	uint8_t firstByte = 0x00, secondByte = 0x00;
	uint8_t localByte = 0x00;
	int dataStartIndex = 0;
	int *dataStatus;
	//	strncpy(ReceiveBuffer, TransmitBuffer, 50);
	printf("\n");

	while(mainByteCount < 700)
	{
		if(firstBitIndex == 0)
		{
			// This is new 8 byte data frame start.
			// So copy this data in the new Buffer
			for(internalBufferCount = 0; internalBufferCount < 50/*receiveBufferLength*/; internalBufferCount++)
			{
				Buffer[internalBufferCount] = ReceiveBuffer[mainByteCount + internalBufferCount];
//				printf("%x", Buffer[internalBufferCount]);
			}
//			printf("\n");
			firstBitIndex = 1;
			secondBitIndex = 0;
			firstByte = ReceiveBuffer[mainByteCount];
			secondByte = ReceiveBuffer[mainByteCount + 1];
		}
		else
		{
			// Start storing bytes in  new Buffer for the firstBit and lastBit
			for(internalBufferCount = 0; internalBufferCount < 50/*receiveBufferLength*/; internalBufferCount++)
			{
				Buffer[internalBufferCount] = 0;
				localByte = 0x00;
				firstByte = ReceiveBuffer[mainByteCount + internalBufferCount];
				secondByte = ReceiveBuffer[mainByteCount + internalBufferCount + 1];

				localByte |= (firstByte << firstBitIndex);
				localByte |= (secondByte >> (7 - secondBitIndex));

				Buffer[internalBufferCount] = localByte;
//				printf("%x", Buffer[internalBufferCount]);
			}
//			printf("\n");
			firstBitIndex++;
			secondBitIndex++;

			if(firstBitIndex == 8 && secondBitIndex == 7)
			{
				firstBitIndex = 0;
				secondBitIndex = 0;
				mainByteCount++;
			}
		}
		dataStatus = FindMessage();
		if(dataStatus[0] == true)
		{
			break;
		}
	}
	dataStartIndex = dataStatus[1];
	return dataStartIndex;
}

#ifdef ReceiveDebug
void SetUpReceiveInterrupt()
{
	/*
	 * Interrupt Enable for 2.7
	 */
	IRQn_Type IRQnType = EINT3_IRQn;
	LPC_GPIOINT->IO2IntEnR = 0;
	LPC_GPIOINT->IO2IntEnR |= (1 << ReceivePin);
	LPC_GPIOINT->IO2IntClr &= ~(1 << ReceivePin);
	NVIC_EnableIRQ(IRQnType);
}
#endif
