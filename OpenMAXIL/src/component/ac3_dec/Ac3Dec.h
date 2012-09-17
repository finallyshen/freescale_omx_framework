/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Ac3Dec.h
 *  @brief Class definition of Ac3Dec Component
 *  @ingroup Ac3Dec
 */

#ifndef Ac3Dec_h
#define Ac3Dec_h

#include "AudioFilter.h"
#include "ac3d_dec_interface.h"
#include "ShareLibarayMgr.h"

class Ac3Dec : public AudioFilter {
    public:
        Ac3Dec();
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
		OMX_ERRORTYPE AudioFilterCheckFrameHeader();
		OMX_AUDIO_PARAM_AC3TYPE Ac3Type;
		AC3DDecoderConfig *pAc3DecConfig;
		OMX_U64 nSkip;

		ShareLibarayMgr *libMgr;
		OMX_PTR hLib;
		AC3D_RET_TYPE (*pAC3D_QueryMem)(AC3DDecoderConfig *);
		AC3D_RET_TYPE (*pAC3D_dec_init)(AC3DDecoderConfig *, AC3D_UINT8 *, AC3D_INT32 );
		AC3D_RET_TYPE (*pAC3D_dec_Frame)(AC3DDecoderConfig *, AC3D_INT32 *, AC3D_INT16 *, AC3D_INT32 );

};

#endif
/* File EOF */
