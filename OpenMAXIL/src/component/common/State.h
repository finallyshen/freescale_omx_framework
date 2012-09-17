/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file State.h
 *  @brief Base class definition of State
 *  @ingroup State
 */

#ifndef State_h
#define State_h

#include "ComponentCommon.h"
#include "ComponentBase.h"

#define IS_PORT_ENABLING(idx) \
    ((base->pendingCmd.cmd == OMX_CommandPortEnable) && (base->pendingCmd.nParam == idx) ? OMX_TRUE : OMX_FALSE)

#define IS_PORT_DISABLING(idx) \
    ((base->pendingCmd.cmd == OMX_CommandPortDisable) && (base->pendingCmd.nParam == idx) ? OMX_TRUE : OMX_FALSE)

class State {
    public:
        State(ComponentBase *pBase);
        virtual ~State() {};
        virtual OMX_ERRORTYPE GetVersion(OMX_STRING pComponentName, OMX_VERSIONTYPE* pComponentVersion, 
                OMX_VERSIONTYPE* pSpecVersion, OMX_UUIDTYPE* pComponentUUID) = 0;
        virtual OMX_ERRORTYPE SendCommand(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData) = 0;
        virtual OMX_ERRORTYPE EmptyThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer) = 0;
        virtual OMX_ERRORTYPE FillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer) = 0;
        OMX_ERRORTYPE ProcessCmd();
        virtual OMX_ERRORTYPE ProcessBuffer() = 0;
        virtual OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure) = 0;
        virtual OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure) = 0;
        virtual OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure) = 0;
        virtual OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure) = 0;
        virtual OMX_ERRORTYPE GetExtensionIndex(OMX_STRING cParameterName, OMX_INDEXTYPE* pIndexType) = 0;
        virtual OMX_ERRORTYPE UseBuffer(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex,
                OMX_PTR pAppPrivate, OMX_U32 nSizeBytes, OMX_U8* pBuffer) = 0;
        virtual OMX_ERRORTYPE UseEGLImage(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex,
                                        OMX_PTR pAppPrivate, void *eglImage) = 0;
        virtual OMX_ERRORTYPE AllocateBuffer(OMX_BUFFERHEADERTYPE** ppBuffer, OMX_U32 nPortIndex,
                OMX_PTR pAppPrivate, OMX_U32 nSizeBytes) = 0;
        virtual OMX_ERRORTYPE FreeBuffer(OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBuffer) = 0;
        virtual OMX_ERRORTYPE TunnelRequest(OMX_U32 nPortIndex, OMX_HANDLETYPE hTunneledComp,
                OMX_U32 nTunneledPort, OMX_TUNNELSETUPTYPE* pTunnelSetup) = 0;
    protected:
        ComponentBase *base;
        OMX_ERRORTYPE DoGetComponentVersion(OMX_STRING pComponentName, 
                OMX_VERSIONTYPE* pComponentVersion, 
                OMX_VERSIONTYPE* pSpecVersion, 
                OMX_UUIDTYPE* pComponentUUID);
        OMX_ERRORTYPE DoSendCommand(OMX_COMMANDTYPE Cmd, OMX_U32 nParam, OMX_PTR pCmdData);
        OMX_ERRORTYPE DoEmptyThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
        OMX_ERRORTYPE DoFillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
        OMX_ERRORTYPE DoGetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE DoSetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE DoGetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE DoSetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE DoGetExtensionIndex(OMX_STRING cParameterName, OMX_INDEXTYPE* pIndexType);
        OMX_ERRORTYPE DoUseBuffer(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex,
                OMX_PTR pAppPrivate, OMX_U32 nSizeBytes, OMX_U8* pBuffer);
        OMX_ERRORTYPE DoAllocateBuffer(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex,
                OMX_PTR pAppPrivate, OMX_U32 nSizeBytes);
        OMX_ERRORTYPE DoFreeBuffer(OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBufferHdr);
        OMX_ERRORTYPE DoTunnelRequest(OMX_U32 nPortIndex, OMX_HANDLETYPE hTunneledComp,
                OMX_U32 nTunneledPort, OMX_TUNNELSETUPTYPE* pTunnelSetup);
        OMX_ERRORTYPE DoUseEGLImage(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex,
                OMX_PTR pAppPrivate, void* eglImage);
    private:
        OMX_ERRORTYPE (ComponentBase::*Process)(MSG_TYPE msg);
        OMX_ERRORTYPE CheckCommand(OMX_COMMANDTYPE Cmd, OMX_U32 nParam, OMX_PTR pCmdData);
        virtual OMX_ERRORTYPE ToInvalid() = 0;
        virtual OMX_ERRORTYPE ToLoaded() = 0;
        virtual OMX_ERRORTYPE ToWaitForResources() = 0;
        virtual OMX_ERRORTYPE ToIdle() = 0;
        virtual OMX_ERRORTYPE ToPause() = 0;
        virtual OMX_ERRORTYPE ToExecuting() = 0;
};

#endif
/* File EOF */
