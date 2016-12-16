/*
 * TransmitterSource.h
 *
 *  Created on: Sep 20, 2016
 *      Author: gocha
 */

#ifndef TRANSMITTERSOURCE_H_
#define TRANSMITTERSOURCE_H_


void CreateSyncStream();
void AppendUserData(char *transmitDataAppend, int transmitDataLength);
void TransmitData();
void EncryptTransmitSyncField();
void ScrambleData(int scrambleAndDescrambleOrder);
void EncodeUsingLinearBlockCoding();
void SetPIparameters(uint8_t * PerformanceIndexParameters, int sizeOfsyncField, int scrambleAndDescrambleOrder, int sizeOfLBCmatrix, int dataSpeed);
void GetPIparameters(uint8_t piOfSyncField, uint8_t piOfScramblingAndDescrambling, uint8_t piOfLinearBlockCoding, uint8_t piOfSpeed);
#endif /* TRANSMITTERSOURCE_H_ */
