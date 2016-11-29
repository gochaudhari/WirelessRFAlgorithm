/*
 * ReceiverSource.h
 *
 *  Created on: Sep 20, 2016
 *      Author: Gaurao Chaudhari and Akshay Kanchar
 */

#ifndef RECEIVERSOURCE_H_
#define RECEIVERSOURCE_H_

void ReceiveData();
void LISAProcessingReceivedData();
int* ProcessLISAOnReceivedData();
void SetUpReceiveInterrupt();
int* FindMessage();
void DecryptReceivedSyncField(uint8_t key);
void DescrambleReceivedData(int descramblingOrder);
uint8_t FindMostOccuringElement(uint8_t key_arr[]);
bool IsSyndromeZero(int k, int n);

#endif /* RECEIVERSOURCE_H_ */
