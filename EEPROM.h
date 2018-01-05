/* 
 * File:   EEPROM.h
 * Author: jeff.sponaugle
 *
 * Created on September 18, 2016, 9:36 PM
 */

#ifndef EEPROM_H
#define	EEPROM_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <stdint.h>        /* Includes uint16_t definition   */ 
#include <stdbool.h>
#include "DEE Emulation 16-bit.h"

// The structure of the EEPROM memory
#define EE_ADDR_VERSION 0   //  Address of version/id tag.
#define EE_ADDR_SIGNATURE 2 //  Addresss of signature (0xAA)
#define EE_ADDR_DATA 3      // Address of start of data.
#define EE_ADDR_SIZE 1      // Address of size of data
// The header is 3 bytes ( Version, Signature, and size of the data chunk)
// The data section starts at address EE_ADDR_DATA, and is of size EE_ADDR_SIZE
// This self described the data section, and you can read that entire data section into the struction
// defined below.
#define EE_TOTALSIZE 255    // Maximum size of the EE array (255 elements)
#define EE_DATASPACESIZE 251    // Size of the data area.  It is the total size minus 4 (since we have 4 elements before the data)

#define EE_CURRENT_VERSION 0x01    
#define EE_CURRENT_SIGNATURE 0xAA
    
   
    
    typedef struct {
        int8_t version;
        unsigned int CanMessage1_ID;
        unsigned int CanMessage1_DATA0;
        unsigned int CanMessage1_DATA1;
        unsigned int CanMessage1_DATA2;
        unsigned int CanMessage1_DATA3;
        unsigned int CanMessage1_DATA4;
        unsigned int CanMessage1_DATA5;
        unsigned int CanMessage1_DATA6;
        unsigned int CanMessage2_ID;
        unsigned int CanMessage2_DATA0;
        unsigned int CanMessage2_DATA1;
        unsigned int CanMessage2_DATA2;
        unsigned int CanMessage2_DATA3;
        unsigned int CanMessage2_DATA4;
        unsigned int CanMessage2_DATA5;
        unsigned int CanMessage2_DATA6;
        unsigned int BootCount;
        unsigned int CanStartup_ID;
        unsigned int CanStartup_SerialNumber;
        
    } st_CAL;
    
    void ConfigurationSystemInit();
    bool WriteConfig(st_CAL *Cal);
    bool ReadConfig(st_CAL *Cal);
    bool FillConfigWithDefault(st_CAL* Config);
    extern st_CAL g_Config;
    
    
/*
 Message_DATA types
 *      0x00 = Fill with zero
 *      0x01 = CH0 LSB
 *      0x02 = CH0 MSB
 *      0x04 = CH1 LSB
 *      0x05 = CH1 MSB
 *      0x06 = CH2 LSB
 *      0x07 = CH2 MSB
 *      0x08 = CH3 LSB
 *      0x09 = CH3 MSB
 *      0x0a = CH4 LSB
 *      0x0b = CH4 MSB 
 *      0x0c = CH5 LSB
 *      0x0d = CH5 MSB 
 *      0x0e = CH6 LSB
 *      0x0f = CH6 MSB  
 *      0x10 = CH7 LSB
 *      0x11 = CH7 MSB
 * 
 * 
 */
    

#ifdef	__cplusplus
}
#endif


#endif	/* EEPROM_H */

