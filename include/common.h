/**
 *
 * @brief variable type redeclaration for misra c
 * @file common.h
 * @details none
 * @author Funzin by brian.hj.moon@funzin.co.kr
 * @date 2020-09-10
 * @version 1.0.0
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


typedef unsigned char 	U08;
typedef unsigned short 	U16;
typedef unsigned int 	U32;
typedef float 			FLOAT;
typedef double 			DOUBLE;
typedef bool			TOF;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MAX_BUF			4096

extern U32 GetTimeTick10ms(void);
extern void PrintLog(U08 *log_data);

//#ifndef NULL
//#define NULL 0
//#endif

#endif /* COMMON_H */
