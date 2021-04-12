/**
 *
 * @brief Ethernet Socket Communication Source File
 * @file socket_main.c
 * @details make thread and running for Socket Communication with event manager
 * @author brian.hj.moon@funzin.co.kr
 * @date 2020-09-14
 * @version 1.0.0
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include "sys/stat.h"

#include "common.h"
#include "socket_main.h"
#include "socket_diagnosis.h"

#include "comm_uart.h"
#include "comm_protocol.h"
#include "comm_protocol_command.h"
#include "radar_control.h"
#include "sys_version.h"


#define BACKLOG     10
#define MAXLINE     1024


#define SOCKET_SERVER_ADDR "127.0.0.1"
//#define SOCKET_SERVER_ADDR "192.168.10.10"
#define SOCKET_SERVER_PORT 50000

#define MAX_SEND_BUFFER_LENGTH	5
/////////////////////////////////////////////////////////
// DEFINE

#define _CLIENT_TYPE_COMMUNICATION			0x01
#define _CLIENT_TYPE_RECORDER				0x02
#define _CLIENT_TYPE_APPLICATION			0x03
#define _CLIENT_TYPE_DIAGNOSIS				0x04



static pthread_t socket_main_thread = 0;
TOF socket_main_task_run = FALSE;

static int client_main_sockfd;


SOCKET_SEND_BUF sSocketMainSendBuf[MAX_SEND_BUFFER_LENGTH];
U08 bRecvMainBuffer[SOCKET_BUFF_MAX];

static U08 bMCUCommBuff[SOCKET_BUFF_MAX];


void socket_main_client_task(void);
void recv_main_parser(void);


int socket_main_task_start(void)
{
	int ret;
	int rfd, wfd;
	int readn;
	char buf[80];
	U08 bSendNumber = 0;
	char logdata[MAX_BUF];
	int fp = -1;

	static U32 wSocketMainTaskAccessTime = 0;

	if( (!socket_main_thread) && (socket_main_task_run == FALSE) )
	{
		if ( (GetTimeTick10ms() - wSocketMainTaskAccessTime) > 100 )
		{
			memset(&logdata, 0, sizeof(logdata));
			sprintf(logdata, "pgrep java");
			fp = popen(logdata, "r");
			fread(buf, sizeof(char), sizeof(buf), fp);
			pclose(fp);
			if ( (buf[0] & 0x30) == 0 )
			{
				return FALSE;
			}

			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setstacksize(&attr, 1024*1024);

			ret = pthread_create(&socket_main_thread, &attr, socket_main_client_task, NULL);
			if(ret)
			{
				sprintf(logdata, "[Main][Connect] %s failed\n", __func__);
				PrintLog(logdata);
				return FALSE;
			}
			wSocketMainTaskAccessTime = GetTimeTick10ms();
		}
	}
	else if( socket_main_thread && (socket_main_task_run == TRUE) )
	{
		for ( bSendNumber = 0 ; bSendNumber < MAX_SEND_BUFFER_LENGTH ; bSendNumber++ )
		{
			if ( sSocketMainSendBuf[bSendNumber].uSendEnbale != 0 )
			{
				/**
				 *
				 * @brief check send buffer and if exist data then send data through opened socket
				 *
				 */
				if(write(client_main_sockfd, sSocketMainSendBuf[bSendNumber].bSendBuf, sSocketMainSendBuf[bSendNumber].uLength) < 0)
				{   
					//perror("(From Client) write error by input string: ");
					PrintLog("[Main][Connect] socket send error\n");
				}
				else
				{
					sSocketMainSendBuf[bSendNumber].uSendEnbale = FALSE;
				}
			}
		}
	}
	else
	{
		return FALSE;
	}
	
	return TRUE;
}


void socket_main_task_stop(void)
{
	LINGER linger = {1, 0};

	if( client_main_sockfd )
	{
		setsockopt(client_main_sockfd, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));
		shutdown(client_main_sockfd, SHUT_RDWR);
		close(client_main_sockfd);
	}

	if ( socket_main_task_run == TRUE )
	{
		PrintLog("[Main][Connect] main socket connect closed\n");
	}
	
	socket_main_task_run = FALSE;

	if( socket_main_thread )
	{
		pthread_join(socket_main_thread, NULL);
	}


	socket_main_thread = 0;
}


void socket_main_client_task(void)
{
	U16 state, recv_len;
	U16 pid, i;

	struct sockaddr_in serveraddr;
	U08 btest = 0;

	U08 recv_buf[SOCKET_BUFF_MAX];

	//add var
	U16 data_length = 0;
	U08 sp = 0;
	char logdata[MAX_BUF];

	state = 0;
	memset(&sSocketMainSendBuf, 0, sizeof(sSocketMainSendBuf));

	if ((client_main_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		PrintLog("[Main][Connect] creation of main socket error\n");
		exit(0);
	}
	else
	{
		PrintLog("[Main][Connect] creation of main socket success\n");
	}

	memset((char *)&serveraddr, '\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SOCKET_SERVER_ADDR);
	serveraddr.sin_port = htons(SOCKET_SERVER_PORT);

	if(connect(client_main_sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		//perror("connect fail");
		PrintLog("[Main][Connect] main socket connect fail\n");
		socket_main_task_stop();
		socket_main_thread = NULL;
		socket_main_task_run = FALSE;
		return;
//		exit(0);
	}

	PrintLog("[Main][Connect] main socket connect success\n");

	socket_main_task_run = TRUE;

	wCommMcuAccessTime = GetTimeTick10ms();
	
	while(1)
	{
		memset(recv_buf, 0, 255);

		recv_len = recv(client_main_sockfd, recv_buf, 255, 0);
		if (recv_len > 0)
		{
			while(recv_len>0)
			{
				data_length = (U16)recv_buf[sp+3]|(U16)recv_buf[sp+2]<<8;
				//memcpy ( &bRecvMainBuffer[0], &recv_buf[0], recv_len);
				memcpy ( &bRecvMainBuffer[0], &recv_buf[sp], data_length+4);
				recv_main_parser();
				sprintf(logdata, "sp : %d legnth : %d recv : %d \n", sp , data_length, recv_buf[sp+3]);
				PrintLog(logdata);
				sp+=data_length+4;
				sprintf(logdata, "sp : %d legnth : %d recv : %d \n", sp , data_length, recv_buf[sp+3]);
				PrintLog(logdata);
				recv_len-=data_length+4;
			}
			sp = 0;
		}
		else if (recv_len <= 0)
		{
			break;
		}
		btest = 1;
	}

	socket_main_task_stop();
}


void recv_main_parser(void)
{
	U16 uRecvCommand = 0, uRecvLength = 0;
	U32 wTempLength = 0;
	int fd = 0;
	struct stat sb;
	char logdata[MAX_BUF];

	int i;

	memcpy ( &uRecvCommand, &bRecvMainBuffer[0], 2);
	memcpy ( &uRecvLength, &bRecvMainBuffer[2], 2);
	uRecvCommand = SWAP_WORD(uRecvCommand);
	//printf("uRecvCommand: 0x%04X\n", uRecvCommand);
	uRecvLength = SWAP_WORD(uRecvLength);
	//printf("RecvBuffer Length : %d\n", uRecvLength);
	//for ( i = 0 ; i < (uRecvLength+4) ; i++ )
	//{
	//	printf("%d : 0x%02X, ", i, bRecvMainBuffer[i]);
	//}
	//printf("\n");
	
	
	switch ( uRecvCommand )
	{
	case _CMD_MAIN_TURN_IRED :				// 0x0401
		sprintf(logdata, "[Main][[Receive] IRED Event (Length = %d) : 0x%02X\n", uRecvLength, bRecvMainBuffer[4]);
		PrintLog(logdata);

		break;

	case _CMD_MAIN_RADAR_CONFIG :			// 0x0503
		wTempLength += (U32)bRecvMainBuffer[5];
		wTempLength += (U32)bRecvMainBuffer[4]<<8;

		memcpy( &bMCUCommBuff[0], &bRecvMainBuffer[6], (uRecvLength-1) );
		sprintf(logdata, "[Main][[Receive] Radar Configuration Event (Length = %d) : %s\n", uRecvLength, bMCUCommBuff);
		PrintLog(logdata);

		PrintLog("[Radar][Send] MMIC_CLI_SEND\n");
		Push_COMM_Data( wTempLength, MMIC_CLI_SEND, &bMCUCommBuff[0]);
		bRadarRequest = MMIC_CLI_SEND;

		break;

	case _CMD_MAIN_RADAR_GET_BUFFER :			// 0x050B
		memset( &bMCUCommBuff[0], 0, sizeof(bMCUCommBuff) );
		sprintf(logdata, "[Main][[Receive] Radar Configuration Event (Length = %d) : %s\n", uRecvLength, bMCUCommBuff);
		PrintLog(logdata);

		PrintLog("[Radar][Send] GET_TDA_FRAMEBUFFER\n");
		Push_COMM_Data( 28, GET_TDA_FRAMEBUFFER, &bMCUCommBuff[0]);
		wRadarTDATimer = GetTimeTick10ms();

		break;

	case _CMD_MAIN_OTA_R_MCU_FILE :			// 0x0504
		wTempLength += (U16)bRecvMainBuffer[3];
		wTempLength += (U16)bRecvMainBuffer[2]<<8;
		memset ( &bUpdateFileName[0], 0, sizeof(bUpdateFileName) );
		memcpy ( &bUpdateFileName[0], &bRecvMainBuffer[4], wTempLength );
		sprintf(logdata, "[Main][[Receive] R_MCU Application Update Event (Length = %d) : %s\n", uRecvLength, bUpdateFileName);
		PrintLog(logdata);
		
		if ( stat (bUpdateFileName, & sb) == 0 )
		{
			bUpdateDevice = DEVICE_R_MCU;
			bUpdateState = UPDATE_READY;
			wUpdateTotalFileSize = sb.st_size;
			if ( (wUpdateTotalFileSize%UPDATE_R_MCU_SEND_SIZE) == 0 )
			{
				wUpdateTotalPartNumber = (wUpdateTotalFileSize/UPDATE_R_MCU_SEND_SIZE);
			}
			else
			{
				wUpdateTotalPartNumber = (wUpdateTotalFileSize/UPDATE_R_MCU_SEND_SIZE) + 1;
			}
			
			wUpdatePresentPartNumber = 0;
			wRadarResponseTimer = GetTimeTick10ms();
		}
		else
		{
			bUpdateDevice = DEVICE_R_MCU;
			bUpdateState = UPDATE_FILE_ERROR;
		}

		break;

	case _CMD_MAIN_OTA_RADAR_FILE :			// 0x0505
		wTempLength += (U16)bRecvMainBuffer[3];
		wTempLength += (U16)bRecvMainBuffer[2]<<8;
		memset ( &bUpdateFileName[0], 0, sizeof(bUpdateFileName) );
		memcpy ( &bUpdateFileName[0], &bRecvMainBuffer[4], wTempLength );
		sprintf(logdata, "[Main][[Receive] Radar Binary Update Event (Length = %d) : %s\n", uRecvLength, bUpdateFileName);
		PrintLog(logdata);

		if ( stat (bUpdateFileName, & sb) == 0 )
		{
			bUpdateDevice = DEVICE_MMIC;
			bUpdateState = UPDATE_READY;
			bUpdateResponse = UPDATE_RESPONSE_WAIT;
			wUpdateTotalFileSize = sb.st_size;
			if ( (wUpdateTotalFileSize%UPDATE_MMIC_SEND_SIZE) == 0 )
			{
				wUpdateTotalPartNumber = (wUpdateTotalFileSize/UPDATE_MMIC_SEND_SIZE);
			}
			else
			{
				wUpdateTotalPartNumber = (wUpdateTotalFileSize/UPDATE_MMIC_SEND_SIZE) + 1;
			}
			
			wUpdatePresentPartNumber = 0;
			wRadarResponseTimer = GetTimeTick10ms();
		}
		else
		{
			bUpdateDevice = DEVICE_MMIC;
			bUpdateState = UPDATE_FILE_ERROR;
		}

		break;

	case _CMD_MAIN_OTA_READY :				// 0x0510
		sprintf(logdata, "[Main][[Receive] R_MCU or Radar update command (Length = %d) : 0x%02X\n", uRecvLength, bRecvMainBuffer[4]);
		PrintLog(logdata);
		if ( bRecvMainBuffer[4] == 0x01 )		//Radar
		{
			if ( (bUpdateDevice == DEVICE_MMIC) && (bUpdateState != UPDATE_FILE_ERROR) )
			{
				memset( &bUpdateSendData, 0, sizeof(bUpdateSendData) );
				bRadarStatus = RADAR_STATUS_MMIC_UPDATE;
				wRadarResponseTimer = GetTimeTick10ms();

				bRadarStatus = RADAR_STATUS_MMIC_UPDATE;
				bUpdateDevice = DEVICE_MMIC;
				bUpdateState = UPDATE_START;
				bUpdateResponse = UPDATE_RESPONSE_WAIT;

				memset ( &bMCUCommBuff, 0, sizeof(bMCUCommBuff) );
				bMCUCommBuff[0] = (U08)(wUpdateTotalFileSize);
				bMCUCommBuff[1] = (U08)(wUpdateTotalFileSize>>8);
				bMCUCommBuff[2] = (U08)(wUpdateTotalFileSize>>16);
				bMCUCommBuff[3] = (U08)(wUpdateTotalFileSize>>24);

				PrintLog("[Radar][Send] MMIC_OTA_START\n");
				Push_COMM_Data( 28, MMIC_OTA_START, &bMCUCommBuff[0]);
			}
			else
			{
				sprintf(logdata, "[Main][Send] R_MCU or Radar update ready event : 0x01 (MMIC not ready)\n");
				PrintLog(logdata);
				send_put_main( _CMD_MAIN_OTA_READY_RES, 1, 1, 0);
			}

		}
		else if ( bRecvMainBuffer[4] == 0x02 )	//R_MCU
		{
			if ( (bUpdateDevice == DEVICE_R_MCU) && (bUpdateState != UPDATE_FILE_ERROR) )
			{
				memset( &bUpdateSendData, 0, sizeof(bUpdateSendData) );
				bRadarStatus = RADAR_STATUS_R_MCU_UPDATE;
				wRadarResponseTimer = GetTimeTick10ms();

				bUpdateDevice = DEVICE_R_MCU;
				bUpdateState = UPDATE_READY;
				bUpdateResponse = UPDATE_RESPONSE_WAIT;

				memset ( &bMCUCommBuff, 0, sizeof(bMCUCommBuff) );
				bMCUCommBuff[0] = (U08)(wUpdateTotalFileSize);
				bMCUCommBuff[1] = (U08)(wUpdateTotalFileSize>>8);
				bMCUCommBuff[2] = (U08)(wUpdateTotalFileSize>>16);
				bMCUCommBuff[3] = (U08)(wUpdateTotalFileSize>>24);

				PrintLog("[Radar][Send] R_MCU_OTA_BOOTLOADER\n");
				Push_COMM_Data( 28, R_MCU_OTA_BOOTLOADER, &bMCUCommBuff[0]);
			}
			else
			{
				sprintf(logdata, "[Main][Send] R_MCU or Radar update ready event : 0x01 (R_MCU not ready)\n");
				PrintLog(logdata);
				send_put_main( _CMD_MAIN_OTA_READY_RES, 1, 1, 0);
			}
		}
		else
		{
			bRadarStatus = RADAR_OTA_MMIC_FAIL;

			sprintf(logdata, "[Main][Send] R_MCU or Radar update ready event : 0x01 (main data error)\n");
			PrintLog(logdata);
			send_put_main( _CMD_MAIN_OTA_READY_RES, 1, 1, 0);
		}
		break;

	case _CMD_MAIN_OTA_START :				// 0x0514
		sprintf(logdata, "[Main][[Receive] R_MCU or Radar update start event (Length = %d) : 0x%02X\n", uRecvLength, bRecvMainBuffer[4]);
		PrintLog(logdata);
		if ( bRecvMainBuffer[4] == 0x01 )		//Radar
		{
			if ( (bRadarStatus == RADAR_STATUS_MMIC_UPDATE) && (bUpdateState == UPDATE_START) && (bUpdateResponse = UPDATE_RESPONSE_OK) )
			{
				bUpdateState = UPDATE_SEND;
				bUpdateResponse = UPDATE_RESPONSE_OK;
				wRadarResponseTimer = GetTimeTick10ms();
			}
			else
			{
				sprintf(logdata, "[Main][Send] R_MCU or Radar update result event : 0x01 (MMIC failure)\n");
				PrintLog(logdata);
				send_put_main( _CMD_MAIN_OTA_UPDATE_RES, 1, 1, 0);
			}
		}
		else if ( bRecvMainBuffer[4] == 0x02 )	//R_MCU
		{
			if ( (bRadarStatus == RADAR_STATUS_R_MCU_UPDATE) && (bUpdateState == UPDATE_START) && (bUpdateResponse = UPDATE_RESPONSE_OK) )
			{
				bUpdateState = UPDATE_SEND;
				bUpdateResponse = UPDATE_RESPONSE_OK;
				wRadarResponseTimer = GetTimeTick10ms();
			}
			else
			{
				sprintf(logdata, "[Main][Send] R_MCU or Radar update result event : 0x01 (R_MCU failure)\n");
				PrintLog(logdata);
				send_put_main( _CMD_MAIN_OTA_UPDATE_RES, 1, 1, 0);
			}
		}
		else
		{
			bRadarStatus = RADAR_OTA_MMIC_FAIL;
			send_put_diagnosis( _CMD_COMM_OTA_FAIL, 1, 1, 0);

			sprintf(logdata, "[Main][Send] R_MCU or Radar update result event : 0x01 (main data error)\n");
			PrintLog(logdata);
			send_put_main( _CMD_MAIN_OTA_UPDATE_RES, 1, 1, 0);
		}

		break;

	case _CMD_MAIN_TURN_LED :				// 0x0506
		sprintf(logdata, "[Main][[Receive] LED Event (Length = %d) : 0x%02X\n", uRecvLength, bRecvMainBuffer[4]);
		PrintLog(logdata);

		if ( bRecvMainBuffer[4] == 0 )
		{
			stRadarConfig.stLedManual.bRun = 1;
		}
		else
		{
			stRadarConfig.stLedManual.bRun = 0;
		}

		PrintLog("[Radar][Send] SET_CONFIG\n");
		Push_COMM_Data( sizeof(stRadarConfig), SET_CONFIG, (U08*)&stRadarConfig);
		break;

	case _CMD_MAIN_LED_CONFIG_BLINK :		// 0x0507
		sprintf(logdata, "[Main][[Receive] LED Configuration Event, LED Blink Pattern (Length = %d) : 0x%02X\n", uRecvLength, bRecvMainBuffer[4]);
		PrintLog(logdata);

		if ( bRecvMainBuffer[4] == 0 )	//static
		{
			stRadarConfig.stLedManual.uOnTime = 0xFFFF;
			stRadarConfig.stLedManual.uOffTime = 0;
			stRadarConfig.stLedManual.bDimmerEnable = 0;
		}
		else if ( bRecvMainBuffer[4] == 1 )		//blink
		{
			if ( stMainLEDConfig.uDutyCycle != 0 )
			{
				stRadarConfig.stLedManual.uOnTime = stMainLEDConfig.uDutyCycle;
				stRadarConfig.stLedManual.uOffTime = stMainLEDConfig.uDutyCycle;
			}
			else
			{
				stRadarConfig.stLedManual.uOnTime = 1000;
				stRadarConfig.stLedManual.uOffTime = 1000;
			}
			stRadarConfig.stLedManual.bDimmerEnable = 0;
		}
		else	//breathe
		{
			if ( stMainLEDConfig.uDutyCycle != 0 )
			{
				stRadarConfig.stLedManual.uOnTime = stMainLEDConfig.uDutyCycle;
				stRadarConfig.stLedManual.uOffTime = stMainLEDConfig.uDutyCycle;
			}
			else
			{
				stRadarConfig.stLedManual.uOnTime = 1000;
				stRadarConfig.stLedManual.uOffTime = 1000;
			}
			stRadarConfig.stLedManual.bDimmerEnable = 1;
		}

		stMainLEDConfig.bBlinkPattern == bRecvMainBuffer[4];
		
		PrintLog("[Radar][Send] SET_CONFIG\n");
		Push_COMM_Data( sizeof(stRadarConfig), SET_CONFIG, (U08*)&stRadarConfig);
		break;

	case _CMD_MAIN_LED_CONFIG_CYCLE :		// 0x0508

		stMainLEDConfig.uDutyCycle = ((U16)bRecvMainBuffer[4]<<8) + (U16)bRecvMainBuffer[5];
		sprintf(logdata, "[Main][[Receive] LED Configuration Event, LED Duty Cycle (Length = %d) : %d\n", uRecvLength, stMainLEDConfig.uDutyCycle);
		PrintLog(logdata);
		stRadarConfig.stLedManual.uOnTime = stMainLEDConfig.uDutyCycle;
		stRadarConfig.stLedManual.uOffTime = stMainLEDConfig.uDutyCycle;

		PrintLog("[Radar][Send] SET_CONFIG\n");
		Push_COMM_Data( sizeof(stRadarConfig), SET_CONFIG, (U08*)&stRadarConfig);
		break;

	case _CMD_MAIN_LED_CONFIG_COLOR :		// 0x0509
		sprintf(logdata, "[Main][[Receive] LED Configuration Event, LED Color (Length = %d) : Red = %d, Green = %d, Blue = %d\n", uRecvLength, bRecvMainBuffer[4],bRecvMainBuffer[5],bRecvMainBuffer[6]);
		PrintLog(logdata);

		stMainLEDConfig.bRed = bRecvMainBuffer[4];
		stMainLEDConfig.bGreen = bRecvMainBuffer[5];
		stMainLEDConfig.bBlue = bRecvMainBuffer[6];
		stRadarConfig.stLedManual.bRed = stMainLEDConfig.bRed;
		stRadarConfig.stLedManual.bGreen = stMainLEDConfig.bGreen;
		stRadarConfig.stLedManual.bBlue = stMainLEDConfig.bBlue;

		PrintLog("[Radar][Send] SET_CONFIG\n");
		Push_COMM_Data( sizeof(stRadarConfig), SET_CONFIG, (U08*)&stRadarConfig);
		break;

	case _CMD_MAIN_LED_LIVESTREAM :			// 0x05A6
		sprintf(logdata, "[Main][[Receive] LED Livestreaming Event (Length = %d) : 0x%02X\n", uRecvLength, bRecvMainBuffer[4]);
		PrintLog(logdata);

		if ( bRecvMainBuffer[4] == LED_SOC_CONTROL_MODE )
		{
			stRadarConfig.wLedMode = LED_SOC_CONTROL_MODE;
		}
		else
		{
			stRadarConfig.wLedMode = LED_TDA_CONTROL_MODE;
		}

		PrintLog("[Radar][Send] SET_CONFIG\n");
		Push_COMM_Data( sizeof(stRadarConfig), SET_CONFIG, (U08*)&stRadarConfig);
		break;

	case _CMD_MAIN_ARMED_CONFIG :			// 0x0701
		sprintf(logdata, "[Main][[Receive] Armed/Disarmed Event (Length = %d) : 0x%02X\n", uRecvLength, bRecvMainBuffer[4]);
		PrintLog(logdata);
		if ( bRecvMainBuffer[4] == 0 )
		{
			bRadarStatus = RADAR_STATUS_NORMAL;
		}
		else
		{
			bRadarStatus = RADAR_STATUS_POWER_OFF;
		}
		memset( bMCUCommBuff, 0, sizeof(bMCUCommBuff) );
		bMCUCommBuff[0] = bRecvMainBuffer[4];

		break;

	case _CMD_MAIN_SLEEP_EVENT :			// 0x0530
		sprintf(logdata, "[Main][[Receive] Sleep / Wake up Event (Length = %d) : 0x%02X\n", uRecvLength, bRecvMainBuffer[4]);
		PrintLog(logdata);
		if ( bRecvMainBuffer[4] == 0 )
		{
			tSleepMode = TRUE;
			radar_status_init();
			comm_mcu_disconnect();
		}
		else
		{
			tSleepMode = FALSE;
		}

		break;

	case _CMD_MAIN_COMM_VERSION :			// 0x0590
		sprintf(logdata, "[Main][[Receive] Resonpse App Version (Length = %d)\n", uRecvLength);
		PrintLog(logdata);

		sprintf(logdata, "[Main][Send] Resonpse App Version : %s\n", oSystemVersion.szSoftwareVer);
		PrintLog(logdata);
		send_put_main( _CMD_MAIN_COMM_VERSION_RES, 6, (U08 *)oSystemVersion.szSoftwareVer, 0);

		break;


	default:
		break;
	}
}


void send_put_main(U16 uCommand, U16 uLength, U08 *bData, U08 bParameter)
{
	static U08 bPutPosition = 0;

	if( socket_main_task_run == FALSE )
	{
		return;
	}

	sSocketMainSendBuf[bPutPosition].bSendBuf[0] = (U08)(uCommand >> 8);
	sSocketMainSendBuf[bPutPosition].bSendBuf[1] = (U08)(uCommand >> 0);

	sSocketMainSendBuf[bPutPosition].bSendBuf[2] = (U08)((uLength+1) >> 8);
	sSocketMainSendBuf[bPutPosition].bSendBuf[3] = (U08)((uLength+1) >> 0);

	if ( uLength == 1 )
	{
		sSocketMainSendBuf[bPutPosition].bSendBuf[4] = bData;
	}
	else
	{
		memcpy ( &sSocketMainSendBuf[bPutPosition].bSendBuf[4], bData, uLength);
	}
	sSocketMainSendBuf[bPutPosition].bSendBuf[4+uLength] = bParameter;
	sSocketMainSendBuf[bPutPosition].uLength = 5 + uLength;
	sSocketMainSendBuf[bPutPosition].uSendEnbale = TRUE;

	bPutPosition++;
	if ( bPutPosition >= MAX_SEND_BUFFER_LENGTH )
	{
		bPutPosition = 0;
	}
}


