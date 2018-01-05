/* 
 * File:   global.h
 * Author: jeff.sponaugle
 *
 * Created on September 14, 2016, 8:57 PM
 */

#ifndef GLOBAL_H
#define	GLOBAL_H

#if defined(__XC16__)
    #include <xc.h>

#include "EEPROM.h"
#elif defined(__C30__)
    #if defined(__PIC24E__)
    	#include <p24Exxxx.h>
    #elif defined (__PIC24F__)||defined (__PIC24FK__)
	#include <p24Fxxxx.h>
    #elif defined(__PIC24H__)
	#include <p24Hxxxx.h>
    #endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#define FP 40000000      // 40MHz FOSC/2
#define FCAN    40000000   // FOSC/2 - 40MHz OSC Input, no PLL
#define CAN_BITRATE  1000000
#define CAN_NTQ 20
#define CAN_BRP_VAL ((FCAN/ (2*CAN_NTQ*CAN_BITRATE))-1)
#define CAN_SID_1 0x400             // default SID for First ADC Packet
#define CAN_SID_2 0x401             // default SID for Second ADC Packet
   
    extern const unsigned int g_ADCBufferSize;
    extern unsigned int g_CANSequenceNumber;
    extern unsigned int g_TimerSeconds;
    extern unsigned int g_TimerMS;
    extern unsigned int g_TimerMSTotal;
    extern unsigned int g_ADCValues[];
    extern unsigned int g_ADCValuesBuffer[8][10];
    extern unsigned int g_ADCValuesBufferIndex;
    extern unsigned int g_ADC5VReferenceRaw;
    extern unsigned int g_TimerSeconds;
    extern unsigned int g_TimerMS;
    extern unsigned int g_CANPacket1[];
    extern unsigned int g_CANPacket2[];
    extern unsigned int g_ADCCaptureTime;
    extern unsigned int g_ADCCaptures;
    extern unsigned int g_InterruptTime;
    extern unsigned int g_ECANTransmitTried;
    extern unsigned int g_ECANTransmitCompleted;
    extern unsigned int g_ECANError;
    extern unsigned int g_ECANTXBO;
    extern unsigned int g_ECANTXBP;
    extern unsigned int g_ECANTXWAR;
    extern unsigned int g_ECANEWARN;
    extern unsigned int g_ECANIVRIF;
    extern unsigned int g_ECANInterrupts;
    extern unsigned int g_ECANTransmitTimout;
    extern unsigned int g_TimerInterruptOverrun;
    extern unsigned int g_DMAInterrupts;
    extern unsigned int g_UARTReceiveErrors;           
    extern unsigned int g_UARTReceiveOverflowErrors;   




#ifdef	__cplusplus
}
#endif

#endif	/* GLOBAL_H */

