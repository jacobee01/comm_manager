/**
 *
 * @brief Ethernet Socket Communication Header File
 * @file socket_diagnosis.h
 * @details 
 * @author brian.hj.moon@funzin.co.kr
 * @date 2020-09-11
 * @version 1.0.0
 *
 */

#ifndef SOCKET_COMM_H_
#define SOCKET_COMM_H_

#include "socket_main.h"


/*==============================================*/
/*                 command list                 */
/*==============================================*/

/* recieve command */

/* send command */
#define	_CMD_COMM_NORMAL					0x1501
#define	_CMD_COMM_SETUP_FAIL				0x1502
#define	_CMD_COMM_RUN_FAIL					0x1503
#define	_CMD_COMM_OTA_FAIL					0x1504


/* power control from main */
#define	_ACC_CONTROL						0x01
#define	_CAMERA_CONTROL						0x02
#define	_RADAR_CONTROL						0x04


extern SOCKET_SEND_BUF sSocketdiagnosisSendBuf[5];
extern TOF socket_diagnosis_task_run;

extern void socket_diagnosis_task_stop(void);
extern int socket_diagnosis_task_start(void);
void send_put_diagnosis(U16 uCommand, U16 uLength, U08 *bData, U08 bParameter);

#endif /* SOCKET_COMM_H_ */

