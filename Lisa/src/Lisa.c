/*
===============================================================================
 Name        : Lisa.c
 Author      : Gaurao Chaudhari
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
 */


#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

#include <AllDefs.h>
#include <TransmitterSource.h>
#include <ReceiverSource.h>
#include <stdio.h>

// TODO: insert other include files here

// TODO: insert other definitions and declarations here

// General Defs
char TransmitBuffer[1024];				// 8 bytes of initial sync and then next is the data. Assume 8 + 8
char TransmittedData[30];
int transmitBufferLength = 1024, transmitDataLength;
int sizeOfsyncField = 32;

char ReceiveBuffer[1024];
char ReceivedData[500];
int receiveBufferLength = 1024, receiveDataLength;
bool bitReceived = false, dataReceived = false, bitSent = false;

int receiverBufferCounter, bitCount = 8, receiverBitCounter;
int transmitBufferCounter, transmitBitCounter;

void SetUpGPIOPins()
{
	// Pin Sel first
	TransmitPinSel &= ~(3 << TransmitBits);
	TransmitPinSel &= ~(3 << ReceiveBits);

	// Selecting pull down resistor
	TransmitPinMode |= 3 << TransmitBits;
	TransmitPinMode |= 3 << ReceiveBits;

	// Setting the direction of 2.6 as transmitter (output)
	LPC_GPIO2->FIODIR0 |= 1 << TransmitPin;
	LPC_GPIO2->FIODIR0 &= ~(1 << ReceivePin);

#ifdef TransmitDebug

#endif

#ifdef ReceiveDebug
	// Enable Interrupt here
#endif
}

#if defined(Transmit) || defined(Receive)
void SetUpTimer()
{
	// PCONP register for Timer 0
	LPC_SC->PCONP |= (1 << Timer0PCONP);

	/*2. Peripheral clock: In the PCLKSEL0 register (Table 40), select PCLK_TIMER0/1*/
	LPC_SC->PCLKSEL0 |= (3 << Timer0PCLK);

	/*3. Pins: Select timer pins through the PINSEL registers. Select the pin modes for the
	port pins with timer functions through the PINMODE registers.*/

	/* Enable the timer, and increment on PCLK */
    LPC_TIM0->TC = 0;
    LPC_TIM0->TCR = 0x2;
    LPC_TIM0->CTCR = 0;

    LPC_TIM0->PR = 0;
    LPC_TIM0->PC = 0;

	/*4. Interrupts: See register T0/1/2/3MCR (Table 430) and T0/1/2/3CCR (Table 431) for
	match and capture events. */
    LPC_TIM0->MCR |= (0x3 << 0);

    /* Interrupts are enabled in the NVIC using the appropriate Interrupt Set Enable register.*/
    LPC_TIM0->MR0 = 0xabcd;

    IRQn_Type timerIRQType = TIMER0_IRQn;
    NVIC_EnableIRQ(timerIRQType);
}
#endif

#if defined(Transmit) || defined(Receive)
void TIMER0_IRQHandler()
{
	// Reset the interrupt
	LPC_TIM0->IR |= (1 << 0);

	// Reset the counter.
	LPC_TIM0->TCR = 0x02;

#ifdef Receive
	bitReceived = true;
#endif

#ifdef Transmit
	bitSent = false;
#endif
}
#endif

#ifdef ReceiveDebug
void EINT3_IRQHandler(void)
{
	LPC_GPIOINT->IO2IntClr |= 1 << ReceivePin;
}
#endif

#if defined(Transmit) || defined(Receive)
void PrintData(char *buffer, int length, int characterPosition)
{
	int counter;
	for(counter = 0; counter < length; counter++)
	{
		if(characterPosition == 0)
		{
			printf("%x", buffer[counter]);
		}
		else if(counter < characterPosition)
		{
			printf("%x", buffer[counter]);
		}
		else
		{
			printf("%c", buffer[counter]);
		}
	}
}
#endif

int main(void)
{

	// Setup the GPIO Ports here at this position
	SetUpGPIOPins();

#ifdef TransmitDebug
	// There won't be anything done by the controller in the transmit mode since the button does everything.
#endif

#ifdef ReceiveDebug
	// Setup the interrupts
	SetUpReceiveInterrupt();
#endif

	// Force the counter to be placed into memory
	volatile static int i = 0 ;

	// The target now is to store the data in the format given and then receive the data in the required format
	/* Algorithm::
	 * 1) T: Create the sync stream for 64 bits.
	 * 2) T: Take data from user
	 * 3) T: Combine the repeating pattern and the user input data
	 * 4) R: Receive all the data in a variable.
	 * 5) R: Extract the repeating pattern from the data for first four numbers.
	 * 6) R: Ignore rest of the bits and then read the actual data
	 */

#ifdef Transmit
	// 1) T: Create the sync stream for 64 bits.
	CreateSyncStream();
	// 1) T: Print the created sync stream
	PrintData(TransmitBuffer, transmitBufferLength, 0);

	// 2) T: Take data from user
	printf("\nEnter the data to be transmitted: ");
	scanf("%s", &TransmittedData);
	transmitDataLength = strlen(TransmittedData);

	// 3) T: Combine the repeating pattern and the user input data
	AppendUserData(TransmittedData);
	// 3) T: Print the created final stream
	PrintData(TransmitBuffer, transmitBufferLength, sizeOfsyncField);

	// T: Transmit the formed data
	TransmitData();
#endif

#ifdef Receive
	// 4) R: Receive all the data in a variable.
//	ReceiveData();
#endif
//	LISAProcessingReceivedData();
	// Never Ending Loop
#if defined(Transmit) || defined(Receive)
	// Turn on the timer
	SetUpTimer();
#endif

//    LPC_TIM0->TCR = 0x1;
	while(1)
	{
#ifdef Transmit
/*		if(!bitSent)
		{
			// Send the bit;
			bitSent = true;
			// Un-reset the counter.
			LPC_TIM0->TCR = 0x1;
		}*/
		if(bitTransmitted && transmitBufferCounter < transmitBufferLength)
		{
			TransmitData();
			bitTransmitted = false;
		}

#endif

#ifdef Receive
		// Checking for the data received flag and receive if the data is not received
		if(dataReceived)
		{
			LISAProcessingReceivedData();
			dataReceived = false;
		}

		if(bitReceived && receiverBufferCounter < receiveBufferLength)
		{
			ReceiveData();
			bitReceived = false;
		}
		else
		{
			dataReceived = true;
		}
		// This loop handles the receiving of the bit
		while(!bitReceived && !dataReceived)
		{
			// Make the controller to go to sleep in this loop
		}
#endif

#ifdef ReceiveDebug
		// Record the value of the pin either 1 or 0
		if((ReceivePinValue >> 7) & 0x01)
		{
			printf("\nReceived = 1");
		}
		else
		{
			printf("\nReceived = 0");
		}
#endif
	}
	return 0;
}
