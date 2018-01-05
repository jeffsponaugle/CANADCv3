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

#include "system.h"          /* variables/params used by system.c */
#include "UART.h"            /* The syslog function uses the serial port output routines */

/******************************************************************************/
/* System Level Functions                                                     */
/*                                                                            */
/* Custom oscillator configuration funtions, reset source evaluation          */
/* functions, and other non-peripheral microcontroller initialization         */
/* functions get placed in system.c                                           */
/*                                                                            */
/******************************************************************************/

/* Refer to the device Family Reference Manual Oscillator section for
information about available oscillator configurations.  Typically
this would involve configuring the oscillator tuning register or clock
switching useing the compiler's __builtin_write_OSCCON functions.
Refer to the C Compiler for PIC24 MCUs and dsPIC DSCs User Guide in the
compiler installation directory /doc folder for documentation on the
__builtin functions. */

/* TODO Add clock switching code if appropriate.  An example stub is below.   */

/*
*   ConfigureOscillators(void) - This is called during device bootup to configure the system clock.  It assumes the external OSC1 input
*                                   is driven by a 40MHZ TTL Oscillator. 
*/
void ConfigureOscillator(void)
{
    /* Disable Watch Dog Timer */
    RCONbits.SWDTEN = 0;
    
    // At startup we will be running at 40MHz based on the input OSC!.  We can double that clock rate with an internal PLL.
    // We configure that PLL then start the switch.

    // To jump from 40MHz to 80Mhz (20Mips to 40Mips) we configure the PLLs
    PLLFBD=38; // M = 40
    CLKDIVbits.PLLPOST = 0; // N2 = 2
    CLKDIVbits.PLLPRE = 8; // N1 = 10

    // Initiate Clock Switch to Primary Oscillator with PLL (NOSC = 0b011)
    __builtin_write_OSCCONH(0x03);
    __builtin_write_OSCCONL(0x01);

    // Wait for Clock switch to occur
    while (OSCCONbits.COSC != 0b011);
     //Wait for PLL to lock
    while(OSCCONbits.LOCK != 1);    

    // Once the PLL has locked, we are running at 80MHz.  We can now configure other peripherals like timers, CAN,ADC, etc.
      
}

void InitApp(void)
{
    /* Setup analog functionality and port direction */

    // Unlock the PPS Registers.   This sequenced is needed to allow reassignment of pins.
    OSCCON = 0x46;
    OSCCON = 0x57; //unlock sequence 46,57
    OSCCONbits.IOLOCK = 1;

    // Once the PPS registers are unlocked, we can reconfigure pins. 
    // First we will configure the input pins.  Those are done via RPINRbits
    RPINR18bits.U1RXR = 8;   // Configure UART1 RX on RP8, Port B #8
    RPINR26bits.C1RXR = 11;   // Configure CAN RX on RP11(Pin 22), Port B #11

    // Configure the PPS registers for outputs via the RPOR bits.
    RPOR3bits.RP7R = 3;       // Configure RP7(Pin 16), for UART1 TX
    RPOR5bits.RP10R  = 16;     // Configure RP10(Pin 21),Port B #10 for CAN TX

    // Lock the PPS Registers
    OSCCONbits.IOLOCK = 0;
    
    // For all of the pins that can be either analog or digital, we configure then as such.
    // Setup which pins are analog or digital
    // AN0-12 = Analog Input, AN6-8 NA on this part.
    AD1PCFGL=  0b0000000000000000;  
    
     
    
    // Setup which pins are input vs output.   Using the above pin configs.
    // RA0,1,2,4 = input,  RA3=output, RA5-7 are NA on this part.
    TRISA = 0b00010111;         
    // INPUT -> 11 (CANRX) 9 (Ext In) 8 (UARTRX)  6,5 (PGM) 15,14,12,13,3,2,1,0 (ADC)  OUTPUT -> 4 (Driver) 7 (UARTTX) 13,10 (CANTX) output
    TRISB =   0b1111101101101111; 
    //TRISB =   0b0000101101101111; 

    /* Initialize peripherals */

    LATA = 0b00000000;     // RA4 is the only output, so set it to zero.
    LATB = 0xffff;
   
}


/*
*   DelaymS(void) - This is called to delay for the specified ms.  This does not use the timer1 counter, so interrupt overhead is
*                   not taken into account.  As such the time delayed will be at a minimum the specified time.   Timer overhead when
*                   running at 1000hz is on the order of 25%, so actual time here will be at least 25% long.
*/
void DelaymS(unsigned int d)
{
    // Minimum delay is 1mS, max is 65535mS
    volatile unsigned int i;
    for (i=0; i<d; i++)
    {
        asm volatile ("REPEAT, #10000"); Nop();
        asm volatile ("REPEAT, #10000"); Nop();
        asm volatile ("REPEAT, #10000"); Nop();
        asm volatile ("REPEAT, #10000"); Nop();
    }
}

/*
*   DelayuS(void) - This is called to delay for the specified us.  This does not use the timer1 counter, so interrupt overhead is
*                   not taken into account.  As such the time delayed will be at a minimum the specified time.   Timer overhead when
*                   running at 1000hz is on the order of 25%, so actual time here will be at least 25% long.  If this routine is used
*                   from within the timer interrupt handler, it will be pretty accurate.
*/
void DelayuS(unsigned int d)
{
    // Minimum delay is 1uS, max is 65535uS [65ms]
    volatile unsigned int x=0;
    if (d<=1)
    {
        asm volatile ("REPEAT, #3"); Nop();
        asm volatile ("REPEAT, #3"); Nop();
        return;
    }
    for (x=0;x<=(d-2);x++)
    {
        asm volatile ("REPEAT, #10"); Nop();
        asm volatile ("REPEAT, #10"); Nop();
    }
}

void syslog(char* logstring)
{
    TransmitStringUART1(logstring);
}

