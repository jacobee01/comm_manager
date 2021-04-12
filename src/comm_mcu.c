/**
 *
 * @brief communication functions for UART with MCU
 * @file comm_mcu.c
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-10
 * @version 1.0.0
 *
 */


/* includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "common.h"
#include "comm_mcu.h"
#include "socket_diagnosis.h"
#include "radar_control.h"


/* define */


/* typedefs */


/* global variables */
TOF comm_mcu_run = FALSE;
U32 wCommMcuAccessTime = 0;
TOF tSleepMode = FALSE;


/* local variables */
static int uartfd = -1;
static U08 szMsg[128];


/* imports */


/* function prototypes */


TOF comm_mcu_connect(void)
{
	struct termios newtio;

	if ( tSleepMode == TRUE )
	{
		wRadarResponseTimer = GetTimeTick10ms();
		bRadarStart = 0;
		return FALSE;
	}

	if ( comm_mcu_run == FALSE )
	{
		if ( bRadarStatus != RADAR_STATUS_POWER_OFF )
		{
			if ( (GetTimeTick10ms() - wCommMcuAccessTime) > 100 )
			{
				wCommMcuAccessTime = GetTimeTick10ms();
				uartfd = open( "/dev/ttyHS1", O_RDWR | O_NOCTTY | O_NONBLOCK );

				// /dev/ttyS0를 사용하기 위해 open()함수를 사용합니다.
				// O_RDWR은 파일 디스크립터인 fd를 읽기와 쓰기 모드로 열기 위한 지정이며
				// O_NOCCTY와 O_NONBLOCK는 시리얼 통신 장치에 맞추어 추가했습니다.

				memset( &newtio, 0, sizeof(newtio) );

				// 시리얼 통신환경 설정을 위한 구조체 변수 newtio 값을 0 바이트로 깨끗이 채웁니다. 

				newtio.c_cflag = B460800;   // 통신 속도 460800
	//			newtio.c_cflag = B115200;   // 통신 속도 460800
				newtio.c_cflag |= CS8;      // 데이터 비트가 8bit 
				newtio.c_cflag |= CLOCAL;   // 외부 모뎀을 사용하지 않고 내부 통신 포트 사용 
				newtio.c_cflag |= CREAD;    // 쓰기는 기본, 읽기도 가능하게 
				newtio.c_iflag = 0;         // parity 비트는 없음
				newtio.c_oflag = 0;
				newtio.c_lflag = 0;
				newtio.c_cc[VTIME] = 0; 
				newtio.c_cc[VMIN] = 1;

				UART_Init();

				tcflush (uartfd, TCIFLUSH );
				if ( tcsetattr(uartfd, TCSANOW, &newtio ) != 0 )   // 포트에 대한 통신 환경을 설정합니다. 
				{
					PrintLog("[Radar][Connect] radar communication error\n");
					return FALSE;
				}
				else
				{
					PrintLog("[Radar][Connect] radar communication start success\n");
				}

				comm_mcu_run = TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}
	else
	{
		if ( bRadarStatus != RADAR_STATUS_POWER_OFF )
		{
			if ( ((GetTimeTick10ms() - wCommMcuAccessTime) > 100) && (wCommMcuAccessTime != 0) )
			{
				PrintLog("[Radar][Send] GET_INFO\n");
				Push_COMM_Data(28, GET_INFO, (U08 *)&szMsg);
				wCommMcuAccessTime = 0;
			}
			
			Uart2_Proc(uartfd);
		}
		else
		{
			wCommMcuAccessTime = GetTimeTick10ms();
			//comm_mcu_disconnect();
		}
	}

	return TRUE;
}


void comm_mcu_disconnect(void)
{
	comm_mcu_run = FALSE;

	PrintLog("[Radar][Connect] radar communication closed\n");

	if ( uartfd > 0 )
	{
//	PrintLog("[Radar][disconnect] uart closed start\n");
		close(uartfd);
//	PrintLog("[Radar][disconnect] uart closed end\n");
	}
	uartfd = -1;
}


