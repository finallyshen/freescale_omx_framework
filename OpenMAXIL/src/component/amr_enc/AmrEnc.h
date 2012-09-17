/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AmrEnc.h
 *  @brief Class definition of AmrEnc Component
 *  @ingroup AmrEnc
 */

#ifndef AmrEnc_h
#define AmrEnc_h

#include "AudioFilter.h"
#include "AmrEncWrapper.h"

/** AMR band mode */
typedef enum AMRBANDMODETYPE {
	AMRBandModeUnused = 0,
	AMRBandModeNB,      
	AMRBandModeWB
} AMRBANDMODETYPE;

class AmrEnc : public AudioFilter {
    public:
        AmrEnc();
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
        OMX_AUDIO_PARAM_AMRTYPE AmrType;
        AMRBANDMODETYPE AmrBandMode;
        AmrEncWrapper *pEncWrapper;
};

#endif
/* File EOF */
