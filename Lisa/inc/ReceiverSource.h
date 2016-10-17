/*
 * ReceiverSource.h
 *
 *  Created on: Sep 20, 2016
 *      Author: gocha
 */

#ifndef RECEIVERSOURCE_H_
#define RECEIVERSOURCE_H_

void ReceiveData();
void LISAProcessingReceivedData();
int* ProcessLISAOnReceivedData();
void SetUpReceiveInterrupt();
int* FindMessage();
void DecryptReceivedSyncField(uint8_t key);

#endif /* RECEIVERSOURCE_H_ */
