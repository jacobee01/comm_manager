/**
 *
 * @brief function for communication Protocol processing
 * @file comm_function.c
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-07-31
 * @version 1.0.0
 *
 */


/* includes */
#include "common.h"
#include "comm_uart.h"
#include "comm_protocol.h"
#include <string.h>


/* define */
#define COMM_SAVE_RX_TIME_(a) (a = GetTimeTick10ms())
#define COMM_COMP_RX_TIME_(a) ((GetTimeTick10ms() < a) ? ((0xFFFFFFFF - a) + GetTimeTick10ms()) : (GetTimeTick10ms() - a))


#define CRC_START_XMODEM	0x0000
#define CRC_POLY_CCITT		0x1021

/* typedefs */


/* global variables */


/* local variables */
static const U08 COMM_HEAD_VALUE[4] = {0x23, 0x01, 0x02, 0x16};
static const U08 COMM_TAIL_VALUE[4] = {0x03, 0x04, 0x17, 0x24};


static U08 bTempBuff[COMM_BUFF_MAX];

COMM_CHANNEL stCommCHList;


static TOF crc_tabccitt_init = false;
static U16 crc_tabccitt[256];

/* imports */


/* function prototypes */
static void init_crcccitt_tab(void)
{
	U16 i;
	U16 j;
	U16 crc;
	U16 c;
		
	for ( i=0; i<256; i++ )
	{
		crc = 0;
		c   = i << 8;
				
		for (j=0; j<8; j++)
		{
			if ( (crc ^ c) & 0x8000 ) crc = ( crc << 1 ) ^ CRC_POLY_CCITT;
			else                      crc =   crc << 1;

			c = c << 1;
		}
		
		crc_tabccitt[i] = crc;
	}
	
	crc_tabccitt_init = true;	
}  /* init_crcccitt_tab */


U16 CommGetChecksum(U08 *pData, U16 uLength)
{
	U16 crc;
	U16 tmp;
	U16 short_c;
	U08 *ptr;
	U16 a;
		
	if ( ! crc_tabccitt_init ) init_crcccitt_tab();
		
	crc = CRC_START_XMODEM;
	ptr = pData;
		
	if ( ptr != NULL )
	{
		for ( a=0 ; a<uLength ; a++ )
		{
			short_c = 0x00ff & (unsigned short) *ptr;
			tmp     = (crc >> 8) ^ short_c;
			crc     = (crc << 8) ^ crc_tabccitt[tmp];
					
			ptr++;
		}
	}
		
	return crc;
}


U16 CommMakeTransferData(U08 *pBuf, U32 uCommand, U16 uDataLength)
{
	COMM_FRAME *pFrame;
	U16 uDataFieldLength;
	U16 uLen;

	pFrame = (COMM_FRAME *)pBuf;
	uDataFieldLength = _MK_ALIGN_2(uDataLength+4);

	memcpy ( (U08*)&pBuf[0], &COMM_HEAD_VALUE[0], 4);
	memcpy ( (U08*)&pBuf[6], &uCommand, 4);
	memcpy ( (U08*)&pBuf[4], &uDataFieldLength, 2);


	memcpy ( &pBuf[6+uDataLength+4+2], &COMM_TAIL_VALUE[0], 4);

	uLen = (U16)((U08*)&pFrame->uChecksum - (U08*)pFrame) + uDataFieldLength + 4;
	*(U16*)((U08*)(&pBuf[10] + uDataLength)) = CommGetChecksum(&pBuf[6], uDataLength+4);
	uLen = sizeof(COMM_FRAME) + uDataLength+4;

#if 0
	pFrame = (COMM_FRAME *)pBuf;
	uDataFieldLength = _MK_ALIGN_2(uDataLength);

	memcpy ( pFrame->wHead, &COMM_HEAD_VALUE[0], 4);
	memcpy ( pFrame->uChecksum, uCommand, 4);
	pFrame->uDataLength = uDataFieldLength + 4;


	memcpy ( (&pFrame->wTail) + uDataFieldLength + sizeof(pFrame->uChecksum ), &COMM_TAIL_VALUE[0], 4);

	uLen = (U16)((U08*)&pFrame->uChecksum - (U08*)pFrame) + uDataFieldLength + 4;
	*(U16*)((U08*)(&pFrame->uChecksum) + uDataFieldLength + 4) = CommGetChecksum(pBuf, uLen);
	uLen = sizeof(COMM_FRAME) + uDataFieldLength;
#endif

	return uLen;
}


U08 CommCheckData(U08 *pBuf)
{
	COMM_FRAME *pFrame;
	U16 uDataFieldLength;
	U16 uReadChecksum;
	U16 uCalcChecksum;
	U16 uLen;

	pFrame = (COMM_FRAME *)pBuf;
	uDataFieldLength = _MK_ALIGN_2(pFrame->uDataLength);

//	printf("CommCheckData datalength : %d\n", pFrame->uDataLength);
//	printf("CommCheckData uDataFieldLength : %d\n", uDataFieldLength);


	uLen = (U16)((U08*)&pFrame->uChecksum - (U08*)pFrame) + uDataFieldLength;
	uCalcChecksum = CommGetChecksum(pBuf+6, uDataFieldLength);
	uReadChecksum = *(U16*)((U08*)(&pFrame->uChecksum) + uDataFieldLength);

	if( uCalcChecksum == uReadChecksum ) return true;
	return false;
}


U16 CommGetDataLength(U08 *pBuf)
{
	COMM_FRAME *pFrame;

	pFrame = (COMM_FRAME *)pBuf;

	return pFrame->uDataLength;
}


void CommInit(void)
{
	memset(&stCommCHList, 0, sizeof(COMM_FRAME));
}


void CommRxRun(int fd)
{
	U16 uTempIdxR;
	U16 uTempIdxW;

	U16 uNeedRxLen;
	int iRxLen;

	COMM_FRAME *pFrame;

	// Read received data count
	iRxLen = SerialGetData(fd);

	if( iRxLen <= 0 )
	{
		// Reset the mode if nothing received for a while
		if( (COMM_COMP_RX_TIME_(stCommCHList.lRxTimer) > COMM_RX_TIME_OUT) && (stCommCHList.uCOMM_RxRunMode != _COMM_RXMGT_NODATA) )
		{
			// 내개 온 Frame이 도중에 끊긴 경우 이에 대한 응답 메시지를 보내야 한다.
			if( stCommCHList.tMyFrame )
			{
				stCommCHList.uCOMM_RxRunMode = _COMM_RXMGT_NODATA;
			}
		}

		return;
	}

	// 저장된 데이터가 처리되지 않았으면 다음에 한다.

	// Read received data.
	memset(&bTempBuff, 0, sizeof(bTempBuff));
	uTempIdxR = 0;
	uTempIdxW = (iRxLen < sizeof(bTempBuff)) ? iRxLen : sizeof(bTempBuff);
	SerialGetByteData(bTempBuff, uTempIdxW);

	COMM_SAVE_RX_TIME_(stCommCHList.lRxTimer);

	while(true)
	{
		// FRAME begin to HEAD_VALUE, seek to start point.
		if( stCommCHList.uCOMM_RxRunMode == _COMM_RXMGT_NODATA )
		{
			// Read 1st Byte, JTP_HEAD_VALUE's first byte
			while( uTempIdxR < uTempIdxW )
			{
				if( memcmp(&bTempBuff[uTempIdxR++], &COMM_HEAD_VALUE[0], 4) == 0 )
				{
					stCommCHList.uCOMM_RxBuffIdx = 0;
					stCommCHList.uCOMM_RxRunMode = _COMM_RXMGT_HEAD;
					pFrame = (COMM_FRAME *)stCommCHList.bFrame;
					memcpy(&pFrame->wHead, &COMM_HEAD_VALUE[0], 4);
					memcpy(&stCommCHList.bRecieveBuffer[stCommCHList.uCOMM_RxBuffIdx], &COMM_HEAD_VALUE[0], 4);
					stCommCHList.tMyFrame == true;
					uTempIdxR += 3;
					stCommCHList.uFrameIdxW = 4;
					stCommCHList.uCOMM_RxBuffIdx = 4;
					break;
				}
			}
		}
		// Read from Frame.uTgtId to Frame.uDataLen.
		else if( stCommCHList.uCOMM_RxRunMode == _COMM_RXMGT_HEAD )
		{
			pFrame = (COMM_FRAME *)stCommCHList.bFrame;
			if( (uTempIdxW - uTempIdxR) > sizeof (pFrame->uDataLength) )
			{
				memcpy(&pFrame->uDataLength, &bTempBuff[uTempIdxR], sizeof (pFrame->uDataLength));
				memcpy(&stCommCHList.bRecieveBuffer[stCommCHList.uCOMM_RxBuffIdx], &bTempBuff[uTempIdxR], sizeof (pFrame->uDataLength));
				uTempIdxR += sizeof (pFrame->uDataLength);
				stCommCHList.uCOMM_RxBuffIdx += sizeof (pFrame->uDataLength);
				stCommCHList.uFrameIdxW += sizeof (pFrame->uDataLength);

				if( pFrame->uDataLength > (COMM_BUFF_MAX - sizeof(COMM_FRAME)) )
				{
					// Don't recieve data anymore.
					stCommCHList.tMyFrame = false;
					stCommCHList.uCOMM_RxRunMode = _COMM_RXMGT_NODATA;
				}
				else
				{
					stCommCHList.tMyFrame = true;
					stCommCHList.uCOMM_RxRunMode = _COMM_RXMGT_DATA;
				}
			}
		}
		// Frame.bBody ~ Frame.wTail
		else if( stCommCHList.uCOMM_RxRunMode == _COMM_RXMGT_DATA )
		{
			pFrame = (COMM_FRAME *)stCommCHList.bFrame;

			// Read data
			uNeedRxLen = sizeof(COMM_FRAME) + _MK_ALIGN_2(pFrame->uDataLength) - stCommCHList.uFrameIdxW;
			if( uNeedRxLen > 0 )
			{
				if( uNeedRxLen > (uTempIdxW - uTempIdxR) )
				{
					uNeedRxLen = uTempIdxW - uTempIdxR;
				}

				// Read data
				if( stCommCHList.tMyFrame == true )
				{
					memcpy(&stCommCHList.bRecieveBuffer[stCommCHList.uCOMM_RxBuffIdx], &bTempBuff[uTempIdxR], uNeedRxLen);
					stCommCHList.uCOMM_RxBuffIdx += uNeedRxLen;
				}

				stCommCHList.uFrameIdxW += uNeedRxLen;
				uTempIdxR += uNeedRxLen;
			}

			// Done.
			if( stCommCHList.uFrameIdxW >= ((sizeof(COMM_FRAME) + pFrame->uDataLength)) )
			{
				stCommCHList.uCOMM_RxRunMode = _COMM_RXMGT_END;
				continue;
			}
		}
		else if( stCommCHList.uCOMM_RxRunMode == _COMM_RXMGT_END )
		{
			// See if JTP protocol is right or not.
			if( stCommCHList.tMyFrame == true )
			{
				
				if( CommCheckData(&stCommCHList.bRecieveBuffer[0]) )
				{
					SerialPushData(&stCommCHList.bRecieveBuffer[0], stCommCHList.uCOMM_RxBuffIdx);
					stCommCHList.tMyFrame = false;
					stCommCHList.uCOMM_RxRunMode = _COMM_RXMGT_NODATA;
//					break;
				}
			}

			stCommCHList.tMyFrame = false;
			stCommCHList.uCOMM_RxRunMode = _COMM_RXMGT_NODATA;
		}

		if( uTempIdxR < uTempIdxW ) continue;

		// See if there is more data in DMA buffer.
		iRxLen = SerialGetData(fd);
		if( iRxLen <= 0 ) break;

		uTempIdxR = 0;
		uTempIdxW = (iRxLen < sizeof(bTempBuff)) ? iRxLen: sizeof(bTempBuff);
		SerialGetByteData(bTempBuff, uTempIdxW);
		COMM_SAVE_RX_TIME_(stCommCHList.lRxTimer);
	}
}

