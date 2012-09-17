/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file Log.cpp
 *
 * @brief log information for debug
 *
 * @ingroup utils
 */


#ifdef ANDROID_BUILD
#include <android/log.h>
#endif
#include "Log.h"

fsl_osal_s32 nLogLevel = LOG_LEVEL_ERROR;
fsl_osal_file pLogFile = NULL;

static void ReadLogLevel(fsl_osal_char *LogLevelFile)
{
	efsl_osal_return_type_t ret;
	fsl_osal_char symbol;
	fsl_osal_s32 ReadLen = 0;
	FILE *pfile = NULL;

	if (LogLevelFile == NULL)
	{
		return;
	}

	ret = fsl_osal_fopen(LogLevelFile, "r", (fsl_osal_file *)&pfile);
	if (ret != E_FSL_OSAL_SUCCESS)
	{
		return;
	}

	ret = fsl_osal_fread(&symbol, 1, pfile, &ReadLen);
	if (ret != E_FSL_OSAL_SUCCESS && ret != E_FSL_OSAL_EOF)
	{
		fsl_osal_fclose(pfile);
		return;
	}

	if (ReadLen < 1)
	{
		fsl_osal_fclose(pfile);
		return;
	}

	nLogLevel = fsl_osal_atoi(&symbol);

	fsl_osal_fclose(pfile);
}

fsl_osal_void LogInit(fsl_osal_s32 nLevel, fsl_osal_char *pFile)
{
	const fsl_osal_char *pDebugLevelEnv = (fsl_osal_char *)"LOG_LEVEL";
	const fsl_osal_char *pLogFileEnv = (fsl_osal_char *)"LOG_FILE";
	fsl_osal_char *cTemp = NULL;

	if (nLevel >= LOG_LEVEL_NONE && nLevel < LOG_LEVEL_COUNT)
		nLogLevel = nLevel;
	else
	{
		cTemp = fsl_osal_getenv_new(pDebugLevelEnv);
		if( (cTemp != NULL) )
		{
			nLogLevel = fsl_osal_atoi(cTemp);
		}
		else
		{
#ifdef ANDROID_BUILD
			fsl_osal_char *LogLevelFile = (fsl_osal_char*)"/data/omx_log_level";
#else
			fsl_osal_char *LogLevelFile = (fsl_osal_char*)"/etc/omx_log_level";
#endif
			ReadLogLevel(LogLevelFile);
		}
	}

	if (nLogLevel < LOG_LEVEL_NONE || nLogLevel >= LOG_LEVEL_COUNT)
	{
		nLogLevel = LOG_LEVEL_NONE;
		printf("Log level is [0, 6]\n");
	}

	if (NULL != pFile)
		cTemp = pFile;
	else
		cTemp = fsl_osal_getenv_new(pLogFileEnv);
	if( (cTemp != NULL) )
	{
		if (NULL != pLogFile)
		{
			fsl_osal_fclose(pLogFile);
			pLogFile = NULL;
		}

		if(fsl_osal_fopen(cTemp, "wb", &pLogFile) != E_FSL_OSAL_SUCCESS)
		{
			printf("Can not open debug log file %s! \n", cTemp);
			return;
		}
	}

	return;
}

fsl_osal_void LogDeInit()
{

	if (NULL != pLogFile)
	{
		fsl_osal_fclose(pLogFile);
		pLogFile = NULL;
	}

	return;
}

#ifdef ANDROID_BUILD

#define LOG_BUF_SIZE 1024

fsl_osal_void LogOutput(const fsl_osal_char *fmt, ...)
{
    va_list ap;
    fsl_osal_char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);

    __android_log_write(ANDROID_LOG_INFO, "OMXPlayer", buf);

    return;
}

#endif
