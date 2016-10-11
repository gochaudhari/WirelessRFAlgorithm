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
extern char ReceivedData[500];
extern int receiveBufferLength, receiveDataLength;
extern bool bitReceived, dataReceived;
extern int receiverBufferCounter, bitCount, receiverBitCounter;
extern char TransmitBuffer[50];				// 8 bytes of initial sync and then next is the data. Assume 8 + 8

// This function receives the data sent from the transmitter
// Algorithm
// 1) This would just listen to the transmitter i.e a while loop would call this function
// 2)
// This function behaves like a loop function
void ReceiveData()
{
	static uint8_t pinValue;

	/*	for(counter = 0; counter < receiveBufferLength; counter++)
	{
	pinValue = 0x00;
	for(bitCounter = bitCount - 1; bitCounter >= 0; bitCounter--)
	{
	// Record the value of the pin either 1 or 0
	if((ReceivePinValue >> 7) & 0x01)
	{
	pinValue |= (0x01 << bitCounter);
	}
	}
	ReceiveBuffer[counter] = pinValue;
	}
	dataReceived = true;*/

	if((ReceivePinValue >> 7) & 0x01)
	{
		pinValue |= (0x01 << receiverBitCounter);
	}
	else
	{
		pinValue |= (0x00 << receiverBitCounter);
	}
//	printf("%d", (ReceivePinValue >> 7) & 0x01);
	receiverBitCounter++;

	if(receiverBitCounter == 8)
	{
		ReceiveBuffer[receiverBufferCounter] = pinValue;
		receiverBufferCounter++;
		receiverBitCounter = 0;
		pinValue = 0x00;
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
						secondHalfDetected = true;
						ignoreFourBits = true;
						numberCounter++;
						countForFourBits = 0;
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
