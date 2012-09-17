/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Template.h
 *  @brief Class definition of Template Component
 *  @ingroup Template
 */

#ifndef Template_h
#define Template_h

#include "ComponentBase.h"

#define PORT_NUM 2
#define IN_PORT 0
#define OUT_PORT 1

class Template : public ComponentBase {
    public:
        Template();
    private:
        OMX_BUFFERHEADERTYPE *pInBufferHdr;
        OMX_BUFFERHEADERTYPE *pOutBufferHdr;
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE ProcessDataBuffer();
        OMX_ERRORTYPE ComponentReturnBuffer(OMX_U32 nPortIndex);
        OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex);
};

#endif
/* File EOF */
