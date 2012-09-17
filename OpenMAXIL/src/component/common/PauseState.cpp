/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "PauseState.h"

OMX_ERRORTYPE PauseState::GetVersion(
        OMX_STRING pComponentName, 
        OMX_VERSIONTYPE* pComponentVersion, 
        OMX_VERSIONTYPE* pSpecVersion, 
        OMX_UUIDTYPE* pComponentUUID) 
{
    return State::DoGetComponentVersion(
            pComponentName, pComponentVersion, pSpecVersion, pComponentUUID);
}

OMX_ERRORTYPE PauseState::SendCommand(
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam, 
        OMX_PTR pCmdData)
{
    return State::DoSendCommand(Cmd, nParam, pCmdData);
}

OMX_ERRORTYPE PauseState::EmptyThisBuffer(
        OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    return State::DoEmptyThisBuffer(pBufferHdr);
}

OMX_ERRORTYPE PauseState::FillThisBuffer(
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    return State::DoFillThisBuffer(pBufferHdr);
}

OMX_ERRORTYPE PauseState::ProcessBuffer() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    OMX_U32 i;

                    
    ret = base->ProcessClkBuffer();
    if(ret == OMX_ErrorNoMore) {
        for(i=0; i<base->nPorts; i++) {
            if(OMX_ErrorNone == base->ports[i]->AcessBuffer(&pBufferHdr, 1))
                if(pBufferHdr->hMarkTargetComponent != NULL)
                {
                    LOG_DEBUG("%s,%d,process a marked buffer.marked target %x\n",__FUNCTION__,__LINE__,pBufferHdr->hMarkTargetComponent);
                    ret = base->ProcessDataBuffer();
                }
        }
    }

    return ret;
}

OMX_ERRORTYPE PauseState::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoGetParameter(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE PauseState::SetParameter(
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

OMX_ERRORTYPE PauseState::GetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoGetConfig(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE PauseState::SetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoSetConfig(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE PauseState::GetExtensionIndex(
        OMX_STRING cParameterName, 
        OMX_INDEXTYPE* pIndexType) 
{
    return State::DoGetExtensionIndex(cParameterName, pIndexType);
}

OMX_ERRORTYPE PauseState::UseBuffer(
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

OMX_ERRORTYPE PauseState::UseEGLImage(
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

OMX_ERRORTYPE PauseState::AllocateBuffer(
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

OMX_ERRORTYPE PauseState::FreeBuffer(
        OMX_U32 nPortIndex, 
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    if(IS_PORT_DISABLING(nPortIndex) == OMX_TRUE) {
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 500000);
        return State::DoFreeBuffer(nPortIndex, pBufferHdr);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE PauseState::TunnelRequest(
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

OMX_ERRORTYPE PauseState::ToInvalid() 
{
    //TODO
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PauseState::ToLoaded() 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE PauseState::ToWaitForResources() 
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE PauseState::ToIdle() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i;

    base->InstanceDeInit();

    /* return all buffers to allocater */
    for(i=0; i<base->nPorts; i++) {
        if(OMX_ErrorNone != base->ports[i]->ReturnBuffers())
            ret = OMX_ErrorNotComplete;
    }

    return ret;
}

OMX_ERRORTYPE PauseState::ToPause() 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PauseState::ToExecuting() 
{
    LOG_DEBUG("%s pause->exec.\n", base->name);
    base->DoPause2Exec();
    if(OMX_TRUE == base->bInContext)
        base->ProcessDataBuffer();

    return OMX_ErrorNone;
}

/* File EOF */
