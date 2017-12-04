/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

/* Device header file */
#if defined(__XC16__)
    #include <xc.h>
#elif defined(__C30__)
    #if defined(__PIC24E__)
    	#include <p24Exxxx.h>
    #elif defined (__PIC24F__)||defined (__PIC24FK__)
	#include <p24Fxxxx.h>
    #elif defined(__PIC24H__)
	#include <p24Hxxxx.h>
    #endif
#endif

#include <stdint.h>          /* For uint32_t definition */
#include <stdbool.h>         /* For true/false definition */

#include "user.h"            /* variables/params used by user.c */

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/* <Initialize variables in user.h and insert code for user algorithms.> */

/* TODO Initialize User Ports/Peripherals/Project here */

void InitApp(void)
{
    /* Setup analog functionality and port direction */

    // Unlock the PPS Registers
    OSCCON = 0x46;
    OSCCON = 0x57; //unlock sequence 46,57
    OSCCONbits.IOLOCK = 1;
    // Configure the PPS registers for inputs
    RPINR18bits.U1RXR = 12;   // UART1 RX on RP12
    RPINR26bits.C1RXR = 11;   // CAN RX on RP11, pin 22, Port B #11
    RPINR20bits.SDI1R = 8;  // SPII1 on RP8(Pin 17), Port B #8

    // Configure the PPS registers for outputs
    RPOR4bits.RP9R = 7;        // RP9 = SPIO1, Pin 18, Port B #9
    RPOR3bits.RP7R = 8;        // RP7 = SPICLK, Pin 16, Port B #7
    RPOR6bits.RP13R = 3;       // RP13 = UART1 TX
    RPOR5bits.RP10R  = 16;     // RP10 = CAN TX

    // Lock the PPS Registers
    OSCCONbits.IOLOCK = 0;


    // Setup which pins are analog or digital
    AD1PCFGL=  0b0001100000000000;  // AN0-5, AN9,10 = Analog Input, AN6-8-NA, AN 11,12=digital
    

    // Setup which pins are input vs output

    TRISA = 0b00001111;         // RA0,1,2,3 = input,  RA4 = output
    TRISB =   0b1101100101101111; // RB15,14,12,11,8,6,5,3,2,1,0 input  RB13,10,9,7,4 output

    /* Initialize peripherals */

    LATA = 0b00000000;     // RA4 is the only output, so set it to zero.
    LATB = 0xffff;
   
}

