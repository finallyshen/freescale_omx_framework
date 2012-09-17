/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file VorbisDec.h
 *  @brief Class definition of VorbisDec Component
 *  @ingroup VorbisDec
 */

#ifndef VorbisDec_h
#define VorbisDec_h

#include "AudioFilter.h"
#include "oggvorbis_dec_api.h"

class VorbisDec : public AudioFilter {
    public:
        VorbisDec();
    private:
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE AudioFilterInstanceInit();
        OMX_ERRORTYPE AudioFilterCodecInit();
        OMX_ERRORTYPE AudioFilterInstanceDeInit();
        OMX_ERRORTYPE AudioFilterGetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE AudioFilterSetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
		AUDIO_FILTERRETURNTYPE AudioFilterFrame();
		OMX_ERRORTYPE AudioFilterReset();
		OMX_ERRORTYPE AudioFilterCheckCodecConfig();
		OMX_AUDIO_PARAM_VORBISTYPE VorbisType;
		sOggVorbisDecObj *pVorbisDecConfig;
		OMX_U8 *pCodecConfigData;
		OMX_S32 nCodecConfigDataOffset;
		OMX_S32 nCodecConfigDataLen;
	
};

#endif
/* File EOF */
