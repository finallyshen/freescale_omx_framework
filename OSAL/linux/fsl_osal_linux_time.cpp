
/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file fsl_osal_linux_time.c
 *
 * @brief Defines the OS Abstraction layer APIs for time in Linux
 *
 * @ingroup osal
 */

#include <sys/time.h>
#include "fsl_osal.h"
#include "Log.h"

/*! Get system time.
 *
 *	@param [out] cond
 *		Identifier of newly created time.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if thread creation is failure
 */
efsl_osal_return_type_t fsl_osal_systime(fsl_osal_timeval *time)
{
    struct timeval tv;

    if(time == NULL)
        return E_FSL_OSAL_FAILURE;

    gettimeofday(&tv, NULL);
    time->sec = tv.tv_sec;
    time->usec = tv.tv_usec;

    return E_FSL_OSAL_SUCCESS;
}

