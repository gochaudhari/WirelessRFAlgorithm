/*
 * AllDefs.h
 *
 *  Created on: Sep 14, 2016
 *      Author: Gaurav
 */

#ifndef ALLDEFS_H_
#define ALLDEFS_H_

// This is for debug purposes
#define Transmit
#define Receive
//define EncryptedCommunication
#define ScramblingAndDescrambling
//#define TransmitDebug
//#define ReceiveDebug

//#define TransmitTest
//#define ReceiveTest
// General defines
#define true						1
#define false						0

typedef int bool;

// Selecting pin P2.6 as transmitter and P2.7 as receiver

// Transmit Pin Defs
#define TransmitPin					6
#define TransmitBits				12
#define TransmitPinSel				LPC_PINCON->PINSEL4
#define TransmitPinMode				LPC_PINCON->PINMODE4
#define TransmitPinDir				LPC_GPIO2->FIODIR0
#define TransmitPinValue			LPC_GPIO2->FIOPIN0

// Receive Pin Defs
#define ReceivePin					7
#define ReceiveBits					14
#define ReceivePinValue				LPC_GPIO2->FIOPIN0

// Timer defs
#define Timer0PCONP					1
#define Timer0PCLK					2

#define MachingSyncCapability		8

#endif /* ALLDEFS_H_ */
