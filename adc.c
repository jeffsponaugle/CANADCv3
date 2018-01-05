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

// filter taps for FIR filter - 10 taps, precomputed
//  These coefficients are sized to be 16 bits on the right side of the decimal point (so all of these are less than 1, and add up to 1)
//  To use the coefficients, multiple each of these numbers by the signal at that location in the buffer, add up all the results, than
//  divide by 65526 (>>16).    For this to work the accumlator has to be big enough to handle the maximum sample value 
//  (4096 in my case) * 65536.
/*static int filter_taps[10] = {
  315,
  1631,
  3447,
  5535,
  6883,
  6883,
  5535,
  3447,
  1631,
  315
};*/


/*
 * Routines for manipulating the Analog to Digital Converter on the PIC dsp33 microcontroller.
 *      This routine assumes the following analog inputs are configured:
 *      AN1, AN2, AN3, AN4, AN5, AN9, AN10, AN11, AN12 (9 inputs total)
 */

 /* 
  *      SetupADC() - Configure and enable the ADC Unit for 9 analog inputs.   No parameters, and no return variable.
  *                   Failure mode - ADC could fail to enable, but no way to detect that.
  */


void SetupADC()
{
    // disable the ADC unit
    AD1CON1bits.ADON = 0;
    AD1PCFGL = 0b0000000000000000;  // AN0-12 = analog
    AD1CON2bits.VCFG = 0b001;       // VRef+ is External VRef+ (AN0, 2.5Vs), VRef- is Vss(ground)
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

/* 
  *      GetADCSample - Get a single channel sample (passed in as a parameter)
  *                     Return value is the data value, or 0 if the channel number is bad
  *                     which is not particularly useful. ;)
  */
unsigned int GetSingleADCSample(unsigned int channel)
{
    unsigned int value=0;
   
    // AD1-AD12 are connected.  AD0 is VRef+.
    if (channel <= 12)
    {      
        // Channel 0/A Positive input is ANx
        AD1CHS0bits.CH0SA = channel;          

        // Setting SAMP to 1 starts the sample process.   We will wait 8us, then stop sampling.
        AD1CON1bits.SAMP = 1;
        DelayuS(8);
        AD1CON1bits.SAMP = 0;
        // Once sampleing is stopped, converstion automaticly starts.   The AD1CON1bits.DONE indicates when the conversion 
        // is complete.
        while (!AD1CON1bits.DONE);

        //  Conversion is complete, lets get the analog value from the ADC buffer
        value = ADC1BUF0;

    }
    // return the captured value, or 0 if the channel number was incorrect.
    return value;
}

/* 
  *      CollectAllADCSamples() - Collect all 8 ADC data elements plus the 5v rail reference..  This procedure captures data 
  *             from the 8 analog channels and stores them in the global g_ADCValuesBuffer array (Size 8*10).  That array hold 
  *             the previous 10 samples of each channel, and those 10 samples are used to generate an average value that is then 
  *             placed into the ADCBuffer array (Size 8).     (Thus the ADCBuffer array is a 10:1 decimation of the call rate)
  *             NOTE:  The final version of this will replace the 'average' routine with an FIR filter.
  */

void CollectAllADCSamples()
{
    unsigned int channelnumber;
    unsigned int stoptime;
    
    // AD1,2,3,4 and AD9,10,11,12 are connected to analog inputs, and AD5 is connected to 1/3 +5VCC.
    // Let's loop and sample all 8 external channels, plus the +5VCC one.
    for (channelnumber=0;channelnumber<=8;channelnumber++)
    {
        switch(channelnumber)
        {
            case 0:
            case 1:
            case 2:
            case 3:
                AD1CHS0bits.CH0SA = channelnumber+1;
                break;
            case 4:
            case 5:
            case 6:
            case 7:
               AD1CHS0bits.CH0SA = channelnumber+5;
                break;
            case 8:
                // the 9th channel will be channel 5 which has a 1/3 VCC +5V connected (for ratiometric reference)
                AD1CHS0bits.CH0SA = 5;
        }

        // Setting SAMP to 1 starts the sample process.   We will wait 8us, then stop sampling.
        AD1CON1bits.SAMP = 1;
        DelayuS(8);                     // 8us Sample Settle time
        AD1CON1bits.SAMP = 0;

        // Once sampleing is stopped, converstion automaticly starts.   The AD1CON1bits.DONE indicates when the conversion 
        // is complete.
        while (!AD1CON1bits.DONE);      // Wait for conversion to complete (~1us)
        
        //  Conversion is complete, lets get the analog value from the ADC buffer and put in the averaging array.
        //  g_ADCValuesBufferIndex is incremented and loops at g_ADCBufferSize

        // If we are getting the 5V VCC measurement put that in a different place.
        if (channelnumber==8)
        {
            g_ADC5VReferenceRaw = ADC1BUF0;
        }
        else
        {
            g_ADCValuesBuffer[channelnumber][g_ADCValuesBufferIndex]=ADC1BUF0;
        }
        
        // increment a statistics global
        g_ADCCaptures++;
    }

    g_ADCValuesBufferIndex = (g_ADCValuesBufferIndex + 1) %  g_ADCBufferSize;
    // Now that we have captured all 8 channels, if we have collected 10 samples ( we can tell that because the 
    // g_ADCValuesBufferIndex has looped back around to 0 ) we will compute the average and store that in the 
    // ADCValues array.  (thus a 10:1 decimation)
    // g_ADCValuesBufferIndex points to the location of the next sample, which if equal to zero means we have collected 10 samples
    // and the last sample is at location 9.   For an FIR filter, we will start at 9 and decrement to 0 appying the filter coefficients.
    if ( g_ADCValuesBufferIndex == 0)
    {
        FilterAndDecimateSamples();        
    }
        
    //  For diagnostic purposes we will record the maximum Timer1 value as an indication of how long this conversion took
    //    from start of interrupt (when the interrupt starts Timer1 is reset to 0)   See timer1.c for details about timer 1.
    stoptime=TMR1;
    if ( stoptime > g_ADCCaptureTime) g_ADCCaptureTime = stoptime;
    return;
}

/* 
  *      FilterAndDecimateSamples - This funcion takes 10 samples stored in g_ADCValuesBuffer[channel][sample] and does
  *                                 averaging to generate a single output value per channel, thus performing both a filtering
  *                                 and decimation.  That output vale is placed in g_ADCValues[channel].    This function will 
  *                                 optionally perform an interger FIR filter (code removed)
  */
void FilterAndDecimateSamples()
{
    unsigned int channelnumber,samplenumber;
    unsigned int avg,sum;

    for (channelnumber=0;channelnumber<=7;channelnumber++)
    {
        avg=sum=0;
        // for each sample channel, add up all 8 samples.
        // since each sample is at most 4096 (12 bits), 10 of them added together will still fit
        // within a 16 bit unsigned integer. 
        for (samplenumber=0;samplenumber<g_ADCBufferSize; samplenumber++)
        {
            sum+=g_ADCValuesBuffer[channelnumber][samplenumber];
        }
        // Integer divide by 10.  ** This assumes the ADCBufferSize is 10. **
        avg = divu10(sum);
        g_ADCValues[channelnumber] = avg;
       
    }
}

/* 
  *      divu10 - divide unsigned int (16 bit) by 10 (using fast shifts)
  *                 This is an optimized interger divid by 10.  To work it needs two unsigned long workspaces 
  *                 (32 bit)
  */
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


/*



void SampleFilter_init(SampleFilter* f) {
  int i;
  for(i = 0; i < SAMPLEFILTER_TAP_NUM; ++i)
    f->history[i] = 0;
  f->last_index = 0;
}

void SampleFilter_put(SampleFilter* f, int input) {
  f->history[f->last_index++] = input;
  if(f->last_index == SAMPLEFILTER_TAP_NUM)
    f->last_index = 0;
}

int SampleFilter_get(SampleFilter* f) {
  long long acc = 0;
  int index = f->last_index, i;
  for(i = 0; i < SAMPLEFILTER_TAP_NUM; ++i) {
    index = index != 0 ? index-1 : SAMPLEFILTER_TAP_NUM-1;
    acc += (long long)f->history[index] * filter_taps[i];
  };
  return acc >> 16;
}
*/
