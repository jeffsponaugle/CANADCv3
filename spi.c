/*
 * File:   spi.c
 * Author: jsponaugle
 *
 * Created on November 1, 2016, 7:08 PM
 */


#include "xc.h"
#include "system.h"
#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */
#include <string.h>
#include <stdio.h>
#include "global.h"
#include "uart.h"
/*
        Function:  SD_InitSPI(int speed)
 *      Purpose:  Initialize the SPI subsystem for SDCard Interactions.
 *      Parameters:
 *                  int speed - if == 0, then select low speed, if == 1 then select high speed
 *                  low speed = /64 and /4 dividers which will net 156Khz
 *                  high speed = /1 and /4 dividers which will net 10MHz  
 * 
 *      Assumptions:    FCY is 40MHZ ( PLL active with 2:1 and 40 MHz OSC connected )
 * 
 */
void SD_InitSPI(int speed)
{
    // Turn off SD Card Access and disable SPI
    SPI1STATbits.SPIEN = 0;
    LATBbits.LATB4=1;
    IFS0bits.SPI1IF = 0;
    IEC0bits.SPI1IE = 0;
    
    // Configure the SPI Interface - Mode 0,0
    SPI1CON1bits.DISSCK = 0;
    SPI1CON1bits.DISSDO = 0;
    SPI1CON1bits.MODE16 = 0;  // 8 bit packets
    SPI1CON1bits.MSTEN = 1;
    SPI1CON1bits.SMP = 1;
    SPI1CON1bits.CKE = 1;
    SPI1CON1bits.CKP = 0;
       
    if (speed == 0)
    {
        // Low Speed  - /64 and /4, 156KHz
        SPI1CON1bits.SPRE = 4;
        SPI1CON1bits.PPRE = 0;
        TransmitStringUART1("SDSPI Configured for 156KHz\r\n");
    }
    else
    {
        // High Speed  - /1 and /4, 10MHz
        SPI1CON1bits.SPRE = 6;
        SPI1CON1bits.PPRE = 3;
        TransmitStringUART1("SDSPI Configured for 10MHz\r\n");
    }
    
    SPI1STATbits.SPIEN = 1;
}


char SendSPI(char c)
{
    char x;
    TransmitStringUART1("Sending SPI:");
    TransmitIntUART1(c);
    TransmitStringUART1("/r/n");
    while (SPI1STATbits.SPITBF);
    SPI1BUF = c;
    while (!SPI1STATbits.SPIRBF);
    x=SPI1BUF;
    TransmitStringUART1("Received SPI:");
    TransmitIntUART1(x);
    TransmitStringUART1("/r/n");
    return x;
}


 uint8_t SD_Write(uint8_t b)
 /**
 *    Read and write a single 8-bit word to the SD/MMC card.
 *     Using standard, non-buffered mode in 8 bit words. 
 *    **Always check SPI1RBF bit before reading the SPI2BUF register
 *    **SPI1BUF is read and/or written to receive/send data
 *
 *    PRECONDITION: SPI bus configured, SD card selected and ready to use
 *    INPUTS: b = byte to transmit (or dummy byte if only a read done)
 *    OUTPUTS: none
 *    RETURNS: 
 */
 {
     uint8_t x;
     while (SPI1STATbits.SPITBF);
    //TransmitStringUART1("Sending SPI:");
    //TransmitIntUART1(b);
    //TransmitStringUART1("    ");
     SPI1BUF = b;                    // write to buffer for TX
     while( !SPI1STATbits.SPIRBF);    // wait for transfer to complete
     SPI2STATbits.SPIROV = 0;        // clear any overflow.
     x=SPI1BUF;                    // read the received value
    //TransmitStringUART1("Received SPI:");
    //TransmitIntUART1(x);
    //TransmitStringUART1("\r\n");
    return x;
  }
 
 // Not worth code defining these since they are all the same as SD_Write()
 #define SD_Read()   SD_Write( 0xFF)
 #define SD_Clock()   SD_Write( 0xFF)
 
 void SD_Disable()
 {
     LATBbits.LATB4=1;
     SD_Write(0xff);
 }
 
 void SD_Enable()
 {
     LATBbits.LATB4=0;
 }
 
uint8_t SD_SendCmd(uint8_t cmd, uint32_t addr)
 /**
 *    Send an SPI mode command to the SD card.
 *
 *    PRECONDITION: SD card powered up, CRC7 table initialized.
 *    INPUTS: cmd = SPI mode command to send
 *            addr= 32bit address
 *    OUTPUTS: none
 *    RETURNS: status read back from SD card (0xFF is fault)
 *    *** NOTE nMEM_CS is still low when this function exits.
 *
 *     expected return responses:
 *   FF - timeout 
 *   00 - command accepted
 *   01 - command received, card in idle state after RESET
 *
 *    R1 response codes:
 *   bit 0 = Idle state
 *   bit 1 = Erase Reset
 *   bit 2 = Illegal command
 *   bit 3 = Communication CRC error
 *   bit 4 = Erase sequence error
 *   bit 5 = Address error
 *   bit 6 = Parameter error
 *   bit 7 = Always 0
 */
 {
     uint16_t     n;
     uint8_t        res;
     uint8_t        byte;
     uint8_t        CRC7 = 0;
 
     //TransmitStringUART1("Sending Command: ");
     //TransmitIntUART1(cmd);
     //TransmitStringUART1(" ");
     
     SD_Enable();                    // enable SD card
     
     byte = cmd | 0x40;
     SD_Write(byte);                    // send command packet (6 bytes)
     //CRC7 = CRCAdd(CRC7, byte);
     byte = addr>>24;
     SD_Write(byte);                   // msb of the address
     //CRC7 = CRCAdd(CRC7, byte);
     byte = addr>>16; 
     SD_Write(byte);
     //CRC7 = CRCAdd(CRC7, byte);
     byte = addr>>8;
     SD_Write(byte);
     //CRC7 = CRCAdd(CRC7, byte);
     SD_Write( addr);                   // lsb
     //CRC7 = CRCAdd(CRC7,addr);
     if (cmd ==0)
        CRC7 = 0x4a;
     if (cmd ==8)
         CRC7 = 0x87 >>1;
     
     CRC7 = (CRC7 <<1) | 0x01;        // CRC7 always has lsb = 1
     
     SD_Write(CRC7);                    // Not used unless CRC mode is turned back on for SPI access.
 
     n = 20;                            // now wait for a response (allow for up to 8 bytes delay)
     do {
         res = SD_Read();              // check if ready   
         if ( res != 0xFF) 
             break;
     } while ( --n > 0);
 
     //TransmitStringUART1("Result was: ");
     //TransmitIntUART1(res);
     //TransmitStringUART1("\r\n");
     return (res);                     // return the result that we got from the SD card.
 }
 
 
 uint8_t SD_InitMedia( void)
 /**
 *    Discover the type and version of the installed SD card.  This routine
 *    will find any SD or SDHC card and properly set it up.
 *
 *    PRECONDITION: none
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURNS: 0 if successful, some other error if not.
 */
 {
     uint16_t     n;
     uint8_t     res = 0;                        // If we get that far...
     uint32_t    timer;
     uint8_t        cmd;
     uint8_t        db[16];                            // for when we get some data back to look at
 
     //if (sdcard.cardInit == 1) {    
     //    return(0);                                // done, don't do it again.
     //}
     
     SD_Disable();                                 // 1. start with the card not selected
     for ( n=0; n<10; n++)                        // 2. send 80 clock cycles so card can init registers
         SD_Clock();
     SD_Enable();                                // 3. now select the card
 
     res = SD_SendCmd( 0, 0); SD_Disable();    // 4. send a reset command and look for "IDLE"
     if ( res != 1) {
         SD_Disable(); 
         TransmitStringUART1("SDCARD: Card did not respond to Init.[ERROR]\r\n");
         return(0);                              // card did not respond with "idle", diagnostic value
     }
      
     res = SD_SendCmd(8, 0x000001AA);    // 5. Check card voltage (type) for SD 1.0 or SD 2.0
     if ( (res == 0xFF) || (res == 0x05)) {             // didn't respond or responded with an "illegal cmd"
         //sdcard.cardVer = 1;                                  // means it's an SD 1.0 or MMC card
         TransmitStringUART1("SDCARD:Card is Version 1.0\r\n");
            
         timer = g_TimerMSTotal + 300;                               // 6. send INIT until receive a 0 or 300ms passes
          while(timer > g_TimerMSTotal) {                        
             res = SD_SendCmd(1,0);
             SD_Disable();                                     // SendSDCmd() enables SD card
             if (!res) {
                 break;                                           // The card is ready
             }
         }
         if (res != 0) {
             TransmitStringUART1("SDCARD: Card failed reset sequence.[ERROR]\r\n");
             return(0);                // failed to reset.
         }
         SD_Disable();                                      // remember to disable the card
      }
     else {                                                  // need to pick up 4 bytes for v2 card voltage description
         TransmitStringUART1("SDCARD:Card is Version 2.0\r\n");
         //sdcard.cardVer = 2;                        // SD version 2.0 card
         for (n=0; n<4; n++) {
             db[n] = SD_Read();
         }                                                       // but we'll ignore it for now, we know what the card is
         SD_Disable();
              
         cmd = 41;                        // 6. send INIT or SEND_APP_OP repeatedly until receive a 0
          timer = g_TimerMSTotal + 600;                         // wait up to .3 seconds for signs of life
          //res = SD_SendCmd(55, 0); SD_Disable();    // will still be in idle mode (0x01) after this
            while (timer > g_TimerMSTotal) {
                res = SD_SendCmd(55, 0); SD_Disable();    // will still be in idle mode (0x01) after this
             res = SD_SendCmd(cmd, 0x40000000); SD_Disable();
              if ( (res &0x0F) == 0x05 ) {        // ACMD41 not recognized, use CMD1
                  cmd = 1;
              }
              else {
                  cmd = 41;
             }
             if (!res) {
                 break;
             } 
         }
         if (res != 0) {
             TransmitStringUART1("SDCARD: Card failed init sequence.[ERROR]\r\n");
             return(0);                          // failed to reset.
         }
  
          res = SD_SendCmd(58,0);            // 7. Check for capacity of the card
          if (res != 0) {
              TransmitStringUART1("SDCARD: Card failed capacity sequence.[ERROR]\r\n");
              return(0);                          // error, bad thing.
          }
          for (n=0; n<4; n++) {
              db[n] = SD_Read();
         }
         SD_Disable();
         if ( ((db[0] & 0x40) == 0x40) && (db[0] != 0xFF) ) { // check CCS bit (bit 30), PoweredUp (bit 31) set if ready.
              //sdcard.cardCap = 1;                            // card is high capacity, uses block addressing
             TransmitStringUART1("SDCARD:Card is High Capacity\r\n");
         }
         else{
             TransmitStringUART1("SDCARD:Card is Low Capacity\r\n");
              //sdcard.cardCap = 0;                // card is low capacity, uses byte addressing
         }        
     }
     
     //sdcard.cardInit = 1;                          // successfully initialized the SD card.
     //sdcard.cardBlock = SD_BSIZE;        // for completeness' sake
     
     // Get the CSD register to find the size of the card
     res = SD_SendCmd(9,0);
     if (res != 0) {
         TransmitStringUART1("SDCARD: Card failed get CSD sequence.[ERROR]\r\n");
         return(0);
     }
     timer = g_TimerMSTotal + 300;                     // wait for a response
     while(timer > g_TimerMSTotal) {
         res = SD_Read();
         if (res == 0xFE) {
             break;
         }
     }
     if (res == 0xFE) {            // if it did not timeout, read a sector of data
         for (n=0; n< 16; n++) { 
             db[n] = SD_Read();            // read the received value
         }
         // ignore CRC (for now)
         SD_Read();
         SD_Read();
         SD_Disable();
         TransmitStringUART1("SDCARD:SD Card Size: ");
         uint16_t sizelow = (db[8]<<8) | db[9];
         uint16_t sizehigh = (db[7]&0x3F);
         sizelow++;
         
         TransmitIntUART1(sizehigh );
         TransmitIntUART1(sizelow);
         
         TransmitStringUART1(" Blocks - ");
         double totalsize = sizelow * 512.0 * 1024.0 + ( sizehigh * 512.0 * 1024.0 * 65536.0 );
         char p[34];
         sprintf(p,"%16.0f",totalsize);
         TransmitStringUART1("(");
         TransmitStringUART1(p);
         TransmitStringUART1(" bytes)\r\n");
         
     }
     else {
         return(0);
     }
     /*
     if (sdcard.cardCap == 1) {            // Uses the SDHC capacity calculation
         sdcard.cardSize = db[9] + 1;
         sdcard.cardSize += (uint32_t)(db[8] << 8);
         sdcard.cardSize += (uint32_t)(db[7] & 0x0F)<<12;
         sdcard.cardSize *= 524288;                // multiply by 512KB
         // (C_SIZE + 1) * 512 * 1024
         sdcard.cardNumBlocks = sdcard.cardSize/sdcard.cardBlock;
     }
     else {                                // Uses the SD capacity calculation
         sdcard.cardSize = ((uint16_t)((db[6] & 0x03)<<10) | (uint16_t)(db[7]<<2) | (uint16_t)((db[8] & 0xC0)>>6)) + 1;
         sdcard.cardSize = sdcard.cardSize <<(((uint16_t)((db[9] & 0x03)<<1) | (uint16_t)((db[10] & 0x80)>>7)) +2);
         sdcard.cardSize = sdcard.cardSize <<((uint16_t)(db[5] & 0x0F));
         // (C_SIZE +1) <<(C_SIZE_MULT + 2) <<(READ_BL_LEN), then set SET_BLOCKLEN to be 512 next.
         sdcard.cardNumBlocks = sdcard.cardSize/sdcard.cardBlock;
         res = SD_SendCmd(SET_WBLEN,0x00000200);        // Set block size to 512 bytes
         SD_Disable();
    }
     */
     res = SD_SendCmd( 59, 0); SD_Disable(); 
     return(res);           
 }
 
 int SD_Read_Sector(uint32_t address, char* buffer)
 {
     uint8_t     res = 0;                        // If we get that far...
     uint32_t    timer;
     uint8_t        cmd;
     int n;
     int c;
     int delay=0;
     uint16_t timestart,timestop;
     
     timestart = g_TimerMSTotal;
     
     for (c=0;c<100;c++)
     {
         
     
        // Assume buffer has 512 bytes of space
        //TransmitStringUART1("SDCARD: Starting read of sector\r\n");
        if (buffer == 0) return 0;
        res = SD_SendCmd(17,address);

        if (res != 0) {
            TransmitStringUART1("SDCARD: Sector Read Command Failure.[ERROR]\r\n");
            return(0);
        }
        delay=0;
        timer = g_TimerMSTotal + 300;                     // wait for a response
        while(timer > g_TimerMSTotal) {
            res = SD_Read();
            delay++;
            if (res == 0xFE) {
                //TransmitStringUART1("SDCARD: Got start of data flag\r\n");
                break;
            }
        }

        if (res == 0xFE) {            // if it did not timeout, read a sector of data
            for (n=0; n< 512; n++) { 
                buffer[n] = SD_Read();            // read the received value
                //TransmitByteUART1(buffer[n]);
            }
            //TransmitStringUART1("\r\n");
            // ignore CRC (for now)
            SD_Read();
            SD_Read();
            SD_Disable();
        }
        else
        {
            return(0);
        }
     }
     
     timestop = g_TimerMSTotal;
     TransmitStringUART1("\r\nElapsed time:");
     TransmitIntUART1(timestop-timestart);
     TransmitStringUART1(" ms\r\n");
     TransmitIntUART1(delay);
     
     
     
     
 }
 
 
void Write7Segment(unsigned int val)
{
    char number[7];
    sprintf(number,"%04u",val);
    SendSPI(number[0]);
    SendSPI(number[1]);
    SendSPI(number[2]);
    SendSPI(number[3]);
    
}
