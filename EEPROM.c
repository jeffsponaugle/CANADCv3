#include "EEPROM.h"
#include "uart.h"


extern st_CAL g_Config;
/*
 *      ConfigurationSystemInit() - This function will check the 'configuration memory' for a valid config, and if there populate all of
 *                                  the configuration stuff into the g_Config structure  If the 'configuration memory' is either invalid
 *                                  or empty, default values will be populated. 
 * 
 * 
 */
void ConfigurationSystemInit()
{
    int ConfigGood=0;
    
    TransmitStringUART1("Config System Init\r\n");
    // Lets start off by filling the config data with the default values before we look at the config memory.
    FillConfigWithDefault(&g_Config);    //  Lets put some default values in the global static config (g_Config)

    // We are using a library that simulates an EEPROM using onboard flash memory. (called DEE Emulation 16-bit)
    // This library simulates the EEPROM by using a list of address-data elements that are write ordered.  That means an given address 
    // may be in that list more than once, and the last value is the one to use.

    //  The Init function looks for that flash segment and maps it into a structure.  
    if (DataEEInit() == 0)
    {
        TransmitStringUART1("Init successful\r\n");
        // Since the init was successful, we need to look and see if the memory has a valid configuration in it that we can use.
        // If the config is valid, we will read it and fill out the g_Config structure.  If it is invalid (or not yet initiliazed)
        // we will initialize it with defauts so next time it will work.
    
        unsigned int version, signature;
        // Let's check and see if the signature is valid
        if ((signature=DataEERead(EE_ADDR_SIGNATURE)) !=0xFFFF )
        {
           
            // Signature was read, so lets make sure it is a good signature.
            if (signature == EE_CURRENT_SIGNATURE)
            {
                TransmitStringUART1("Signature Matches\r\n");
                // Signature is correct, lets check version
                if ((version=DataEERead(EE_ADDR_VERSION)) !=0xFFFF )
                {
                    // Version was read, so lets make sure it matches the current compiled version
                    if (version == EE_CURRENT_VERSION)
                    {
                        TransmitStringUART1("Version Matches - Reading Complete Config from EEPROM\r\n");
                        // We are good to read the config into the global config variable.
                        ReadConfig(&g_Config);
                        // if Config read correctly, lets increment the boot count and write back the config.
                        g_Config.BootCount++;
                        WriteConfig(&g_Config);
                        // A flag is set saying we have a good config.
                        ConfigGood=1;
                    }
                    else
                    {
                        // If there is a version mismatch, something weird is going on.  If the version is changed in the sourcecode
                        // because new stuff was added to the g_Config structure, the code has to be recompiled and reflashed.  That reflash
                        // reflash will also zero out the simulated EEPROM memory, so this should really never happen.  If it does we will log
                        // this error.  Note the config will still be the default values, and we will initialize the config memory again.
                        TransmitStringUART1("Config Version mismatch\r\n");
                    }
                    
                }
                else
                {
                    // If we get here there was a signature but no version.  That is also an odd failure case, but we will just fall through
                    // with the default values and re-init the config memory.
                    TransmitStringUART1("Version not found in EEPROM\r\n");
                }
            }
            else
            {
                // If the signature is there, but not a match, we will just go with the default config and reset the config memory.
                TransmitStringUART1("Signature mismatch\r\n");
            }
        }
        else
        {
            // If there is no signature at all, this is probably the first time running since a flash, so we will go with the defaults 
            // and reset the config memory.
            TransmitStringUART1("Signature not found in EEPROM\r\n");
        }
        
        // If the init failed, but we made it this far without getting a good config, the EEPROM is probably not initialized.   Lets
        //  write a the new config back.
        if (ConfigGood != 1)
        {
            WriteConfig(&g_Config);
            TransmitStringUART1("No Config found, writing new default config to EEPROM\r\n");
        }
    }
    else
    {
        // If the datainit failed, something is wrong with the EEPROM emulation layer.  In that case we will just stick with the defaults.
        TransmitStringUART1("Data init failed\r\n");
    }
}

/*
 *      FillConfigWithDefault() - Put default values into a st_CAL structure.  This is used as the default config on powerup when no other
 *                                  config has been saved (or in the event of a config corruption)
 *   
 */

bool FillConfigWithDefault(st_CAL* Config)
{
   if (Config != 0)
   {      
       // The default config sends out the ADC data over two messages, with 4 channels per message.  The data is the raw ADC values
       // without any scaling, calibration, offset, or error correction.
        Config->version=0x01;
        Config->CanMessage1_ID=0x600;
        Config->CanMessage1_DATA0=0x01;
        Config->CanMessage1_DATA1=0x02;
        Config->CanMessage1_DATA2=0x03;
        Config->CanMessage1_DATA3=0x04;
        Config->CanMessage1_DATA4=0x05;
        Config->CanMessage1_DATA5=0x06;
        Config->CanMessage1_DATA6=0x07;
        Config->CanMessage2_ID=0x602;
        Config->CanMessage2_DATA0=0x08;
        Config->CanMessage2_DATA1=0x09;
        Config->CanMessage2_DATA2=0x0a;
        Config->CanMessage2_DATA3=0x0b;
        Config->CanMessage2_DATA4=0x0c;
        Config->CanMessage2_DATA5=0x0d;
        Config->CanMessage2_DATA6=0x0e;
        Config->BootCount = 1;
        Config->CanStartup_ID = 0x603;
        Config->CanStartup_SerialNumber = 0x0001;
        return true;
       
   }
   else
   {
       return false;
   }
}

/*
 *      WriteConfig() - Write the st_CAL strucuture to the EEPROM memory. 
 *   
 */
bool WriteConfig(st_CAL *Cal)
{
    int i;
    int8_t* c=(int8_t*)Cal;

    // We want to make sure the st_CAL structure fits in the EEPROM memory. 
    if (sizeof(st_CAL) > (EE_DATASPACESIZE))
    {
        return false;
    }
    else
    {
        // Lets write the cal structure into the EEPROM memory.
        for (i=0; i<sizeof(st_CAL); i++)
        {      
            DataEEWrite(c[i],i+EE_ADDR_DATA);
        }
        DataEEWrite(EE_CURRENT_VERSION,EE_ADDR_VERSION);
        DataEEWrite(sizeof(st_CAL),EE_ADDR_SIZE);
        DataEEWrite(EE_CURRENT_SIGNATURE, EE_ADDR_SIGNATURE);    
        return true;
    }
}

/*
 *      ReadConfig() - Red the st_CAL strucuture from the EEPROM memory. 
 *                      This assumes the EEPROM has already been verified to have the right signature, version, and size.
 *   
 */
bool ReadConfig(st_CAL *Cal)
{
    int i=0;
    char* x = (char*)Cal;
    int size = DataEERead(EE_ADDR_SIZE);
    
    if ( size != 0xFFFF)
    {
        // This means there was a valid EE_ADDR_SIZE value
        if ( size == sizeof(st_CAL) )
        {
            // The structure size is the same as the EEPROM data element size, so read the structure in.
            for (i=0; i<sizeof(st_CAL); i++)
            {   
                x[i] = DataEERead(i+EE_ADDR_DATA);
            }
        }
        else
        {   
            // if the structures are the wrong size, we can't read the config.  Return an error.
            return false;
        }
    }
    
    if ( DataEERead(EE_ADDR_SIZE) == sizeof(st_CAL) )
    {
        for (i=0; i<sizeof(st_CAL); i++)
        {
            x[i] = DataEERead(i+EE_ADDR_DATA);
        
        }
    }   
    return true;
}

