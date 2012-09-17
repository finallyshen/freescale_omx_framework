/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AudioFakeRender.h
 *  @brief Class definition of AudioFakeRender Component
 *  @ingroup AudioFakeRender
 */

#ifndef AudioFakeRender_h
#define AudioFakeRender_h

#include "ComponentBase.h"
#include "AudioRender.h"

class AudioFakeRender : public AudioRender {
    public:
        AudioFakeRender();
    private:
		/** Audio render device related */
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
        OMX_ERRORTYPE SetDevice();
        OMX_ERRORTYPE ResetDevice();
        OMX_ERRORTYPE DrainDevice();
		OMX_ERRORTYPE DeviceDelay(OMX_U32 *nDelayLen);
        OMX_ERRORTYPE WriteDevice(OMX_U8 *pBuffer, OMX_U32 nActuralLen);

};

#endif
/* File EOF */
