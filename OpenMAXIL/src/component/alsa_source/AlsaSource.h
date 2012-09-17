/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AlsaSource.h
 *  @brief Class definition of AlsaSource Component
 *  @ingroup AlsaSource
 */

#ifndef AlsaSource_h
#define AlsaSource_h

#include <alsa/asoundlib.h>
#include "AudioSource.h"

class AlsaSource : public AudioSource {
    public:
        AlsaSource();
    private:
        OMX_ERRORTYPE InitSourceComponent();
        OMX_ERRORTYPE DeInitSourceComponent();
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
        OMX_ERRORTYPE SetDevice();
		OMX_ERRORTYPE StartDevice();
		OMX_ERRORTYPE StopDevice();
		int xrun();
		int suspend();
		OMX_ERRORTYPE GetOneFrameFromDevice();
		OMX_ERRORTYPE GetDeviceDelay(OMX_U32 *nDelayLen);
		OMX_S16 getMaxAmplitude();

		snd_pcm_t *hCapture;
		snd_pcm_hw_params_t *hw_params;
		snd_pcm_sw_params_t *sw_params;

        OMX_U32 nFrameBuffer;
        OMX_U32 nAllocating;
        OMX_U32 nQueuedBuffer;
        OMX_U32 nSourceFrames;
        fsl_osal_mutex lock;
		OMX_TICKS nCurTS;
        OMX_S32 EnqueueBufferIdx;

};

#endif
/* File EOF */
