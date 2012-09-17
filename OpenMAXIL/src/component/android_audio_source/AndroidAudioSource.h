/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AndroidAudioSource.h
 *  @brief Class definition of AndroidAudioSource Component
 *  @ingroup AndroidAudioSource
 */

#ifndef AndroidAudioSource_h
#define AndroidAudioSource_h

#include <media/AudioSystem.h>
#include <media/AudioRecord.h>
#include "AudioSource.h"

using android::AudioSystem;
using android::AudioRecord;

class AndroidAudioSource : public AudioSource {
    public:
        AndroidAudioSource();
	private:
		enum {
			kMaxBufferSize = 2048,

			// After the initial mute, we raise the volume linearly
			// over kAutoRampDurationUs.
			kAutoRampDurationUs = 700000,

			// This is the initial mute duration to suppress
			// the video recording signal tone
			kAutoRampStartUs = 1000000,
		};

        OMX_ERRORTYPE InitSourceComponent();
        OMX_ERRORTYPE DeInitSourceComponent();
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
        OMX_ERRORTYPE SetDevice();
		OMX_ERRORTYPE StartDevice();
		OMX_ERRORTYPE StopDevice();
		void trackMaxAmplitude(OMX_S16 *data, OMX_S32 nSamples);
		OMX_S16 getMaxAmplitude();
		OMX_ERRORTYPE GetOneFrameFromDevice();
		OMX_ERRORTYPE GetDeviceDelay(OMX_U32 *nDelayLen);

		AudioRecord *mRecord;

		OMX_BOOL mTrackMaxAmplitude;
		OMX_BOOL mStarted;
		OMX_S32 mMaxAmplitude;
		OMX_S32 mInitCheck;
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
