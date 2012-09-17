/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "ExecutingState.h"

OMX_ERRORTYPE ExecutingState::GetVersion(
        OMX_STRING pComponentName, 
        OMX_VERSIONTYPE* pComponentVersion, 
        OMX_VERSIONTYPE* pSpecVersion, 
        OMX_UUIDTYPE* pComponentUUID) 
{
    return State::DoGetComponentVersion(
            pComponentName, pComponentVersion, pSpecVersion, pComponentUUID);
}

OMX_ERRORTYPE ExecutingState::SendCommand(
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam, 
        OMX_PTR pCmdData)
{
    return State::DoSendCommand(Cmd, nParam, pCmdData);
}

OMX_ERRORTYPE ExecutingState::EmptyThisBuffer(
        OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    return State::DoEmptyThisBuffer(pBufferHdr);
}

OMX_ERRORTYPE ExecutingState::FillThisBuffer(
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    return State::DoFillThisBuffer(pBufferHdr);
}

OMX_ERRORTYPE ExecutingState::ProcessBuffer() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = base->ProcessClkBuffer();
    if(ret == OMX_ErrorNoMore) {
        ret = base->ProcessDataBuffer();
    }

    return ret;
}

OMX_ERRORTYPE ExecutingState::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoGetParameter(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE ExecutingState::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    OMX_U32 i;
    OMX_BOOL bHasPortDisabled = OMX_FALSE;

    for(i=0; i<base->nPorts; i++) {
        if(base->ports[i]->IsEnabled() == OMX_FALSE) {
            bHasPortDisabled = OMX_TRUE;
            break;
        }
    }

    if(bHasPortDisabled == OMX_TRUE)
        return State::DoSetParameter(nParamIndex, pComponentParameterStructure);
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE ExecutingState::GetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoGetConfig(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE ExecutingState::SetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoSetConfig(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE ExecutingState::GetExtensionIndex(
        OMX_STRING cParameterName, 
        OMX_INDEXTYPE* pIndexType) 
{
    return State::DoGetExtensionIndex(cParameterName, pIndexType);
}

OMX_ERRORTYPE ExecutingState::UseBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes, 
        OMX_U8* pBuffer) 
{
    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    if(IS_PORT_ENABLING(nPortIndex) == OMX_TRUE) {
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 500000);
        return State::DoUseBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE ExecutingState::UseEGLImage(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        void *eglImage) 
{
    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    if(IS_PORT_ENABLING(nPortIndex) == OMX_TRUE) {
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 500000);
        return State::DoUseEGLImage(ppBufferHdr, nPortIndex, pAppPrivate, eglImage);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE ExecutingState::AllocateBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes) 
{
    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    if(IS_PORT_ENABLING(nPortIndex) == OMX_TRUE) {
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 500000);
        return State::DoAllocateBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE ExecutingState::FreeBuffer(
        OMX_U32 nPortIndex, 
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    if(IS_PORT_DISABLING(nPortIndex) == OMX_TRUE) {
		/* Change longer for drain component internal data when port disable */
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 5000000);
        return State::DoFreeBuffer(nPortIndex, pBufferHdr);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE ExecutingState::TunnelRequest(
        OMX_U32 nPort, 
        OMX_HANDLETYPE hTunneledComp,
        OMX_U32 nTunneledPort, 
        OMX_TUNNELSETUPTYPE* pTunnelSetup) 
{
    if(nPort >= base->nPorts)
        return OMX_ErrorBadParameter;

    if(base->ports[nPort]->IsEnabled() != OMX_TRUE)
        return State::DoTunnelRequest(nPort, hTunneledComp, nTunneledPort, pTunnelSetup);
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE ExecutingState::ToInvalid() 
{
    //TODO
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ExecutingState::ToLoaded() 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE ExecutingState::ToWaitForResources() 
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ExecutingState::ToIdle() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i;

    base->InstanceDeInit();

    for(i=0; i<base->nPorts; i++) {
        if(OMX_ErrorNone != base->ports[i]->ReturnBuffers())
            ret = OMX_ErrorNotComplete;
    }

    return ret;
}

OMX_ERRORTYPE ExecutingState::ToPause() 
{
    return base->DoExec2Pause();
}

OMX_ERRORTYPE ExecutingState::ToExecuting() 
{
    return OMX_ErrorNone;
}

/* File EOF */
