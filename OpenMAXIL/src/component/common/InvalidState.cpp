/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "InvalidState.h"

OMX_ERRORTYPE InvalidState::GetVersion(
        OMX_STRING pComponentName, 
        OMX_VERSIONTYPE* pComponentVersion, 
        OMX_VERSIONTYPE* pSpecVersion, 
        OMX_UUIDTYPE* pComponentUUID) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::SendCommand(
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam1, 
        OMX_PTR pCmdData)
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::EmptyThisBuffer(
        OMX_BUFFERHEADERTYPE* pBuffer)
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::FillThisBuffer(
        OMX_BUFFERHEADERTYPE* pBuffer) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::ProcessBuffer() 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::GetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::SetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::GetExtensionIndex(
        OMX_STRING cParameterName, 
        OMX_INDEXTYPE* pIndexType) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::UseBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes, 
        OMX_U8* pBuffer) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::UseEGLImage(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        void *eglImage) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::AllocateBuffer(
        OMX_BUFFERHEADERTYPE** ppBuffer, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::FreeBuffer(
        OMX_U32 nPortIndex, 
        OMX_BUFFERHEADERTYPE* pBuffer) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::TunnelRequest(
        OMX_U32 nPort, 
        OMX_HANDLETYPE hTunneledComp,
        OMX_U32 nTunneledPort, 
        OMX_TUNNELSETUPTYPE* pTunnelSetup) 
{
    return OMX_ErrorInvalidState;
}

OMX_ERRORTYPE InvalidState::ToInvalid() 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE InvalidState::ToLoaded() 
{
    return OMX_ErrorIncorrectStateTransition;
}

OMX_ERRORTYPE InvalidState::ToWaitForResources() 
{
    return OMX_ErrorIncorrectStateTransition;
}

OMX_ERRORTYPE InvalidState::ToIdle() 
{
    return OMX_ErrorIncorrectStateTransition;
}

OMX_ERRORTYPE InvalidState::ToPause() 
{
    return OMX_ErrorIncorrectStateTransition;
}

OMX_ERRORTYPE InvalidState::ToExecuting() 
{
    return OMX_ErrorIncorrectStateTransition;
}

/* File EOF */
