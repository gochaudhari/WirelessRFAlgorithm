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
#include <Common.h>
#include <stdlib.h>

extern char ReceiveBuffer[1024];
extern uint8_t Buffer[1024];
extern char *ReceivedData;
extern int receiveBufferLength, receivedDataLength, actualDataLength;
extern bool bitReceived, dataReceived;
extern int receiverBufferCounter, bitCount, receiverBitCounter;
extern char TransmitBuffer[50]; // 8 bytes of initial sync and then next is the data. Assume 8 + 8
extern int transposeMatrix[12][4];

extern int sizeOfsyncField;

// For LBC Coding
extern uint16_t CMatrix[256];			// This matrix is used for caculation of distances
extern int generatorMatrix[8][12];
extern uint16_t receivedCMatrix[256];

#ifdef EncryptedCommunication
extern bool encryptEntireData;
#endif
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
	int dataCounter = 0, count;
	char dataByte;
	int no_of_sync_bytes = 32;
	bool startOfDataString = false;
	int error_count_block[4] = {0, 0, 0, 0}, error_count;
	int final_error_count = 0, block_detected = 0;
	static int retValues[4] = {0, 0, 0, 0};
	int receiveBufferLength = 50;
	uint8_t key;
	uint8_t key_arr[31];
	uint8_t sync_field_size;
	bool keyFound=false;

	uint8_t Kernel[32]=
	{       0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
			0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf
	};

#if defined(EncryptedCommunication)

	for(dataCounter = 0; dataCounter < receiveBufferLength; dataCounter++)
	{
		if(sync_field_count < no_of_sync_bytes)
		{
			dataByte = Buffer[dataCounter];

			key_arr[dataCounter] = dataByte ^ Kernel[dataCounter];
			sync_field_count++;
		}
		else if(!keyFound)
		{
			key = FindMostOccuringElement(key_arr);
			keyFound =true;
		}
	}

	sync_field_count=0;

	for(dataCounter = 0; dataCounter<receiveBufferLength;dataCounter++)
	{
		if(sync_field_count < no_of_sync_bytes)
		{
			Buffer[dataCounter] = Buffer[dataCounter] ^ key;
			sync_field_count++;
		}
	}

	sync_field_count=0;
#endif

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

			if(sync_field_count == 8)
			{
				if(error_count <= 2)
				{
					sync_field_size = 8;
					block_detected = 1;
				}
				error_count_block[0] = error_count;
				error_count = 0;
			}
			else if(sync_field_count == 16)
			{
				if(error_count <= 2)
				{
					sync_field_size = 16;
					block_detected = 2;
				}
				error_count_block[1] = error_count;
				error_count = 0;
			}
			else if(sync_field_count == 24)
			{
				if(error_count <= 2)
				{
					sync_field_size = 24;
					block_detected = 3;
				}
				error_count_block[2] = error_count;
				error_count = 0;
			}
			else if(sync_field_count == 32)
			{
				if(error_count <= 2)
				{
					sync_field_size = 32;
					block_detected = 4;
				}
				error_count_block[3] = error_count;
				error_count = 0;
			}
		}
		else
		{
			for(count = 0; count < block_detected; count++)
			{
				final_error_count = final_error_count + error_count_block[count];
			}

			sync_field_count = 0;

			if((final_error_count <= (2*block_detected)) && (block_detected > 0))
			{
				printf("\nerror count %d\n", final_error_count);
				startOfDataString = true;
			}
			retValues[0] = startOfDataString;
			retValues[1] = final_error_count;
			retValues[2] = key;
			retValues[3] = sync_field_size;
			return retValues;
		}
	}
}

uint8_t FindMostOccuringElement(uint8_t key_arr[])
{
	uint8_t current_key=key_arr[0];
	int size_kernel=32, i=0;
	int key_occurence_counter=0;

	for(i=0;i<size_kernel;i++)
	{
		if(current_key==key_arr[i])
		{
			key_occurence_counter++;
		}
		else if(key_occurence_counter==0)
		{
			current_key=key_arr[i];
			key_occurence_counter++;
		}
		else
		{
			key_occurence_counter--;
		}
	}

	return current_key;
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

int* ProcessLISAOnReceivedData()
{
	int mainByteCount = 0, internalBufferCount = 0, firstBitIndex = 0, secondBitIndex = 0;
	uint8_t firstByte = 0x00, secondByte = 0x00;
	uint8_t localByte = 0x00;
	int* dataStatus;

	while(mainByteCount < 1024)
	{
		if(firstBitIndex == 0)
		{
			// This is new 8 byte data frame start.
			// So copy this data in the new Buffer
			for(internalBufferCount = 0; internalBufferCount < 65/*receiveBufferLength*/; internalBufferCount++)
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
			for(internalBufferCount = 0; internalBufferCount < 65/*receiveBufferLength*/; internalBufferCount++)
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
	return dataStatus;
}

#ifdef EncryptedCommunication
void DecryptReceivedSyncField(uint8_t key)
{
	int byteCounter = 0, encryptionLength = 0;

	if(encryptEntireData)
	{
		encryptionLength = Buffer[sizeOfsyncField];
	}
	else
	{
		encryptionLength = 32;
	}

	for(byteCounter = 0; byteCounter < encryptionLength; byteCounter++)
	{
		Buffer[byteCounter] = Buffer[byteCounter] ^ key;
	}
}
#endif

#ifdef ScramblingAndDescrambling
// descramblingOrder should strictly be odd. Even descramblingOrder would give irregular results.
// Would be making this more flexible for even order in future.
void DescrambleReceivedData(int descramblingOrder)
{
	int counter = 0;
	uint8_t lowerStageCount = (descramblingOrder + 1)/2;
	uint8_t *shiftByLowerStage, *shiftByFullStages;

	shiftByLowerStage = (uint8_t *)malloc(actualDataLength);
	shiftByFullStages = (uint8_t *)malloc(actualDataLength);

	ShiftRegister((uint8_t *)ReceivedData, shiftByLowerStage, actualDataLength, right, lowerStageCount);
	ShiftRegister((uint8_t *)ReceivedData, shiftByFullStages, actualDataLength, right, descramblingOrder);

	//	PrintData((uint8_t *)ReceivedData, actualDataLength, actualDataLength);
	//	PrintData(shiftByFive, actualDataLength, actualDataLength);
	// Adding all these three data's and then getting the final buffer data
	for(counter = 0; counter < actualDataLength; counter++)
	{
		ReceivedData[counter] = ((shiftByLowerStage[counter] ^ shiftByFullStages[counter]) ^ ReceivedData[counter]);
	}
	free(shiftByLowerStage);
	shiftByLowerStage = NULL;				// Making this NULL to handle the dangling pointers
	free(shiftByFullStages);
	shiftByFullStages = NULL;					// Making this NULL to handle the dangling pointers
}

#endif

#ifdef LinearBlockCoding
void CreationOfCMatrices()
{
	int counter;
	int totalCount = 256;
	for(counter = 0; counter < totalCount; counter++)
	{
		CMatrix[counter] = counter;
	}
}

int min(int num1, int num2) {
	return num1 < num2 ? num1 : num2;
}

// Returns the index of minimum distance found
int DistanceCalculationAndDetectionOfData(uint16_t receivedEncodedBytes)
{
	int counter = 0, similarCount, minVal = CMatrix[0], minIndex = 0;
	uint16_t xoredNumber = 0;

	// The input length would be the length of
	for(counter = 0; counter < 256; counter++)
	{
		xoredNumber = 0;
		xoredNumber = (receivedEncodedBytes^receivedCMatrix[counter]);

		xoredNumber = xoredNumber - ((xoredNumber >> 1) & 0x5555);
		xoredNumber = (xoredNumber & 0x3333) + ((xoredNumber >> 2) & 0x3333);
		similarCount = (((xoredNumber + (xoredNumber >> 4)) & 0x0F0F) * 0x0101) >> 8;

		if(similarCount == 0)
		{
			minIndex = counter;
			break;
		}
		else if(counter > 0)
		{
			minVal = min(similarCount, minVal);
			if(similarCount == minVal)
			{
				minIndex = counter;
			}
		}
	}
	return minIndex;
}

void IntroduceErrorBit(int numberOfErrorBits)
{
	uint8_t errorByte;

	if(numberOfErrorBits == 1)
	{
		errorByte = 0x01;
	}
	else if(numberOfErrorBits == 2)
	{
		errorByte = 0x03;
	}
	else
	{
		errorByte = 0x00;
	}
	ReceivedData[0] = ReceivedData[0] | errorByte;
}

// This function realizes Linear Block Decoding. It calls the syndrome function.
// Extract bytes from the main data length and then feed it to syndrome and
void LinearBlockDecoding()
{
	int nVal = 12, minDistanceIndex, finalDataIndex = 0;
	uint16_t encodedBytes;
	int encodedBytesSeparately[12];
	int firstByteCount, secondByteCount = 0, encodedByteCount, nCount;
	bool isError = false;
	uint8_t dataExtracted;

	for(secondByteCount = 1; secondByteCount < actualDataLength;)
	{
		firstByteCount = secondByteCount - 1;

		finalDataIndex = (firstByteCount - (firstByteCount/3));
		encodedBytes = 0;
		// This is the first byte full and second byte half
		if(finalDataIndex % 2 == 0)
		{
			encodedBytes |= (ReceivedData[firstByteCount] << 4);
			encodedBytes |= (ReceivedData[secondByteCount] >> 4);
			secondByteCount++;
		}
		// This is second byte full and first byte half
		else
		{
			encodedBytes |= ((ReceivedData[firstByteCount] & 0x0F) << 8);
			encodedBytes |= (ReceivedData[secondByteCount] << 0);
			secondByteCount = secondByteCount + 2;
		}

		for(nCount = 0; nCount < nVal; nCount++)
		{
			encodedBytesSeparately[nCount] = ((encodedBytes >> (11 - nCount)) & 0x01);
		}

		// Akshay to call the Syndrome function
		isError = IsSyndromeNonZero(encodedBytesSeparately);

		if(isError)
		{
			minDistanceIndex = DistanceCalculationAndDetectionOfData(encodedBytes);
			dataExtracted = minDistanceIndex;
		}
		else
		{
			dataExtracted = (encodedBytes >> 4);
		}
		ReceivedData[finalDataIndex] = dataExtracted;
	}
	actualDataLength = finalDataIndex + 1;
}

bool IsSyndromeNonZero(int *receivedMatrix)
{
	bool syndromeResult = false;
	int syndromeMatrix[12 - 8];
	int rowCounter,columnCounter;
	int rowLimit = 12;
	int columnLimit = 12 - 8;

	//Loops to computer Syndrome word
	for(columnCounter = 0; columnCounter < columnLimit; columnCounter++)
	{
		for(rowCounter = 0;rowCounter < rowLimit; rowCounter++)
		{
			syndromeMatrix[columnCounter] =   syndromeMatrix[columnCounter]
															 + receivedMatrix[rowCounter]*transposeMatrix[rowCounter][columnCounter];
		}
	}

	//Check if computed syndrome word is zero
	for(columnCounter=0;columnCounter<columnLimit;columnCounter++)
	{
		if(syndromeMatrix[columnCounter] == 0){
			syndromeResult = false;
		}
		else
		{
			syndromeResult = true;
			return syndromeResult;
		}

	}

	return syndromeResult;
}
#endif

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
