/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AmrDec.h
 *  @brief Class definition of AmrDec Component
 *  @ingroup AmrDec
 */

#ifndef AmrDec_h
#define AmrDec_h

#include "AudioFilter.h"
#include "AmrDecWrapper.h"

/** AMR band mode */
typedef enum AMRBANDMODETYPE {
	AMRBandModeUnused = 0,
	AMRBandModeNB,      
	AMRBandModeWB
} AMRBANDMODETYPE;

class AmrDec : public AudioFilter {
    public:
        AmrDec();
    private:
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE AudioFilterInstanceInit();
        OMX_ERRORTYPE AudioFilterCodecInit();
        OMX_ERRORTYPE AudioFilterInstanceDeInit();
        OMX_ERRORTYPE AudioFilterGetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE AudioFilterSetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        AUDIO_FILTERRETURNTYPE AudioFilterFrame();
        OMX_ERRORTYPE AudioFilterCheckCodecConfig();
        OMX_ERRORTYPE AudioFilterReset();
        
        OMX_AUDIO_PARAM_AMRTYPE AmrType;
        AMRBANDMODETYPE AmrBandMode;
        AmrDecWrapper *pDecWrapper;
};

#endif
/* File EOF */
