/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Queue.h"
#include "Mem.h"
#include "Log.h"

QUEUE_ERRORTYPE Queue::Create(
        fsl_osal_u32 maxQSize, 
        fsl_osal_u32 msgSize, 
        efsl_osal_bool block)
{
    QUEUE_ERRORTYPE ret = QUEUE_SUCCESS;
    fsl_osal_u32 i, aligned_msg_size;
    fsl_osal_ptr ptr = NULL;
    QNODE *pQNode = NULL;
    fsl_osal_u8 *pMsg = NULL;

    pQNodeMem = pQMsgMem = NULL;
    pFreeNodes = pHead = pTail = NULL;
    nQSize = 0;
    lock = usedNodesSem = freeNodesSem = NULL;

    nMaxQSize = maxQSize;
    nMsgSize = msgSize;
    bBlocking = block;

    /* allocate for queue nodes */
    ptr = FSL_MALLOC(sizeof(QNODE) * nMaxQSize);
    if(ptr == NULL) {
        LOG_ERROR("Failed to allocate memory for queue node.\n");
        ret = QUEUE_INSUFFICIENT_RESOURCES;
        goto err;
    }
    pQNodeMem = ptr;
    pQNode = (QNODE*)ptr;

    /* alloate for node messages */
    aligned_msg_size = (nMsgSize + 3)/4*4;
    ptr = FSL_MALLOC(nMaxQSize * aligned_msg_size);
    if(ptr == NULL) {
        LOG_ERROR("Failed to allocate memory for queue node message.\n");
        ret = QUEUE_INSUFFICIENT_RESOURCES;
        goto err;
    }
    pQMsgMem = ptr;
    pMsg = (fsl_osal_u8*)ptr;

    for(i=0; i<nMaxQSize-1; i++) {
        pQNode[i].NextNode = &(pQNode[i+1]);
        pQNode[i].pMsg = pMsg;
        pMsg += aligned_msg_size;
    }
    pQNode[nMaxQSize-1].NextNode = NULL;
    pQNode[nMaxQSize-1].pMsg = pMsg;
    pFreeNodes = pQNode;

    if(fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal) != E_FSL_OSAL_SUCCESS) {
        LOG_ERROR("Failed to create mutex for queue.\n");
        ret = QUEUE_INSUFFICIENT_RESOURCES;
        goto err;
    }

    if(fsl_osal_sem_init(&usedNodesSem, 0, 0) != E_FSL_OSAL_SUCCESS) {
        LOG_ERROR("Failed to create mutex for used nodes.\n");
        ret = QUEUE_INSUFFICIENT_RESOURCES;
        goto err;
    }

    if(fsl_osal_sem_init(&freeNodesSem, 0, nMaxQSize) != E_FSL_OSAL_SUCCESS) {
        LOG_ERROR("Failed to create mutex for free nodes.\n");
        ret = QUEUE_INSUFFICIENT_RESOURCES;
        goto err;
    }

    return ret;

err:
    Free();
    return ret;
}

QUEUE_ERRORTYPE Queue::Free() 
{
    if(pQNodeMem != NULL)
        FSL_FREE(pQNodeMem);
    if(pQMsgMem != NULL)
        FSL_FREE(pQMsgMem);
    if(lock != NULL)
        fsl_osal_mutex_destroy(lock);
    if(usedNodesSem != NULL)
        fsl_osal_sem_destroy(usedNodesSem);
    if(freeNodesSem != NULL)
        fsl_osal_sem_destroy(freeNodesSem);

    return QUEUE_SUCCESS;
}

fsl_osal_u32 Queue::Size() 
{
    fsl_osal_u32 ret = 0;

    fsl_osal_mutex_lock(lock);
    ret = nQSize;
    fsl_osal_mutex_unlock(lock);

    return ret;
}

QUEUE_ERRORTYPE Queue::Add(fsl_osal_ptr pMsg) 
{
    QNODE *pQNode = NULL;

    if(bBlocking == E_FSL_OSAL_TRUE)
        fsl_osal_sem_wait(freeNodesSem);
    else {
        if(fsl_osal_sem_trywait(freeNodesSem) != E_FSL_OSAL_SUCCESS)
            return QUEUE_NOT_READY;
    }

    fsl_osal_mutex_lock(lock);

    if(pFreeNodes == NULL) {
        fsl_osal_mutex_unlock(lock);
        return QUEUE_OVERFLOW;
    }

    pQNode = pFreeNodes;
    pFreeNodes = pFreeNodes->NextNode;
    pQNode->NextNode = NULL;
    fsl_osal_memcpy(pQNode->pMsg, pMsg, nMsgSize);
    if(pHead == NULL && pTail == NULL) {
        pHead = pQNode;
        pTail = pQNode;
    }
    else {
        pTail->NextNode = pQNode;
        pTail = pQNode;
    }
    nQSize ++;
    fsl_osal_sem_post(usedNodesSem);
    fsl_osal_mutex_unlock(lock);

    return QUEUE_SUCCESS;
}

QUEUE_ERRORTYPE Queue::Get(fsl_osal_ptr pMsg) 
{
    QNODE *pQNode = NULL;

    if(bBlocking == E_FSL_OSAL_TRUE)
        fsl_osal_sem_wait(usedNodesSem);
    else {
        if(fsl_osal_sem_trywait(usedNodesSem) != E_FSL_OSAL_SUCCESS)
            return QUEUE_NOT_READY;
    }

    fsl_osal_mutex_lock(lock);

    if(pHead == NULL) {
        fsl_osal_mutex_unlock(lock);
        return QUEUE_OVERFLOW;
    }

    pQNode = pHead;
    pHead = pHead->NextNode;
    fsl_osal_memcpy(pMsg, pQNode->pMsg, nMsgSize);
    pQNode->NextNode = pFreeNodes;
    pFreeNodes = pQNode;
    nQSize --;
    if(pHead == NULL)
        pTail = NULL;

    fsl_osal_sem_post(freeNodesSem);
    fsl_osal_mutex_unlock(lock);

    return QUEUE_SUCCESS;
}

QUEUE_ERRORTYPE Queue::Access(fsl_osal_ptr pMsg, fsl_osal_u32 nIndex)
{
    QNODE *pQNode = NULL;
    fsl_osal_u32 i;

    if(nIndex > nQSize) {
        fsl_osal_memset(pMsg, 0, nMsgSize);
        return QUEUE_FAILURE;
    }

    pQNode = pHead;

    for(i=1; i<nIndex; i++)
        pQNode = pQNode->NextNode;

    fsl_osal_memcpy(pMsg, pQNode->pMsg, nMsgSize);

    return QUEUE_SUCCESS;
}

/**< C type functions */

QUEUE_ERRORTYPE CreateQueue(
        fsl_osal_ptr *pQHandle,
        fsl_osal_u32 nMaxQueueSize,
        efsl_osal_bool bBlockingQueue,
        fsl_osal_u32 nMessageSize)
{
    Queue *pQueue = NULL; 

    pQueue = FSL_NEW(Queue, ());
    if(pQueue == NULL)
        return QUEUE_INSUFFICIENT_RESOURCES;
    pQueue->Create(nMaxQueueSize, nMessageSize, bBlockingQueue);
    *pQHandle = pQueue;

    return QUEUE_SUCCESS;
}

QUEUE_ERRORTYPE EnQueue(
        fsl_osal_ptr hQHandle, 
        fsl_osal_ptr pMessage, 
        efsl_osal_bool bMaxPriority)
{
    Queue *pQueue = NULL; 

    pQueue = (Queue*) hQHandle;
    pQueue->Add(pMessage);

    return QUEUE_SUCCESS;
}

fsl_osal_u32 GetQueueSize(
        fsl_osal_ptr hQHandle)
{
    Queue *pQueue = NULL; 

    pQueue = (Queue*) hQHandle;
    return pQueue->Size();
}

QUEUE_ERRORTYPE ReadQueue(
        fsl_osal_ptr hQHandle, 
        fsl_osal_ptr pMessage, 
        efsl_osal_bool bDeQueue)
{
    Queue *pQueue = NULL; 

    pQueue = (Queue*) hQHandle;
    if(bDeQueue == E_FSL_OSAL_TRUE)
        pQueue->Get(pMessage);
    else
        pQueue->Access(pMessage, 1);

    return QUEUE_SUCCESS;
}

QUEUE_ERRORTYPE DeleteQueue(
        fsl_osal_ptr hQHandle)
{
    Queue *pQueue = NULL; 

    pQueue = (Queue*) hQHandle;
    pQueue->Free();
    FSL_DELETE(pQueue);

    return QUEUE_SUCCESS;
}

/* File EOF */
