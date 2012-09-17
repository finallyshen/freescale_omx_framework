/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Mp3Dec.h
 *  @brief Class definition of Mp3Dec Component
 *  @ingroup Mp3Dec
 */

#ifndef Mp3Dec_h
#define Mp3Dec_h

#include "AudioFilter.h"
#include "mp3_dec_interface.h"

class Mp3Dec : public AudioFilter {
    public:
        Mp3Dec();
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
		OMX_ERRORTYPE AudioFilterCheckFrameHeader();
              OMX_BOOL IsValidFrameHeader(OMX_U8 * pHeader);
        OMX_AUDIO_PARAM_MP3TYPE Mp3Type;
		MP3D_Decode_Config *pMp3DecConfig;
		MP3D_Decode_Params *pMp3DecParams;
};

#endif
/* File EOF */
