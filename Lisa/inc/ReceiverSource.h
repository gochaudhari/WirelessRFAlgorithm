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
void ProcessLISAOnReceivedData();
void SetUpReceiveInterrupt();
void FindMessage(char ReceiveBuffer[]);
void IncrementSyncbytes();

#endif /* RECEIVERSOURCE_H_ */
