/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file FlacDec.h
 *  @brief Class definition of FlacDec Component
 *  @ingroup FlacDec
 */

#ifndef FlacDec_h
#define FlacDec_h

#include "AudioFilter.h"
#include "flac_dec_interface.h"

class FlacDec : public AudioFilter {
    public:
        FlacDec();
		OMX_U8 *pInBuffer;
    private:
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE AudioFilterInstanceInit();
        OMX_ERRORTYPE AudioFilterCodecInit();
        OMX_ERRORTYPE AudioFilterInstanceDeInit();
        OMX_ERRORTYPE AudioFilterGetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE AudioFilterSetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
		OMX_ERRORTYPE AudioFilterCheckFrameHeader();
		AUDIO_FILTERRETURNTYPE AudioFilterFrame();
		OMX_ERRORTYPE AudioFilterReset();
		OMX_ERRORTYPE AudioFilterCheckCodecConfig();
		OMX_AUDIO_PARAM_FLACTYPE FlacType;
		FLACD_Decode_Config *pFlacDecConfig;
		OMX_U8 *pOutBuffer;
};

#endif
/* File EOF */
