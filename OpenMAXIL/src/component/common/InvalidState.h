/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file InvalidState.h
 *  @brief class definition of InvalidState
 *  @ingroup State
 */

#ifndef InvalidState_h
#define InvalidState_h

#include "State.h"

class InvalidState : public State {
    public:
        InvalidState(ComponentBase *pBase) : State(pBase) {};
        virtual OMX_ERRORTYPE GetVersion(OMX_STRING pComponentName, OMX_VERSIONTYPE* pComponentVersion, 
                                         OMX_VERSIONTYPE* pSpecVersion, OMX_UUIDTYPE* pComponentUUID);
        virtual OMX_ERRORTYPE SendCommand(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData);
        virtual OMX_ERRORTYPE EmptyThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
        virtual OMX_ERRORTYPE FillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
        virtual OMX_ERRORTYPE ProcessBuffer();
        virtual OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual OMX_ERRORTYPE GetExtensionIndex(OMX_STRING cParameterName, OMX_INDEXTYPE* pIndexType);
        virtual OMX_ERRORTYPE UseBuffer(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex,
                                        OMX_PTR pAppPrivate, OMX_U32 nSizeBytes, OMX_U8* pBuffer);
        virtual OMX_ERRORTYPE UseEGLImage(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex,
                                        OMX_PTR pAppPrivate, void *eglImage);
        virtual OMX_ERRORTYPE AllocateBuffer(OMX_BUFFERHEADERTYPE** ppBuffer, OMX_U32 nPortIndex,
                                             OMX_PTR pAppPrivate, OMX_U32 nSizeBytes);
        virtual OMX_ERRORTYPE FreeBuffer(OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBuffer);
        virtual OMX_ERRORTYPE TunnelRequest(OMX_U32 nPort, OMX_HANDLETYPE hTunneledComp,
                                            OMX_U32 nTunneledPort, OMX_TUNNELSETUPTYPE* pTunnelSetup);
    private:
        OMX_ERRORTYPE ToInvalid();
        OMX_ERRORTYPE ToLoaded();
        OMX_ERRORTYPE ToWaitForResources();
        OMX_ERRORTYPE ToIdle();
        OMX_ERRORTYPE ToPause();
        OMX_ERRORTYPE ToExecuting();
        OMX_ERRORTYPE DoGetParameter();

};

#endif
/* File EOF */
