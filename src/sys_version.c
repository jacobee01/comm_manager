/**
 *
 * @brief System Versions Source File
 * @file sourcefile.c
 * @details software and system version information
 * @author brian.hj.moon@funzin.co.kr
 * @date 2020-09-14
 * @version 1.0.0
 *
 */

/* includes */
#include "sys_version.h"


/* define */


/* typedefs */


/* global variables */
const SYSTEM_VERSION oSystemVersion =
{
	"Sentinel",	// Fixed string "$VerSion DaTa$", for searching this data
	"COMMUNICATION",	// Model
	"Ver.01",			// Hardware Version
	"V1.1.0",			// Firmware Version
	"Qualcomm C410",	// Processor
	__DATE__,			// Build Date
	__TIME__,			// Build Time
};


/* local variables */


/* imports */


/* function prototypes */



