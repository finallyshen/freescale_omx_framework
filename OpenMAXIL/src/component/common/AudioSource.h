/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AudioSource.h
 *  @brief Class definition of AudioSource Component
 *  @ingroup AudioSource
 */

#ifndef AudioSource_h
#define AudioSource_h

#include "ComponentBase.h"
#include "RingBuffer.h"
#include "FadeInFadeOut.h"

#define NUM_PORTS 2
#define OUTPUT_PORT 0
#define CLK_PORT 1

#define FADEPROCESSTIME 100  /**< 100 ms for fade process */

class AudioSource : public ComponentBase {
    public:
		AudioSource();
    protected:
        OMX_U32 nFadeInFadeOutProcessLen;
        OMX_AUDIO_PARAM_PCMMODETYPE PcmMode;
		OMX_BUFFERHEADERTYPE *pOutBufferHdr;
		OMX_AUDIO_CONFIG_VOLUMETYPE Volume;
		OMX_AUDIO_CONFIG_MUTETYPE Mute;
		OMX_U32 nPeriodSize;
		OMX_U32 nSampleSize;
		OMX_U8 *pDeviceReadBuffer;
		OMX_U32 nDeviceReadLen;
		OMX_S32 nAudioSource;
	private:
		OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE InstanceInit();
        OMX_ERRORTYPE InstanceDeInit();
        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
		OMX_ERRORTYPE DoLoaded2Idle();
		OMX_ERRORTYPE DoIdle2Loaded();
		OMX_ERRORTYPE DoPause2Exec();
		OMX_ERRORTYPE ComponentReturnBuffer(OMX_U32 nPortIndex);
		OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex);
		OMX_ERRORTYPE ProcessDataBuffer();
		OMX_ERRORTYPE ProcessOutputBufferData();
		OMX_ERRORTYPE ProcessOutputBufferFlag();
		OMX_ERRORTYPE SendOutputBuffer();
		OMX_ERRORTYPE ClockGetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
		OMX_ERRORTYPE ClockSetConfig(OMX_INDEXTYPE nParamIndex, \
				OMX_TIME_CONFIG_TIMESTAMPTYPE *pTimeStamp);
        OMX_ERRORTYPE ProcessClkBuffer();

		/** Audio source device related */
        virtual OMX_ERRORTYPE InitSourceComponent() = 0;
        virtual OMX_ERRORTYPE DeInitSourceComponent() = 0;
        virtual OMX_ERRORTYPE OpenDevice() = 0;
		virtual OMX_ERRORTYPE CloseDevice() = 0;
		virtual OMX_ERRORTYPE StartDevice() = 0;
		virtual OMX_ERRORTYPE StopDevice() = 0;
        virtual OMX_ERRORTYPE GetOneFrameFromDevice() = 0;
		virtual OMX_ERRORTYPE GetDeviceDelay(OMX_U32 *nDelayLen) = 0;
		virtual OMX_S16 getMaxAmplitude() = 0;

		OMX_TIME_CONFIG_TIMESTAMPTYPE StartTime;
		RingBuffer AudioRenderRingBuffer;
		FadeInFadeOut AudioRenderFadeInFadeOut;
		OMX_S64 TotalRecordedLen;
		OMX_U32 nDeviceReadOffset;
		OMX_BOOL bFirstFrame;
		OMX_BOOL bAddZeros;
		OMX_BOOL bSendEOS;
		OMX_CONFIG_BOOLEANTYPE EOS;
		OMX_TICKS nMaxDuration;
};

#endif
/* File EOF */
