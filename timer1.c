/* 
 * File:   timer1.c
 * Author: jeff.sponaugle
 *
 * Created on September 14, 2016, 10:04 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "global.h"

/*
 *      SetupTimer1() - Configure Timer1 to fire an interrupt at a fixed interval (1000hz)
 */
void SetupTimer1()
{
    // Setup Timer1 to fire every 1ms (for 1000hz sample and CAN output)
    // Timer1 (TMR1) increments every 40MHZ(Instruction Clock) / 64, or 625000Hz
    // To interrupt every 1ms, the timer resets when it reaches 625000 * .0010 seconds = 625

    T1CONbits.TON = 0; // Disable Timer
    T1CONbits.TCS = 0; // Select internal instruction cycle clock
    T1CONbits.TGATE = 0; // Disable Gated Timer mode
    T1CONbits.TCKPS = 0b10; // Select 64:1 Prescaler
    TMR1 = 0x00; // Clear timer register
    //PR1 = 6250; // Load the period value so the interrupt occurs every 10ms.
    PR1 = 625 ; // Load the period value so the interrupt occurs every 1ms.
    
    IPC0bits.T1IP = 0x01; // Set Timer1 Interrupt Priority Level
    IFS0bits.T1IF = 0; // Clear Timer1 Interrupt Flag
    IEC0bits.T1IE = 1; // Enable Timer1 interrupt
    T1CONbits.TON = 1; // Start Timer
}



