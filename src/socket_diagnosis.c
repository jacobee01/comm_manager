/**
 *
 * @brief Ethernet Socket Communication Source File
 * @file socket_diagnosis.c
 * @details make thread and running for Socket Communication with event manager
 * @author brian.hj.moon@funzin.co.kr
 * @date 2020-09-11
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

#include "common.h"
#include "socket_diagnosis.h"
#include "socket_main.h"


#define BACKLOG     10
#define MAXLINE     1024


#define SOCKET_DIAGNOSIS_SERVER_ADDR "127.0.0.1"
#define SOCKET_DIAGNOSIS_SERVER_PORT 50005

#define MAX_SEND_BUFFER_LENGTH	5


static pthread_t socket_diagnosis_thread = 0;
TOF socket_diagnosis_task_run = FALSE;

static int client_diagnosis_sockfd;


SOCKET_SEND_BUF sSocketdiagnosisSendBuf[MAX_SEND_BUFFER_LENGTH];
U08 bRecvBuffer[SOCKET_BUFF_MAX];

void socket_diagnosis_client_task(void);
void recv_diagnosis_parser(void);


int socket_diagnosis_task_start(void)
{
	int ret;
	int rfd, wfd;
	int readn;
	char buf[80];
	U08 bSendNumber = 0;
	char logdata[MAX_BUF];

	static U32 wSocketDiagnosisTaskAccessTime = 0;


	if( (!socket_diagnosis_thread) && (socket_diagnosis_task_run == FALSE) )
	{
		if ( (GetTimeTick10ms() - wSocketDiagnosisTaskAccessTime) > 500 )
		{
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setstacksize(&attr, 1024*1024);

			ret = pthread_create(&socket_diagnosis_thread, &attr, socket_diagnosis_client_task, NULL);
			if(ret)
			{
				sprintf(logdata, "[Diagnosis][Connect] %s failed\n", __func__);
				PrintLog(logdata);
				return FALSE;
			}
			wSocketDiagnosisTaskAccessTime = GetTimeTick10ms();
		}
	}
	else if( socket_diagnosis_thread && (socket_diagnosis_task_run == TRUE) )
	{
		for ( bSendNumber = 0 ; bSendNumber < MAX_SEND_BUFFER_LENGTH ; bSendNumber++ )
		{
			if ( sSocketdiagnosisSendBuf[bSendNumber].uSendEnbale != 0 )
			{
				/**
				 *
				 * @brief check send buffer and if exist data then send data through opened socket
				 *
				 */
				if(write(client_diagnosis_sockfd, sSocketdiagnosisSendBuf[bSendNumber].bSendBuf, sSocketdiagnosisSendBuf[bSendNumber].uLength) < 0)
				{   
					//perror("(From Client) write error by input string: ");
					PrintLog("[Diagnosis][Connect] socket send error\n");
				}
				else
				{
					sSocketdiagnosisSendBuf[bSendNumber].uSendEnbale = FALSE;
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


void socket_diagnosis_task_stop(void)
{
	LINGER linger = {1, 0};

	if( client_diagnosis_sockfd )
	{
		setsockopt(client_diagnosis_sockfd, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger));
		shutdown(client_diagnosis_sockfd, SHUT_RDWR);
		close(client_diagnosis_sockfd);
	}

	if ( socket_diagnosis_task_run == TRUE )
	{
		PrintLog("[Diagnosis][Connect] diagnosis socket connect closed\n");
	}
	
	socket_diagnosis_task_run = FALSE;

	if( socket_diagnosis_thread )
	{
		pthread_join(socket_diagnosis_thread, NULL);
	}

	socket_diagnosis_thread = 0;
}


void socket_diagnosis_client_task(void)
{
	unsigned int state, recv_len;
	int pid, i;

	struct sockaddr_in serveraddr;

	U08 recv_buf[SOCKET_BUFF_MAX];

	state = 0;

	if ((client_diagnosis_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		PrintLog("[Diagnosis][Connect] creation of diagnosis socket error\n");
		socket_diagnosis_task_stop();
		return;
//		exit(0);
	}
	else
	{
		PrintLog("[Diagnosis][Connect] creation of diagnosis socket success\n");
	}

	memset((char *)&serveraddr, '\0', sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SOCKET_DIAGNOSIS_SERVER_ADDR);
	serveraddr.sin_port = htons(SOCKET_DIAGNOSIS_SERVER_PORT);

	if(connect(client_diagnosis_sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		//perror("connect fail");
		PrintLog("[Diagnosis][Connect] diagnosis socket connect fail\n");
		socket_diagnosis_task_stop();
		socket_diagnosis_thread = NULL;
		socket_diagnosis_task_run = FALSE;
		return;
//		exit(0);
	}

	socket_diagnosis_task_run = TRUE;

	send_put_diagnosis( _CMD_COMM_NORMAL, 1, 0, 0);
	
	while(1)
	{
		memset(recv_buf, 0, 255);

		recv_len = recv(client_diagnosis_sockfd, recv_buf, 255, 0);
		if (recv_len > 0)
		{
			memcpy ( &bRecvBuffer[0], &recv_buf[0], recv_len);
			recv_diagnosis_parser();
		}
		else if (recv_len <= 0)
		{
			break;
		}
	}

	socket_diagnosis_task_stop();
}


void recv_diagnosis_parser(void)
{
	U16 uRecvCommand = 0;
	char logdata[MAX_BUF];

	memcpy ( &uRecvCommand, &bRecvBuffer[0], 2);

	sprintf(logdata, "Diagnosis][Receive] Command : 0x%04X\n", uRecvCommand);
	PrintLog(logdata);
	
	switch ( uRecvCommand )
	{
	default:
		break;
	}
}


void send_put_diagnosis(U16 uCommand, U16 uLength, U08 *bData, U08 bParameter)
{
	static U08 bPutPosition = 0;

	if( socket_diagnosis_task_run == FALSE )
	{
		return;
	}

	sSocketdiagnosisSendBuf[bPutPosition].bSendBuf[0] = (U08)(uCommand >> 8);
	sSocketdiagnosisSendBuf[bPutPosition].bSendBuf[1] = (U08)(uCommand >> 0);

	sSocketdiagnosisSendBuf[bPutPosition].bSendBuf[2] = (U08)((uLength+1) >> 8);
	sSocketdiagnosisSendBuf[bPutPosition].bSendBuf[3] = (U08)((uLength+1) >> 0);

	if ( uLength == 1 )
	{
		sSocketdiagnosisSendBuf[bPutPosition].bSendBuf[4] = bData;
	}
	else
	{
		memcpy ( &sSocketdiagnosisSendBuf[bPutPosition].bSendBuf[4], bData, uLength);
	}

	sSocketdiagnosisSendBuf[bPutPosition].bSendBuf[4+uLength] = bParameter;
	sSocketdiagnosisSendBuf[bPutPosition].uLength = 5 + uLength;
	sSocketdiagnosisSendBuf[bPutPosition].uSendEnbale = TRUE;

	bPutPosition++;
	if ( bPutPosition >= MAX_SEND_BUFFER_LENGTH )
	{
		bPutPosition = 0;
	}
}


