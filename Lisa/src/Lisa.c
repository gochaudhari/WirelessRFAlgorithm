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
#include <GHmatrix.h>
#include <stdio.h>
#include <Common.h>
#include <stdlib.h>
#include <string.h>

// General Defs
int maxTransmitData = 30;
uint8_t TransmitBuffer[1024];				// 8 bytes of initial sync and then next is the data. Assume 8 + 8
char TransmittedData[30];
char BinaryData[30];
int transmitBufferLength;
int transmitDataLength;
int transmitBufferCounter, transmitBitCounter = 7;

int sizeOfsyncField = 32, dataLengthAdditions = 3, dataLengthByte = 34;
int k=8, n=12;
int generatorMatrix[8][12];
int transposeMatrix[12][4];
uint16_t CMatrix[256];
uint16_t receivedCMatrix[256];

#ifdef ScramblingAndDescrambling
int scrambleAndDescrambleOrder;
#endif

//char ReceiveBuffer[] = {0xa0, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 0xbc, 0xbf, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d, 0x4f, 0x51, 0x53, 0x55, 0x57, 0x59, 0x5b, 0x5d, 0x5e, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x00, 0x00};
#ifdef ReceiveTest
char ReceiveBuffer[] = {0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0x03, 0x04, 0x09, 0x61, 0x26, 0xB6, 0x73, 0x56, 0x8F, 0x61, 0x27, 0x91};
#else
char ReceiveBuffer[1024];
#endif
uint8_t Buffer[1024];
char *ReceivedData;
int receiveBufferLength = 1024, receivedDataLength, actualDataLength = 0, receivedBinaryDataLength;
bool bitReceived = false, receiveBufferFull = false, bitReadyForTransmit = false, dataReceived = false;

#ifdef EncryptedCommunication
	bool encryptEntireData = false;
#endif
int receiverBufferCounter, bitCount = 8, receiverBitCounter = 7;
const char *acknowledgement = "ACKSENT";

bool binaryDataFormat = false, characterDataFormat = true;

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

	LPC_TIM0->PR = 100;			//200
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
	char dataFormat;
	bool transmit = false, receive = false, sendAckowledgement = false, receiveAcknowledgement = false;
	int* dataReceivedStatus;
	int counter, byteCounter = 0, printLength = 0, errorBitCount = 0;

	SetUpGPIOPins();

//	CreationOfCMatrices();
//	DistanceCalculationAndDetectionOfData(12);
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
			printf("\nWant to Transmit or Receive (T or R): ");
			scanf("%c", &communicationSelect);
			getchar();
			printf("Enter the data format (B: Binary, C: Character): ");
			scanf("%c", &dataFormat);

#ifdef ScramblingAndDescrambling
			printf("Scrabmling order: ");
			scanf("%d", &scrambleAndDescrambleOrder);
#endif

#ifdef LinearBlockCoding
			GenerateMatrix(k,n);
			TransposeMatrix(8,12);
			ReceiverSideCMatrix();
			printf("Enter the number of error bits to induce in data: ");
			scanf("%d", &errorBitCount);
#endif

			if(dataFormat == 'B')
			{
				binaryDataFormat = true;
				characterDataFormat = false;
			}
			else if(dataFormat == 'C')
			{
				characterDataFormat = true;
				binaryDataFormat = false;
			}

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
				strcpy((char *)TransmittedData, acknowledgement);
				transmitDataLength = strlen(acknowledgement);

				// Storing Source ID
				TransmitBuffer[transmitBufferLength] = 0x03;
				transmitBufferLength++;

				// Storing Destination ID
				TransmitBuffer[transmitBufferLength] = 0x04;
				transmitBufferLength++;

#ifdef ScramblingAndDescrambling
				ScrambleData(scrambleAndDescrambleOrder);
#endif

#ifdef LinearBlockCoding
				EncodeUsingLinearBlockCoding();
#endif

				// Storing Data Length: Before scrambling and LBC Encoding
				TransmitBuffer[transmitBufferLength] = transmitDataLength;
				transmitBufferLength++;

				// 3) T: Combine the repeating pattern and the user input data
				AppendUserData(TransmittedData);
			}
			else
			{
				if(characterDataFormat)
				{
					// 2) T: Take data from user
					printf("Enter the data to be transmitted (Characters): ");
					transmitDataLength = 30;						// Fixing the maximum buffer length
					scanf("%s", &TransmittedData);
					transmitDataLength = strlen(TransmittedData);
				}
				else if(binaryDataFormat)
				{
					printf("Enter the data to be transmitted (in Binary): ");
					scanf("%s", &BinaryData);

					// Should return transmitDataLength and it would be number of bits not bytes
					transmitDataLength = BinaryDataFormatConversion((int8_t *)TransmittedData, transmitDataLength, (int8_t *)BinaryData, strlen(BinaryData), "BtoC");
					transmitDataLength = strlen(BinaryData);
				}

				// Storing Source ID
				TransmitBuffer[transmitBufferLength] = 0x03;
				transmitBufferLength++;

				// Storing Destination ID
				TransmitBuffer[transmitBufferLength] = 0x04;
				transmitBufferLength++;

#ifdef ScramblingAndDescrambling
				ScrambleData(scrambleAndDescrambleOrder);
#endif

#ifdef LinearBlockCoding
				EncodeUsingLinearBlockCoding();
#endif

				// Storing Data Length: Before scrambling and LBC Encoding
				TransmitBuffer[transmitBufferLength] = transmitDataLength;
				transmitBufferLength++;

				// 3) T: Combine the repeating pattern and the user input data
				AppendUserData(TransmittedData);
			}

			printf("\nData transmission: ");
			// 3) T: Print the created final stream
			// transmitBufferLength variable can also be used but needs to be edited to actual value.
			printLength = sizeOfsyncField + TransmitBuffer[dataLengthByte] + dataLengthAdditions;
			PrintData(TransmitBuffer, printLength, dataLengthByte);

			#ifdef EncryptedCommunication
				EncryptTransmitSyncField();
				printf("\nEncrypted data transmission: ");
				// 3) T: Print the created final stream
				printLength = sizeOfsyncField + TransmitBuffer[dataLengthByte] + dataLengthAdditions;
				PrintData(TransmitBuffer, printLength, dataLengthByte);
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
				receiveBufferFull = true;
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
						byteCounter = 0;
						
						// Copy the received data
						if(binaryDataFormat)
						{
							receivedBinaryDataLength = Buffer[dataLengthByte];				// Store the incoming number of bits
							Buffer[dataLengthByte] = (receivedBinaryDataLength / 8) + 1;	// Store the number of bytes in the received Buffer
						}
						else
						{
							receivedBinaryDataLength = Buffer[dataLengthByte];				// Does not matter
						}

						ReceivedData = (char *)malloc(sizeof(char) * Buffer[dataLengthByte]);
						receivedDataLength = sizeOfsyncField + Buffer[dataLengthByte] + dataLengthAdditions;

						for(counter = dataLengthByte + 1; counter < receivedDataLength; counter++)
						{
							ReceivedData[byteCounter] = Buffer[counter];
							byteCounter++;
						}
						counter = 0;				// Not needed, but useful
						actualDataLength = byteCounter;

						printf("\n\nData Reception Complete (Error Count = %d)\nReceived Data", dataReceivedStatus[1]);						
#ifdef EncryptedCommunication
						DecryptReceivedSyncField(dataReceivedStatus[2]);
#endif					
						// Displaying all Sync Field + Data
						PrintData((uint8_t *)Buffer, receivedDataLength, dataLengthByte);

#ifdef EncryptedCommunication
						DecryptReceivedSyncField(dataReceivedStatus[2]);						
						printf("\n Decrypted Data");
						PrintData(Buffer, receivedDataLength, dataLengthByte);
#endif

#ifdef LinearBlockCoding
						CreationOfCMatrices();
						IntroduceErrorBit(errorBitCount);
						LinearBlockDecoding();
#endif
						// Descramble and Print Received data
#ifdef ScramblingAndDescrambling
						printf("\nReceived Scrambled Data: ");
						PrintData((uint8_t *)ReceivedData, actualDataLength, -1);
						DescrambleReceivedData(scrambleAndDescrambleOrder);

						printf("\nReceived Descrambled Data: ");
						if(binaryDataFormat)
						{
							BinaryDataFormatConversion((int8_t *)ReceivedData, actualDataLength, (int8_t *)BinaryData, receivedBinaryDataLength, "CtoB");
							PrintData((uint8_t *)BinaryData, receivedBinaryDataLength, -1);
						}
						else
						{
							PrintData((uint8_t *)ReceivedData, actualDataLength, -1);
						}
#else
						printf("\nReceived Data: ");
						PrintData((uint8_t *)ReceivedData, actualDataLength, -1);
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
