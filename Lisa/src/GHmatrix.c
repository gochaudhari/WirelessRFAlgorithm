/*
 * GHmatrix.c
 *
 *  Created on: Nov 27, 2016
 *      Author: Akshay
 */

#include <GHmatrix.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

int parityMatrix[8][4] ={
		{1,1,0,0},
		{0,1,1,0},
		{0,0,1,1},
		{1,0,0,1},
		{1,0,1,0},
		{0,1,0,1},
		{1,1,1,0},
		{0,1,1,1},
};
extern int generatorMatrix[8][12];
extern int transposeMatrix[12][4];
extern uint16_t receivedCMatrix[256];

void GenerateMatrix(int k, int n){

	int i=0,j=0;
	int rows=k;
	int coloumns=n;

	for(i = 0; i < rows; i++)
	{
		for(j = 0; j < coloumns; j++)
		{
			if(i == j)
			{
				generatorMatrix[i][j] = 1;
			}
			else if(i != j)
			{
				if(j >= k)
				{
					generatorMatrix[i][j] = parityMatrix[i][j-k];
				}
				else{
					generatorMatrix[i][j]=0;
				}

			}
		}
	}
}

void TransposeMatrix(int k, int n)
{
	int i = 0,j=0;
	int rows = n;
	int coloumns = n-k ;

	for(i = 0; i < rows; i++)
	{
		for(j = 0; j < coloumns; j++)
		{
			if(i<8)
			{
				transposeMatrix[i][j] = parityMatrix[i][j];
			}
			else if(i == j+8)
			{
				transposeMatrix[i][j]=1;
			}
			else
			{
				transposeMatrix[i][j] = 0;
			}
		}
	}
}

void ReceiverSideCMatrix()
{
	int nCount = 0, kCount = 0;
	int nVal = 12, kVal = 8, tempCount = 0, similarCount;
	uint8_t gMatrix, middleCalc;

	// Encoding using generator matrix
	for(tempCount = 0; tempCount < 256; tempCount++)
	{
		for(nCount = 0; nCount < nVal; nCount++)
		{
			gMatrix = 0;
			for(kCount = 0; kCount < kVal; kCount++)
			{
				// Performing matrix multiplication of input and Generator matrix bits
				gMatrix |= ((generatorMatrix[kCount][nCount] & 0x01) << (7 - kCount));
			}
			middleCalc = gMatrix ^ tempCount;

			middleCalc = middleCalc - ((middleCalc >> 1) & 0x55);
			middleCalc = (middleCalc & 0x33) + ((middleCalc >> 2) & 0x33);
			similarCount = (((middleCalc + (middleCalc >> 4)) & 0x0F) * 0x01) >> 0;

			if(similarCount%2 == 0)
			{
				receivedCMatrix[tempCount] |= (0 << (11 - nCount));
			}
			else
			{
				receivedCMatrix[tempCount] |= (1 << (11 - nCount));
			}
		}
	}
}

