/**
 *
 * @brief System Versions Header File
 * @file sys_version.h
 * @details 
 * @author brian.hj.moon@funzin.co.kr
 * @date 2020-09-14
 * @version 1.0.0
 *
 */


#ifndef _SYS_VERSION_H
#define _SYS_VERSION_H

/* includes */
#include "common.h"


/* define */


/* typedefs */
typedef struct
{
	const U08 szProjectName	[16];
	const U08 szAppName		[16];
	const U08 szHardwareVer	[16];
	const U08 szSoftwareVer	[16];
	const U08 szProcessor	[16];
	const U08 szBuildDate	[16];
	const U08 szBuildTime	[16];
} SYSTEM_VERSION;


/* global variables */
extern const SYSTEM_VERSION oSystemVersion;


/* global functions */


#endif /* _SYS_VERSION_H */






