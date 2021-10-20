/*****************************************
	Printf() functions for debug support
******************************************/

#ifndef _DEBUG_PRINTF_H_
#define _DEBUG_PRINTF_H_

#include "stdint.h"

int DebugPrintf(const char *format, ...);
// int DebugSprintf(char *out, const char *format, ...);

#endif /* _DEBUG_PRINTF_H_ */
