/**
 *
 * @brief communication functions for Usart
 * @file comm_uart.h
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-10
 * @version 1.0.0
 *
 */


#ifndef _COMM_UART_H
#define _COMM_UART_H

/* includes */
#include <sys/time.h>

#include "comm_mcu.h"
#include "common.h"
#include "comm_protocol.h"
//#include "flash.h"


/* define */


/* typedefs */
#pragma pack(1)

typedef struct
{
	U08 *pReceiveBuf;						// DMA 수신 버퍼 포인터
	U08 *bSendBuf;							// 하나의 송신 패킷을 담는 버퍼 포인터
	U08 *pPacket;							// 온전한 패킷을 모아두는 버퍼
	U16 uMaxBufferSize;						// 버퍼의 크기
	U16 uReceivePosition;					// 새로운 데이터가 들어올 위치
	U16 uArrReadIdx;						// 온전한 패킷을 처리할 인덱스
	U16 uArrNextIdx;						// 온전한 패킷을 저장할 인덱스
} _UART_DATA;


typedef struct
{
	U08 bSendIndex;
	U08 bSendEnableCount;
	U32 wSendCmd[MAX_SEND_ARRAY];
	U16 uSendDataLen[MAX_SEND_ARRAY];
	U08 *uSendData[MAX_SEND_ARRAY];
} _UART_SEND;

typedef struct
{
	U32 ulErrTick;
	U32 ulSendTick;
	U32 ulTxCnt;
	U32 ulRxCnt;
	_UART_SEND mUartSend;
} _UART_MANAGE;


////////////////////////////////////////////////////
// Available Baud Rates
#define _UART_BPS_4800		4800
#define _UART_BPS_9600		9600
#define _UART_BPS_14400		14400
#define _UART_BPS_19200		19200
#define _UART_BPS_38400		38400
#define _UART_BPS_56K		57600
#define _UART_BPS_112K		115200
#define _UART_BPS_128K		128000
#define _UART_BPS_230K		230400
#define _UART_BPS_460K		460800
#define _UART_BPS_912K		921600


/* global variables */
extern U08 bRadarStatus;

extern _UART_DATA UartData;

extern _UART_MANAGE stUartMgt;

extern U16 uUSARTBufIdx;
extern U08 bSendUartBuf[COMM_BUFF_MAX];
extern U08 bReceiveBuffer[COMM_BUFF_MAX];
extern U08 arReceivePacket[MAX_RCV_ARRAY][COMM_BUFF_MAX];

extern TOF comm_mcu_run;

/* global functions */
extern TOF Push_COMM_Data(U16 uLength, U32 wCommand, U08 *pBuf);
extern TOF Send_Data(U08 *pBuf, U16 uLen);
extern int SerialGetData(int fd);
extern U16 SerialGetByteData(U08 *pBuf, U16 uLength);
extern void SerialPushData(U08 *pBuf, U16 uLength);

extern void UART_Init(void);

void Uart2_Proc(int fd);


#endif /* _COMM_UART_H */


