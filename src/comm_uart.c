/**
 *
 * @brief communication functions for Usart
 * @file comm_uart.c
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-10
 * @version 1.0.0
 *
 */


/* includes */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>

#include "common.h"
#include "comm_uart.h"
#include "comm_protocol.h"
#include "comm_protocol_command.h"

#include "radar_control.h"

#include "socket_diagnosis.h"
#include "socket_main.h"

#include "radar_control.h"

/* define */


/* typedefs */


/* global variables */
_UART_MANAGE stUartMgt;

static _UART_DATA UartData;

U16 uUSARTBufIdx;
U08 bSendUartBuf[COMM_BUFF_MAX];
U08 bReceiveBuffer[COMM_BUFF_MAX];
U08 arReceivePacket[MAX_RCV_ARRAY][COMM_BUFF_MAX];

U08 arbSendMainBuf[COMM_BUFF_MAX];
U08 truck_bed_config[32];


/* local variables */
static U08 szMsg[128];
static U32 wData;
static U16 uData;


/* imports */


/* function prototypes */
TOF Push_COMM_Data(U16 uLength, U32 wCommand, U08 *pBuf)
{
	TOF bRet = true;

	if ( comm_mcu_run == FALSE )
	{
		return;
	}

	if ( stUartMgt.mUartSend.bSendEnableCount < MAX_SEND_ARRAY )
	{
		stUartMgt.mUartSend.uSendDataLen[stUartMgt.mUartSend.bSendIndex] = uLength;
		stUartMgt.mUartSend.wSendCmd[stUartMgt.mUartSend.bSendIndex] = wCommand;
		stUartMgt.mUartSend.uSendData[stUartMgt.mUartSend.bSendIndex] = pBuf;

		stUartMgt.mUartSend.bSendEnableCount++;
	}
	else
	{
		bRet = false;
	}

	return bRet;
}


TOF Send_COMM_Data(int fd)
{
	TOF bRet = true;
	U08 *pData;
	U16 uLen;
	int i;

	if ( stUartMgt.mUartSend.bSendEnableCount > 0 )
	{
		if ( stUartMgt.mUartSend.uSendDataLen[stUartMgt.mUartSend.bSendIndex] != 0 )
		{
			memcpy(UartData.bSendBuf + 10, stUartMgt.mUartSend.uSendData[stUartMgt.mUartSend.bSendIndex], stUartMgt.mUartSend.uSendDataLen[stUartMgt.mUartSend.bSendIndex]);
			uLen = CommMakeTransferData(UartData.bSendBuf, stUartMgt.mUartSend.wSendCmd[stUartMgt.mUartSend.bSendIndex], stUartMgt.mUartSend.uSendDataLen[stUartMgt.mUartSend.bSendIndex]);
			bRet = write( fd, UartData.bSendBuf, uLen);
			//for ( i = 0 ; i < uLen ; i++ )
			//{
			//	if ( i%10 == 0 ) printf("\n");
			//	printf("0x%02X ", UartData.bSendBuf[i]);
			//}
			//printf("\n");
			
		}
		else
		{
			uLen = CommMakeTransferData(UartData.bSendBuf, stUartMgt.mUartSend.wSendCmd[stUartMgt.mUartSend.bSendIndex], 0);
			bRet = write( fd, UartData.bSendBuf, uLen);
		}

		if ( bRet == true )
		{
			stUartMgt.mUartSend.bSendEnableCount--;
			if ( stUartMgt.mUartSend.bSendIndex == (MAX_SEND_ARRAY - 1) )
			{
				stUartMgt.mUartSend.bSendIndex = 0;
			}
			else
			{
				stUartMgt.mUartSend.bSendIndex++;
			}
		}
		else
		{
			bRet = false;
		}
	}
	else
	{
		bRet = false;
	}

	return bRet;
}


int SerialGetData(int fd)
{
    int iCount;

	iCount = read(fd, UartData.pReceiveBuf, COMM_BUFF_MAX);

    return iCount;
}


U16 SerialGetByteData(U08 *pBuf, U16 uLength)
{
	U16 uReadCnt, i = 0;

	uReadCnt = 0;
	while(1)
	{
		if( uReadCnt >= uLength ) break;

		pBuf[uReadCnt++] = UartData.pReceiveBuf[i++];
	}

	return uReadCnt;
}

void SerialPushData(U08 *pBuf, U16 uLength)
{
    U08 *pData;

	pData = UartData.pPacket + (UartData.uArrNextIdx * UartData.uMaxBufferSize);
	UartData.uArrNextIdx = (UartData.uArrNextIdx + 1) % MAX_RCV_ARRAY;

	memset(pData, 0, UartData.uMaxBufferSize);
	memcpy(pData, pBuf, uLength);
}


// PC
void Radar_Command_Processing(U08 *pucRecievBuf, U08 *pucSendBuf, U16 *uSendLength)
{
	COMM_FRAME *pFrame;
	U16 uDataFieldLength;
	U32 wCommand, wTemp;
	static U32 wPrevMMICStatus = MMIC_STATUS_OK;
	U16 uTail;
	char logdata[MAX_BUF];

	int i;


	pFrame = (COMM_FRAME *)pucRecievBuf;
	uDataFieldLength = _MK_ALIGN_2(pFrame->uDataLength);
//	wCommand = (U32)pucRecievBuf[9]<<24 + (U32)pucRecievBuf[8]<<16 + (U32)pucRecievBuf[7]<<8 + (U32)pucRecievBuf[6];
	memcpy (&wCommand, &pucRecievBuf[6], 4);
	uTail = *(U16*)((U08*)(&pFrame->wTail) + uDataFieldLength);

	//printf("wCommand : 0x%08X\n", wCommand);   
	//		for ( i = 0 ; i < (uDataFieldLength+12) ; i++ )
	//		{
	//			if ( i%10 == 0 ) printf("\n");
	//			printf("0x%02X ", pucRecievBuf[i]);
	//		}
	//		printf("\n");

	sprintf(logdata, "[Radar][Receive] ");

	switch ( wCommand )
	{
	case INFO_RES :					// 0x21		// RESPONSE
		sprintf(logdata, "%sINFO_RES (Length = %d) ", logdata, uDataFieldLength);
		//printf("%s", logdata);

		if ( (bUpdateState == UPDATE_READY2) )
		{
			sprintf(logdata, "%sOTA_READY\n", logdata);
			PrintLog(logdata);
			memset( szMsg, 0, sizeof(szMsg) );
			szMsg[0] = (U08)(wUpdateTotalFileSize);
			szMsg[1] = (U08)(wUpdateTotalFileSize>>8);
			szMsg[2] = (U08)(wUpdateTotalFileSize>>16);
			szMsg[3] = (U08)(wUpdateTotalFileSize>>24);
			sleep(1);

			sprintf(logdata, "[Radar][Send] R_MCU_OTA_START\n");
			PrintLog(logdata);
			Push_COMM_Data( 28, R_MCU_OTA_START, &szMsg[0]);
			bUpdateResponse = UPDATE_RESPONSE_OK;
		}
		else if ( uDataFieldLength == (sizeof(stRadarConfig)+ 4) )
		{
			memcpy(&stRadarConfig, &pucRecievBuf[10], sizeof(stRadarConfig) );
			bRadarStatus = RADAR_STATUS_NORMAL;
			tRadatStatusSend = false;
			stMainLEDConfig.bLEDEvent = stRadarConfig.stLedManual.bRun;
			stMainLEDConfig.bBlinkPattern = stRadarConfig.stLedManual.bDimmerEnable + 1;
			stMainLEDConfig.uDutyCycle = stRadarConfig.stLedManual.uOnTime;
			stMainLEDConfig.bRed = stRadarConfig.stLedManual.bRed;
			stMainLEDConfig.bGreen = stRadarConfig.stLedManual.bGreen;
			stMainLEDConfig.bBlue =stRadarConfig.stLedManual.bBlue;

			if ( stRadarConfig.wR_MCUStatus == R_MCU_STATUS_BOOTLOADER)
			{
				sprintf(logdata, "%s: R_MCU Bootloader Mode\n", logdata);
				PrintLog(logdata);
				bRadarStatus = RADAR_R_MCU_BOOTLOADER;

				sprintf(logdata, "[Main][Send] Radar RunMode Event : 0x01\n");
				PrintLog(logdata);
				send_put_main( _CMD_MAIN_RADAR_RUNMODE, 1, RADAR_RUNMODE_R_MCU_BOOTLOADER, 0);
			}
			else if ( stRadarConfig.wMMICStatus == MMIC_STATUS_BOOT_ERROR )
			{
				sprintf(logdata, "%s: MMIC Boot Error\n", logdata);
				PrintLog(logdata);

				sprintf(logdata, "[Main][Send] Radar RunMode Event : 0x02\n");
				PrintLog(logdata);
				send_put_main( _CMD_MAIN_RADAR_RUNMODE, 1, RADAR_RUNMODE_MMIC_ERROR, 0);
			}
			else
			{
				sprintf(logdata, "%s: Radar Normal Boot\n", logdata);
				PrintLog(logdata);

				sprintf(logdata, "[Main][Send] Radar RunMode Event : 0x00\n");
				PrintLog(logdata);
				send_put_main( _CMD_MAIN_RADAR_RUNMODE, 1, RADAR_RUNMODE_NORMAL, 0);
			}
		}
		else
		{
			bRadarStatus = RADAR_CONFIG_FAIL;
		}
		
		break;

	case CONFIG_RES :				// 0x31		// RESPONSE
		sprintf(logdata, "%sCONFIG_RES (Length = %d)\n", logdata, uDataFieldLength);
		PrintLog(logdata);

		if ( uDataFieldLength == (sizeof(stRadarConfig)+ 4) )
		{
			memcpy(&stRadarConfig, &pucRecievBuf[10], sizeof(stRadarConfig) );
			stMainLEDConfig.bLEDEvent = stRadarConfig.stLedManual.bRun;
			stMainLEDConfig.bBlinkPattern = stRadarConfig.stLedManual.bDimmerEnable + 1;
			stMainLEDConfig.uDutyCycle = stRadarConfig.stLedManual.uOnTime;
			stMainLEDConfig.bRed = stRadarConfig.stLedManual.bRed;
			stMainLEDConfig.bGreen = stRadarConfig.stLedManual.bGreen;
			stMainLEDConfig.bBlue =stRadarConfig.stLedManual.bBlue;

			if ( (bRadarStatus != RADAR_STATUS_R_MCU_UPDATE) && (bRadarStatus != RADAR_STATUS_R_MCU_UPDATE) )
			{
				bRadarStatus = RADAR_STATUS_NORMAL;
			}
			else if ( (bRadarStatus == RADAR_STATUS_R_MCU_UPDATE) && (bUpdateState == UPDATE_START))
			{
				bUpdateResponse = UPDATE_RESPONSE_OK;
			}
			bRadarStatus = RADAR_STATUS_NORMAL;
		}
		else
		{
			bRadarStatus = RADAR_CONFIG_FAIL;
		}
		break;

	case STATUS_RES :				// 0x41		// RESPONSE
		wRadarStatusTimer = GetTimeTick10ms();
		tRadatStatusSend = false;
		memcpy (&wTemp, &pucRecievBuf[10], 4);
		stRadarConfig.wR_MCUStatus = wTemp;
		memcpy (&wTemp, &pucRecievBuf[14], 4);
		stRadarConfig.wMMICStatus = wTemp;
		sprintf(logdata, "%sSTATUS_RES (Length = %d) : R_MCU Staus 0x%08X, MMIC Staus 0x%08X\n", logdata, uDataFieldLength, stRadarConfig.wR_MCUStatus, stRadarConfig.wMMICStatus);
		PrintLog(logdata);

		if ( wPrevMMICStatus != stRadarConfig.wMMICStatus )
		{
			if ( stRadarConfig.wMMICStatus == MMIC_STATUS_BOOT_ERROR )
			{
				sprintf(logdata, "[Main][Send] Radar RunMode Event : 0x02\n");
				PrintLog(logdata);
				send_put_main( _CMD_MAIN_RADAR_RUNMODE, 1, RADAR_RUNMODE_MMIC_ERROR, 0);
			}
			else
			{
				sprintf(logdata, "[Main][Send] Radar RunMode Event : 0x00\n");
				PrintLog(logdata);
				send_put_main( _CMD_MAIN_RADAR_RUNMODE, 1, RADAR_RUNMODE_NORMAL, 0);
			}
			wPrevMMICStatus = stRadarConfig.wMMICStatus;
		}

		bRadarStatus = RADAR_STATUS_NORMAL;
		
		break;

	case TDA_FRAMEBUFFER :			// 0x51		// PUSH
		memcpy(	&arbSendMainBuf[0], &pucRecievBuf[10], uDataFieldLength-4);
		sprintf(logdata, "%sTDA_FRAMEBUFFER (Length = %d) : %s\n", logdata, uDataFieldLength, arbSendMainBuf);
		PrintLog(logdata);

		sprintf(logdata, "[Main][Send] Radar Data Buffer : %s\n", arbSendMainBuf);
		PrintLog(logdata);
		send_put_main( _CMD_MAIN_RADAR_SEND_BUFFER, (uDataFieldLength-4), &arbSendMainBuf[0], 0);
		wRadarTDATimer = 0;
		bRadarStatus = RADAR_STATUS_NORMAL;
		break;
	case SEND_INTRUSION :				// 0x00000053		// PUSH
		sprintf(logdata, "%sSEND_INTRUSION (Length = %d)\n", logdata, uDataFieldLength);
		PrintLog(logdata);

		sprintf(logdata, "[Main][Send] Radar Threat Event : 0x01\n");
		PrintLog(logdata);
		send_put_main( _CMD_MAIN_RADAR_THREAT, 1, RADAR_THREAT_INTRUSION, 0);
		break;

	case SEND_LOITER :					// 0x00000055		// PUSH
		sprintf(logdata, "%sSEND_LOITER (Length = %d)\n", logdata, uDataFieldLength);
		PrintLog(logdata);

		sprintf(logdata, "[Main][Send] Radar Threat Event : 0x02\n");
		PrintLog(logdata);
		send_put_main( _CMD_MAIN_RADAR_THREAT, 1, RADAR_THREAT_LOITER, 0);
		break;

	case MMIC_OTA_START_RES :		// 0x61		// RESPONSE
		sprintf(logdata, "%sMMIC_OTA_START_RES (Length = %d) : 0x%02X\n", logdata, uDataFieldLength, pucRecievBuf[22]);
		PrintLog(logdata);
		if ( pucRecievBuf[22] == 0x79 )
		{
			sprintf(logdata, "[Main][Send] R_MCU or Radar update ready event : 0x00 (MMIC ready)\n");
			PrintLog(logdata);
			send_put_main( _CMD_MAIN_OTA_READY_RES, 1, 0, 0);
			bUpdateResponse = UPDATE_RESPONSE_OK;
		}
		else if ( pucRecievBuf[22] == 0x1F )
		{
			sprintf(logdata, "[Main][Send] R_MCU or Radar update ready event : 0x01 (MMIC not ready)\n");
			PrintLog(logdata);
			send_put_main( _CMD_MAIN_OTA_READY_RES, 1, 1, 0);
			bRadarStatus = RADAR_OTA_MMIC_FAIL;
		}
		else
		{
		}
		
		break;

	case MMIC_OTA_SEND_RES :		// 0x63		// RESPONSE
		sprintf(logdata, "%sMMIC_OTA_SEND_RES (Length = %d) : 0x%02X\n", logdata, uDataFieldLength, pucRecievBuf[22]);
		PrintLog(logdata);
		if ( pucRecievBuf[22] == 0x79 )
		{
			wUpdatePresentPartNumber++;
			bUpdateResponse = UPDATE_RESPONSE_OK;
			wRadarResponseTimer = GetTimeTick10ms();
		}
		else if ( pucRecievBuf[22] == 0x1F )
		{
			memset ( &szMsg, 0, sizeof(szMsg) );
			szMsg[0] = (U08)(wUpdateTotalFileSize);
			szMsg[1] = (U08)(wUpdateTotalFileSize>>8);
			szMsg[2] = (U08)(wUpdateTotalFileSize>>16);
			szMsg[3] = (U08)(wUpdateTotalFileSize>>24);

			PrintLog("[Radar][Send] MMIC_OTA_START\n");
			Push_COMM_Data(20, MMIC_OTA_START, &szMsg[0]);

			bUpdateState = UPDATE_SEND;
			bUpdateResponse = UPDATE_RESPONSE_OK;
		}
		else
		{
			bRadarStatus = RADAR_OTA_MMIC_FAIL;

			sprintf(logdata, "[Diagnosis][Send] _CMD_COMM_OTA_FAIL : 0x00 (MMIC success)\n");
			PrintLog(logdata);
			send_put_diagnosis( _CMD_COMM_OTA_FAIL, 1, 1, 0);
		}
		
		break;

	case MMIC_OTA_COMPLETE_RES :	// 0x65		// PUSH
		sprintf(logdata, "%sMMIC_OTA_COMPLETE_RES (Length = %d)\n", logdata, uDataFieldLength);
		PrintLog(logdata);

		sprintf(logdata, "[Main][Send] R_MCU or Radar update result event : 0x00 (MMIC success)\n");
		PrintLog(logdata);
		send_put_main( _CMD_MAIN_OTA_UPDATE_RES, 1, 0, 0);
		bRadarStatus = RADAR_STATUS_NORMAL;
		bUpdateDevice = DEVICE_NONE;
		wRadarResponseTimer = GetTimeTick10ms();
		wRadarStatusTimer = 0;
		break;

	case R_MCU_OTA_BOOTLOADER_RES :	// 0x79		// RESPONSE
		sprintf(logdata, "%sR_MCU_OTA_BOOTLOADER_RES (Length = %d) : 0x%02X\n", logdata, uDataFieldLength, pucRecievBuf[22]);
		PrintLog(logdata);
		if ( pucRecievBuf[22] == 0x79 )
		{
			if ( bUpdateState == UPDATE_READY )
			{
				bUpdateState = UPDATE_READY2;
				bUpdateResponse = UPDATE_RESPONSE_OK;
				wRadarResponseTimer = GetTimeTick10ms();
			}
			else if ( bUpdateState == UPDATE_START )
			{
				memset ( &szMsg, 0, sizeof(szMsg) );
				szMsg[0] = (U08)((U32)wUpdateTotalFileSize);
				szMsg[1] = (U08)((U32)wUpdateTotalFileSize>>8);
				szMsg[2] = (U08)((U32)wUpdateTotalFileSize>>16);
				szMsg[3] = (U08)((U32)wUpdateTotalFileSize>>24);

				PrintLog("[Radar][Send] R_MCU_OTA_START\n");
				Push_COMM_Data(20, R_MCU_OTA_START, &szMsg[0]);
				bUpdateResponse = UPDATE_RESPONSE_WAIT;
			}
			else
			{
				;
			}
			
		}
		else if ( pucRecievBuf[22] == 0x1F )
		{
			memset ( &szMsg, 0, sizeof(szMsg) );

			PrintLog("[Radar][Send] R_MCU_OTA_BOOTLOADER\n");
			Push_COMM_Data( 28, R_MCU_OTA_BOOTLOADER, &szMsg[0]);
		}
		else
		{
			;
		}
		
		break;

	case R_MCU_OTA_START_RES :		// 0x71		// RESPONSE
		sprintf(logdata, "%sR_MCU_OTA_START_RES (Length = %d) : 0x%02X\n", logdata, uDataFieldLength, pucRecievBuf[22]);
		PrintLog(logdata);
		if ( pucRecievBuf[22] == 0x79 )
		{
			bUpdateResponse = UPDATE_RESPONSE_OK;
			bUpdateState = UPDATE_START;

			sprintf(logdata, "[Main][Send] R_MCU or Radar update ready event : 0x00 (R_MCU ready)\n");
			PrintLog(logdata);
			send_put_main( _CMD_MAIN_OTA_READY_RES, 1, 0, 0);
			wRadarResponseTimer = GetTimeTick10ms();
		}
		else if ( pucRecievBuf[22] == 0x1F )
		{
			bRadarStatus = RADAR_OTA_R_MCU_FAIL;

			sprintf(logdata, "[Main][Send] R_MCU or Radar update ready event : 0x01 (R_MCU not ready)\n");
			PrintLog(logdata);
			send_put_main( _CMD_MAIN_OTA_READY_RES, 1, 1, 0);
		}
		else
		{
		}
		
		break;

	case R_MCU_OTA_SEND_RES :		// 0x73		// RESPONSE
		sprintf(logdata, "%sR_MCU_OTA_SEND_RES (Length = %d) : 0x%02X\n", logdata, uDataFieldLength, pucRecievBuf[22]);
		PrintLog(logdata);
		if ( pucRecievBuf[22] == 0x79 )
		{
			wUpdatePresentPartNumber++;
			bUpdateResponse = UPDATE_RESPONSE_OK;
			wRadarResponseTimer = GetTimeTick10ms();
		}
		else if ( pucRecievBuf[22] == 0x1F )
		{
			bUpdateResponse = UPDATE_RESPONSE_FAIL;
		}
		else
		{
			bRadarStatus = RADAR_OTA_MMIC_FAIL;
			send_put_diagnosis( _CMD_COMM_OTA_FAIL, 1, 1, 0);
		}
		break;

	case R_MCU_OTA_COMPLETE_RES :	// 0x75		// PUSH
		sprintf(logdata, "%sR_MCU_OTA_COMPLETE_RES (Length = %d)\n", logdata, uDataFieldLength);
		PrintLog(logdata);

		sprintf(logdata, "[Main][Send] R_MCU or Radar update result event : 0x00 (R_MCU success)\n");
		PrintLog(logdata);
		send_put_main( _CMD_MAIN_OTA_UPDATE_RES, 1, 0, 0);
		bRadarStatus = RADAR_STATUS_NORMAL;
		bUpdateDevice = DEVICE_NONE;
		wRadarResponseTimer = GetTimeTick10ms();
		wRadarStatusTimer = 0;
		break;

	case MMIC_CLI_RECV_RES :		// 0x03		// RESPONSE
		memset ( &arbSendMainBuf, 0, sizeof(arbSendMainBuf) );
		memcpy(	&arbSendMainBuf[0], &pucRecievBuf[10], uDataFieldLength-4);
		sprintf(logdata, "%sMMIC_CLI_RECV_RES (Length = %d) : %s\n", logdata, uDataFieldLength, arbSendMainBuf);
		PrintLog(logdata);

		sprintf(logdata, "[Main][Send] Radar CLI Message Event : %s\n", arbSendMainBuf);
		PrintLog(logdata);
		send_put_main( _CMD_MAIN_MMIC_CLI, (uDataFieldLength-4), &arbSendMainBuf[0], 0);
		break;

	case SET_TRUCK_BED_CONFIG_RES :		// 0x33 	// RESPONSE

		memcpy(&truck_bed_config, &pucRecievBuf[10], 12 );
		stTruckBedConfig.xRadiusPositive = (U32)truck_bed_config[3] | (U32)truck_bed_config[2]<<8 | (U32)truck_bed_config[1]<<16 | (U32)truck_bed_config[0]<<24;
		stTruckBedConfig.xRadiusNegative = (U32)truck_bed_config[7] | (U32)truck_bed_config[6]<<8 | (U32)truck_bed_config[5]<<16 | (U32)truck_bed_config[4]<<24;
		stTruckBedConfig.yDiamter = (U32)truck_bed_config[11] | (U32)truck_bed_config[10]<<8 | (U32)truck_bed_config[9]<<16 | (U32)truck_bed_config[8]<<24;

		sprintf(logdata, "[Main][Send] Truck bed Response : %x %x %x \n", stTruckBedConfig.xRadiusPositive, stTruckBedConfig.xRadiusNegative, stTruckBedConfig.yDiamter );
		PrintLog(logdata);
		break;


	default:
		break;
	}
}


void Uart2_Proc(int fd)
{
	U08 *pData;
	U16 uLen;
	U32 wDiff;
	static U32 wTimerTest = 0;

	// Check to received data from display unit.
	CommRxRun(fd);

	if( UartData.uArrReadIdx != UartData.uArrNextIdx )
	{
		pData = UartData.pPacket + (UartData.uArrReadIdx * UartData.uMaxBufferSize);
		Radar_Command_Processing(pData, UartData.bSendBuf, &uLen);
		stUartMgt.ulRxCnt++;

		// Increase received index.
		UartData.uArrReadIdx = (UartData.uArrReadIdx + 1) % MAX_RCV_ARRAY;

		// Clear
		stUartMgt.ulErrTick = GetTimeTick10ms();
		return;
	}
	else
	{
		wDiff = GetTimeTick10ms();
		if( wDiff < stUartMgt.ulErrTick )
			 wDiff = (0xFFFFFFFF - stUartMgt.ulErrTick) + wDiff;
		else wDiff = wDiff - stUartMgt.ulErrTick;
	}

	wDiff = GetTimeTick10ms();
	if( wDiff < stUartMgt.ulSendTick )
		wDiff = (0xFFFFFFFF - stUartMgt.ulSendTick) + wDiff;
	else wDiff = wDiff - stUartMgt.ulSendTick;
	if( wDiff > 500 )
	{
	}

	if( stUartMgt.mUartSend.bSendEnableCount > 0 )
	{
		Send_COMM_Data(fd);
	}
}


void UART_Init(void)
{
	uUSARTBufIdx = 0;
	memset(bReceiveBuffer, 0, COMM_BUFF_MAX);

	memset(&UartData, 0, sizeof(_UART_DATA));
	memset(&stUartMgt, 0, sizeof(_UART_MANAGE));

	UartData.pReceiveBuf = bReceiveBuffer;
	UartData.bSendBuf = bSendUartBuf;
	UartData.pPacket = *arReceivePacket;
	UartData.uMaxBufferSize = COMM_BUFF_MAX;
	UartData.uReceivePosition = COMM_BUFF_MAX - 1;
}




