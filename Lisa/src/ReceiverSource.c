/*
 * ReceiverSource.c
 *
 *  Created on: Sep 20, 2016
 *      Author: gocha
 */

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <AllDefs.h>
#include <ReceiverSource.h>
#include <stdint.h>

extern char ReceiveBuffer[50];
extern char ReceivedData[30];
extern int receiveBufferLength, receiveDataLength;
extern bool bitReceived, dataReceived;

// This function receives the data sent from the transmitter
// Algorithm
// 1) This would just listen to the transmitter i.e a while loop would call this function
// 2)
void ReceiveData()
{
	int counter = 0, bitCount = 8, bitCounter = 0;
	uint8_t pinValue;

	for(counter = 0; counter < receiveBufferLength; counter++)
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
	dataReceived = true;
}

void ProcessReceivedData()
{
	int counter = 0, syncFieldCount = 0;
	bool firstSection = false, secondSection = false, goodStream = false;
	char *messageStatus;

	for(counter = 0; counter < receiveBufferLength; counter++)
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
		/*		switch(messageStatus)
		{
		case "FirstSection":
			firstSection = true;


		}*/

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
