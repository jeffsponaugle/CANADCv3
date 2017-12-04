/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */
#include <string.h>
#include <stdio.h>

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "user.h"          /* User funct/params, such as InitApp              */
#include "interrupt.h"
#include "global.h"        // externs for all global variables
#include "adc.h"
#include "uart.h"
#include "ecan.h"
#include "timer1.h"
#include "EEPROM.h"
#include "spi.h"



/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

// Global Statics
const unsigned int ADCBufferSize = 10;
unsigned int ADCValuesBuffer[8][10];
unsigned int ADCValuesBufferIndex=0;
unsigned int ADCValues[8] = {0,0,0,0,0,0,0,0};
unsigned int g_TimerSeconds = 0;
unsigned int g_TimerMS = 0;
unsigned int g_TimerMSTotal = 0;
uint16_t g_CANPacket1[8]; 
uint16_t g_CANPacket2[8];

double ADCVoltage[8]={0,0,0,0,0,0,0};
double ADCVoltageMin[8]={0,0,0,0,0,0,0};
double ADCVoltageMax[8]={0,0,0,0,0,0,0};
double ADCVoltageAvgSum[8]={0,0,0,0,0,0,0};
unsigned int ADCVoltageAvgCount=0;


const double caloffset[8]={3.0,3.0,3.0,3.0,3.0,3.0,3.0,3.0};
const double calslope[8]={812.25,812.09,811.51,811.13,812.90,811.55,809.51,809.90};

// Statistics

unsigned int g_ADCCaptureTime = 0;
unsigned int g_ADCCaptures=0;
unsigned int g_InterruptTime=0;
unsigned int g_ECANTransmitTried = 0;
unsigned int g_ECANTransmitTimout = 0;
unsigned int g_ECANTransmitCompleted = 0;
unsigned int g_ECANError = 0;
unsigned int g_ECANTXBO = 0;
unsigned int g_ECANTXBP = 0;
unsigned int g_ECANTXWAR = 0;
unsigned int g_ECANEWARN = 0;
unsigned int g_ECANIVRIF = 0;
unsigned int g_ECANInterrupts = 0;
unsigned int g_DMAInterrupts = 0;
unsigned int g_TimerInterruptOverrun = 0;

st_CAL g_Config;


int16_t main(void)
{
  
    char buffer[513];
    /* Configure the oscillator for the device */
    ConfigureOscillator();
    IFS0=0;
    IFS1=0;
    IFS2=0;
    IFS3=0;
    IFS4=0;
    
    InitApp();
    SetupUART1();
    SetupADC();
    
    TransmitStringUART1("\r\nCAN+ADC Interface Version 1.0\r\n");
   
    ConfigurationSystemInit();
    
    DelaymS(1);
    ConfigureECAN1();
    //SD_InitSPI(0);
    SetupTimer1();
    DelaymS(10);
    
    
    TransmitStringUART1("\r\nConfigured CanID=");
    TransmitIntUART1(g_Config.CanMessage1_ID);
    TransmitStringUART1(" BootCount=");
    TransmitIntUART1(g_Config.BootCount);
    TransmitStringUART1("\r\n");
    //SD_InitMedia();
    //SD_InitSPI(1);
    //SD_Read_Sector((uint32_t)0x1233, &buffer);
    //SD_Read_Sector((uint32_t)0x1233, &buffer);
    //SD_Read_Sector((uint32_t)0x1234, &buffer);
    //SD_Read_Sector((uint32_t)0x1234, &buffer);
    
    //while (1==1);
            
   LATBbits.LATB4=0;
   DelaymS(10);
   LATBbits.LATB4=1;
   DelaymS(200);
   
//    Write7Segment(1234);
    
    char p[100];
    sprintf(p,"%c[2J %c[H",27,27);
    TransmitStringUART1(p);
        
    double sum=0.0;
    int i=0;
    int count=0;
    
    for (i=0;i<=7;i++)
    {
        ADCVoltageMax[i]=ADCVoltageMin[i]=ADCVoltage[i]=ADCVoltageAvgSum[i]=((double)ADCValues[i]-caloffset[i])/calslope[i];
    }
    ADCVoltageAvgCount = 1;
    
    while(1)
    {
        DisplayStatus();
        DelaymS(50);
        
    }
}

void DisplayStatus()
{
    
    char line[100];
    int i=0;
    
    //sprintf(line,"%c[2J %c[H ADC-CAN Status\r\n",27,27);
    
    // Every 1 min, we send a clear screen command, to help in debugging
    
    if (g_TimerSeconds >60 )
    {
        g_TimerSeconds = 0;
        sprintf(line,"%c[2J %c[H",27,27);
        TransmitStringUART1(line);
    }
    
    sprintf(line,"%c[HADC-CAN Status\r\n",27);
    TransmitStringUART1(line);
    
    sprintf(line,"------------------------------------------\r\n" );
    TransmitStringUART1(line);

    for (i=0;i<=7;i++)
    {
        ADCVoltage[i]=((double)ADCValues[i]-caloffset[i])/calslope[i];
        if (ADCVoltageAvgCount == 1000)
        {
            ADCVoltageAvgSum[i] = ADCVoltage[i];    
        } 
        else
        {
            ADCVoltageAvgSum[i] += ADCVoltage[i];
        }
        
        if (ADCVoltage[i] > ADCVoltageMax[i]) ADCVoltageMax[i]=ADCVoltage[i];
        if (ADCVoltage[i] < ADCVoltageMin[i]) ADCVoltageMin[i]=ADCVoltage[i];
    }
    
    if (ADCVoltageAvgCount == 1000)
    {
        ADCVoltageAvgCount=1;
    }
    else
    {
        ADCVoltageAvgCount++;
    }
    
    
    
    
    sprintf(line,"Timer:%u.%u \r\n",g_TimerSeconds,g_TimerMS);
    TransmitStringUART1(line);
    sprintf(line,"ADC Values          MAX     MIN     AVG\t\t\tCAN Status\r\n");
    TransmitStringUART1(line);
    sprintf(line,"ADC0: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX: %05u\r\n",ADCValues[0],ADCVoltage[0],ADCVoltageMax[0],ADCVoltageMin[0],ADCVoltageAvgSum[0]/(double)ADCVoltageAvgCount,g_ECANTransmitCompleted);
    TransmitStringUART1(line);
    sprintf(line,"ADC1: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Tried: %05u\r\n",ADCValues[1],ADCVoltage[1],ADCVoltageMax[1],ADCVoltageMin[1],ADCVoltageAvgSum[1]/(double)ADCVoltageAvgCount,g_ECANTransmitTried);
    TransmitStringUART1(line);
    sprintf(line,"ADC2: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Error BO: %05u\r\n",ADCValues[2],ADCVoltage[2],ADCVoltageMax[2],ADCVoltageMin[2],ADCVoltageAvgSum[2]/(double)ADCVoltageAvgCount,g_ECANTXBO);
    TransmitStringUART1(line);
    sprintf(line,"ADC3: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Error BP: %05u\r\n",ADCValues[3],ADCVoltage[3],ADCVoltageMax[3],ADCVoltageMin[3],ADCVoltageAvgSum[3]/(double)ADCVoltageAvgCount,g_ECANTXBP);
    TransmitStringUART1(line);
    sprintf(line,"ADC4: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Error WARN: %05u\r\n",ADCValues[4],ADCVoltage[4],ADCVoltageMax[4],ADCVoltageMin[4],ADCVoltageAvgSum[4]/(double)ADCVoltageAvgCount,g_ECANTXWAR);
    TransmitStringUART1(line);
    sprintf(line,"ADC5: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Error IVRIF: %05u\r\n",ADCValues[5],ADCVoltage[5],ADCVoltageMax[5],ADCVoltageMin[5],ADCVoltageAvgSum[5]/(double)ADCVoltageAvgCount,g_ECANIVRIF);
    TransmitStringUART1(line);
    sprintf(line,"ADC6: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Interrupts: %05u\r\n",ADCValues[6],ADCVoltage[6],ADCVoltageMax[6],ADCVoltageMin[6],ADCVoltageAvgSum[6]/(double)ADCVoltageAvgCount,g_ECANInterrupts);
    TransmitStringUART1(line);
    sprintf(line,"ADC7: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tADC Interrupts: %05u\r\n",ADCValues[7],ADCVoltage[7],ADCVoltageMax[7],ADCVoltageMin[7],ADCVoltageAvgSum[7]/(double)ADCVoltageAvgCount,g_ADCCaptures);
    TransmitStringUART1(line);
    sprintf(line,"\t\t\t\t\tCAN TX Timeouts: %05u\r\n",g_ECANTransmitTimout);
    TransmitStringUART1(line);
    
    sprintf(line,"%c[1m\r\n\r\nSystem Boots: %03u \r\n",27,g_Config.BootCount);
    TransmitStringUART1(line);
    sprintf(line,"CAN ID1:%04x \t\t CAN ID2:%04x \r\n",g_Config.CanMessage1_ID, g_Config.CanMessage2_ID);
    TransmitStringUART1(line);
    sprintf(line,"ADC Capture Time: %05u TMR1 cycles (1.6us each) \r\n",g_ADCCaptureTime);
    TransmitStringUART1(line);
    sprintf(line,"Interrupt Service Time: %05u TMR1 cycles (1.6us each) \r\n",g_InterruptTime);
    TransmitStringUART1(line);
    sprintf(line,"Interrupt Overrun: %05u\r\n",g_TimerInterruptOverrun);
    TransmitStringUART1(line);
    
    sprintf(line,"DMA Interrupts: %05u\r\n",g_DMAInterrupts);
    TransmitStringUART1(line);
    
    sprintf(line,"%c[m%c[H",27,27);
    TransmitStringUART1(line);
      
    
}
