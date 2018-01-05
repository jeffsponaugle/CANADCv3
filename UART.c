/* 
 * File:   UART.c
 * Author: jeff.sponaugle
 *
 * Created on September 14, 2016, 9:28 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "uart.h"
#include "global.h"
#include "system.h"


/*
 *      SetupUART1() - Configure the first UART, which is mapped to the RPxx pins below.   
 *                     We are configing based on the BAUDRATE in the header file.
 *                         Configure UART1 RX on RP12, Port B #12
 *                         Configure RP13(Pin 24),Port B #13 for UART1 TX
 */
void SetupUART1()
{
    // Setup UART1 with no Interrupts, 8 bit, 1 stop.  Since we are using this in simple mode we don't need interrupts
    U1MODEbits.STSEL = 0;       // 1-Stop bit
    U1MODEbits.PDSEL = 0;       // No Parity, 8-Data bits
    U1MODEbits.ABAUD = 0;       // Auto-Baud disabled
    U1MODEbits.BRGH = 1;        // High-Speed mode
    U1BRG = BRGVAL;             // Baud Rate setting for 115200
    U1STAbits.UTXISEL0 = 0;     // If Interrupts were enabld, interrupt after one TX character is transmitted
    U1STAbits.UTXISEL1 = 0;     // If Interrupts were enabld, interrupt after one TX character is transmitted
    IEC0bits.U1TXIE = 0;        // Disable UART TX interrupt since we don't need it.
    U1MODEbits.UARTEN = 1;      // Enable UART
    U1STAbits.UTXEN = 1;        // Enable UART TX
    // TX has been enabled.  Lets also enable receive
    U1STAbits.URXISEL = 0;      // If interrupts were on, we would interrupt every character received.
    // Delay for 100us to give the UART device time to complete configuration.
    DelayuS(100);
}

char ReceiveUART1()
{
    char ReceivedChar;
    // Loop forever waiting for a received character
    while (1)
    {
        
        /* Check for receive errors */
        if(U1STAbits.FERR == 1)
        {
            g_UARTReceiveErrors++;
            continue;
        }
        /* Must clear the overrun error to keep UART receiving */   
        if(U1STAbits.OERR == 1)
        {
            U1STAbits.OERR = 0;
            g_UARTReceiveOverflowErrors++;
            continue;
        }
        /* Get the data */
        if(U1STAbits.URXDA == 1)
        {
            ReceivedChar = U1RXREG;
            break;
        }
    }
    return ReceivedChar;
}

bool ReceiveUART1String(char* returnstring, unsigned int maxlength, bool returnonCRorLF, bool echo)
{
    unsigned int currentstringposition = 0;
    if (returnstring == 0) return false;
    if (maxlength <= 1) return false;
    // loop for the maxlength minus 1 so we have space for the terminating zero.
    while (currentstringposition != (maxlength-1) )
    {
        char c;
        c = ReceiveUART1();
        if (c == 0x0d) 
        {
            returnstring[currentstringposition++]=0;
            break;
        }
        else
        {
            returnstring[currentstringposition++] = c;
            if (echo==true)
            {
                TransmitUART1(c);
            }
        }
    }
    // Add zero termination to the end of the string.
    returnstring[currentstringposition]= 0x0;
    return true;
}

/*
 *      TransmitUART1(char t) - Transmit a single character over the UART.  Since we are not using interrupts, we will just 
 *                              watch the transmit buffer flag, and once cleared transmit a new character.  This is not a method you
 *                              would want to use from within the timer interrupt handler.  The UART has a 4 byte buffer, so this will
 *                              only block after that buffer is full.
 */
void TransmitUART1(char t)
{
    // wait for the trasnmit buffer to be empty.
    while (U1STAbits.UTXBF == 1);  
    // Writing to the U1TXREG starts a transmission, and that transmission completion is indicated by the clearing of UTXBF.
    U1TXREG = t;
}

/*
 *      TransmitStringUART1(char* string) - Transmit a zero terminated strong over the serial port.   This routine has not protection 
 *                                          from a non zero terminated string, but does have a string length limit of 255 characters.
 */
void TransmitStringUART1(char* string)
{
    int i,tlength;
    // If the pointer is null, return.
    if (string==0) return;
    // max out at 255 characters based on the strlen function.
    tlength = strlen(string);
    if (tlength>255) tlength=255;
    // transmit each character in string.
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


