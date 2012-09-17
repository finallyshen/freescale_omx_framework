
/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file fsl_osal_linux_condition.c
 *
 * @brief Defines the OS Abstraction layer APIs for condition variable in Linux
 *
 * @ingroup osal
 */

#include <fsl_osal.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include "Log.h"

typedef struct _CONDITION {
    pthread_cond_t cv;
    pthread_mutex_t mp;
}CONDITION;

/*! Creates a condition.
 *
 *	@param [out] cond
 *		Identifier of newly created condition.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if thread creation is failure
 */
efsl_osal_return_type_t fsl_osal_cond_create(fsl_osal_ptr *cond)
{
    fsl_osal_s32 ret = 0;
    CONDITION *pCond = NULL;

    pCond = (CONDITION*)fsl_osal_malloc_new(sizeof(CONDITION));
    if(pCond == NULL) {
        LOG_ERROR("Allocate condition variable failed.\n");
        return E_FSL_OSAL_UNAVAILABLE;
    }

    ret = pthread_cond_init(&(pCond->cv), NULL);
    if(ret != 0) {
        printf("Creat condition variable failed.\n");
        return E_FSL_OSAL_FAILURE;
    }

    ret = pthread_mutex_init(&(pCond->mp), NULL);
    if(ret != 0) {
        printf("Creat mutex for condition variable failed.\n");
        return E_FSL_OSAL_FAILURE;
    }

    *cond = pCond;

    return E_FSL_OSAL_SUCCESS;
}

/*! Destroy a condition.
 *
 *	@param [out] cond
 *		Identifier of newly created condition.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if condition variable destroy is failure
 */
efsl_osal_return_type_t fsl_osal_cond_destroy(fsl_osal_ptr cond)
{
    fsl_osal_s32 ret = 0;
    CONDITION *pCond = NULL;

    pCond = (CONDITION*)cond;
    ret = pthread_mutex_destroy(&pCond->mp);
    if(ret != 0) {
        LOG_ERROR("Destroy mutex for condition variable failed.\n");
        return E_FSL_OSAL_FAILURE;
    }

    ret = pthread_cond_destroy(&pCond->cv);
    if(ret != 0) {
        LOG_ERROR("Destroy condition variable failed.\n");
        return E_FSL_OSAL_FAILURE;
    }

 	fsl_osal_dealloc(pCond);

    return E_FSL_OSAL_SUCCESS;
}

/*! Timed wait a condition.
 *
 *	@param [out] cond
 *		Identifier of newly created condition.
 *	@param [in] time
 *		Wait time in milliseconds.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if timed wait condition is failure
 */
efsl_osal_return_type_t fsl_osal_cond_timedwait(fsl_osal_ptr cond, fsl_osal_timeval *tv)
{
    CONDITION *pCond = NULL;
    struct timespec to;

    pCond = (CONDITION*)cond;
    pthread_mutex_lock(&pCond->mp);

    to.tv_sec = tv->sec;
    to.tv_nsec = tv->usec * 1000;

    pthread_cond_timedwait(&pCond->cv, &pCond->mp, &to);
    pthread_mutex_unlock(&pCond->mp);

    return E_FSL_OSAL_SUCCESS;
}

/*! Broadcase a condition.
 *
 *	@param [out] cond
 *		Identifier of newly created condition.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if broadcast condition is failure
 */
efsl_osal_return_type_t fsl_osal_cond_broadcast(fsl_osal_ptr cond)
{
    CONDITION *pCond = NULL;

    pCond = (CONDITION*)cond;
    pthread_mutex_lock(&pCond->mp);
    pthread_cond_broadcast(&pCond->cv);
    pthread_mutex_unlock(&pCond->mp);

    return E_FSL_OSAL_SUCCESS;
}



