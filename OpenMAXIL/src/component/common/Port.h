/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Port.h
 *  @brief class definition of port
 *  @ingroup Port
 */

#ifndef Port_h
#define Port_h

#include "Queue.h"
#include "ComponentCommon.h"
#include "ComponentBase.h"

class Port {
    public:
        Port(ComponentBase *pBase, OMX_U32 nPortIdx);
        OMX_ERRORTYPE Init();
        OMX_ERRORTYPE DeInit();
        OMX_ERRORTYPE SetPortDefinition(OMX_PARAM_PORTDEFINITIONTYPE *pPortDef);
        OMX_ERRORTYPE GetPortDefinition(OMX_PARAM_PORTDEFINITIONTYPE *pPortDef);
        OMX_ERRORTYPE UseBuffer(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_PTR pAppPrivate,
                OMX_U32 nSizeBytes, OMX_U8* pBuffer);
        OMX_ERRORTYPE UseEGLImage(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_PTR pAppPrivate, void* eglImage);
        OMX_ERRORTYPE AllocateBuffer(OMX_BUFFERHEADERTYPE** ppBuffer, OMX_PTR pAppPrivate,
                OMX_U32 nSizeBytes);
        OMX_ERRORTYPE FreeBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
        OMX_ERRORTYPE TunnelRequest(OMX_HANDLETYPE hTunneledComp, OMX_U32 nTunneledPort,
                OMX_TUNNELSETUPTYPE* pTunnelSetup);
        OMX_ERRORTYPE Disable();
        OMX_ERRORTYPE Enable();
        OMX_ERRORTYPE Flush();
        OMX_ERRORTYPE ReturnBuffers();
        OMX_ERRORTYPE SendBuffer(OMX_BUFFERHEADERTYPE* pBufferHdr);
        OMX_ERRORTYPE MarkBuffer(OMX_MARKTYPE *pMarkData);
        OMX_ERRORTYPE SupplierAllocateBuffers();
        OMX_ERRORTYPE SupplierFreeBuffers();
        OMX_ERRORTYPE AddBuffer(OMX_BUFFERHEADERTYPE* pBufferHdr);
        OMX_ERRORTYPE GetBuffer(OMX_BUFFERHEADERTYPE** ppBufferHdr);
        OMX_ERRORTYPE AcessBuffer(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nIndex);
        OMX_U32 BufferNum();
        OMX_ERRORTYPE GetTunneledInfo(TUNNEL_INFO *pTunnelInfo);
        OMX_ERRORTYPE SetSupplierType(OMX_BUFFERSUPPLIERTYPE eSupplier);
        OMX_ERRORTYPE GetSupplierType(OMX_BUFFERSUPPLIERTYPE *pSupplier);
        OMX_DIRTYPE GetPortDir();
        OMX_BOOL IsEnabled();
        OMX_BOOL IsPopulated();
        OMX_BOOL IsAllocater();
        OMX_BOOL IsTunneled();
        OMX_BOOL IsSupplier();
        OMX_BOOL IsBufferMarked();
        OMX_BOOL NeedReturnBuffers();
        OMX_ERRORTYPE SendEventMark(OMX_BUFFERHEADERTYPE* pBufferHdr) ;
    private:
        OMX_U32 nPortIndex;
        ComponentBase *base;
        Queue *pPortQueue;
        OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
        OMX_U32 nBufferHdrNum;
        OMX_BUFFERHEADERTYPE *BufferHdrArry[MAX_PORT_BUFFER];
        OMX_U32 nMarkDataNum;
        OMX_U32 nGetMarkData;
        OMX_U32 nAddMarkData;
        OMX_MARKTYPE MarkDatas[MAX_PORT_BUFFER];
        OMX_BOOL bAllocater;
        OMX_BOOL bTunneled;
        OMX_BOOL bSupplier;
        OMX_BOOL bNeedReturnBuffers;
        OMX_BUFFERSUPPLIERTYPE eBufferSupplier;
        TUNNEL_INFO sTunnelInfo;
        OMX_BOOL IsAllBuffersReturned();
        OMX_BOOL bPendingDisable;
};

#endif
/* File EOF */
