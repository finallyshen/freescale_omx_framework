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

#ifndef Queue_h
#define Queue_h

#include "fsl_osal.h"

typedef enum {
    QUEUE_SUCCESS,
    QUEUE_FAILURE,
    QUEUE_NOT_READY,
    QUEUE_OVERFLOW,
    QUEUE_INSUFFICIENT_RESOURCES
}QUEUE_ERRORTYPE;


typedef struct _QNODE{
    fsl_osal_ptr pMsg;
    struct _QNODE *NextNode;
}QNODE;


class Queue {
    public:
        QUEUE_ERRORTYPE Create(
                fsl_osal_u32 maxQSize, /**< total number of messages */
                fsl_osal_u32 msgSize, /**< each message size */
                efsl_osal_bool block); /**< if true, block call when add or get */
        QUEUE_ERRORTYPE Free();
        /**< Get current msg number in the queue*/
        fsl_osal_u32 Size(); 
        /**< Add a msg to the queue tail */
        QUEUE_ERRORTYPE Add(fsl_osal_ptr pMsg); 
        /**< Get a msg from the queue head, this function will remove the message from queue */
        QUEUE_ERRORTYPE Get(fsl_osal_ptr pMsg); 
        /**< Access the nIndex(start from 1) node message */
        QUEUE_ERRORTYPE Access(fsl_osal_ptr pMsg, fsl_osal_u32 nIndex); 
    private:
        QNODE *pFreeNodes;
        QNODE *pHead;
        QNODE *pTail;
        fsl_osal_ptr pQNodeMem;
        fsl_osal_ptr pQMsgMem;
        fsl_osal_u32 nQSize;
        fsl_osal_u32 nMaxQSize;
        fsl_osal_u32 nMsgSize;
        efsl_osal_bool bBlocking;
        fsl_osal_mutex lock;
        fsl_osal_sem usedNodesSem;
        fsl_osal_sem freeNodesSem;
};

extern "C" {
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
}

#endif
/* File EOF */
