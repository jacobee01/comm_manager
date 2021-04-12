/**
 *
 * @brief This program is diagnosing system failure and notifing system state
 * @file communication_main.c
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-14
 * @version 1.0.0
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <string.h>
//#include <arpa/inet.h>
//#include <sys/socket.h>
//#include <sys/types.h> 
#include <sys/wait.h>

#include "common.h"
#include "socket_main.h"
#include "socket_diagnosis.h"
#include "radar_control.h"
#include "comm_mcu.h"
#include "sys_version.h"


void sig_handler(int signo); 

/**
 *
 * @fn main
 * @brief main process
 * @details 
 It make thread and initialize for communicatione with vent manager
 It run cyclic process at 200msec for diagnosis system failure
 * @param none
 * @return none
 * @date 2020-09-23
 *
 */
int main()
{
	int i = 0;
	char logdata[MAX_BUF];

	
    PrintLog("SENTINEL COMMUNICATIOM MANAGER Start\n");
	sprintf(logdata, "[Communication] Software Version : %s\n", oSystemVersion.szSoftwareVer);
	PrintLog(logdata);

	printf("\n");
	printf("--------------------------------------------------\n");
	printf("         SENTINEL COMMUNICATIOM MANAGER\n");
	printf("--------------------------------------------------\n");
	printf("Project Name     : %s\n", oSystemVersion.szProjectName);
	printf("Application Name : %s\n", oSystemVersion.szAppName);
	printf("Hardware Version : %s\n", oSystemVersion.szHardwareVer);
	printf("Software Version : %s\n", oSystemVersion.szSoftwareVer);
	printf("Processor        : %s\n", oSystemVersion.szProcessor);
	printf("Build Date       : %s\n", oSystemVersion.szBuildDate);
	printf("Build Time       : %s\n", oSystemVersion.szBuildTime);
	printf("\n");

	radar_status_init();

	signal(SIGINT, (void *)sig_handler);
	signal(SIGTERM, (void *)sig_handler);

    while(1)
    {
		if ( socket_main_task_start() == TRUE )
		{
			if ( socket_main_task_run == TRUE )
			{
				if ( socket_diagnosis_task_start() == TRUE )
				{
					if ( socket_diagnosis_task_run == TRUE )
					{
						;
					}
				}
			}
		}
				
		if ( comm_mcu_connect() == TRUE )
		{
			radar_status_proc();
		}
    }

	socket_diagnosis_task_stop();
	socket_main_task_stop();
	comm_mcu_disconnect();
}


void sig_handler(int signo)
{
	socket_diagnosis_task_stop();
	socket_main_task_stop();
	comm_mcu_disconnect();
	sleep(1);
	raise(SIGKILL);
	//exit(0);
}


