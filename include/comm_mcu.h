/**
 *
 * @brief communication functions for UART with MCU
 * @file comm_mcu.h
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-10
 * @version 1.0.0
 *
 */


#ifndef _COMM_MCU_H
#define _COMM_MCU_H

/* includes */
#include "common.h"
#include "comm_protocol.h"
#include "comm_protocol_command.h"
#include "comm_uart.h"


/* define */


/* typedefs */


/* global variables */
extern U32 wCommMcuAccessTime;
extern TOF tSleepMode;


/* global functions */
extern TOF comm_mcu_connect(void);
extern void comm_mcu_disconnect(void);


#endif /* _COMM_MCU_H */


