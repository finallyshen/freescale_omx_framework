/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file FileWrite.h
 *  @brief Class definition of FileWrite Component
 *  @ingroup FileWrite
 */

#ifndef FileWrite_h
#define FileWrite_h

#include "ComponentBase.h"
#include "OMX_ContentPipe.h"

#define PORT_NUM 2
#define AUDIO_PORT 0
#define VIDEO_PORT 1

class FileWrite : public ComponentBase {
    public:
        CP_PIPETYPE *hPipe;
        FileWrite();
    private:
        OMX_BUFFERHEADERTYPE *pAudioBufferHdr;
        OMX_BUFFERHEADERTYPE *pVideoBufferHdr;
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE InstanceInit();
        OMX_ERRORTYPE InstanceDeInit();
        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE ProcessDataBuffer();
        OMX_ERRORTYPE ComponentReturnBuffer(OMX_U32 nPortIndex);
        OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex);
		OMX_S8 *pMediaName;
		FILE *pOutFile0;
		FILE *pOutFile1;
};

#endif
/* File EOF */
