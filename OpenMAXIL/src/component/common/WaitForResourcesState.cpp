/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "WaitForResourcesState.h"

OMX_ERRORTYPE WaitForResourcesState::GetVersion(
        OMX_STRING pComponentName, 
        OMX_VERSIONTYPE* pComponentVersion, 
        OMX_VERSIONTYPE* pSpecVersion, 
        OMX_UUIDTYPE* pComponentUUID) 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE WaitForResourcesState::SendCommand(
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam1, 
        OMX_PTR pCmdData)
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::EmptyThisBuffer(
        OMX_BUFFERHEADERTYPE* pBuffer)
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::FillThisBuffer(
        OMX_BUFFERHEADERTYPE* pBuffer) 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::ProcessBuffer() 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::GetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WaitForResourcesState::SetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::GetExtensionIndex(
        OMX_STRING cParameterName, 
        OMX_INDEXTYPE* pIndexType) 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::UseBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes, 
        OMX_U8* pBuffer) 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::UseEGLImage(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        void *eglImage) 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::AllocateBuffer(
        OMX_BUFFERHEADERTYPE** ppBuffer, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes) 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::FreeBuffer(
        OMX_U32 nPortIndex, 
        OMX_BUFFERHEADERTYPE* pBuffer) 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::TunnelRequest(
        OMX_U32 nPort, 
        OMX_HANDLETYPE hTunneledComp,
        OMX_U32 nTunneledPort, 
        OMX_TUNNELSETUPTYPE* pTunnelSetup) 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::ToInvalid() 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::ToLoaded() 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::ToWaitForResources() 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::ToIdle() 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::ToPause() 
{
    return OMX_ErrorNone;

}

OMX_ERRORTYPE WaitForResourcesState::ToExecuting() 
{
    return OMX_ErrorNone;

}

/* File EOF */
