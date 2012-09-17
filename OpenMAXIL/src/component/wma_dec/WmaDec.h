/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file WmaDec.h
 *  @brief Class definition of WmaDec Component
 *  @ingroup WmaDec
 */

#ifndef WmaDec_h
#define WmaDec_h

#include "AudioFilter.h"
#include "wma10_dec/wma10_dec_interface.h"
#include "ShareLibarayMgr.h"

class WmaDec : public AudioFilter {
    public:
        WmaDec();
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
		OMX_AUDIO_PARAM_WMATYPE WmaType;
		OMX_AUDIO_PARAM_WMATYPE_EXT WmaTypeExt;
		WMADDecoderConfig *pWmaDecConfig;
		WMADDecoderParams *pWmaDecParams;
		WAVEFORMATEXTENSIBLE sWfx;
		OMX_U32 dwChannelMask;    
		OMX_U32 nAdvancedEncodeOpt;
		OMX_U32 nAdvancedEncodeOpt2;

		ShareLibarayMgr *libMgr;
		OMX_PTR hLib;
		tWMAFileStatus (*pWMADQueryMem)    (WMADDecoderConfig *psDecConfig);

		tWMAFileStatus (*pInitWMADecoder)  (WMADDecoderConfig *psDecConfig,
                                                                        WMADDecoderParams *sDecParams,
                                                                        WMAD_UINT8 *pus8InputBuffer,
                                                                        WMAD_INT32 ps32InputBufferLength);

		tWMAFileStatus (*pWMADecodeFrame)  (WMADDecoderConfig *psDecConfig,
                                                                            WMADDecoderParams *sDecParams,
                                                                            WMAD_INT16 *iOUTBuffer,WMAD_INT32 lLength);

};

#endif
/* File EOF */
