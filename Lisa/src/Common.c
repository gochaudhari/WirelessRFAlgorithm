/*
 * Common.c
 *
 *  Created on: Nov 5, 2016
 *      Author: Gaurao Chaudhari
 */

#include <Common.h>
#include <AllDefs.h>
#include <LPC17xx.h>

#if defined(Transmit) || defined(Receive)
void PrintData(uint8_t *buffer, int length, int characterPosition)
{
	int counter;
	//	int totalLengthOfData = characterPosition + buffer[characterPosition + 1] + 1;
	printf("\n");
	for(counter = 0; counter < length; counter++)
	{
		if(characterPosition == 0)
		{
			printf("%x", buffer[counter]);
		}

		if(counter <= characterPosition)
		{
			printf("%x", buffer[counter]);
		}
		else if((counter > characterPosition) && (counter < length))
		{
			printf("%c", buffer[counter]);
		}
	}
}
#endif

// For maximum 127 shiftCount
void ShiftRegister(uint8_t *ShiftBuffer, uint8_t *ShiftedBuffer, int lengthOfBuffer, ShiftDirection shiftDirection, int shiftCount)
{
	int counter = 0;
	uint8_t copyHandle = 0, pasteHandle = 0;
	uint8_t localCharHandle = 0;
	int setZeroCount = 0;

	// Setting the zero indexes for shift more than 7 bits
	if(shiftCount/8 > 0)
	{
		setZeroCount = shiftCount/8;
		shiftCount = shiftCount % 8;				// Setting the actual shift count now which is between 0 and 7
	}
	else
	{
		setZeroCount = 0;
	}

	for(counter = 0; counter < lengthOfBuffer; counter++)
	{
		if(shiftDirection == right)
		{
			if((setZeroCount > 0) && counter < setZeroCount)
			{
				ShiftedBuffer[counter] = 0;
			}
			else
			{
				localCharHandle = ShiftBuffer[counter - setZeroCount];
				copyHandle = 0;
				copyHandle = localCharHandle << (8 - shiftCount);				// 8 for number of bits in a byte
				localCharHandle = (localCharHandle >> shiftCount);
				localCharHandle = localCharHandle | pasteHandle;
				pasteHandle = copyHandle;
				ShiftedBuffer[counter] = localCharHandle;
			}
		}
		else
		{
			// To change this. Not yet complete.
			/*			copyHandle = ShiftBuffer[counter] << (8 - shiftCount);				// 8 for number of bits in a byte
			ShiftBuffer[counter] = (ShiftBuffer[counter]  >> shiftCount);
			ShiftBuffer[counter] |= (pasteHandle << (8 - shiftCount));
			pasteHandle = copyHandle;*/
		}
	}
}


// This function converts the separated binary data to consolidated bits into one single bytes or converts
// consolidated bits to separated binary data.
// If Separate Binary -> Consolidated: 		BtoC
// If Consolidated    -> Separate Binary:   CtoB
// Returns the value of the end converted data length
// If converted to consolidated -> gives length of consolidated data
// If converted to separate binary -> gives length of separate binary data
//
int BinaryDataFormatConversion(int8_t *ConsolidatedData, int lengthOfConsolidatedData, int8_t *BinaryData, int lengthOfBinaryData, char *conversionType)
{
	int byteCounter = 0, bitCounter = 0, lengthReturnData;
	char *binaryToConsolidated = "BtoC";
	char *consolidatedToBinary = "CtoB";

	if(!strcmp(conversionType, binaryToConsolidated))
	{
		if(lengthOfBinaryData % 8 == 0)
		{
			lengthReturnData = lengthOfBinaryData / 8;
		}
		else
		{
			lengthReturnData = (lengthOfBinaryData / 8) + 1;
		}

		// To convert separate binary bits into one
		// Unknown lengthOfConsolidatedData
		for (bitCounter = 0; bitCounter < lengthOfBinaryData; bitCounter++)
		{
			if(BinaryData[bitCounter] == '1')
			{
				ConsolidatedData[bitCounter/8] |= (1 << (8 - ((bitCounter%8)+1)));
			}
			else
			{
				ConsolidatedData[bitCounter/8] |= (0 << (8 - ((bitCounter%8)+1)));
			}
		}
	}
	else if(!strcmp(conversionType, consolidatedToBinary))
	{
		// To convert consolidated bits into separate bits
		// Unknown lengthOfBinaryData
		for(byteCounter = 0; byteCounter < lengthOfConsolidatedData; byteCounter++)
		{
			for(bitCounter = 0; bitCounter < 8; bitCounter++)
			{
				if(ConsolidatedData[byteCounter] & (0x01 << (7 - bitCounter)))
				{
					BinaryData[(8*byteCounter) + bitCounter] = '1';
				}
				else
				{
					BinaryData[(8*byteCounter) + bitCounter] = '0';
				}
			}
		}
	}

	return lengthReturnData;
}

/*
Function decides the data communication speed from the parameters
*/
int GetDataSpeed()
{
	int dataSpeed = 0;

	if(LPC_TIM0->PR == 50 && LPC_TIM0->MR0 == 50 && (LPC_SC->PCLKSEL0 & 0x0C) == 0x0C)
	{
		dataSpeed = 9;
	}
	else if(LPC_TIM0->PR == 50 && LPC_TIM0->MR0 == 50)
	{
		dataSpeed = 4;
	}
	else if(LPC_TIM0->PR == 100 && LPC_TIM0->MR0 == 100)
	{
		dataSpeed = 2;
	}
	else if(LPC_TIM0->PR == 200 && LPC_TIM0->MR0 == 200)
	{
		dataSpeed = 1;
	}

	return dataSpeed;
}
