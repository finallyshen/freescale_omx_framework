/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file PcmDec.h
 *  @brief Class definition of PcmDec Component
 *  @ingroup PcmDec
 */

#ifndef PcmDec_h
#define PcmDec_h

#include "AudioFilter.h"

#define PCMD_INPUT_BUF_PUSH_SIZE 1024
#define PCMD_INPUT_BUF_PUSH_SIZE 1024

class PcmDec : public AudioFilter {
    public:
        PcmDec();
    private:
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE AudioFilterInstanceInit();
        OMX_ERRORTYPE AudioFilterCodecInit();
        OMX_ERRORTYPE AudioFilterInstanceDeInit();
        OMX_ERRORTYPE AudioFilterGetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE AudioFilterSetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
		OMX_ERRORTYPE AudioFilterSetParameterPCM();
		void PcmProcess(OMX_U8 *pOutBuffer, OMX_U32 *pOutLen, OMX_U8 *pInBuffer, OMX_U32 nInLen);
		AUDIO_FILTERRETURNTYPE AudioFilterFrame();
		OMX_ERRORTYPE AudioFilterCheckCodecConfig();
		OMX_ERRORTYPE AudioFilterReset();
		OMX_AUDIO_PARAM_PCMMODETYPE PcmModeOut; 
};

#endif
/* File EOF */
