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
uint8_t TransmitBuffer[1024];				// 8 bytes of initial sync and then next is the data. Assume 8 + 8
char TransmittedData[30];
int transmitBufferLength;
uint8_t transmitDataLength;
int sizeOfsyncField = 32;

//char ReceiveBuffer[] = {0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbf, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d, 0x4f, 0x51, 0x53, 0x55, 0x57, 0x59, 0x5b, 0x5d, 0x5e, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x00, 0x00};
//char ReceiveBuffer[] = {"a0a2a4a6a8aaacaeb0b2b4b6b8babcbf41434547494b4d4f51535557595b5d5eaaaaaaaaaa0000000000000"};
char ReceiveBuffer[1024];
uint8_t Buffer[1024];
int receiveBufferLength = 1024, receiveDataLength;
bool bitReceived = false, receiveBufferFull = false, bitReadyForTransmit = false, dataReceived = false;

int receiverBufferCounter, bitCount = 8, receiverBitCounter = 7;
int transmitBufferCounter, transmitBitCounter = 7;

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

	LPC_GPIO2->FIOCLR0 |= (1 << TransmitPin);
	LPC_GPIO2->FIOCLR0 |= (1 << ReceivePin);

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

	LPC_TIM0->PR = 200;			//1000
	LPC_TIM0->PC = 0;

	/*4. Interrupts: See register T0/1/2/3MCR (Table 430) and T0/1/2/3CCR (Table 431) for
	match and capture events. */
	LPC_TIM0->MCR |= (0x3 << 0);

	/* Interrupts are enabled in the NVIC using the appropriate Interrupt Set Enable register.*/
	LPC_TIM0->MR0 = 100;			//1000

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
	bitReadyForTransmit = true;
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
void PrintData(uint8_t *buffer, int length, int characterPosition)
{
	int counter;
	int totalLengthOfData = characterPosition + buffer[characterPosition + 1] + 1;
	printf("\n");
	for(counter = 0; counter < totalLengthOfData; counter++)
	{
		if(characterPosition == 0)
		{
			printf("%x", buffer[counter]);
		}

		if(counter < characterPosition)
		{
			printf("%x", buffer[counter]);
		}
		else if((counter > characterPosition) && (counter < totalLengthOfData))
		{
			printf("%c", buffer[counter]);
		}
	}
	printf("\nData Printed");
}
#endif

int main(void)
{

	// Setup the GPIO Ports here at this position
	char communicationSelect;
	bool transmit = false, receive = false, sendAckowledgement = false, receiveAcknowledgement = false;
	char acknowledgement[8] = "ACKSENT";

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

#if defined(Transmit) || defined(Receive)
	// Turn on the timer
	SetUpTimer();
#endif

#ifdef Transmit
	// 1) T: Create the sync stream for 64 bits.
	CreateSyncStream();
	//	PrintData(TransmitBuffer, transmitBufferLength, 0);
#endif

	while(1)
	{

#if defined(Transmit) && defined(Receive)
		if(sendAckowledgement)
		{
			transmit = true;
			receive = false;
			// Starting the timer
			LPC_TIM0->TCR = 0x1;
		}
		else if(receiveAcknowledgement)
		{
			receive = true;
			transmit = false;

			// Starting the timer
			LPC_TIM0->TCR = 0x1;
		}
		else
		{
			printf("\n Want to Transmit or Receive (T or R): ");
			scanf("%c", &communicationSelect);

			if(communicationSelect == 'T')
			{
				transmit = true;
				receive = false;
			}
			else if(communicationSelect == 'R')
			{
				receive = true;
				transmit = false;
			}
			else
			{
				receive = false;
				transmit = false;
			}
			LPC_TIM0->TCR = 0x1;
		}
#endif

#ifdef Transmit
		// If asked for transmitting, then send the data
		if(transmit)
		{
			if(sendAckowledgement)
			{
				transmitDataLength = strlen(acknowledgement);

				TransmitBuffer[transmitBufferLength] = transmitDataLength;
				transmitBufferLength++;

				// 3) T: Combine the repeating pattern and the user input data
				AppendUserData(acknowledgement);
			}
			else
			{
				// 2) T: Take data from user
				printf("\nEnter the data to be transmitted: ");
				scanf("%s", &TransmittedData);
				transmitDataLength = strlen(TransmittedData);

				TransmitBuffer[transmitBufferLength] = transmitDataLength;
				transmitBufferLength++;

				// 3) T: Combine the repeating pattern and the user input data
				AppendUserData(TransmittedData);
			}

			// 3) T: Print the created final stream
			PrintData(TransmitBuffer, transmitBufferLength, sizeOfsyncField);

			#ifdef EncryptedCommunication
				EncryptTransmitSyncField();
				// 3) T: Print the created final stream
				PrintData(TransmitBuffer, transmitBufferLength, sizeOfsyncField);
			#endif
		}
#endif

		// This loop handles the transmission and receiving of the bit
		while(transmit || receive)
		{
#ifdef Transmit
			if(transmit)
			{
				// Transmit the data here in this loop
				if(bitReadyForTransmit && transmitBufferCounter < transmitBufferLength)
				{
					TransmitData();
					bitReadyForTransmit = false;
					LPC_TIM0->TCR = 0x1;
				}
				else if(transmitBufferCounter == transmitBufferLength)
				{
					transmitBufferCounter = 0;
					transmit = false;
					receive = false;
					sendAckowledgement = false;
					receiveAcknowledgement = true;
				}
			}
#endif

			int* dataReceivedStatus;
#ifdef Receive
			if(receive)
			{
				// Checking for the data received flag and receive if the data is not received
				if(receiveBufferFull)
				{
					dataReceivedStatus = ProcessLISAOnReceivedData();
					dataReceived = dataReceivedStatus[0];

					if(dataReceived)
					{
						dataReceived = false;
						printf("\nData Reception Complete (Error Count = %d)", dataReceivedStatus[1]);

						PrintData(Buffer, receiveBufferLength, 32);
						if(!receiveAcknowledgement)
						{
							sendAckowledgement = true;
						}
						else
						{
							receiveAcknowledgement = false;
						}
						transmit = false;
						receive = false;
					}
					else
					{
						bitReceived = true;
					}
					receiveBufferFull = false;
				}

				if(bitReceived)
				{
					bitReceived = false;
					if(receiverBufferCounter < receiveBufferLength)
					{
						ReceiveData();
						LPC_TIM0->TCR = 0x1;
						// Re-enable the timer to receive next bit
					}
					else
					{
						receiverBufferCounter = 0;
						receiveBufferFull = true;
					}
				}
			}
#endif
		}

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
