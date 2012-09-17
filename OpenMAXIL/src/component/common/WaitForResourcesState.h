/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file WaitForResourcesState.h
 *  @brief class definition of WaitForResourcesState
 *  @ingroup State
 */

#ifndef WaitForResourcesState_h
#define WaitForResourcesState_h

#include "State.h"

class WaitForResourcesState : public State {
    public:
        WaitForResourcesState(ComponentBase *pBase) : State(pBase) {};
        OMX_ERRORTYPE GetVersion(OMX_STRING pComponentName, OMX_VERSIONTYPE* pComponentVersion, 
                                 OMX_VERSIONTYPE* pSpecVersion, OMX_UUIDTYPE* pComponentUUID);
        OMX_ERRORTYPE SendCommand(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData);
        OMX_ERRORTYPE EmptyThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
        OMX_ERRORTYPE FillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
        OMX_ERRORTYPE ProcessBuffer();
        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE GetExtensionIndex(OMX_STRING cParameterName, OMX_INDEXTYPE* pIndexType);
        OMX_ERRORTYPE UseBuffer(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex,
                                OMX_PTR pAppPrivate, OMX_U32 nSizeBytes, OMX_U8* pBuffer);
        OMX_ERRORTYPE UseEGLImage(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex,
                                OMX_PTR pAppPrivate, void *eglImage);
        OMX_ERRORTYPE AllocateBuffer(OMX_BUFFERHEADERTYPE** ppBuffer, OMX_U32 nPortIndex,
                                OMX_PTR pAppPrivate, OMX_U32 nSizeBytes);
        OMX_ERRORTYPE FreeBuffer(OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBuffer);
        OMX_ERRORTYPE TunnelRequest(OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp,
                                            OMX_U32 nTunneledPort, OMX_TUNNELSETUPTYPE* pTunnelSetup);
    private:
        OMX_ERRORTYPE ToInvalid();
        OMX_ERRORTYPE ToLoaded();
        OMX_ERRORTYPE ToWaitForResources();
        OMX_ERRORTYPE ToIdle();
        OMX_ERRORTYPE ToPause();
        OMX_ERRORTYPE ToExecuting();
};

#endif
/* File EOF */
