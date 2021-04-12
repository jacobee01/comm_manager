/**
 *
 * @brief radar control functions
 * @file radar_control.h
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-14
 * @version 1.0.0
 *
 */


#ifndef _RADAR_CONTROL_H
#define _RADAR_CONTROL_H

/* includes */
#include <sys/time.h>

#include "common.h"


/* define */
#define MAX_BUFFER_LENGTH	(1024)


// Radar Status Define
#define RADAR_STATUS_NONE			0x0000
#define RADAR_STATUS_POWER_OFF		0x0001
#define RADAR_STATUS_NORMAL			0x0002
#define RADAR_STATUS_MMIC_UPDATE	0x0004
#define RADAR_STATUS_R_MCU_UPDATE	0x0008
#define RADAR_CONFIG_FAIL			0x0010
#define RADAR_DATA_READ_FAIL		0x0020
#define RADAR_OTA_R_MCU_FAIL		0x0040
#define RADAR_OTA_MMIC_FAIL			0x0080
#define RADAR_R_MCU_BOOTLOADER		0x0100


// Radar Threat Event
#define RADAR_THREAT_INTRUSION		0x01
#define RADAR_THREAT_LOITER			0x02


// OTA Define
#define DEVICE_NONE					0x00
#define DEVICE_MMIC					0x01
#define DEVICE_R_MCU				0x02

#define UPDATE_NONE					0x00
#define UPDATE_READY				0x01
#define UPDATE_READY2				0x02
#define UPDATE_START				0x03
#define UPDATE_SEND					0x04
#define UPDATE_COMPLETE_WAIT		0x05
#define UPDATE_DONE					0x06
#define UPDATE_FILE_ERROR			0x10

#define UPDATE_RESPONSE_NONE		0x00
#define UPDATE_RESPONSE_WAIT		0x01
#define UPDATE_RESPONSE_OK			0x02
#define UPDATE_RESPONSE_FAIL		0x03

#define UPDATE_MMIC_HEAD_SIZE		12
#define UPDATE_MMIC_SEND_SIZE		240
#define UPDATE_MMIC_RESERVED_SIZE	32

#define UPDATE_R_MCU_HEAD_SIZE		12
#define UPDATE_R_MCU_SEND_SIZE		256
#define UPDATE_R_MCU_RESERVED_SIZE	16

// LED Mode
#define LED_TDA_CONTROL_MODE		0x00
#define LED_SOC_CONTROL_MODE		0x01


// R_MCU Status
#define R_MCU_STATUS_BOOTLOADER		0x00000078
#define R_MCU_STATUS_APP_RUNNING	0x00000001


// MMIC Status
#define MMIC_STATUS_UNKNOWN			0x00000000
#define MMIC_STATUS_OK				0x00000001
#define MMIC_STATUS_BOOT_ERROR		0x00000002
//#define MMIC_STATUS_TLV_ERROR		0x00000003


// Radar RunMode Event
#define RADAR_RUNMODE_NORMAL				0x00
#define RADAR_RUNMODE_R_MCU_BOOTLOADER		0x01
#define RADAR_RUNMODE_MMIC_ERROR			0x02


/* typedefs */
#pragma pack(1)

typedef struct
{
	U08 bDimmerEnable;
	U08 bGreen;
	U08 bBlue;
	U08 bRed;
	U16 uOnTime;
	U16 uOffTime;
	U08 bRun;
	U08 bREserved[3];
} LED_STRUCT;


typedef struct
{
	U08 bFirmwareVersion[4];
	U08 bREserved1[4];
	U32 wR_MCUStatus;
	U32 wMMICStatus;
	U08 bREserved2[20];
	U32 wLedMode;
	LED_STRUCT stLedManual;
	U08 bREserved3[72];
} RADAR_CONFIG;


typedef struct
{
	U08 bLEDEvent;
	U08 bBlinkPattern;
	U16 uDutyCycle;
	U08 bRed;
	U08 bGreen;
	U08 bBlue;
} MAIN_LED_CONFIG;


/* global variables */
extern U32 wRadarResponseTimer;
extern U32 wRadarTDATimer;
extern U32 wRadarStatusTimer;
extern TOF tRadatStatusSend;
extern U08 bRadarStatus;
extern U08 bRadarRequest;
extern U08 bRadarResponse;

extern RADAR_CONFIG stRadarConfig;
extern MAIN_LED_CONFIG stMainLEDConfig;


extern U08 bUpdateDevice;
extern U08 bUpdateState;
extern U08 bUpdateResponse;
extern U32 wUpdateTotalFileSize;
extern U08 bUpdateFileName[MAX_BUFFER_LENGTH];

extern U32 wUpdateTotalPartNumber;
extern U32 wUpdatePresentPartNumber;
extern U32 wUpdateSendSize;
extern U08 bUpdateSendData[MAX_BUFFER_LENGTH];


extern U08 bRadarStart;


/* global functions */
extern void radar_status_init(void);
extern void radar_status_proc(void);


#endif /* _RADAR_CONTROL_H */


