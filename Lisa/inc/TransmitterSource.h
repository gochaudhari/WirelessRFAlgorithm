/*
 * TransmitterSource.h
 *
 *  Created on: Sep 20, 2016
 *      Author: gocha
 */

#ifndef TRANSMITTERSOURCE_H_
#define TRANSMITTERSOURCE_H_


void CreateSyncStream();
void AppendUserData(char *transmitDataAppend);
void TransmitData();
void EncryptTransmitSyncField();
void ScrambleData(int scrambleAndDescrambleOrder);
void EncodeUsingLinearBlockCoding();

#endif /* TRANSMITTERSOURCE_H_ */
