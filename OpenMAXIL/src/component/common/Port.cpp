/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Port.h"

#define FLASH_WORKAROUND
#ifdef FLASH_WORKAROUND
#define PRE_SIZE 8

#ifdef FSL_MALLOC
#undef FSL_MALLOC
#endif
#define FSL_MALLOC(size) \
    ({\
     fsl_osal_ptr __buf = NULL; \
     __buf = fsl_osal_malloc_new(size+PRE_SIZE); \
     __buf = (fsl_osal_ptr)((fsl_osal_u32)__buf + PRE_SIZE); \
     __buf; \
     })

#ifdef FSL_FREE
#undef FSL_FREE
#endif
#define FSL_FREE(ptr) \
    do { \
        fsl_osal_ptr __mem = ptr; \
        if(__mem) { \
            __mem = (fsl_osal_ptr)((fsl_osal_u32)__mem - PRE_SIZE); \
            fsl_osal_dealloc(__mem); \
            ptr=NULL; \
        }\
    }while(0);
#endif

static OMX_BOOL IntersectBufferSupplier(
        OMX_BUFFERSUPPLIERTYPE Supplier1,
        OMX_BUFFERSUPPLIERTYPE Supplier2,
        OMX_BUFFERSUPPLIERTYPE *pSupplierResult)
{
    if(Supplier1 == OMX_BufferSupplyUnspecified) {
        if(Supplier2 == OMX_BufferSupplyUnspecified)
            *pSupplierResult = OMX_BufferSupplyOutput;
    }
    else {
        if(Supplier2 == Supplier1
                ||Supplier2 == OMX_BufferSupplyUnspecified)
            *pSupplierResult = Supplier1;
        else
            return OMX_FALSE;
    }

    return OMX_TRUE;
}

static OMX_BOOL IntersectPortDef(
        OMX_PARAM_PORTDEFINITIONTYPE *pDef1,
        OMX_PARAM_PORTDEFINITIONTYPE *pDef2)
{
    OMX_U32 nBufferCount, nBufferSize, nBufferAlignment;

    if(pDef1->eDomain != pDef2->eDomain)
        return OMX_FALSE;
    if(pDef1->eDomain == OMX_PortDomainAudio)
        if(pDef1->format.audio.eEncoding != pDef2->format.audio.eEncoding)
            return OMX_FALSE;
    if(pDef1->eDomain == OMX_PortDomainVideo)
        if(pDef1->format.video.eCompressionFormat != pDef2->format.video.eCompressionFormat)
            return OMX_FALSE;
    if(pDef1->eDomain == OMX_PortDomainImage)
        if(pDef1->format.image.eCompressionFormat != pDef2->format.image.eCompressionFormat)
            return OMX_FALSE;

    nBufferCount = MAX(pDef1->nBufferCountActual, pDef2->nBufferCountActual);
    pDef1->nBufferCountActual = pDef2->nBufferCountActual = nBufferCount;
    nBufferSize = MAX(pDef1->nBufferSize, pDef2->nBufferSize);
    pDef1->nBufferSize = pDef2->nBufferSize = nBufferSize;
    nBufferAlignment = MAX(pDef1->nBufferAlignment, pDef2->nBufferAlignment);
    pDef1->nBufferAlignment = pDef2->nBufferAlignment = nBufferAlignment;

    return OMX_TRUE;
}

Port::Port(ComponentBase *pBase, OMX_U32 nPortIdx)
{
    base = pBase;
    nPortIndex = nPortIdx;
    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    pPortQueue = NULL;
    nBufferHdrNum = 0;
    nMarkDataNum = 0;
    nGetMarkData = 0;
    nAddMarkData = 0;
    bTunneled = OMX_FALSE;
    bNeedReturnBuffers = OMX_FALSE;
    eBufferSupplier = OMX_BufferSupplyUnspecified;
    bPendingDisable = OMX_FALSE;
}

OMX_ERRORTYPE Port::Init()
{
    /**< setup Port queue */
    pPortQueue = FSL_NEW(Queue, ());
    if(pPortQueue == NULL) {
        LOG_ERROR("New port queue for port[%d] failed.\n", nPortIndex);
        return OMX_ErrorInsufficientResources;
    }

    if(pPortQueue->Create(MAX_PORT_BUFFER, sizeof(OMX_BUFFERHEADERTYPE*), E_FSL_OSAL_TRUE)
            != QUEUE_SUCCESS) {
        LOG_ERROR("Init port queue failed.\n");
        DeInit();
        return OMX_ErrorUndefined;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::DeInit()
{
    /**< Free Port queue */
    pPortQueue->Free();
    FSL_DELETE(pPortQueue);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::SetPortDefinition(
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDef) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(pPortDef == NULL)
        return OMX_ErrorBadParameter;

    OMX_CHECK_STRUCT(pPortDef, OMX_PARAM_PORTDEFINITIONTYPE, ret);
    if(ret != OMX_ErrorNone)
        return ret;

    if(pPortDef->nPortIndex != nPortIndex)
        return OMX_ErrorBadPortIndex;

    fsl_osal_memcpy(&sPortDef, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

    return ret;
}

OMX_ERRORTYPE Port::GetPortDefinition(
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDef)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(pPortDef == NULL)
        return OMX_ErrorBadParameter;
    
    OMX_CHECK_STRUCT(pPortDef, OMX_PARAM_PORTDEFINITIONTYPE, ret);
    if(ret != OMX_ErrorNone)
        return ret;

    if(pPortDef->nPortIndex != nPortIndex)
        return OMX_ErrorBadPortIndex;

    fsl_osal_memcpy(pPortDef, &sPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::UseBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_PTR pAppPrivate,
        OMX_U32 nSizeBytes, 
        OMX_U8* pBuffer) 
{
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;

    if(ppBufferHdr == NULL || nSizeBytes < sPortDef.nBufferSize )
        return OMX_ErrorBadParameter;

    if(sPortDef.bPopulated == OMX_TRUE)
        return OMX_ErrorIncorrectStateOperation;

    if(nBufferHdrNum >= sPortDef.nBufferCountActual)
        return OMX_ErrorOverflow;

    if(bTunneled == OMX_TRUE && bSupplier == OMX_TRUE) {
        LOG_ERROR("Supplier shall not use a buffer.\n");
        return OMX_ErrorUndefined;
    }

    if(bTunneled != OMX_TRUE) {
        pBufferHdr = (OMX_BUFFERHEADERTYPE*)FSL_MALLOC(sizeof(OMX_BUFFERHEADERTYPE));
        if(pBufferHdr == NULL) {
            LOG_ERROR("Failed to allocate buffer header for port #%d.\n", nPortIndex);
            base->SendEvent(OMX_EventError, OMX_ErrorInsufficientResources, 0, NULL);
            return OMX_ErrorInsufficientResources;
        }
        fsl_osal_memset(pBufferHdr, 0, sizeof(OMX_BUFFERHEADERTYPE));
        OMX_INIT_STRUCT(pBufferHdr, OMX_BUFFERHEADERTYPE);
        pBufferHdr->pBuffer = pBuffer;
        pBufferHdr->nAllocLen = nSizeBytes;
        pBufferHdr->pAppPrivate = pAppPrivate;
        *ppBufferHdr = pBufferHdr;
    }
    else
        pBufferHdr = *ppBufferHdr;

    bAllocater = OMX_FALSE;
    BufferHdrArry[nBufferHdrNum] = pBufferHdr;
    if(sPortDef.eDir == OMX_DirInput)
        pBufferHdr->nInputPortIndex = nPortIndex;
    if(sPortDef.eDir == OMX_DirOutput)
        pBufferHdr->nOutputPortIndex = nPortIndex;

    nBufferHdrNum ++;

    if(nBufferHdrNum >= sPortDef.nBufferCountActual) {
        sPortDef.bEnabled = OMX_TRUE;
        sPortDef.bPopulated = OMX_TRUE;
        base->PortNotify(nPortIndex,PORT_ON);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::UseEGLImage(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_PTR pAppPrivate, 
        void* eglImage)
{
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;

    if(ppBufferHdr == NULL)
        return OMX_ErrorBadParameter;

    if(sPortDef.bPopulated == OMX_TRUE)
        return OMX_ErrorIncorrectStateOperation;

    if(nBufferHdrNum >= sPortDef.nBufferCountActual)
        return OMX_ErrorOverflow;

    if(bTunneled == OMX_TRUE) {
        LOG_ERROR("EglImage shall not be used for tunneled port.\n");
        return OMX_ErrorUndefined;
    }

    pBufferHdr = (OMX_BUFFERHEADERTYPE*)FSL_MALLOC(sizeof(OMX_BUFFERHEADERTYPE));
    if(pBufferHdr == NULL) {
        LOG_ERROR("Failed to allocate buffer header for port #%d.\n", nPortIndex);
        base->SendEvent(OMX_EventError, OMX_ErrorInsufficientResources, 0, NULL);
        return OMX_ErrorInsufficientResources;
    }
    fsl_osal_memset(pBufferHdr, 0, sizeof(OMX_BUFFERHEADERTYPE));
    OMX_INIT_STRUCT(pBufferHdr, OMX_BUFFERHEADERTYPE);
    pBufferHdr->pAppPrivate = pAppPrivate;
    pBufferHdr->pPlatformPrivate = eglImage;
    *ppBufferHdr = pBufferHdr;

    bAllocater = OMX_FALSE;
    BufferHdrArry[nBufferHdrNum] = pBufferHdr;
    if(sPortDef.eDir == OMX_DirInput)
        pBufferHdr->nInputPortIndex = nPortIndex;
    if(sPortDef.eDir == OMX_DirOutput) 
        pBufferHdr->nOutputPortIndex = nPortIndex;

    nBufferHdrNum ++;

    if(nBufferHdrNum >= sPortDef.nBufferCountActual) {
        sPortDef.bEnabled = OMX_TRUE;
        sPortDef.bPopulated = OMX_TRUE;
        base->PortNotify(nPortIndex,PORT_ON);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::AllocateBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_PTR pAppPrivate,
        OMX_U32 nSizeBytes) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    OMX_PTR pBuffer = NULL;

    LOG_DEBUG("Port AllocateBuffer.\n");

    if(ppBufferHdr == NULL || nSizeBytes < sPortDef.nBufferSize )
        return OMX_ErrorBadParameter;

    if(sPortDef.bPopulated == OMX_TRUE)
        return OMX_ErrorIncorrectStateOperation;

    if(nBufferHdrNum >= sPortDef.nBufferCountActual)
        return OMX_ErrorOverflow;

    pBufferHdr = (OMX_BUFFERHEADERTYPE*)FSL_MALLOC(sizeof(OMX_BUFFERHEADERTYPE));
    if(pBufferHdr == NULL) {
        LOG_ERROR("Failed to allocate buffer header for port #%d.\n", nPortIndex);
        base->SendEvent(OMX_EventError, OMX_ErrorInsufficientResources, 0, NULL);
        return OMX_ErrorInsufficientResources;
    }

    ret = base->DoAllocateBuffer(&pBuffer, nSizeBytes,nPortIndex);
    if(ret != OMX_ErrorNone) {
        FSL_FREE(pBufferHdr);
        base->SendEvent(OMX_EventError, ret, 0, NULL);
        LOG_ERROR("Failed to allocate buffer resource for port #%d\n", nPortIndex);
        return ret;
    }

    fsl_osal_memset(pBufferHdr, 0, sizeof(OMX_BUFFERHEADERTYPE));
    OMX_INIT_STRUCT(pBufferHdr, OMX_BUFFERHEADERTYPE);
    BufferHdrArry[nBufferHdrNum] = pBufferHdr;
    nBufferHdrNum ++;

    bAllocater = OMX_TRUE;
    pBufferHdr->pBuffer = (OMX_U8*)pBuffer;
    pBufferHdr->nAllocLen = nSizeBytes;
    pBufferHdr->pAppPrivate = pAppPrivate;
    if(sPortDef.eDir == OMX_DirInput)
        pBufferHdr->nInputPortIndex = nPortIndex;
    if(sPortDef.eDir == OMX_DirOutput)
        pBufferHdr->nOutputPortIndex = nPortIndex;

    *ppBufferHdr = pBufferHdr;

    LOG_DEBUG("Allocated: %d, required: %d\n", nBufferHdrNum, sPortDef.nBufferCountActual);

    if(nBufferHdrNum >= sPortDef.nBufferCountActual) {
        sPortDef.bEnabled = OMX_TRUE;
        sPortDef.bPopulated = OMX_TRUE;
        base->PortNotify(nPortIndex,PORT_ON);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::FreeBuffer(
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_BOOL bMatch = OMX_FALSE;
    OMX_U32 nMatchIdx, i;

    LOG_DEBUG("%s Port %d FreeBuffer.\n", base->name, nPortIndex);

    if(pBufferHdr == NULL)
        return OMX_ErrorBadParameter;

    if(IsAllBuffersReturned() != OMX_TRUE) {
        LOG_DEBUG("Not all buffer returned when freebuffer of Port[%d], return now.\n", nPortIndex);
        ReturnBuffers();
    }

    for(i=0; i<sPortDef.nBufferCountActual; i++) {
        if(pBufferHdr == BufferHdrArry[i]) {
            bMatch = OMX_TRUE;
            nMatchIdx = i;
            break;
        }
    }

    if(bMatch != OMX_TRUE) {
        LOG_ERROR("Buffer header [%p] not belongs to component when Free.\n", pBufferHdr);
        return OMX_ErrorBadParameter;
    }

    if(bAllocater == OMX_TRUE) {
        ret = base->DoFreeBuffer(pBufferHdr->pBuffer,nPortIndex);
        if(ret != OMX_ErrorNone) {
            LOG_ERROR("Failed to free buffer resource.\n");
        }
        pBufferHdr->pBuffer = NULL;
    }

    if(bTunneled != OMX_TRUE || (bTunneled == OMX_TRUE && bSupplier == OMX_TRUE))
        FSL_FREE(pBufferHdr);

    BufferHdrArry[nMatchIdx] = NULL;
    nBufferHdrNum --;

    LOG_DEBUG("Remained buffer cnt: %d\n", nBufferHdrNum);

    if(nBufferHdrNum == 0) {
        sPortDef.bEnabled = OMX_FALSE;
        sPortDef.bPopulated = OMX_FALSE;
        base->PortNotify(nPortIndex,PORT_OFF);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::TunnelRequest(
        OMX_HANDLETYPE hTunneledComp, 
        OMX_U32 nTunneledPort,
        OMX_TUNNELSETUPTYPE* pTunnelSetup) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(pTunnelSetup == NULL)
        return OMX_ErrorBadParameter;

    if(hTunneledComp == NULL) {
        bTunneled = OMX_FALSE;
        sTunnelInfo.hTunneledComp = NULL;
        return OMX_ErrorNone;
    }

    if(sPortDef.eDir == OMX_DirOutput) {
        bTunneled = OMX_TRUE;
        sTunnelInfo.hTunneledComp = hTunneledComp;
        sTunnelInfo.nTunneledPort = nTunneledPort;
        pTunnelSetup->eSupplier = eBufferSupplier;
    }
    else {
        OMX_BUFFERSUPPLIERTYPE sSupplier = OMX_BufferSupplyUnspecified;
        OMX_PARAM_PORTDEFINITIONTYPE sPortOutDef;

        if(OMX_TRUE != IntersectBufferSupplier(pTunnelSetup->eSupplier, eBufferSupplier, &sSupplier))
            return OMX_ErrorPortsNotCompatible;

        if(sSupplier != pTunnelSetup->eSupplier) {
            OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplierType;
            OMX_INIT_STRUCT(&sBufferSupplierType, OMX_PARAM_BUFFERSUPPLIERTYPE);
            sBufferSupplierType.nPortIndex = nTunneledPort;
            sBufferSupplierType.eBufferSupplier = sSupplier;
            ret = OMX_SetParameter(hTunneledComp, OMX_IndexParamCompBufferSupplier, &sBufferSupplierType);
            if(ret != OMX_ErrorNone)
                return ret;
        }

        SetSupplierType(sSupplier);
        pTunnelSetup->eSupplier = sSupplier;

        OMX_INIT_STRUCT(&sPortOutDef, OMX_PARAM_PORTDEFINITIONTYPE);
        sPortOutDef.nPortIndex = nTunneledPort;
        OMX_GetParameter(hTunneledComp, OMX_IndexParamPortDefinition, &sPortOutDef);
        if(OMX_TRUE != IntersectPortDef(&sPortOutDef, &sPortDef))
            return OMX_ErrorPortsNotCompatible;

        ret = OMX_SetParameter(hTunneledComp, OMX_IndexParamPortDefinition, &sPortOutDef);
        if(ret != OMX_ErrorNone)
            return ret;

        bTunneled = OMX_TRUE;
        sTunnelInfo.hTunneledComp = hTunneledComp;
        sTunnelInfo.nTunneledPort = nTunneledPort;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::SupplierAllocateBuffers()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    OMX_U32 i;

    for(i=0; i<sPortDef.nBufferCountActual ; i++)
        AllocateBuffer(&pBufferHdr,NULL,sPortDef.nBufferSize);

    for(i=0; i<sPortDef.nBufferCountActual ; i++) {
        //TODO: UseBuffer may return NotReady as Tunneded componnet not received command
        OMX_UseBuffer(sTunnelInfo.hTunneledComp, &BufferHdrArry[i], 
                sTunnelInfo.nTunneledPort, NULL, sPortDef.nBufferSize, BufferHdrArry[i]->pBuffer);
    }

    /* supplier add all allocated buffers to its port queue */
    for(i=0; i<sPortDef.nBufferCountActual ; i++)
        AddBuffer(BufferHdrArry[i]);

    return ret;
}

OMX_ERRORTYPE Port::SupplierFreeBuffers()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    OMX_U32 i;

    LOG_DEBUG("%s Supplier port %d free buffer\n", base->name, nPortIndex);

        for(i=0; i<nBufferHdrNum; i++)
        //TODO: FreeBuffer may return NotReady as Tunneded componnet not received command
        OMX_FreeBuffer(sTunnelInfo.hTunneledComp, sTunnelInfo.nTunneledPort, BufferHdrArry[i]);

    while(BufferNum() > 0)
        GetBuffer(&pBufferHdr);

    for(i=0; nBufferHdrNum>0 && i<sPortDef.nBufferCountActual ; i++)
        FreeBuffer(BufferHdrArry[i]);

    return ret;
}

OMX_ERRORTYPE Port::Disable() 
{
    OMX_STATETYPE eState = OMX_StateInvalid;

    LOG_DEBUG("%s disable port %d\n", base->name, nPortIndex);

    if(sPortDef.bEnabled == OMX_FALSE)
        return OMX_ErrorSameState;

    base->GetState(&eState);
    if(eState == OMX_StateLoaded) {
        sPortDef.bEnabled = OMX_FALSE;
        return OMX_ErrorNone;
    }

    if(bTunneled == OMX_FALSE) {
        sPortDef.bEnabled = OMX_FALSE;
        ReturnBuffers();
    }
    else {
        if(bSupplier == OMX_TRUE) {
            bNeedReturnBuffers = OMX_TRUE;
            OMX_SendCommand(sTunnelInfo.hTunneledComp, OMX_CommandPortDisable, sTunnelInfo.nTunneledPort, NULL);
            while(BufferNum() < nBufferHdrNum) {
                LOG_DEBUG("%s wait for buffer return to supplier port %d.\n", base->name, nPortIndex);
                fsl_osal_sleep(10000);
            }
            bNeedReturnBuffers = OMX_FALSE;
            SupplierFreeBuffers();
        }
        else
            ReturnBuffers();
    }

    LOG_DEBUG("%s disable port %d done\n", base->name, nPortIndex);

    return OMX_ErrorNotComplete;
}

OMX_ERRORTYPE Port::Enable() 
{
    OMX_STATETYPE eState = OMX_StateInvalid;

    if(sPortDef.bEnabled == OMX_TRUE)
        return OMX_ErrorSameState;

    base->GetState(&eState);
    if(eState == OMX_StateLoaded) {
        sPortDef.bEnabled = OMX_TRUE;
        return OMX_ErrorNone;
    }

    if(bTunneled == OMX_TRUE && bSupplier == OMX_TRUE) {
        OMX_SendCommand(sTunnelInfo.hTunneledComp, OMX_CommandPortEnable, sTunnelInfo.nTunneledPort, NULL);
        SupplierAllocateBuffers();
    }

    return OMX_ErrorNotComplete;
}

OMX_ERRORTYPE Port::Flush() 
{
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    OMX_U32 nBuffer = 0;
    OMX_U32 i;

    base->FlushComponent(nPortIndex);
    nBuffer = BufferNum();
    LOG_DEBUG("%s flush port %d return buffer: %d\n", base->name, nPortIndex, nBuffer);
    for(i=0; i<nBuffer; i++) {
        GetBuffer(&pBufferHdr);
        SendBuffer(pBufferHdr);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::ReturnBuffers() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("%s port %d return buffers.\n", base->name, nPortIndex);

    base->ComponentReturnBuffer(nPortIndex);

    if(bTunneled != OMX_TRUE ||
            (bTunneled == OMX_TRUE && bSupplier != OMX_TRUE)) {
        while(BufferNum() > 0) {
            OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
            GetBuffer(&pBufferHdr);
            SendBuffer(pBufferHdr);
        }
    }

    LOG_DEBUG("%s port %d return buffers done.\n", base->name, nPortIndex);

    return ret;
}

// in case that someone wants to deal with mark data without calling SendBuffer()
OMX_ERRORTYPE Port::SendEventMark(
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    if (nMarkDataNum > 0)
    {
        OMX_MARKTYPE *pMarkData = NULL;
        pMarkData = &MarkDatas[nGetMarkData];

        if(pMarkData->hMarkTargetComponent == base->GetComponentHandle()){
            LOG_DEBUG("%s,%d,component %s send mark event for target %x.\n",__FUNCTION__,__LINE__,base->name,pMarkData->hMarkTargetComponent);
            base->SendEvent(OMX_EventMark, 0, 0, pMarkData);
            pBufferHdr->hMarkTargetComponent = NULL;
            pMarkData->hMarkTargetComponent = NULL;
        }
         
    }

    return OMX_ErrorNone;

}

OMX_ERRORTYPE Port::SendBuffer(
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    if(pBufferHdr == NULL)
        return OMX_ErrorBadParameter;

    if (nMarkDataNum > 0)
    {
        OMX_MARKTYPE *pMarkData = NULL;
        pMarkData = &MarkDatas[nGetMarkData];
        nGetMarkData ++;
        nMarkDataNum --;
        if(nGetMarkData >= MAX_PORT_BUFFER)
            nGetMarkData = 0;

        if(pMarkData->hMarkTargetComponent == base->GetComponentHandle()){
            LOG_DEBUG("%s,%d,component %s send mark event for target %x.\n",__FUNCTION__,__LINE__,base->name,pMarkData->hMarkTargetComponent);
            base->SendEvent(OMX_EventMark, 0, 0, pMarkData);
            pBufferHdr->hMarkTargetComponent = NULL;
        }
        else if(pMarkData->hMarkTargetComponent != NULL){
            if(sPortDef.eDir == OMX_DirOutput) {
                pBufferHdr->hMarkTargetComponent = pMarkData->hMarkTargetComponent;
                pBufferHdr->pMarkData = pMarkData->pMarkData;
                LOG_DEBUG("%s,%d,component %s,out port %d, do mark target %x on buf %x.\n",__FUNCTION__,__LINE__,base->name,nPortIndex,pBufferHdr->hMarkTargetComponent,pBufferHdr);
            }else {
                base->MarkOtherPortsBuffer(pMarkData);
                LOG_DEBUG("%s,%d,in component %s relay mark target %x to output port.\n",__FUNCTION__,__LINE__,base->name,pMarkData->hMarkTargetComponent);
            }
        }
         
    }

    if(sPortDef.eDir == OMX_DirInput) {
        if(bTunneled == OMX_TRUE)
            OMX_FillThisBuffer(sTunnelInfo.hTunneledComp, pBufferHdr);
        else
            base->EmptyDone(pBufferHdr);
    }

    if(sPortDef.eDir == OMX_DirOutput) {
        if(bTunneled == OMX_TRUE)
            OMX_EmptyThisBuffer(sTunnelInfo.hTunneledComp, pBufferHdr);
        else
            base->FillDone(pBufferHdr);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::MarkBuffer(
        OMX_MARKTYPE *pMarkData) 
{
    if(pMarkData == NULL)
        return OMX_ErrorBadParameter;

    fsl_osal_memcpy(&MarkDatas[nAddMarkData],pMarkData,sizeof(OMX_MARKTYPE));
    nMarkDataNum ++;
    nAddMarkData ++;
    if(nAddMarkData >= MAX_PORT_BUFFER)
        nAddMarkData = 0;
        
//    LOG_ERROR("%s,%d,mark target %x.\n",__FUNCTION__,__LINE__,pMarkData->hMarkTargetComponent);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::AddBuffer(
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    if(pBufferHdr == NULL)
        return OMX_ErrorBadParameter;

    pPortQueue->Add(&pBufferHdr);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::GetBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr) 
{
    if(ppBufferHdr == NULL)
        return OMX_ErrorBadParameter;

    pPortQueue->Get(ppBufferHdr);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::AcessBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nIndex)
{
    if(ppBufferHdr == NULL)
        return OMX_ErrorBadParameter;

    if(QUEUE_SUCCESS != pPortQueue->Access(ppBufferHdr, nIndex))
        return OMX_ErrorOverflow;

    return OMX_ErrorNone;
}

OMX_U32 Port::BufferNum() 
{
    return pPortQueue->Size();
}

OMX_ERRORTYPE Port::GetTunneledInfo(
        TUNNEL_INFO *pTunnelInfo) 
{
    fsl_osal_memcpy(pTunnelInfo, &sTunnelInfo, sizeof(TUNNEL_INFO));

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::SetSupplierType(
        OMX_BUFFERSUPPLIERTYPE eSupplier) 
{
    eBufferSupplier = eSupplier;
    if((eBufferSupplier == OMX_BufferSupplyInput && sPortDef.eDir == OMX_DirInput)
            || (eBufferSupplier == OMX_BufferSupplyOutput && sPortDef.eDir == OMX_DirOutput))
        bSupplier = OMX_TRUE;
    else
        bSupplier = OMX_FALSE;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Port::GetSupplierType(
        OMX_BUFFERSUPPLIERTYPE *pSupplier) 
{
    *pSupplier = eBufferSupplier;
    return OMX_ErrorNone;
}

OMX_DIRTYPE Port::GetPortDir() 
{
    return sPortDef.eDir;
}

OMX_BOOL Port::IsEnabled() 
{
    return sPortDef.bEnabled;
}

OMX_BOOL Port::IsPopulated() 
{
    return sPortDef.bPopulated;
}

OMX_BOOL Port::IsAllocater() 
{
    return bAllocater;
}

OMX_BOOL Port::IsTunneled() 
{
    return bTunneled;
}

OMX_BOOL Port::IsSupplier() 
{
    return bSupplier;
}

OMX_BOOL Port::IsBufferMarked() 
{
    return OMX_TRUE;

}

OMX_BOOL Port::NeedReturnBuffers()
{
    return bNeedReturnBuffers;
}

OMX_BOOL Port::IsAllBuffersReturned() 
{
    if(bTunneled == OMX_TRUE && bSupplier == OMX_TRUE) {
        if(nBufferHdrNum != BufferNum())
            return OMX_FALSE;
    }
    else {
        if(BufferNum() > 0)
            return OMX_FALSE;
    }

    return OMX_TRUE;
}

/* File EOF */
