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


void BuildCANPackets()
{
    // Take the data from the ADC buffer and put them in the CAN Packet Buffers
    
    g_CANPacket1[0] = (g_Config.CanMessage1_ID & 0x000007FF) << 2 ;   // Simple SID
    g_CANPacket1[1] = 0;                          // No EID
    g_CANPacket1[2] = 8;                          // 8 bytes of data
    g_CANPacket1[3] = ADCValues[0];               // Bytes 0 & 1  
    g_CANPacket1[4] = ADCValues[1];               // Bytes 2 & 3
    g_CANPacket1[5] = ADCValues[2];               // Bytes 4 & 5
    //g_CANPacket1[6] = ADCValues[3];             // Bytes 6 & 7
    g_CANPacket1[6] = 0xAABB;                       // Bytes 6 & 7
    g_CANPacket1[7] = 0x00;                       //  Unused
    
    g_CANPacket2[0] = (g_Config.CanMessage2_ID & 0x000007FF) << 2 ;   // Simple SID
    g_CANPacket2[1] = 0;                          // No EID
    g_CANPacket2[2] = 8;                          // 8 bytes of data
    g_CANPacket2[3] = ADCValues[4];               // Bytes 0 & 1  
    g_CANPacket2[4] = ADCValues[5];               // Bytes 2 & 3
    g_CANPacket2[5] = ADCValues[6];               // Bytes 4 & 5
    //g_CANPacket2[6] = ADCValues[7];             // Bytes 6 & 7
    g_CANPacket2[6] = 0xBBCC;                       // Bytes 6 & 7
    g_CANPacket2[7] = 0x00;                       //  Unused
    
}


void TransmitCANPackets()
{
    TransmitECANFrame( &g_CANPacket2 );
    TransmitECANFrame( &g_CANPacket1 );
    
}

bool TransmitECANFrame(unsigned int (*packet)[])
{
   //  If the TXREQ bit is still set, there is a transmission in process.

    int buffernumber = 0;
    
    int maxtime = 0;
    // If the TXREQ0 bit is still set, the previous message has not be transmitted.  This is normal if we have been called right after a previous send.
    // However, it should never stay in this state for more then a few ms.  (message is ~144 bits at 500kbs = 288us).   This will timeout the transmision after
    // 4000 us, or 4ms.    Given the timer for sending packets is set to 10ms, this should allow both packet transmission to fail and still return before missing an interrupt.

   
    if ( C1TR01CONbits.TXREQ0 == 1)
    {
        
        // If buffer 0 is still transmitting, lets try using buffer 1
        if ( C1TR01CONbits.TXREQ1 == 1)
        {
            // If both buffers are busy, lets wait a bit and see it buffer zero clears out.
            while ( C1TR01CONbits.TXREQ0 == 1)
            {
                maxtime++;
                DelayuS(10);
                if (maxtime >= 40) 
                {
                    // Set buffernumber to 99 to indicate no buffer found before timeout.
                    buffernumber = 99;
                    break;
                }
            }
            // At this point, we are either here because buffer 0 is available, or we have timed out.
            
            if ( C1TR01CONbits.TXREQ0 == 0)
            {
                buffernumber = 0;
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
        buffernumber = 0; // Buffer 0 is ready for use
    }

    // buffernumer is either 0,1, or 99.  
    
    //  If we timed out, add to the diag counters and fail out
    if ( buffernumber == 99)
    {
        g_ECANTransmitTimout++;
        return false;
    }

    //  We are good to overwrite a new message in the transmit buffer and send.

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
    
    g_ECANTransmitTried++;
    return true;

}


void ConfigureECAN1()
{
    // Put CAN Module in Configuration mode
    C1CTRL1bits.REQOP=4;
    while (C1CTRL1bits.OPMODE!=4);

    // Setup ECAN Timing Bits
    C1CFG1bits.BRP = CAN_BRP_VAL;
    C1CFG1bits.SJW = 0x3;
    C1CFG2bits.SEG1PH = 0x7;
    C1CFG2bits.SEG2PHTS = 0x1;
    C1CFG2bits.SEG2PH = 0x5;
    C1CFG2bits.PRSEG = 0x4;
    C1CFG2bits.SAM = 0x1;

    // Configure DMA Buffers
    C1FCTRLbits.DMABS = 0b010;   //  On a 24H this means the DMA buffer is 4 messages wide (8 words for each message)

    // Switch to Normal Operation mode
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
    
    // DMA0CNT - Number of words to transfer (+1 is added to this number).   ECAN needs 8 words of data per transfer, so set to 7.
    DMA0CNT = 5;
    
    // DMA0REQ - Automatic DMA by Request, DMA Request from ' ECAN1 TX Data Request '
    DMA0REQ = 0x0046;
    
    // DMA0STA - Set to the address of the DMA buffer
    DMA0STA = __builtin_dmaoffset(&ecan1msgBuf);
    
    // Enable the DMA Channel now (which means as soon as the ECAN is triggered, it will start a DMA transfer)
    DMA0CONbits.CHEN = 1;

    // Enable ECAN1 Interrupts, and enable transmit interrupt.
    IEC2bits.C1IE = 1;
    C1INTEbits.TBIE = 1;
    C1INTEbits.ERRIE = 1;
    IEC0bits.DMA0IE = 1;
    
    // ECAN1 is ready for transmission.  To send, set TXREQ flag for corresponding buffer.   ECAN Interrupt will confirm message transmission.
    C1RXFUL1=C1RXFUL2=C1RXOVF1=C1RXOVF2=0x0000;

}
