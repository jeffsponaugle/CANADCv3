/* 
 * File:   uart.h
 * Author: jeff.sponaugle
 *
 * Created on September 14, 2016, 9:32 PM
 */

#ifndef UART_H
#define	UART_H

#ifdef	__cplusplus
extern "C" {
#endif

// relies of FP (FCy) being defined
#define BAUDRATE 115200
#define BRGVAL ((FP/BAUDRATE)/4 )-1

void SetupUART1();
void TransmitUART1(char t);
void TransmitIntUART1(int x);
void TransmitStringUART1(char* string);



#ifdef	__cplusplus
}
#endif

#endif	/* UART_H */

