/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */
#include <string.h>
#include <stdio.h>

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "interrupt.h"
#include "global.h"        // externs for all global variables
#include "adc.h"
#include "uart.h"
#include "ecan.h"
#include "timer1.h"
#include "EEPROM.h"
#include "spi.h"
#include "main.h"



/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

// START - Global Data
//      Global Constants
const unsigned int g_ADCBufferSize = 10;
//      Global Data Storage
unsigned int g_ADCValuesBuffer[8][10];           // Buffer used for storing the last 10 samples for each channel.  Used for decimation.
unsigned int g_ADCValuesBufferIndex=0;           // Index into the above buffer. Points to location next sample should go.
unsigned int g_ADCValues[8] = {0,0,0,0,0,0,0,0}; // The post decimation ADC Values. These are the values to be sent over CAN (after conversion)
unsigned int g_ADC5VReferenceRaw;                // The last 5V VCC measurement divided by 3. 
unsigned int g_TimerSeconds = 0;                 // Current system timer (seconds)
unsigned int g_TimerMS = 0;                      // Current system timer ms
unsigned int g_TimerMSTotal = 0;
unsigned int g_CANSequenceNumber = 0;
uint16_t g_CANPacket1[8];                        // Buffer for final CAN message 1
uint16_t g_CANPacket2[8];                        // Buffer for final CAN message 2

//      Global Diagnostic Storage
double ADCVoltage[8]={0,0,0,0,0,0,0};           // ADC values converted to voltage display. 
double ADCVoltageMin[8]={0,0,0,0,0,0,0};        // ADC minimum values (in voltage).  Cleared every 1000 displays.
double ADCVoltageMax[8]={0,0,0,0,0,0,0};        // ADC maximum values (in voltage).  Cleared every 1000 displays.
double ADCVoltageAvgSum[8]={0,0,0,0,0,0,0};     // ADC 'sum for average' values (in voltage).  Cleared every 1000 displays.
unsigned int ADCVoltageAvgCount=0;              // Count of number of 'sums' in above 'sum for average' variable.  Rolls to 0 at 1000.
unsigned int g_ADCCaptureTime = 0;              // Max Number of timer1 (1.6us) ticks from start of timer1 interrupt to ADCs complete.
unsigned int g_ADCCaptures=0;                   // Number of ADC captures completed (8 per sample interval)
unsigned int g_InterruptTime=0;                 // Max Number of timer1 (1.6us) ticks from start of timer1 interrupt to timer1 int complete.
unsigned int g_ECANTransmitTried = 0;           // Number of ECAN frames built for transmission and sent to CAN controller.
unsigned int g_ECANTransmitTimout = 0;          // Number of timeouts waiting for transmission buffer to become available (should be 0)
unsigned int g_ECANTransmitCompleted = 0;       // Number of completed CAN transmissions (from the CAN TX interrupt)
unsigned int g_ECANError = 0;                   // Number of ECAN1 errors (from CAN Error Interrupt)
unsigned int g_ECANTXBO = 0;                    // Number of ECAN1 'Transmitter Bus is Off State' errors (from CAN Error Interrupt)
unsigned int g_ECANTXBP = 0;                    // Number of ECAN1 'Transmitter Bus is Passive State' errors (from CAN Error Interrupt)
unsigned int g_ECANTXWAR = 0;                   // Number of ECAN1 'Transmitter Bus is Error State' errors (from CAN Error Interrupt)
unsigned int g_ECANEWARN = 0;                   // Number of ECAN1 'At least one error state' errors (from CAN Error Interrupt)
unsigned int g_ECANIVRIF = 0;                   // Number of ECAN1 'Invalid Message Received' errors (from CAN Error Interrupt)
unsigned int g_ECANInterrupts = 0;              // Number of ECAN1 Interrupts (either TX success or errors)
unsigned int g_DMAInterrupts = 0;               // Number of DMA0 Interrupts (not used)
unsigned int g_TimerInterruptOverrun = 0;       // Number of times a pending timer1 interrupt was present at the end of the timer1 handler
unsigned int g_UARTReceiveErrors = 0;           // Number of UART1 receive errors
unsigned int g_UARTReceiveOverflowErrors = 0;   // Number of UART1 receive overflow errors


//      Global Calibration Data - These should be stored in the global config.
const double caloffset[8]={5.0,5.0,5.0,5.0,5.0,5.0,5.0,5.0};
const double calslope[8]={819.83,820.47,819.19,819.40,818.34,819.18,818.76,820.04};
st_CAL g_Config;

// END - Global Data

/*
 *   StartupConfigurationPhase1() - This is one of the first functions called on system start.  It will reconfigure the clocks, the pin 
 *                                  assignments, and ADC module, and the UARTs.
 */
void StartupConfigurationPhase1()
{
    // First let's switch the internal clock to 80MHZ using the PLL (from 40MHz)
    ConfigureOscillator();
    // Clear the IFS flags
    IFS0=0;
    IFS1=0;
    IFS2=0;
    IFS3=0;
    IFS4=0;
    // InitApp configures the pin inputs and outputs
    InitApp();
    // Once the pins are configured, we can set up the peripherals, starting with the serial port output.
    SetupUART1();
    // Next up configure the analog to digital converter peripheral.
    SetupADC();
    // We still need to configure the timer and CAN device, but that will be done in phase 2.
}

/*
 *   StartupConfigurationPhase2() - This is the second phase of system startup, including starting the timer which will start 
 *                                  both ADC conversions, as well as CAN output, as well as configuring the ECAN module.
 */
void StartupConfigurationPhase2()
{
    // First lets configure the ECAN module.
    ConfigureECAN1();
    // Setup Timer1.  This function will both configure, and start timer 1.  Once timer1 starts, data collection and 
    // CAN transmission will happen
    DelaymS(1);
    SetupTimer1();
    DelaymS(10); 
}


int16_t main(void)
{
    // First phase of startup config.
    StartupConfigurationPhase1();    
    //  The phase 1 includes configuring the serial port console output, so we can start console logging.
    syslog("\r\nCAN+ADC Interface Version 1.0\r\n");
    //  Let's read the system config from the EEPROM.  This will include information about the CAN system as well as logging and filter 
    // information.
    ConfigurationSystemInit();
    // Small delay before configuring the CAN device and the timer.
    DelaymS(1);
    // Second phase of startup config, incuding the CAN system and the timer interrupts.
    StartupConfigurationPhase2();
    
    // At this point, the system in functioning with ADC conversion and CAN output.   Everything from this point on it just for 
    // console output, configuration, and system monitoring.

    // Delay for 10ms while timer1 interrupts and CAN transfers start, the toggle the LED output
    DelaymS(10);  
    LATAbits.LATA4=0;
    DelaymS(10);
    LATAbits.LATA4=1;
    DelaymS(200);

    syslog("\r\nPress any key to start\r\n");
    // Let's wait for a character before we get started.
    // bool ReceiveUART1String(char* returnstring, unsigned int maxlength, bool returnonCRorLF)
    //char test[10];
    //ReceiveUART1String(test,10,true,true);
    // Start the console.  This function does not return.
    Console();   
    return 0;
}

void Console()
{
    char p[20];

    // Send a clear-screen command to the syslog
    sprintf(p,"%c[2J %c[H",27,27);
    syslog(p);
    
    // Initialize the diagnostic variables for each ADC channel (min/max/avg) to the current value.  
    ADCVoltageAvgCount = 0;
    
    // Run a loop displaying the console statistics.  Eventually we will add an interacive interface to allow configuration
    // of the CAN IDs, formats, and calbrations.
    while(1)
    {
        UpdateDiagnosticADCVariables();
        DisplayStatus();
        DelaymS(100);
        
    }

}
void UpdateDiagnosticADCVariables()
{
    int i;

    //  Auto reset - if we get to 1000 averages, lets set to 0 and reset all the variables.
    if (ADCVoltageAvgCount == 1000)
    {
        ADCVoltageAvgCount = 0;
    }


    for (i=0;i<=7;i++)
    {
        unsigned int CalibratedValue;
        //ADCVoltage[i]=((double)g_ADCValues[i]-caloffset[i])/calslope[i];
        if (g_ADCValues[i]<=caloffset[i])
        {
            CalibratedValue=0;
        }
        else
        {
            CalibratedValue = g_ADCValues[i]-caloffset[i];
        }
        
       
        ADCVoltage[i]=(double)(CalibratedValue/calslope[i])+.01;
        //ADCVoltage[i]=(double)(g_ADCValues[i]);
        
        // If ADCVoltageAvgCount is 0, this is the first time this routine has been called, or we are resetting  We will initialize all of the
        // diagnostic variables, set the count to one, and record the first value for the average sum array.

        if (ADCVoltageAvgCount == 0)
        {
            ADCVoltageAvgSum[i] = ADCVoltage[i];
            ADCVoltageMax[i]=ADCVoltage[i];
            ADCVoltageMin[i]=ADCVoltage[i];
        } 
        else
        {
            // If this is not the first time in, lets add to the average accumulator, and update the min and max variables.
            ADCVoltageAvgSum[i] += ADCVoltage[i];
            if (ADCVoltage[i] > ADCVoltageMax[i]) ADCVoltageMax[i]=ADCVoltage[i];
            if (ADCVoltage[i] < ADCVoltageMin[i]) ADCVoltageMin[i]=ADCVoltage[i];
            
        } 
    }
    
    // Now we need to increment the ADCVoltageAvgCount.  This will accuratly represent the number of samples that were
    // accumlated in the AvgSum array.   
    ADCVoltageAvgCount++;
        
}

void DisplayStatus()
{
    
    char line[100];
  

    // Every 1 min, we send a clear screen command, to help in debugging
    
    if (g_TimerSeconds >60 )
    {
        g_TimerSeconds = 0;
        sprintf(line,"%c[2J %c[H",27,27);
        syslog(line);
    }

    double ADC5VReferenceV = ((g_ADC5VReferenceRaw*2.49)/4096.0)*3.0;

    sprintf(line,"%c[HADC-CAN Status\r\n",27);
    syslog(line);
    sprintf(line,"------------------------------------------\r\n" );
    syslog(line);
    sprintf(line,"Timer:%u.%u %u\r\n",g_TimerSeconds,g_TimerMS,ADCVoltageAvgCount);
    syslog(line);
    sprintf(line,"ADC Values          MAX     MIN     AVG\t\t\tCAN Status\r\n");
    syslog(line);
    sprintf(line,"ADC0: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX: %05u\r\n",g_ADCValues[0],ADCVoltage[0],ADCVoltageMax[0],ADCVoltageMin[0],ADCVoltageAvgSum[0]/(double)ADCVoltageAvgCount,g_ECANTransmitCompleted);
    syslog(line);
    sprintf(line,"ADC1: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Tried: %05u\r\n",g_ADCValues[1],ADCVoltage[1],ADCVoltageMax[1],ADCVoltageMin[1],ADCVoltageAvgSum[1]/(double)ADCVoltageAvgCount,g_ECANTransmitTried);
    syslog(line);
    sprintf(line,"ADC2: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Error BO: %05u\r\n",g_ADCValues[2],ADCVoltage[2],ADCVoltageMax[2],ADCVoltageMin[2],ADCVoltageAvgSum[2]/(double)ADCVoltageAvgCount,g_ECANTXBO);
    syslog(line);
    sprintf(line,"ADC3: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Error BP: %05u\r\n",g_ADCValues[3],ADCVoltage[3],ADCVoltageMax[3],ADCVoltageMin[3],ADCVoltageAvgSum[3]/(double)ADCVoltageAvgCount,g_ECANTXBP);
    syslog(line);
    sprintf(line,"ADC4: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Error WARN: %05u\r\n",g_ADCValues[4],ADCVoltage[4],ADCVoltageMax[4],ADCVoltageMin[4],ADCVoltageAvgSum[4]/(double)ADCVoltageAvgCount,g_ECANTXWAR);
    syslog(line);
    sprintf(line,"ADC5: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Error IVRIF: %05u\r\n",g_ADCValues[5],ADCVoltage[5],ADCVoltageMax[5],ADCVoltageMin[5],ADCVoltageAvgSum[5]/(double)ADCVoltageAvgCount,g_ECANIVRIF);
    syslog(line);
    sprintf(line,"ADC6: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tCAN TX Interrupts: %05u\r\n",g_ADCValues[6],ADCVoltage[6],ADCVoltageMax[6],ADCVoltageMin[6],ADCVoltageAvgSum[6]/(double)ADCVoltageAvgCount,g_ECANInterrupts);
    syslog(line);
    sprintf(line,"ADC7: %04u %+4.3fV %+4.3fV %+4.3fV %+4.3fV\t\tADC Interrupts: %05u\r\n",g_ADCValues[7],ADCVoltage[7],ADCVoltageMax[7],ADCVoltageMin[7],ADCVoltageAvgSum[7]/(double)ADCVoltageAvgCount,g_ADCCaptures);
    syslog(line);
    sprintf(line,"VCC5: %04u %+4.3fV\t\t\tCAN TX Timeouts: %05u\r\n",g_ADC5VReferenceRaw,ADC5VReferenceV,g_ECANTransmitTimout);
    syslog(line);
    sprintf(line,"%c[1m\r\n\r\nSystem Boots: %03u \r\n",27,g_Config.BootCount);
    syslog(line);
    sprintf(line,"CAN ID1:%04x \t\t CAN ID2:%04x \r\n",g_Config.CanMessage1_ID, g_Config.CanMessage2_ID);
    syslog(line);
    sprintf(line,"ADC Capture Time: %05u TMR1 cycles (1.6us each) \r\n",g_ADCCaptureTime);
    syslog(line);
    sprintf(line,"Interrupt Service Time: %05u TMR1 cycles (1.6us each) \r\n",g_InterruptTime);
    syslog(line);
    sprintf(line,"Interrupt Overrun: %05u\r\n",g_TimerInterruptOverrun);
    syslog(line);
    sprintf(line,"DMA Interrupts: %05u\r\n",g_DMAInterrupts);
    syslog(line);    
    sprintf(line,"CAN ERRIF Interrupts: %05u\r\n",g_ECANError);
    syslog(line);    
    sprintf(line,"%c[m%c[H",27,27);
    syslog(line);
      
    
}
