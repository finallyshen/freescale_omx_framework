/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Mp3Enc.h
 *  @brief Class definition of Mp3Enc Component
 *  @ingroup Mp3Enc
 */

#ifndef Mp3Enc_h
#define Mp3Enc_h

#include "AudioFilter.h"
#include "mp3_enc_interface.h"

class Mp3Enc : public AudioFilter {
    public:
        Mp3Enc();
    private:
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE AudioFilterInstanceInit();
        OMX_ERRORTYPE AudioFilterCodecInit();
        OMX_ERRORTYPE AudioFilterInstanceDeInit();
        OMX_ERRORTYPE AudioFilterGetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE AudioFilterSetParameterPCM();
        OMX_ERRORTYPE AudioFilterSetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
		AUDIO_FILTERRETURNTYPE AudioFilterFrame();
		OMX_ERRORTYPE AudioFilterReset();
        OMX_AUDIO_PARAM_MP3TYPE Mp3Type;
		MP3E_Encoder_Config *pMp3EncConfig;
		MP3E_Encoder_Parameter *pMp3EncParams;
		OMX_BOOL bNeedFlush;
		OMX_U8 *MemPtr[ENC_NUM_MEM_BLOCKS];
};

#endif
/* File EOF */
