/**
 *
 * @brief Communication Protocol Command
 * @file comm_protocol_command.h
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-10
 * @version 1.0.0
 *
 */


#ifndef _COMM_PROTOCOL_COMMAND_H
#define _COMM_PROTOCOL_COMMAND_H

/* includes */


/* define */
////////////// Systeom On / Communication Check Commands:	0x0000 ~ 0x00FF ///////////////////////////////////
#define GET_INFO					0x00000020		// REQUEST
#define INFO_RES					0x00000021		// RESPONSE
#define SET_CONFIG					0x00000030		// REQUEST
#define CONFIG_RES					0x00000031		// RESPONSE
#define GET_STATUS					0x00000040		// REQUEST
#define STATUS_RES					0x00000041		// RESPONSE
#define GET_TDA_FRAMEBUFFER			0x00000050		// REQUEST
#define TDA_FRAMEBUFFER				0x00000051		// RESPONSE
#define SEND_INTRUSION				0x00000053		// PUSH
#define SEND_LOITER					0x00000055		// PUSH
#define MMIC_OTA_START				0x00000060		// REQUEST
#define MMIC_OTA_START_RES			0x00000061		// RESPONSE
#define MMIC_OTA_SEND				0x00000062		// REQUEST
#define MMIC_OTA_SEND_RES			0x00000063		// RESPONSE
#define MMIC_OTA_COMPLETE_RES		0x00000065		// PUSH
#define R_MCU_OTA_BOOTLOADER		0x00000078		// REQUEST
#define R_MCU_OTA_BOOTLOADER_RES	0x00000079		// RESPONSE
#define R_MCU_OTA_START				0x00000070		// REQUEST
#define R_MCU_OTA_START_RES			0x00000071		// RESPONSE
#define R_MCU_OTA_SEND				0x00000072		// REQUEST
#define R_MCU_OTA_SEND_RES			0x00000073		// RESPONSE
#define R_MCU_OTA_COMPLETE_RES		0x00000075		// PUSH
#define MMIC_CLI_SEND				0x00000002		// REQUEST
#define MMIC_CLI_RECV_RES			0x00000003		// RESPONSE

/* typedefs */


/* global variables */


/* global functions */


#endif /* _COMM_PROTOCOL_COMMAND_H */


