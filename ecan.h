/* 
 * File:   ecan.h
 * Author: jeff.sponaugle
 *
 * Created on September 14, 2016, 9:36 PM
 */

#ifndef ECAN_H
#define	ECAN_H

#ifdef	__cplusplus
extern "C" {
#endif

#define  ECAN1_MSG_BUF_LENGTH 	8
// ECAN1MSGBUF is a collection of ECAN1_MSG_BUG_LENGTH message buffers, each 8 words in size.)
// 8 words corresponds to word 0,1,2 being setup and SID, and word 3,4,5,6 being the packet data. Word 7 is unused.   
typedef uint16_t ECAN1MSGBUF [ECAN1_MSG_BUF_LENGTH][8];
extern ECAN1MSGBUF  ecan1msgBuf __attribute__((space(dma)));


void BuildCANPackets();
void TransmitCANPackets();
void ConfigureECAN1();
bool TransmitECANFrame(unsigned int (*packet)[]);
void TransmitECANStartupFrame();

#ifdef	__cplusplus
}
#endif

#endif	/* ECAN_H */

