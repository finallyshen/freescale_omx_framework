/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "LoadedState.h"

OMX_ERRORTYPE LoadedState::GetVersion(
        OMX_STRING pComponentName, 
        OMX_VERSIONTYPE* pComponentVersion, 
        OMX_VERSIONTYPE* pSpecVersion, 
        OMX_UUIDTYPE* pComponentUUID) 
{
    return State::DoGetComponentVersion(
            pComponentName, pComponentVersion, pSpecVersion, pComponentUUID);
}

OMX_ERRORTYPE LoadedState::SendCommand(
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam, 
        OMX_PTR pCmdData)
{
    return State::DoSendCommand(Cmd, nParam, pCmdData);
}

OMX_ERRORTYPE LoadedState::EmptyThisBuffer(
        OMX_BUFFERHEADERTYPE* pBuffer)
{
    return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE LoadedState::FillThisBuffer(
        OMX_BUFFERHEADERTYPE* pBuffer) 
{
    return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE LoadedState::ProcessBuffer() 
{
    return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE LoadedState::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoGetParameter(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE LoadedState::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoSetParameter(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE LoadedState::GetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoGetConfig(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE LoadedState::SetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return State::DoSetConfig(nParamIndex, pComponentParameterStructure);
}

OMX_ERRORTYPE LoadedState::GetExtensionIndex(
        OMX_STRING cParameterName, 
        OMX_INDEXTYPE* pIndexType) 
{
    return State::DoGetExtensionIndex(cParameterName, pIndexType);
}

OMX_ERRORTYPE LoadedState::UseBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes, 
        OMX_U8* pBuffer) 
{
    /* check if component in loaded->idle state trans*/
    if(base->pendingCmd.cmd == OMX_CommandStateSet
            && base->pendingCmd.nParam == OMX_StateIdle) {
		/* FIXME: workround for ALSA open need long time */
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 1000000);
        if(base->pendingCmd.cmd != OMX_CommandStateSet)
            return OMX_ErrorIncorrectStateOperation;
        return State::DoUseBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
    }
    else{
       OMX_S32 nCount =1000;
        do{
            fsl_osal_sleep(1000);
            nCount--;
            if(nCount <=0){ 
                LOG_WARNING("%s UseBuffer in wrong state\n",base->name);
                return OMX_ErrorIncorrectStateOperation;
            }
        }while(base->pendingCmd.cmd !=OMX_CommandStateSet || base->pendingCmd.nParam != OMX_StateIdle);
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 1000000);
        return State::DoUseBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
    }
}

OMX_ERRORTYPE LoadedState::UseEGLImage(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        void *eglImage) 
{
    /* check if component in loaded->idle state trans*/
    if(base->pendingCmd.cmd == OMX_CommandStateSet
            && base->pendingCmd.nParam == OMX_StateIdle) {
		/* FIXME: workround for ALSA open need long time */
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 1000000);
        if(base->pendingCmd.cmd != OMX_CommandStateSet)
            return OMX_ErrorIncorrectStateOperation;
        return State::DoUseEGLImage(ppBufferHdr, nPortIndex, pAppPrivate, eglImage);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE LoadedState::AllocateBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes) 
{
    /* check if component in loaded->idle state trans*/
    if(base->pendingCmd.cmd == OMX_CommandStateSet
            && base->pendingCmd.nParam == OMX_StateIdle) {
		/* FIXME: workround for ALSA and camera open need long time */
        WAIT_CONDTION(base->pendingCmd.bProcessed, OMX_TRUE, 3000000);
        if(base->pendingCmd.cmd != OMX_CommandStateSet)
            return OMX_ErrorIncorrectStateOperation;
        return State::DoAllocateBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes);
    }
    else
        return OMX_ErrorIncorrectStateOperation;
}

OMX_ERRORTYPE LoadedState::FreeBuffer(
        OMX_U32 nPortIndex, 
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    return State::DoFreeBuffer(nPortIndex, pBufferHdr);
}

OMX_ERRORTYPE LoadedState::TunnelRequest(
        OMX_U32 nPort, 
        OMX_HANDLETYPE hTunneledComp,
        OMX_U32 nTunneledPort, 
        OMX_TUNNELSETUPTYPE* pTunnelSetup) 
{
    return State::DoTunnelRequest(nPort, hTunneledComp, nTunneledPort, pTunnelSetup);
}

OMX_ERRORTYPE LoadedState::ToInvalid() 
{
    //TODO
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE LoadedState::ToLoaded() 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE LoadedState::ToWaitForResources() 
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE LoadedState::ToIdle() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i;

    ret = base->DoLoaded2Idle();
    if(ret != OMX_ErrorNone) {
        base->DoIdle2Loaded();
        return ret;
    }

    for(i=0; i<base->nPorts; i++) {
        if(base->ports[i]->IsEnabled() != OMX_TRUE)
            continue;

        if(base->ports[i]->IsTunneled() == OMX_TRUE
                && base->ports[i]->IsSupplier() == OMX_TRUE) {
            base->ports[i]->SupplierAllocateBuffers();
        }

        ret = OMX_ErrorNotComplete;
    }

    return ret;
}

OMX_ERRORTYPE LoadedState::ToPause() 
{
    return OMX_ErrorIncorrectStateTransition;
}

OMX_ERRORTYPE LoadedState::ToExecuting() 
{
    return OMX_ErrorIncorrectStateTransition;
}

/* File EOF */
