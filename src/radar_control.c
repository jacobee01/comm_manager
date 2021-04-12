/**
 *
 * @brief radar control functions
 * @file radar_control.c
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-14
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
#include <string.h>
#include <errno.h>

#include "radar_control.h"
#include "socket_diagnosis.h"
#include "socket_main.h"
#include "comm_uart.h"
#include "comm_protocol.h"
#include "comm_protocol_command.h"

/* define */


/* typedefs */


/* global variables */
U32 wRadarResponseTimer = 0;
U32 wRadarTDATimer = 0;
U32 wRadarStatusTimer = 0;
TOF tRadatStatusSend = false;
U08 bRadarStatus = RADAR_STATUS_NONE;
U08 bRadarRequest = RADAR_STATUS_NONE;
U08 bRadarResponse = RADAR_STATUS_NONE;

RADAR_CONFIG stRadarConfig;
MAIN_LED_CONFIG stMainLEDConfig;


U08 bUpdateDevice;
U08 bUpdateState;
U08 bUpdateResponse;
U32 wUpdateTotalFileSize;
U08 bUpdateFileName[MAX_BUFFER_LENGTH];

U32 wUpdateTotalPartNumber;
U32 wUpdatePresentPartNumber;
U32 wUpdateSendSize;
U08 bUpdateSendData[MAX_BUFFER_LENGTH];

U08 bRadarStart = 0;

//RADAR_UPDATE_SEND_DATA stRadarSendData;


//RADAR_UPDATE_SEND_DATA stRadarSendData;


/* local variables */
static U08 szMsg[128];


/* imports */


/* function prototypes */
void radar_status_proc(void)
{
	static U08 bPrevRadarStatus;
	static U32 wRadarSendTimer = 0;
	static U08 bErrorCount = 0;
	static U08 bFailCount = 0;
	int fd;


	switch ( bRadarStatus )
	{
	case RADAR_STATUS_NONE :		// 0x0000
		if ( (GetTimeTick10ms() - wRadarResponseTimer) > 500 )
		{
			bRadarStatus = RADAR_DATA_READ_FAIL;
			PrintLog("[Diagnosis][Send] _CMD_COMM_SETUP_FAIL : 0x01 (Radar fail)\n");
			send_put_diagnosis( _CMD_COMM_SETUP_FAIL, 1, 1, 0);
			wRadarResponseTimer = GetTimeTick10ms();
		}
		else if ( ((GetTimeTick10ms() - wRadarResponseTimer) > 300) && (bRadarStart == 0) )
		{
			memset( &szMsg, 0, sizeof(szMsg));
			PrintLog("[Radar][Send] GET_INFO\n");
			Push_COMM_Data(28, GET_INFO, (U08 *)&szMsg);
			bRadarStart = 1;
		}

		break;

	case RADAR_STATUS_POWER_OFF :		// 0x0001

		break;

	case RADAR_STATUS_NORMAL :		// 0x0002
		if ( bPrevRadarStatus != RADAR_STATUS_NORMAL )
		{
			wRadarStatusTimer = 0;
			bErrorCount = 0;
			if ( (bPrevRadarStatus == RADAR_CONFIG_FAIL) | (bPrevRadarStatus == RADAR_DATA_READ_FAIL) | (bPrevRadarStatus == RADAR_OTA_R_MCU_FAIL) | (bPrevRadarStatus == RADAR_OTA_MMIC_FAIL) )
			{
				PrintLog("[Diagnosis][Send] _CMD_COMM_NORMAL : Communication manager is running\n");
				send_put_diagnosis( _CMD_COMM_NORMAL, 1, 0, 0);
			}
		}
		else
		{
			if ( (wRadarTDATimer != 0) && ((GetTimeTick10ms() - wRadarTDATimer) > 300) )
			{
				bRadarStatus = RADAR_DATA_READ_FAIL;
				PrintLog("[Diagnosis][Send] _CMD_COMM_RUN_FAIL : 0x00 (Radar run normal)\n");
				send_put_diagnosis( _CMD_COMM_RUN_FAIL, 1, 0, 0);
				wRadarTDATimer = 0;
			}
			
			if ( (wRadarStatusTimer != 0) && ((GetTimeTick10ms() - wRadarStatusTimer) > 300) )
			{
				if ( bErrorCount > 5 )
				{
					bRadarStatus = RADAR_DATA_READ_FAIL;
					PrintLog("[Diagnosis][Send] _CMD_COMM_RUN_FAIL : 0x01 (Radar run fail)\n");
					send_put_diagnosis( _CMD_COMM_RUN_FAIL, 1, 1, 0);
					wRadarStatusTimer = 0;
				}
				else
				{
					bErrorCount++;
					wRadarStatusTimer = GetTimeTick10ms();
				}
			}

			if ( (tRadatStatusSend == false) && ((GetTimeTick10ms() - wRadarSendTimer) > 200) )
			{
				memset( &szMsg, 0, sizeof(szMsg));
				PrintLog("[Radar][Send] GET_STATUS\n");
				Push_COMM_Data(28, GET_STATUS, (U08 *)&szMsg);
				wRadarSendTimer = GetTimeTick10ms();
				tRadatStatusSend = true;
				wRadarStatusTimer = GetTimeTick10ms();
				bErrorCount = 0;
			}
			
		}

		break;

	case RADAR_STATUS_MMIC_UPDATE : 		// 0x0004
		if ( ((GetTimeTick10ms() - wRadarResponseTimer) > 6000) && (bPrevRadarStatus == bRadarStatus) )
		{
			bRadarStatus = RADAR_OTA_MMIC_FAIL;
			PrintLog("[Diagnosis][Send] _CMD_COMM_OTA_FAIL : 0x01 (Radar OTA fail)\n");
			send_put_diagnosis( _CMD_COMM_OTA_FAIL, 1, 1, 0);
		}
		else
		{
			if ( (bUpdateState == UPDATE_SEND) && bUpdateResponse == UPDATE_RESPONSE_OK )
			{
				memset(&bUpdateSendData, 0, sizeof(bUpdateSendData));
				fd = open(bUpdateFileName, O_RDONLY);
				lseek(fd, (UPDATE_MMIC_SEND_SIZE*wUpdatePresentPartNumber), SEEK_SET);

				wUpdateSendSize = read(fd, &bUpdateSendData[UPDATE_MMIC_HEAD_SIZE], UPDATE_MMIC_SEND_SIZE);

				bUpdateSendData[0] = (U08)(wUpdateTotalPartNumber);
				bUpdateSendData[1] = (U08)(wUpdateTotalPartNumber>>8);
				bUpdateSendData[2] = (U08)(wUpdateTotalPartNumber>>16);
				bUpdateSendData[3] = (U08)(wUpdateTotalPartNumber>>24);
				
				bUpdateSendData[4] = (U08)(wUpdatePresentPartNumber);
				bUpdateSendData[5] = (U08)(wUpdatePresentPartNumber>>8);
				bUpdateSendData[6] = (U08)(wUpdatePresentPartNumber>>16);
				bUpdateSendData[7] = (U08)(wUpdatePresentPartNumber>>24);
				
				bUpdateSendData[8] = (U08)(wUpdateSendSize);
				bUpdateSendData[9] = (U08)(wUpdateSendSize>>8);
				bUpdateSendData[10] = (U08)(wUpdateSendSize>>16);
				bUpdateSendData[11] = (U08)(wUpdateSendSize>>24);

				if ( wUpdateSendSize > 0 )
				{
					PrintLog("[Radar][Send] MMIC_OTA_SEND\n");
					Push_COMM_Data(wUpdateSendSize + UPDATE_MMIC_HEAD_SIZE + UPDATE_MMIC_RESERVED_SIZE, MMIC_OTA_SEND, (U08 *)&bUpdateSendData);
					bUpdateResponse = UPDATE_RESPONSE_WAIT;
				}
				else
				{
					bUpdateState = UPDATE_COMPLETE_WAIT;
					bUpdateResponse == UPDATE_RESPONSE_WAIT;
				}
				close(fd);
			}
		}
		break;

	case RADAR_STATUS_R_MCU_UPDATE : 		// 0x0008
		if ( ((GetTimeTick10ms() - wRadarResponseTimer) > 300) && (bPrevRadarStatus == bRadarStatus) )
		{
			bRadarStatus = RADAR_OTA_R_MCU_FAIL;
			PrintLog("[Diagnosis][Send] _CMD_COMM_OTA_FAIL : 0x01 (Radar OTA fail radar status)\n");
			send_put_diagnosis( _CMD_COMM_OTA_FAIL, 1, 1, 0);
		}
		else
		{
			if ( (bUpdateState == UPDATE_SEND) && bUpdateResponse == UPDATE_RESPONSE_OK )
			{
				memset(&bUpdateSendData, 0, sizeof(bUpdateSendData));
				fd = open(bUpdateFileName, O_RDONLY);
				lseek(fd, (UPDATE_R_MCU_SEND_SIZE*wUpdatePresentPartNumber), SEEK_SET);

				wUpdateSendSize = read(fd, &bUpdateSendData[UPDATE_R_MCU_HEAD_SIZE], UPDATE_R_MCU_SEND_SIZE);

				bUpdateSendData[0] = (U08)(wUpdateTotalPartNumber);
				bUpdateSendData[1] = (U08)(wUpdateTotalPartNumber>>8);
				bUpdateSendData[2] = (U08)(wUpdateTotalPartNumber>>16);
				bUpdateSendData[3] = (U08)(wUpdateTotalPartNumber>>24);
				
				bUpdateSendData[4] = (U08)(wUpdatePresentPartNumber);
				bUpdateSendData[5] = (U08)(wUpdatePresentPartNumber>>8);
				bUpdateSendData[6] = (U08)(wUpdatePresentPartNumber>>16);
				bUpdateSendData[7] = (U08)(wUpdatePresentPartNumber>>24);
				
				bUpdateSendData[8] = (U08)(wUpdateSendSize);
				bUpdateSendData[9] = (U08)(wUpdateSendSize>>8);
				bUpdateSendData[10] = (U08)(wUpdateSendSize>>16);
				bUpdateSendData[11] = (U08)(wUpdateSendSize>>24);
				
				if ( wUpdateSendSize > 0 )
				{
					PrintLog("[Radar][Send] R_MCU_OTA_SEND\n");
					Push_COMM_Data(wUpdateSendSize + UPDATE_R_MCU_HEAD_SIZE + UPDATE_R_MCU_RESERVED_SIZE, R_MCU_OTA_SEND, (U08 *)&bUpdateSendData);
					bUpdateResponse = UPDATE_RESPONSE_WAIT;
				}
				else
				{
					bUpdateState = UPDATE_COMPLETE_WAIT;
					bUpdateResponse == UPDATE_RESPONSE_WAIT;
				}
				close(fd);
			}
		}
		break;

	case RADAR_CONFIG_FAIL :		// 0x0010
		if ( bPrevRadarStatus != RADAR_CONFIG_FAIL )
		{
			PrintLog("[Diagnosis][Send] _CMD_COMM_OTA_FAIL : 0x01 (Radar OTA fail)\n");
			send_put_diagnosis( _CMD_COMM_SETUP_FAIL, 1, 1, 0);
		}
		
		break;

	case RADAR_DATA_READ_FAIL : 	// 0x0020
		if ( bPrevRadarStatus != RADAR_DATA_READ_FAIL )
		{
			bRadarStatus = RADAR_DATA_READ_FAIL;
			bFailCount = 0;
			PrintLog("[Diagnosis][Send] _CMD_COMM_RUN_FAIL : 0x01 (Radar run fail)\n");
			send_put_diagnosis( _CMD_COMM_RUN_FAIL, 1, 1, 0);
		}
		else
		{
			if ( bFailCount > 6 )
			{
				bErrorCount = 0;
				bFailCount = 0;
				radar_status_init();
				comm_mcu_disconnect();
				wRadarResponseTimer = GetTimeTick10ms();
				bRadarStart = 0;				
			}
			if ( (GetTimeTick10ms() - wRadarSendTimer) > 500 )
			{
				memset( &szMsg, 0, sizeof(szMsg));
				PrintLog("[Radar][Send] GET_STATUS\n");
				Push_COMM_Data(28, GET_STATUS, (U08 *)&szMsg);
				wRadarSendTimer = GetTimeTick10ms();
				tRadatStatusSend = true;
				wRadarStatusTimer = GetTimeTick10ms();
				bErrorCount = 0;
				bFailCount++;
			}
		}

		break;

	case RADAR_OTA_R_MCU_FAIL :		// 0x0040
		if ( bPrevRadarStatus != RADAR_OTA_R_MCU_FAIL )
		{
			PrintLog("[Diagnosis][Send] _CMD_COMM_OTA_FAIL : 0x01 (Radar OTA fail)\n");
			send_put_diagnosis( _CMD_COMM_OTA_FAIL, 1, 1, 0);
		}
		
		break;

	case RADAR_OTA_MMIC_FAIL :		// 0x0080
		if ( bPrevRadarStatus != RADAR_OTA_MMIC_FAIL )
		{
			PrintLog("[Diagnosis][Send] _CMD_COMM_OTA_FAIL : 0x01 (Radar OTA fail)\n");
			send_put_diagnosis( _CMD_COMM_OTA_FAIL, 1, 1, 0);
		}
		
		break;

	case RADAR_R_MCU_BOOTLOADER :		// 0x0100
		
		break;

	default:
		break;
	}

	bPrevRadarStatus = bRadarStatus;
}


void radar_status_init(void)
{
	wRadarResponseTimer = GetTimeTick10ms();
	wRadarTDATimer = 0;
	bRadarStatus = RADAR_STATUS_NONE;
	bRadarRequest = RADAR_STATUS_NONE;
	bRadarResponse = RADAR_STATUS_NONE;
	memset( &stRadarConfig, 0, sizeof(stRadarConfig) );
//	memset( &stRadarSendData, 0, sizeof(stRadarSendData) );
}


