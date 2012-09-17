/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "IdleState.h"

OMX_ERRORTYPE IdleState::GetVersion(
        OMX_STRING pComponentName, 
        OMX_VERSIONTYPE* pComponentVersion, 
        OMX_VERSIONTYPE* pSpecVersion, 
        OMX_UUIDTYPE* pComponentUUID) 
{
    return State::DoGetComponentVersion(
            pComponentName, pComponentVersion, pSpecVersion, pComponentUUID);
}

OMX_ERRORTYPE IdleState::SendCommand(
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam, 
        OMX_PTR pCmdData)
{
    return State::DoSendCommand(Cmd, nParam, pCmdData);
}

OMX_ERRORTYPE IdleState::EmptyThisBuffer(
        OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    return State::DoEmptyThisBuffer(pBufferHdr);
}

OMX_ERRORTYPE IdleState::FillThisBuffer(
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    return State::DoFillThisBuffer(pBufferHdr);
}

OMX_ERRORTYPE IdleState::ProcessBuffer() 
{
    return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE IdleState::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoGetParameter(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE IdleState::SetParameter(
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

OMX_ERRORTYPE IdleState::GetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoGetConfig(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE IdleState::SetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoSetConfig(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE IdleState::GetExtensionIndex(
        OMX_STRING cParameterName, 
        OMX_INDEXTYPE* pIndexType) 
{
    return State::DoGetExtensionIndex(cParameterName, pIndexType);
}

OMX_ERRORTYPE IdleState::UseBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes, 
        OMX_U8* pBuffer) 
{
    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    if(base->ports[nPortIndex]->IsEnabled() != OMX_TRUE) {
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 500000);
        return State::DoUseBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE IdleState::UseEGLImage(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        void *eglImage) 
{
    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    if(base->ports[nPortIndex]->IsEnabled() != OMX_TRUE) {
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 500000);
        return State::DoUseEGLImage(ppBufferHdr, nPortIndex, pAppPrivate, eglImage);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE IdleState::AllocateBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes) 
{
    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    if(base->ports[nPortIndex]->IsEnabled() != OMX_TRUE) {
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 500000);
        return State::DoAllocateBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE IdleState::FreeBuffer(
        OMX_U32 nPortIndex, 
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    /* check if component in loaded->idle state trans
     * or port is disabling */
    if((base->pendingCmd.cmd == OMX_CommandStateSet && base->pendingCmd.nParam == OMX_StateLoaded)
            || (IS_PORT_DISABLING(nPortIndex) == OMX_TRUE)) {
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 500000);
        return State::DoFreeBuffer(nPortIndex, pBufferHdr);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE IdleState::TunnelRequest(
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

OMX_ERRORTYPE IdleState::ToInvalid() 
{
    //TODO
    return OMX_ErrorNone;
}

OMX_ERRORTYPE IdleState::ToLoaded() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i;

    for(i=0; i<base->nPorts; i++) {
        if(base->ports[i]->IsEnabled() != OMX_TRUE)
            continue;

        if(base->ports[i]->IsTunneled() == OMX_TRUE
                && base->ports[i]->IsSupplier() == OMX_TRUE) {
            base->ports[i]->SupplierFreeBuffers();
        }

        ret = OMX_ErrorNotComplete;
    }
	
    if (ret == OMX_ErrorNone)
        base->DoIdle2Loaded();

    return ret;
}

OMX_ERRORTYPE IdleState::ToWaitForResources() 
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE IdleState::ToIdle() 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE IdleState::ToPause() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = base->InstanceInit();
    if(ret != OMX_ErrorNone)
        base->InstanceDeInit();

    return ret;
}

OMX_ERRORTYPE IdleState::ToExecuting() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = base->InstanceInit();
    if(ret != OMX_ErrorNone)
        base->InstanceDeInit();

    return ret;
}

/* File EOF */
