/* 
 * File:   adc.c
 * Author: jeff.sponaugle
 *
 * Created on September 14, 2016, 9:12 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include "adc.h"
#include "global.h"
#include "system.h"

/*
 * 
 */

void SetupADC()
{
    // disable the ADC unit
    AD1CON1bits.ADON = 0;
    AD1PCFGL = 0b0001100000000000;  // AN0-5 = analog, AN9-12 = digital, AN6-8 = NA
    AD1CON2bits.VCFG = 0b000;       // VRef+ is Vdd, VRef- is Vss(ground)
    AD1CON1bits.AD12B = 1;          // 12 bit single channel ADC
    AD1CON1bits.FORM = 0b00;        // Integer output
    AD1CON1bits.SSRC = 0b000;       // Clearing SAMP bit ends sampling and starts conversion.  This is for a simple polled sampling.
    AD1CON1bits.ASAM = 0b0;         // Sampling starts when SAMP bit set.  This is for simple polled sampling.
    AD1CON3bits.SAMC = 8;           // Autosample time set to 8*Tad (Not used for simple polling). This would ba a sample time of 2us.
    AD1CON3bits.ADRC = 1;           // Use the internal RC ADC clock which is is 4MHz (250ns)
    AD1CHS0bits.CH0NA = 0;          // Channel 0/A Negative input is VRef - (which is ground)
    AD1CHS0bits.CH0SA = 0;          // Channel 0/A Positive input is AN0
    //enable the ADC unit
    AD1CON1bits.ADON = 1;
}


unsigned int GetADCSample(unsigned int channel)
{
    unsigned int value;
    unsigned int starttime,stoptime;
    starttime = TMR1;
    // AD0-AD5 plus AD9 and A10 are connected.
    if (channel > 10)
        return 0;
    AD1CHS0bits.CH0SA = channel;          // Channel 0/A Positive input is ANx
    AD1CON1bits.SAMP = 1;
    DelayuS(100);
    AD1CON1bits.SAMP = 0;
    while (!AD1CON1bits.DONE);
    value = ADC1BUF0;
    stoptime=TMR1;
    g_ADCCaptureTime = stoptime;
    return value;
}

unsigned divu10(unsigned n) {
    unsigned long q, r;
    q = (n >> 1) + (n >> 2);
    q = q + (q >> 4);
    q = q + (q >> 8);
    q = q + (q >> 16);
    q = q >> 3;
    r = n - (((q << 2) + q) << 1);
    return q + (r > 9);
}

void CollectAllADCSamples()
{
    unsigned int i,j;
    unsigned int starttime,stoptime;
    unsigned sum, avg;
    starttime = TMR1;
    // AD0-AD5 are connected in this design
    for (i=0;i<=7;i++)
    {
        if (i==6)
        {
            AD1CHS0bits.CH0SA = 9;          // Channel 0/A Positive input is ANx
        } 
        else if (i==7)
        {
            AD1CHS0bits.CH0SA = 10;          // Channel 0/A Positive input is ANx
        } 
        else
        {
            AD1CHS0bits.CH0SA = i;          // Channel 0/A Positive input is ANx
        }
        
        AD1CON1bits.SAMP = 1;
        DelayuS(8);                     // 2us Sample Settle time
        AD1CON1bits.SAMP = 0;
        while (!AD1CON1bits.DONE);      // Wait for conversion to complete (~1us)
        ADCValuesBuffer[i][ADCValuesBufferIndex]=ADC1BUF0;
        g_ADCCaptures++;
    }
    if ( ADCValuesBufferIndex == 0)
    {
        for (i=0;i<=7;i++)
        {
            avg=sum=0;
            
            for (j=0;j< ADCBufferSize; j++)
            {
             sum+=ADCValuesBuffer[i][j];
            }
            avg = divu10(sum);
            ADCValues[i] = avg;
        }
        
    }
    
    
    ADCValuesBufferIndex = (ADCValuesBufferIndex + 1) %  ADCBufferSize;
        
    stoptime = TMR1;
    if ( stoptime > g_ADCCaptureTime) g_ADCCaptureTime = stoptime;
}
