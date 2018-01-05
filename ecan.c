/* 
 * File:   ecan.c
 * Author: jeff.sponaugle
 *
 * Created on September 14, 2016, 9:36 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>  
#include "global.h"
#include "ecan.h"
#include "system.h"
#include "EEPROM.h"
#include "global.h"

/*
 * 
 */

// This is the actual DMA Buffer, which holds up to 4 messages.
ECAN1MSGBUF ecan1msgBuf __attribute__((space(dma),section(".dmabuffer"), aligned(ECAN1_MSG_BUF_LENGTH*16)));

/*
 *      BuildCANPackets() -     This routine till fill in the global CAN packet structures with data from the global
 *                              ADC structures.  Obviously this is not a nested thread safe process given the use of
 *                              single global variables.   This routine will eventually be significantly enhanced so it
 *                              can follow a configurable structure conversion.    This routeine does get the two CAN ID 
 *                              numbers from the CanMessageN_ID globals, and those globals are configured from the 
 *                              simulated EEPROM memory.   No return values or failures states.
 */
void BuildCANPackets()
{
    // Take the data from the ADC buffer and put them in the CAN Packet Buffers
    /* g_ADCValues[0]= 0x0123;
    g_ADCValues[1]= 0x0456;
    g_ADCValues[2]= 0x0789;
    g_ADCValues[3]= 0x0ABC; */
    
    
    g_CANPacket1[0] = (g_Config.CanMessage1_ID & 0x000007FF) << 2 ; // Simple SID
    g_CANPacket1[1] = 0;                                            // No EID
    g_CANPacket1[2] = 8;                                            // 8 bytes of data
    g_CANPacket1[3] = (g_ADCValues[0]<<4)|(g_ADCValues[1]>>8);      // Bytes 0 & 1  
    g_CANPacket1[4] = (g_ADCValues[1]<<8)|(g_ADCValues[2]>>4);      // Bytes 2 & 3
    g_CANPacket1[5] = (g_ADCValues[2]<<12)|(g_ADCValues[3]);        // Bytes 4 & 5
    g_CANPacket1[6] = g_CANSequenceNumber;                          // Bytes 6 & 7
    g_CANPacket1[7] = 0x00;                                         // Unused
    
    g_CANPacket2[0] = (g_Config.CanMessage2_ID & 0x000007FF) << 2 ; // Simple SID
    g_CANPacket2[1] = 0;                                            // No EID
    g_CANPacket2[2] = 8;                                            // 8 bytes of data
    g_CANPacket2[3] = g_ADCValues[4];                               // Bytes 0 & 1  
    g_CANPacket2[4] = g_ADCValues[5];                               // Bytes 2 & 3
    g_CANPacket2[5] = g_ADCValues[6];                               // Bytes 4 & 5
    g_CANPacket2[6] = g_CANSequenceNumber++;                        // Bytes 6 & 7
    g_CANPacket2[7] = 0x00;                                         // Unused
    
}

/*
 *      TransmitCANPackets() -     This routine will transmit the two global CAN packets as built using the 
 *                                 BuldCANPackets() routine.   The TransmitECANFrame function used to transmit
 *                                 will add the packet to the DMA transmit buffers and flag for transmission but
 *                                 will not wait for the transmit to complete.  If the transmission fails later, the
 *                                 DMA buffers are cleared for transmit and the transmission is lost.
 *                              
 */
void TransmitCANPackets()
{
    TransmitECANFrame( &g_CANPacket2 );
    TransmitECANFrame( &g_CANPacket1 );    
}

/*
 *      TransmitECANFrame() -   Transmit a single CAN frame.  The frame is put into the DMA buffer frame in a slot that is not
 *                              currently being used, and that slot is flagged for transmit.  Transmission happens automaticly
 *                              and if there is a problem that is detected in the interrrupt vector functions.
 *                              
 */
bool TransmitECANFrame(unsigned int (*packet)[])
{
 

    g_ECANTransmitTried++;
    // buffernumber is the DMAbuffer we are going to use. ECAN1_MSG_BUF_LENGTH is defined as the number of buffers in the DMA
    // space allocation.  That number (typically 4 or 8) is also configued in the CAN module.  The first 8 buffers in the DMA buffer 
    // are the only ones that can be used for ECAN transmission, so no need to have more than 8.  This code currently only uses the first
    // two buffers since we are only transmitting 2 frames at a time.  We should expand it to be more generic.
    // Once you fill a DMA buffer with stuff to send, you set a flag in the CAN module (corresponding to which buffer you are using)
    // to tell the CAN module to start transmitting (using DMA)
    int buffernumber = 0;
    
    int maxtime = 0;

    // We will first check the TXREQ0 flag, which tells us if buffer 0 is still being used. 

      
    if ( C1TR01CONbits.TXREQ0 == 1)
    {
        
        // Since buffer 0 is still being used, lets look to see if buffer 1 is available. 
        if ( C1TR01CONbits.TXREQ1 == 1)
        {
            // If both buffers are busy, we are going to loop and wait for buffer 0 to become free.  We could loop looking at both buffers,
            // but we know that since we submit buffer 0 before buffer 1, buffer 0 will be the first to get freed.  This while loop will
            // only delay 400us for the buffer to become free.  This number should really be based on the CAN bus rate.  In the case of a 
            // 1mbit bus a frame should be transmitted in less than 288us, so 400us is enough time.
            while ( C1TR01CONbits.TXREQ0 == 1)
            {
                maxtime++;
                DelayuS(10);
                if (maxtime >= 40) 
                {
                    // This is a failsafe.  If we are waiting for more than 400us for space, something is wrong and we will just skip
                    //   this packet transmission.
                    break;
                }
            }

            // At this point, we are either here because buffer 0 is available, or we have timed out.
            // Let's check the flag one last time.. if it is free we will use buffer 0, if not that means
            // we have timed out.  
            if ( C1TR01CONbits.TXREQ0 == 0)
            {
                buffernumber = 0;
            }
            else
            {
                // If we run out of time, something must be wrong.  We will set the buffernumber to 99 to indicate
                // a timeout condition.
                buffernumber = 99;
            }
           
        } 
        else
        {
            // it looks like buffer 1 is available for transmit. 
            buffernumber = 1;
        }
    }
    else
    {
        // Buffer 0 is free, so lets use it.
        buffernumber = 0;
    }

    // buffernumer is either 0,1, or 99.  
    
    //  If we timed out, add to the diag counters and fail out
    if ( buffernumber == 99)
    {
        g_ECANTransmitTimout++;
        return false;
    }

    //  We are good to overwrite a new message in the transmit buffer and send.  buffernumber has the buffer we should use.
    ecan1msgBuf[buffernumber][0] = (*packet)[0];   
    ecan1msgBuf[buffernumber][1] = (*packet)[1];   
    ecan1msgBuf[buffernumber][2] = (*packet)[2];   
    ecan1msgBuf[buffernumber][3] = (*packet)[3];
    ecan1msgBuf[buffernumber][4] = (*packet)[4];
    ecan1msgBuf[buffernumber][5] = (*packet)[5];
    ecan1msgBuf[buffernumber][6] = (*packet)[6];
   

    if (buffernumber == 0)
    {
        // Enable this buffer to transmit
        C1TR01CONbits.TXREQ0 = 1;
    }
    else
    {
        C1TR01CONbits.TXREQ1 = 1;
    }
    
    
    return true;

}

/*
 *      ConfigureECAN1() -   Configure the ECAN1 module for tranmission use.
 *                              
 */
void ConfigureECAN1()
{
    // Put CAN Module in Configuration mode and wait for it to get there.
    C1CTRL1bits.REQOP=4;
    while (C1CTRL1bits.OPMODE!=4);

    // Setup ECAN Timing Bits

    // Baud Rate Prescaler based on CAN_BRP_VAL.  This determines the basic CANbus datarate.
    C1CFG1bits.BRP = CAN_BRP_VAL;

    // Synronization Jump Width set of 4 (based on datasheet)
    C1CFG1bits.SJW = 0x3;
    // Phase 1 Segment Time is 8 (based on datasheet)
    C1CFG2bits.SEG1PH = 0x7;
    // Allow Phase 2 Segment time to be programmable. 
    C1CFG2bits.SEG2PHTS = 0x1;
    // and then set the Phase 2 Segment Time to 6 (based on datasheet)
    C1CFG2bits.SEG2PH = 0x5;
    // Propegation Segment time is 5 (based on datasheet)
    C1CFG2bits.PRSEG = 0x4;
    // Bus is sampled 3 times (based on datasheet)
    C1CFG2bits.SAM = 0x1;

    // Configure the size of the DMA buffer ( in number of messages).  Only the first 8 messages in the DMA buffer can be used for
    // transmit, so we will only support 4 or 8 message buffers.
    if ( ECAN1_MSG_BUF_LENGTH == 4)
    {
        C1FCTRLbits.DMABS = 0b000;   //  On a 24H this means the DMA buffer is 4 messages wide (8 words for each message)
    }
    else if (ECAN1_MSG_BUF_LENGTH == 8)
    {
        C1FCTRLbits.DMABS = 0b010;   //  On a 24H this means the DMA buffer is 8 messages wide (8 words for each message)
    }

    // Switch to Normal Operation mode.  This will loop and wait for the module to get ready.
    C1CTRL1bits.REQOP = 0;
    while (C1CTRL1bits.OPMODE!= 0);

    // Configure Buffer 0 (First buffer in the DMA Area) as a transmit buffer)

    C1TR01CONbits.TXEN0 =1;
    C1TR01CONbits.TX0PRI = 0b11;
    C1TR01CONbits.TXREQ0 = 0;
    
    // Configure Buffer 1 (Second buffer in the DMA Area) as a transmit buffer)
    
    C1TR01CONbits.TXEN1 =1;
    C1TR01CONbits.TX1PRI = 0b11;
    C1TR01CONbits.TXREQ1 = 0;

    // Configure the DMA Channel 0

    DMACS0 = 0;
    
    // DMA0CON - Disable Channel, Word Transfer Size, Direct to Peripheral from RAM, Interrupt on full transfer, 
    //           Normal Operation, Peripheral Indirect Addressing, Continuous no Ping-Pong
    DMA0CON = 0x2020;
    
    // DMA0PAD - This is set to the address of the ECAN module from the datasheet.  0x0442 is the Transmit ECAN Address (C1TXD))
    DMA0PAD = 0x0442;
    
    // DMA0CNT - Number of words to transfer (+1 is added to this number).   ECAN needs needs 6 words for a standard message
    // Note that while the 'message' in the DMA buffer is 8 words long, the CAN module will only request 6 words over DMA for a simple 
    // message.  If this number is configured incorrectly the only fault is the DMA interrupts will not occur at the correct interval.
    // Effectivly this number is just used to count DMA transfers for the purpose of firing the DMA interrupt on completion.
    DMA0CNT = 5;
    
    // DMA0REQ - Automatic DMA by Request, DMA Request from ' ECAN1 TX Data Request '
    DMA0REQ = 0x0046;
    
    // DMA0STA - Set to the address of the DMA buffer
    DMA0STA = __builtin_dmaoffset(&ecan1msgBuf);
    
    // Enable the DMA Channel now (which means as soon as the ECAN is triggered, it will start a DMA transfer)
    DMA0CONbits.CHEN = 1;

    // Enable ECAN1 Interrupts, and enable transmit interrupt.  This will turn on the ECAN1 interrupt, and configure that interrupt
    // to occur for either transmission completion, or error.
    IEC2bits.C1IE = 1;
    C1INTEbits.TBIE = 1;
    C1INTEbits.ERRIE = 1;

    // Enable the DMA Channel 0 completion interrupt.  This interrupt will fire at the completion of each DMA transfer.  This is being used
    // for debugging and statistics purposes only.  In production we will probably turn this off.
    IEC0bits.DMA0IE = 1;
    
    // Clear the full and overflow flags.
    C1RXFUL1=C1RXFUL2=C1RXOVF1=C1RXOVF2=0x0000;

    // ECAN1 is ready for transmission.  To send, set TXREQ flag for corresponding buffer.   ECAN Interrupt will confirm message transmission.
    

}
