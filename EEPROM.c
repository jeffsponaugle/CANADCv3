#include "EEPROM.h"
#include "uart.h"


extern st_CAL g_Config;
  
void ConfigurationSystemInit()
{
    int ConfigGood=0;
    
    TransmitStringUART1("Config System Init\r\n");
    FillConfigWithDefault(&g_Config);    //  Lets put some default values in the global static config (g_Config)
    if (DataEEInit() == 0)
    {
        TransmitStringUART1("Init successful\r\n");
        // Init was successful.  Let's check the signature and verstion
        unsigned int version, signature;
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
                        
                        if (1==1)
                        {
                            // if Config read correctly, lets increment the boot count and write back the config.
                            g_Config.BootCount++;
                            WriteConfig(&g_Config);
                        }
                        ConfigGood=1;
                    }
                    else
                    {
                        TransmitStringUART1("Config Version mismatch\r\n");
                    }
                    
                }
                else
                {
                    TransmitStringUART1("Version not found in EEPROM\r\n");
                }
            }
            else
            {
                TransmitStringUART1("Signature mismatch\r\n");
            }
        }
        else
        {
            TransmitStringUART1("Signature not found in EEPROM\r\n");
        }
        
        // If the init failed, but we made it this far without getting a good config, the EEPROM is probably not initialized.   Lets
        //  write a the new config back.
        if (ConfigGood != 1)
        {
            WriteConfig(&g_Config);
            TransmitStringUART1("No Config found, writing new config to EEPROM\r\n");
        }
    }
    else
    {
        TransmitStringUART1("Data init failed\r\n");
    }
}

int FillConfigWithDefault(st_CAL* Config)
{
   if (Config != 0)
   {      
        Config->version=1;
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
        return 0;
       
   }
   else
   {
       return 1;
   }
}

void WriteConfig(st_CAL *Cal)
{
    int i;
    int8_t* c;
    c=(int8_t*)Cal;
    for (i=0; i<sizeof(st_CAL); i++)
    {      
        DataEEWrite(c[i],i+EE_ADDR_DATA);
    }
    DataEEWrite(EE_CURRENT_VERSION,EE_ADDR_VERSION);
    DataEEWrite(sizeof(st_CAL),EE_ADDR_SIZE);
    DataEEWrite(EE_CURRENT_SIGNATURE, EE_ADDR_SIGNATURE);
}

void ReadConfig(st_CAL *Cal)
{
    int i=0;
    char* x = (char*)Cal;
    int size = DataEERead(EE_ADDR_SIZE);
    char string[100];
    
    if ( DataEERead(EE_ADDR_SIZE) == sizeof(st_CAL) )
    {
        for (i=0; i<sizeof(st_CAL); i++)
        {
            x[i] = DataEERead(i+EE_ADDR_DATA);
        
        }
    }    
}

