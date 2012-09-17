/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AudioProcessor.h
 *  @brief Class definition of AudioProcessor Component
 *  @ingroup AudioProcessor
 */

#ifndef AudioProcessor_h
#define AudioProcessor_h

#include "AudioFilter.h"
#include "FadeInFadeOut.h"
#include "peq_ppp_interface.h"

#define POSTPROCESSFADEPROCESSTIME 50
#define	PROCESSOR_INPUT_FRAME_SIZE 1024
#define	PROCESSOR_OUTPUT_FRAME_SIZE 1024

typedef enum {
    AUDIO_POSTPROCESS_SUCCESS,
    AUDIO_POSTPROCESS_EOS,
    AUDIO_POSTPROCESS_FAILURE,
}AUDIO_POSTPROCESSRETURNTYPE;


class AudioProcessor : public AudioFilter {
    public:
        AudioProcessor();
    private:
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE AudioFilterInstanceInit();
		OMX_ERRORTYPE AudioFilterInstanceReset();
        OMX_ERRORTYPE AudioFilterCodecInit();
        OMX_ERRORTYPE AudioFilterInstanceDeInit();
        OMX_ERRORTYPE AudioFilterGetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
		OMX_ERRORTYPE AudioFilterSetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE AudioFilterSetParameterPCM();
		AUDIO_FILTERRETURNTYPE AudioFilterFrame();
		OMX_ERRORTYPE AudioFilterReset();
		AUDIO_POSTPROCESSRETURNTYPE VolumeInit(OMX_U32 Channels, OMX_U32 SamplingRate, OMX_U32 BitsPerSample, OMX_U32 nVolume);
		AUDIO_POSTPROCESSRETURNTYPE VolumeSetConfig(OMX_U32 Channels, OMX_U32 SamplingRate, OMX_U32 BitsPerSample, OMX_U32 nVolume);
		AUDIO_POSTPROCESSRETURNTYPE VolumeProcess(OMX_U8 *pOutBuffer, OMX_U32 *pOutLen, OMX_U8 *pInBuffer, OMX_U32 nInLen);
		OMX_ERRORTYPE CheckVolumeConfig();
        OMX_ERRORTYPE ComponentReturnBuffer(OMX_U32 nPortIndex);
		FadeInFadeOut AudioRenderFadeInFadeOut;
		OMX_AUDIO_CONFIG_VOLUMETYPE Volume;
		OMX_U32 nFadeInFadeOutProcessLen;
		OMX_U32 nFadeOutProcessLen;
		OMX_AUDIO_CONFIG_POSTPROCESSTYPE PostProcess; 
		OMX_AUDIO_PARAM_PCMMODETYPE PcmModeOut; 
		OMX_AUDIO_CONFIG_EQUALIZERTYPE PEQ[BANDSINGRP];
		PEQ_PPP_Config *pPeqConfig;
		PEQ_INFO *pPeqInfo;
		PEQ_PL *pPeqPl;
		OMX_U32 nSampleSize;
		OMX_BOOL bSetConfig;
		OMX_BOOL bPostProcess;
		OMX_BOOL bNeedChange;
		OMX_BOOL bPEQEnable;
		OMX_U32 nPostProcessLen;
		OMX_U32 nPostProcessLenFrame;
		fsl_osal_float Scale;
};

#endif
/* File EOF */
