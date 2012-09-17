/**
 *  Copyright (c) 2009,2012 Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file FakeVideoRenderr.h
 *  @brief Class definition of FakeVideoRender Component
 *  @ingroup VideoRender
 */

#ifndef FakeVideoRender_h
#define FakeVideoRender_h

#include "VideoRender.h"

class FakeVideoRender : public VideoRender {
    public:
        FakeVideoRender();
    private:
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
        OMX_ERRORTYPE WriteDevice(OMX_BUFFERHEADERTYPE *pBufferHdr);
};

#endif
/* File EOF */
