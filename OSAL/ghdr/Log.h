/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file Log.h
 *
 * @brief log information for debug
 *
 * @ingroup utils
 */

#ifndef Log_h
#define Log_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include "fsl_osal.h"



/**
 * LogLevel:
 * @LOG_LEVEL_NONE: No debugging level specified or desired. Used to deactivate
 *  debugging output.
 * @LOG_LEVEL_ERROR: Error messages are to be used only when an error occured
 *  that stops the application from keeping working correctly.
 * @LOG_LEVEL_WARNING: Warning messages are to inform about abnormal behaviour
 *  that could lead to problems or weird behaviour later on.
 * @LOG_LEVEL_INFO: Informational messages should be used to keep the developer
 *  updated about what is happening.
 *  Examples where this should be used are when a typefind function has
 *  successfully determined the type of the stream or when an mp3 plugin detects
 *  the format to be used. ("This file has mono sound.")
 * @LOG_LEVEL_DEBUG: Debugging messages should be used when something common
 *  happens that is not the expected default behavior.
 *  An example would be notifications about state changes or receiving/sending of
 *  events.
 * @LOG_LEVEL_BUFFER: Debugging messages should be used to trace the buffer
 *  transport between component.
 * @LOG_LEVEL_LOG: Log messages are messages that are very common but might be
 *  useful to know. As a rule of thumb a pipeline that is iterating as expected
 *  should never output anzthing else but LOG messages.
 * @LOG_LEVEL_COUNT: The number of defined debugging levels.
 *
 * The level defines the importance of a debugging message. The more important a
 * message is, the greater the probability that the debugging system outputs it.
 */
typedef enum {
  LOG_LEVEL_NONE = 0,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_INFO,
  LOG_LEVEL_APIINFO,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_BUFFER,
  LOG_LEVEL_LOG,
  /* add more */
  LOG_LEVEL_COUNT
} LogLevel;

extern fsl_osal_s32 nLogLevel;
extern fsl_osal_file pLogFile;

#define START do
#define END while (0)

#ifdef ANDROID_BUILD

#ifdef LOG
#undef LOG
#endif

#define LOG(LEVEL, ...)      START{ \
    if (nLogLevel >= LEVEL) { \
        LogOutput("LEVEL: %d FUNCTION: %s LINE: %d", LEVEL, __FUNCTION__, __LINE__); \
        LogOutput(__VA_ARGS__); \
    } \
}END

#define LOG_INS(LEVEL, ...)      START{ \
    if (nLogLevel >= LEVEL) { \
        LogOutput("LEVEL: %d FUNCTION: %s LINE: %d INSTANCE: %p ", LEVEL, \
                 __FUNCTION__, __LINE__, this); \
        LogOutput(__VA_ARGS__); \
    } \
}END

#define printf LogOutput

fsl_osal_void LogOutput(const fsl_osal_char *fmt, ...);

#else

#define LOG(LEVEL, ...)      START{ \
	if (nLogLevel >= LEVEL) { \
		if (NULL != pLogFile) { \
			fprintf((FILE *)pLogFile, "LEVEL: %d FILE: %s FUNCTION: %s LINE: %d ", LEVEL, \
					__FILE__, __FUNCTION__, __LINE__); \
			fprintf((FILE *)pLogFile, __VA_ARGS__); \
			fflush((FILE *)pLogFile); \
		} \
		else { \
			fprintf(stdout, "LEVEL: %d FILE: %s FUNCTION: %s LINE: %d ", LEVEL, __FILE__, \
					__FUNCTION__, __LINE__); \
			fprintf(stdout, __VA_ARGS__); \
			fflush(stdout); \
		} \
	} \
}END

#define LOG_INS(LEVEL, ...)      START{ \
	if (nLogLevel >= LEVEL) { \
		if (NULL != pLogFile) { \
			fprintf((FILE *)pLogFile, "LEVEL: %d FILE: %s FUNCTION: %s LINE: %d INSTANCE: %p ", LEVEL, __FILE__, __FUNCTION__, __LINE__, this); \
			fprintf((FILE *)pLogFile, __VA_ARGS__); \
			fflush((FILE *)pLogFile); \
		} \
		else { \
			fprintf(stdout, "LEVEL: %d FILE: %s FUNCTION: %s LINE: %d INSTANCE: %p ", LEVEL, \
				__FILE__, __FUNCTION__, __LINE__, this); \
			fprintf(stdout, __VA_ARGS__); \
			fflush(stdout); \
		} \
	} \
}END

#endif
#define LOG_ERROR(...)   LOG(LOG_LEVEL_ERROR, __VA_ARGS__) 
#define LOG_WARNING(...) LOG(LOG_LEVEL_WARNING, __VA_ARGS__) 
#define LOG_INFO(...)    LOG(LOG_LEVEL_INFO, __VA_ARGS__) 
#define LOG_DEBUG(...)   LOG(LOG_LEVEL_DEBUG, __VA_ARGS__) 
#define LOG_BUFFER(...)  LOG(LOG_LEVEL_BUFFER, __VA_ARGS__) 
#define LOG_LOG(...)     LOG(LOG_LEVEL_LOG, __VA_ARGS__) 

#define LOG_ERROR_INS(...)   LOG_INS(LOG_LEVEL_ERROR, __VA_ARGS__) 
#define LOG_WARNING_INS(...) LOG_INS(LOG_LEVEL_WARNING, __VA_ARGS__) 
#define LOG_INFO_INS(...)    LOG_INS(LOG_LEVEL_INFO, __VA_ARGS__) 
#define LOG_DEBUG_INS(...)   LOG_INS(LOG_LEVEL_DEBUG, __VA_ARGS__) 
#define LOG_BUFFER_INS(...)  LOG_INS(LOG_LEVEL_BUFFER, __VA_ARGS__) 
#define LOG_LOG_INS(...)     LOG_INS(LOG_LEVEL_LOG, __VA_ARGS__) 

fsl_osal_void LogInit(fsl_osal_s32 nLevel, fsl_osal_char *pFile);
fsl_osal_void LogDeInit();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
