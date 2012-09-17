/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Queue.h
 *  @brief Class definition of Queue
 *  @ingroup State
 */

#ifndef Utils_h
#define Utils_h

#include "fsl_osal.h"

typedef enum {
    QUEUE_SUCCESS,
    QUEUE_FAILURE,
    QUEUE_NOT_READY,
    QUEUE_OVERFLOW,
    QUEUE_INSUFFICIENT_RESOURCES
}QUEUE_ERRORTYPE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    QUEUE_ERRORTYPE CreateQueue(
            fsl_osal_ptr *pQHandle,
            fsl_osal_u32 nMaxQueueSize,
            efsl_osal_bool bBlockingQueue,
            fsl_osal_u32 nMessageSize);

    QUEUE_ERRORTYPE EnQueue(
            fsl_osal_ptr hQHandle, 
            fsl_osal_ptr pMessage, 
            efsl_osal_bool bMaxPriority);

    fsl_osal_u32 GetQueueSize(
            fsl_osal_ptr hQHandle);

    QUEUE_ERRORTYPE ReadQueue(
            fsl_osal_ptr hQHandle, 
            fsl_osal_ptr pMessage, 
            efsl_osal_bool bDeQueue);

    QUEUE_ERRORTYPE DeleteQueue(
            fsl_osal_ptr hQHandle);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
