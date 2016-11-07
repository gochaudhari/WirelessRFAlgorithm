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
#include <Common.h>

// General Defs
uint8_t TransmitBuffer[1024];				// 8 bytes of initial sync and then next is the data. Assume 8 + 8
char TransmittedData[30];
int transmitBufferLength;
uint8_t transmitDataLength;
int sizeOfsyncField = 32;

#ifdef ScramblingAndDescrambling
uint8_t scrambleAndDescrambleOrder = 5;
#endif

//char ReceiveBuffer[] = {0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbf, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d, 0x4f, 0x51, 0x53, 0x55, 0x57, 0x59, 0x5b, 0x5d, 0x5e, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x00, 0x00};
#ifdef ReceiveTest
char ReceiveBuffer[] = {0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0x3, 0x59, 0xA9, 0x3C};
#elif
//char ReceiveBuffer[1024];
#endif
uint8_t Buffer[1024];
char *ReceivedData;
int receiveBufferLength = 1024, receivedDataLength, actualDataLength = 0;
bool bitReceived = false, receiveBufferFull = false, bitReadyForTransmit = false, dataReceived = false;

#ifdef EncryptedCommunication
	bool encryptEntireData = false;
#endif

int receiverBufferCounter, bitCount = 8, receiverBitCounter = 7;
int transmitBufferCounter, transmitBitCounter = 7;
const char *acknowledgement = "ACKSENT";

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
//	LPC_SC->PCLKSEL0 &= ~(3 << Timer0PCLK);
	LPC_SC->PCLKSEL0 |= (3 << Timer0PCLK);

	/*3. Pins: Select timer pins through the PINSEL registers. Select the pin modes for the
	port pins with timer functions through the PINMODE registers.*/

	/* Enable the timer, and increment on PCLK */
	LPC_TIM0->TC = 0;
	LPC_TIM0->TCR = 0x2;
	LPC_TIM0->CTCR = 0;

	LPC_TIM0->PR = 200;			//200
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

int main(void)
{

	// Setup the GPIO Ports here at this position
	char communicationSelect;
	bool transmit = false, receive = false, sendAckowledgement = false, receiveAcknowledgement = false;
	int* dataReceivedStatus;
	int counter, byteCounter = 0, printLength = 0;

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
#ifdef ScramblingAndDescrambling
				printf("\nScrabmling order: ");
				scanf("%s", &scrambleAndDescrambleOrder);
#endif
				// 2) T: Take data from user
				printf("\nEnter the data to be transmitted: ");
				scanf("%s", &TransmittedData);
				transmitDataLength = strlen(TransmittedData);

				TransmitBuffer[transmitBufferLength] = transmitDataLength;
				transmitBufferLength++;

				// 3) T: Combine the repeating pattern and the user input data
				AppendUserData(TransmittedData);
			}

			printf("\nData transmission: ");
			// 3) T: Print the created final stream
			// transmitBufferLength variable can also be used but needs to be edited to actual value.
			printLength = sizeOfsyncField + TransmitBuffer[sizeOfsyncField + 1] + 1;
			PrintData(TransmitBuffer, printLength, sizeOfsyncField);

			#ifdef EncryptedCommunication
				EncryptTransmitSyncField();
				printf("\nEncrypted data transmission: ");
				// 3) T: Print the created final stream
				printLength = sizeOfsyncField + TransmitBuffer[sizeOfsyncField + 1] + 1;
				PrintData(TransmitBuffer, printLength, sizeOfsyncField);
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
				if(bitReadyForTransmit && transmitBufferCounter < transmitBufferLength+1)
				{
					TransmitData();
					bitReadyForTransmit = false;
					LPC_TIM0->TCR = 0x1;
				}
				else if(transmitBufferCounter == transmitBufferLength+1)
				{
					transmitBufferCounter = 0;
					transmit = false;
					receive = false;
					sendAckowledgement = false;
					receiveAcknowledgement = true;

					// This makes the transmitter go low when the transmission is done
					LPC_GPIO2->FIOCLR0 |= (1 << TransmitPin);
				}
			}
#endif

#ifdef Receive
			if(receive)
			{
#ifdef ReceiveTest
				receiverBufferCounter = 1024;
#endif
				// Checking for the data received flag and receive if the data is not received
				if(receiveBufferFull)
				{
					dataReceivedStatus = ProcessLISAOnReceivedData();
					dataReceived = dataReceivedStatus[0];

#ifdef ReceiveTest
					dataReceived = true;
#endif

					if(dataReceived)
					{
						dataReceived = false;
						receivedDataLength = sizeOfsyncField + 1 + ReceiveBuffer[sizeOfsyncField];
						byteCounter = 0;
						
						// Copy the received data
//						ReceivedData = (char *)malloc(sizeof(char) * Buffer[sizeOfsyncField]);
						ReceivedData = (char *)malloc(sizeof(char) * ReceiveBuffer[sizeOfsyncField]);
						receivedDataLength = sizeOfsyncField + 1 + ReceiveBuffer[sizeOfsyncField];

						for(counter = sizeOfsyncField + 1; counter < receivedDataLength; counter++)
						{
							ReceivedData[byteCounter] = ReceiveBuffer[counter];
							byteCounter++;
						}
						counter = 0;				// Not needed, but useful
						actualDataLength = byteCounter;

						printf("\n\nData Reception Complete (Error Count = %d)\nReceived Data", dataReceivedStatus[1]);						
#ifdef EncryptedCommunication
						DecryptReceivedSyncField(dataReceivedStatus[2]);
#endif					
						PrintData((uint8_t *)ReceiveBuffer, receivedDataLength, sizeOfsyncField);

#ifdef EncryptedCommunication
						DecryptReceivedSyncField(dataReceivedStatus[2]);						
						printf("\n Decrypted Data");
						PrintData(Buffer, receivedDataLength, sizeOfsyncField);
#endif

						// Descramble and Print Received data
#ifdef ScramblingAndDescrambling
						DescrambleReceivedData();
#endif

						if(!receiveAcknowledgement)
						{
							sendAckowledgement = true;
						}
						else
						{
								printf("\nAcknowledgement Received.Transmission Successful.");
								receiveAcknowledgement = false;
						}
						transmit = false;
						receive = false;
						free(ReceivedData);
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
