/* 
 * File:   adc.h
 * Author: jeff.sponaugle
 *
 * Created on September 14, 2016, 9:12 PM
 * XXX
 */

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


#ifndef ADC_H
#define	ADC_H

#ifdef	__cplusplus
extern "C" {
#endif

void SetupADC();
void CollectAllADCSamples();
unsigned int GetSingleADCSample(unsigned int channel);
unsigned divu10(unsigned n);
void FilterAndDecimateSamples();
#define SAMPLEFILTER_TAP_NUM 10

typedef struct {
  double history[SAMPLEFILTER_TAP_NUM];
  unsigned int last_index;
} SampleFilter;

void SampleFilter_init(SampleFilter* f);
void SampleFilter_put(SampleFilter* f, double input);
double SampleFilter_get(SampleFilter* f);





#ifdef	__cplusplus
}
#endif

#endif	/* ADC_H */

