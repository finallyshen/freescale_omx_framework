/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AacEnc.h
 *  @brief Class definition of AacEnc Component
 *  @ingroup AacEnc
 */

#ifndef AacEnc_h
#define AacEnc_h

#include "ShareLibarayMgr.h"
#include "AudioFilter.h"
#include "voAAC.h"

class AacEnc : public AudioFilter {
    public:
        AacEnc();
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
        OMX_AUDIO_PARAM_AACPROFILETYPE AacType;
        void *mEncoderHandle;
        VO_AUDIO_CODECAPI *mApiHandle;
		VO_MEM_OPERATOR* mMemOperator;
		ShareLibarayMgr *libMgr;
		OMX_PTR hLibStagefright;
		OMX_BOOL bFirstFrame;
};

#endif
/* File EOF */
