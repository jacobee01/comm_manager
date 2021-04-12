/**
 *
 * @brief variable type redeclaration for misra c and TimeTick function
 * @file common.c
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-10
 * @version 1.0.0
 *
 */


/* includes */
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "common.h"


/* define */
#define LOG_FILE_FOLDER "/data/logs"
#define LOG_FILE_NAME "/data/logs/comm_mcu_"
#define FILE_NAME "comm_mcu"

#define LOG_FILE_COUNT	10

/* typedefs */


/* global variables */


/* local variables */


/* imports */


/* function prototypes */
U32 GetTimeTick10ms(void)
{
	U32 wReturnTick = 0;
	timeval tempTime;
	
	gettimeofday(&tempTime, NULL);
	wReturnTick = tempTime.tv_sec*100 + tempTime.tv_usec/10000;

	return wReturnTick;
}


void PrintLog(U08 *log_data)
{
	int logfd = -1;
	
	time_t now_time;
	struct tm *now_date;
	timeval tempTime;
	char filename[128];
	char logtime[128];
	U08 bLogBuffer[MAX_BUF];
	U16 uLength = 0, i = 0;
	U16 uFileCount = 0;

    struct stat statbuf;

	struct dirent **namelist;
	int count, idx;
	
    if( stat(LOG_FILE_FOLDER, &statbuf) < 0 )
    {
        mkdir(LOG_FILE_FOLDER,0755);
    }
	else
	{
		if( S_ISDIR(statbuf.st_mode) == 0 )
    	{
	        mkdir(LOG_FILE_FOLDER,0755);
    	}
		else
		{
			if((count = scandir(LOG_FILE_FOLDER, &namelist, NULL, alphasort)) == -1){
				//fprintf(stderr,"directory open error\n");
			}

			idx = count - 1;
			while( idx )
			{
				sprintf(filename, "%s/%s",LOG_FILE_FOLDER, namelist[idx--]->d_name);
				if ( strstr(filename, FILE_NAME) != NULL )
				{
					uFileCount++;
					if (uFileCount > 10 )
					{
						remove(filename);
					}
				}
			}

			// 건별 데이터 메모리 해제
			for(idx = 0; idx < count; idx++)
			{
				free(namelist[idx]);
			}
			// namelist에 대한 메모리 해제
			free(namelist);
		}
	}

	time(&now_time);
	now_date = localtime(&now_time);

	gettimeofday(&tempTime, NULL);
	sprintf(bLogBuffer, "[%04d-%02d-%02d %02d:%02d:%02d.%02d] ", now_date->tm_year+1900, now_date->tm_mon+1, now_date->tm_mday, now_date->tm_hour, now_date->tm_min, now_date->tm_sec, tempTime.tv_usec/10000);
	uLength = 25;

	while ( log_data[i] != '\n' )
	{
		bLogBuffer[uLength++] = log_data[i++];
		if ( i >= MAX_BUF ) break;
	}
	bLogBuffer[uLength++] = '\n';

	if( logfd )
	{
		close(logfd);
	}

	sprintf(filename, "%s%04d%02d%02d.log",LOG_FILE_NAME, now_date->tm_year+1900, now_date->tm_mon+1, now_date->tm_mday);
	logfd = open(filename, O_CREAT | O_APPEND | O_RDWR);
	if ( logfd < 0 )
	{
		return;
	}
	
	write(logfd, bLogBuffer, uLength);
//	printf(bLogBuffer);
	
	close(logfd);
}



