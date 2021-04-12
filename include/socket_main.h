/**
 *
 * @brief Ethernet Socket Communication Header File
 * @file socket_main.h
 * @details 
 * @author brian.hj.moon@funzin.co.kr
 * @date 2020-09-14
 * @version 1.0.0
 *
 */

#ifndef SOCKET_MAIN_H_
#define SOCKET_MAIN_H_

#define SOCKET_BUFF_MAX		(4096)	// 2KBytes


/*==============================================*/
/*                 command list                 */
/*==============================================*/

/* recieve command */
#define	_CMD_MAIN_TURN_IRED					0x0401
#define	_CMD_MAIN_RADAR_CONFIG				0x0503
#define	_CMD_MAIN_RADAR_GET_BUFFER			0x050B
#define	_CMD_MAIN_OTA_READY					0x0510
#define	_CMD_MAIN_OTA_START					0x0514
#define	_CMD_MAIN_OTA_R_MCU_FILE			0x0504
#define	_CMD_MAIN_OTA_RADAR_FILE			0x0505
#define	_CMD_MAIN_TURN_LED					0x0506
#define	_CMD_MAIN_LED_CONFIG_BLINK			0x0507
#define	_CMD_MAIN_LED_CONFIG_CYCLE			0x0508
#define	_CMD_MAIN_LED_CONFIG_COLOR			0x0509
#define	_CMD_MAIN_LED_LIVESTREAM			0x05A6

#define	_CMD_MAIN_ARMED_CONFIG				0x0701

#define	_CMD_MAIN_COMM_VERSION				0x0590

#define	_CMD_MAIN_SLEEP_EVENT				0x0530

/* send command */
#define	_CMD_MAIN_RADAR_THREAT				0x0502
#define	_CMD_MAIN_RADAR_SEND_BUFFER			0x050C
#define	_CMD_MAIN_OTA_READY_RES				0x0512
#define	_CMD_MAIN_OTA_UPDATE_RES			0x0518
#define	_CMD_MAIN_MMIC_CLI					0x05A3
#define	_CMD_MAIN_RADAR_RUNMODE				0x0520

#define	_CMD_MAIN_COMM_VERSION_RES			0x0591


/**
 *
 * @brief buffer structure for socket send
 *
 */
typedef  struct
{
	U16 uSendEnbale;
	U16 uLength;
	U08 bSendBuf[SOCKET_BUFF_MAX];
} SOCKET_SEND_BUF;


struct LINGER
{
     int l_onoff;                        // linger 옵션 설정(l_onoff = 1), 해제(l_onoff=0)
     int l_linger;                       // linger timeout
};


extern SOCKET_SEND_BUF sSocketMainSendBuf[5];
extern TOF socket_main_task_run;


extern void socket_main_task_stop(void);
extern int socket_main_task_start(void);
extern void send_put_main(U16 uCommand, U16 uLength, U08 *bData, U08 bParameter);

#endif /* SOCKET_MAIN_H_ */

