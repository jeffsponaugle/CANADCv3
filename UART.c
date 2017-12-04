/* 
 * File:   UART.c
 * Author: jeff.sponaugle
 *
 * Created on September 14, 2016, 9:28 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart.h"
#include "global.h"
#include "system.h"


/*
 * 
 */
void SetupUART1()
{
    // Setup UART1 with no Interrupts, 8 bit, 1 stop.
    U1MODEbits.STSEL = 0;       // 1-Stop bit
    U1MODEbits.PDSEL = 0;       // No Parity, 8-Data bits
    U1MODEbits.ABAUD = 0;       // Auto-Baud disabled
    U1MODEbits.BRGH = 1;        // High-Speed mode
    U1BRG = BRGVAL;             // Baud Rate setting for 9600
    U1STAbits.UTXISEL0 = 0;     // Interrupt after one TX character is transmitted
    U1STAbits.UTXISEL1 = 0;
    IEC0bits.U1TXIE = 0;        // Disable UART TX interrupt
    U1MODEbits.UARTEN = 1;      // Enable UART
    U1STAbits.UTXEN = 1;        // Enable UART TX
    DelayuS(100);
}

void TransmitUART1(char t)
{
    while (U1STAbits.UTXBF == 1);  // wait for the trasnmit buffer to be empty.
    U1TXREG = t;
}

void TransmitStringUART1(char* string)
{
    int i = 0;
    if (string==0) return;
    for (i=0; i<strlen(string); i++)
    {
     TransmitUART1(string[i]);
    }
}

void TransmitIntUART1(int x)
{
    char s[15];
    sprintf(s,"%u(0x%x)",x,x);
    TransmitStringUART1(s);
}

void TransmitByteUART1(uint8_t x)
{
    char s[15];
    sprintf(s,"%2X ",x);
    TransmitStringUART1(s);
}


