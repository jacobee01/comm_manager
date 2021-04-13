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

				// /dev/ttyS0�� ����ϱ� ���� open()�Լ��� ����մϴ�.
				// O_RDWR�� ���� ��ũ������ fd�� �б�� ���� ���� ���� ���� �����̸�
				// O_NOCCTY�� O_NONBLOCK�� �ø��� ��� ��ġ�� ���߾� �߰��߽��ϴ�.

				memset( &newtio, 0, sizeof(newtio) );

				// �ø��� ���ȯ�� ������ ���� ����ü ���� newtio ���� 0 ����Ʈ�� ������ ä��ϴ�. 

				newtio.c_cflag = B460800;   // ��� �ӵ� 460800
	//			newtio.c_cflag = B115200;   // ��� �ӵ� 460800
				newtio.c_cflag |= CS8;      // ������ ��Ʈ�� 8bit 
				newtio.c_cflag |= CLOCAL;   // �ܺ� ���� ������� �ʰ� ���� ��� ��Ʈ ��� 
				newtio.c_cflag |= CREAD;    // ����� �⺻, �б⵵ �����ϰ� 
				newtio.c_iflag = 0;         // parity ��Ʈ�� ����
				newtio.c_oflag = 0;
				newtio.c_lflag = 0;
				newtio.c_cc[VTIME] = 0; 
				newtio.c_cc[VMIN] = 1;

				UART_Init();

				tcflush (uartfd, TCIFLUSH );
				if ( tcsetattr(uartfd, TCSANOW, &newtio ) != 0 )   // ��Ʈ�� ���� ��� ȯ���� �����մϴ�. 
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


int comm_mcu_disconnect(void)
{
	int rt = 0;
	comm_mcu_run = FALSE;

	PrintLog("[Radar][Connect] radar communication closed\n");

	if ( uartfd > 0 )
	{
//	PrintLog("[Radar][disconnect] uart closed start\n");
		rt = close(uartfd);
//	PrintLog("[Radar][disconnect] uart closed end\n");
	}
	uartfd = -1;
	return rt;
}


