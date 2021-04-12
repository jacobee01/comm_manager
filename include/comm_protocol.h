/**
 *
 * @brief Communication Protocol
 * @file comm_protocol.h
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-10
 * @version 1.0.0
 *
 */

#ifndef _COMM_PROTOCOL_H
#define _COMM_PROTOCOL_H

/* includes */
#include <sys/time.h>
#include "common.h"

/* define */
//protocol
#define COMM_ID_MC ( ('M'<<0) | ('C'<<8) )			// MCU
#define COMM_ID_SO ( ('S'<<0) | ('O'<<8) )			// SOC

#define COMM_CMD_NO_CHKSUM			0x8000			// if (uCommand & COMM_CMD_NO_CHKSUM) && (uDataLength != 0) -> uCheckSum = uDataLength

#define COMM_BUFF_MAX				(4096+16+32)	// 2KBytes + sizeof(COMM_FRAME)(16Byte) + reserved(32Byte)

#define MAX_RCV_ARRAY				8
#define MAX_SEND_ARRAY				3

////////////////////// Response List //////////////////////////////////////////////
#define COMM_RESP_OK				0x0000	// No Error (All OK)						: ��� ���������� ó����
#define COMM_RESP_TRANS_TIMEOUT		0xFFFF	// Transmit Error - Time out				: ������ ������ 1�� ���� ������ ������ ������
#define COMM_RESP_TRANS_TOO_BIG		0xFFFE	// Transmit Error - Too big size data		: ���۵Ǵ� �������� ũ�Ⱑ �ʹ� Ŀ ó���� �� ����
#define COMM_RESP_TRANS_CHKSUM		0xFFFD	// Transmit Error - Invalid CheckSum		: üũ�� ����
#define COMM_RESP_TRANS_MSGEND		0xFFFC	// Transmit Error - Invalid Tail			: �޽��� ���ᰪ�� ���� ����
#define COMM_RESP_TRANS_ERR			0xFFF0	// Transmit Error - Others					: ��Ÿ ���� ����(0xFFF0~0xFFFB)
#define COMM_RESP_CMD_RSV			0xFFEF	// Command Error - Reserved Command			: ���� �������� ���� ��ɾ���
#define COMM_RESP_CMD_BAD			0xFFEE	// Command Error - Unknown Command			: �߸��� ��ɾ���
#define COMM_RESP_CMD_ERR			0xFFE0	// Command Error - Others					: ��Ÿ ��ɾ� ����(0xFFE0~0xFFED)
#define COMM_RESP_BAD_PARA			0xFFD1	// Parameter Error - Invalid Parameter		: ��ɾ ���� �ĸ����� ����
#define COMM_RESP_BAD_DATALEN		0xFFD2	// Parameter Error - Invalid Data Length	: ��ɾ ���� ������ ũ�� ����
#define COMM_RESP_BAD_DATA			0xFFD4	// Parameter Error - Invalid Data			: �߸��� ������
#define COMM_RESP_ERROR				0xFFD0	// Run Time Error(0xFFD0~0xFFFF)


/////// Define Time Out
#define COMM_RX_TIME_OUT			1000	// milisec. Invalid Time Interval for Frame data


/* typedefs */
#define _MK_ALIGN_2(lData)			(((U32)(lData) + 1) & 0xFFFFFFFE)
#define _MK_ALIGN_4(lData)			(((U32)(lData) + 3) & 0xFFFFFFFC)
#define SWAP_WORD(a)				((a << 8) & 0xFF00) | ((a >> 8) & 0x00FF)

#pragma pack(1)

// Communication Data Frame Structure
typedef struct
{
	U32 wHead;
	U16 uDataLength;
	U16 uChecksum;
	U32 wTail;
} COMM_FRAME;


// Communication RX information list buffer
typedef struct
{
	TOF tMyFrame;
	U08 bFrame[sizeof(COMM_FRAME)];
	U16 uFrameIdxW;
	U08 bRecieveBuffer[COMM_BUFF_MAX];
	U16 uCOMM_RxBuffIdx;
	U16 uCOMM_RxRunMode;
	U32 lRxTimer;
} COMM_CHANNEL;


// For check message RX state
enum COMM_RXMGT_MODE_tag
{
	_COMM_RXMGT_NODATA = 0,		// empty RX buffer
	_COMM_RXMGT_HEAD,			// Read Frame.wHead ~ Frame.uDataLength;
	_COMM_RXMGT_DATA,			// Read RX data
	_COMM_RXMGT_END,			// RX end
	_COMM_RXMGT_TIME_OUT,		// Start RX, but not end until timeout
	_COMM_RXMGT_TOO_BIG,		// RX data is too big size
	_COMM_RXMGT_SKIP			// RX process skip
};


/* global variables */


/* global functions */
U08 CommCheckAcceptId(U16 a_uRxTargetId);
U16 CommGetChecksum(U08 *pData, U16 uLength);
U16 CommMakeTransferData(U08 *pBuf, U32 uCommand, U16 uDataLength);
U08 CommCheckData(U08 *pBuf);
U16 CommGetDataLength(U08 *pBuf);
void CommInit(void);

U16 CommProtocol(U08 *pucRecieveBuf, U08 *pucSendBuf, U16 *uSendLength);

void CommRxRun(int fd);

// For command processing, �� ä�κ��� ���� �ʿ� 
void CommPC_CommandParser(U08 *pucRecieveBuf, U08 *pucSendBuf, U16 *uSendLength);

#endif /* _COMM_PROTOCOL_H */


