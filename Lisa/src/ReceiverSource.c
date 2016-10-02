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

extern char ReceiveBuffer[1024];
extern char ReceivedData[500];
extern int receiveBufferLength, receiveDataLength;
extern bool bitReceived, dataReceived;
extern int receiverBufferCounter, bitCount, receiverBitCounter;

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
	int dataCounter = 0;
	bool syncFieldDetected = false;
	char dataByte;
	int bitCounter = 0;

	if(!syncFieldDetected)
	{
//		for()
//		dataByte = (ReceiveBuffer << 1);
	}
	for(dataCounter = 0; dataCounter < receiveBufferLength; dataCounter++)
	{
		dataByte = ReceiveBuffer[dataCounter];


	}

	/*for(dataCounter = 0; dataCounter < receiveBufferLength; counter++)
	{
		if(ReceiveBuffer[counter] == 0x5)
		{
			messageStatus = "FirstSection";
			firstSection = true;
		}
		else if(syncFieldCount == ReceiveBuffer[counter])
		{

			messageStatus = "SecondSection";
			secondSection = true;
		}

		if(firstSection && secondSection)
		{
			goodStream = true;
			firstSection = false;
			secondSection = false;
		}
				switch(messageStatus)
		{
		case "FirstSection":
			firstSection = true;
		}
	}*/
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
