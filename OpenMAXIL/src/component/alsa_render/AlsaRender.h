/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AlsaRender.h
 *  @brief Class definition of AlsaRender Component
 *  @ingroup AlsaRender
 */

#ifndef AlsaRender_h
#define AlsaRender_h

#include <alsa/asoundlib.h>
#include "ComponentBase.h"
#include "AudioRender.h"

class AlsaRender : public AudioRender {
    public:
        AlsaRender();
    private:
		/** Audio render device related */
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
        OMX_ERRORTYPE SetDevice();
        OMX_ERRORTYPE ResetDevice();
        OMX_ERRORTYPE DrainDevice();
		OMX_ERRORTYPE DeviceDelay(OMX_U32 *nDelayLen);
        OMX_ERRORTYPE WriteDevice(OMX_U8 *pBuffer, OMX_U32 nActuralLen);
		OMX_S8 Device[32];
		snd_pcm_t *hPlayback;
		snd_pcm_hw_params_t *hw_params;
		snd_pcm_sw_params_t *sw_params;

};

#endif
/* File EOF */
