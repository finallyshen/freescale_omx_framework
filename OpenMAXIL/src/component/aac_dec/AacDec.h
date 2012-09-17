/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AacDec.h
 *  @brief Class definition of AacDec Component
 *  @ingroup AacDec
 */

#ifndef AacDec_h
#define AacDec_h

#include "AudioFilter.h"
#include "aacd_dec_interface.h"
#include "aacplus_dec_interface.h"
#include "ShareLibarayMgr.h"

#define AACD_INPUT_BUF_PUSH_SIZE AACD_INPUT_BUFFER_SIZE

typedef struct FRAME_INFO
{
	OMX_U32 frm_size;
	OMX_U32 index;
	OMX_U32 b_rate;
	OMX_U32 flags;
	OMX_U32 total_frame_num;
	OMX_U32 total_bytes;
	OMX_U32 sampling_rate;
	OMX_U32 sample_per_fr;
	OMX_U32 samples;
	OMX_U32 layer;
	OMX_U32 version;
}FRAME_INFO;

class AacDec : public AudioFilter {
    public:
        AacDec();
    private:
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE AudioFilterInstanceInit();
        OMX_ERRORTYPE AudioFilterCodecInit();
        OMX_ERRORTYPE AudioFilterInstanceDeInit();
        OMX_ERRORTYPE AudioFilterGetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE AudioFilterSetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
		AUDIO_FILTERRETURNTYPE AudioFilterFrame();
		OMX_ERRORTYPE CheckFormatType(OMX_U8 *pBuffer, OMX_U32 nBufferLen);
		OMX_ERRORTYPE AudioFilterReset();
		OMX_ERRORTYPE AudioFilterCheckCodecConfig();

		ShareLibarayMgr *libMgr;
		OMX_PTR hLib;

		AACD_RET_TYPE (*aacpd_query_dec_mem)( AACD_Decoder_Config *);
		AACD_RET_TYPE (*SBRD_decoder_init)(AACD_Decoder_Config *);
		AACD_RET_TYPE (*SBRD_decode_frame)(
				AACD_Decoder_Config *,
				AACD_Decoder_info *,
				AACD_INT32 *,
				SBR_FRAME_TYPE *);

		OMX_AUDIO_PARAM_AACPROFILETYPE AacType;
		OMX_U32 nChannelsRaw;
		OMX_U32 nSampleRateRaw;
		AACD_Decoder_Config *pAacDecConfig;
		AACD_Decoder_info *pAacDecInfo;
		SBR_FRAME_TYPE SbrTypeFlag;
		FRAME_INFO FrameInfo;
		OMX_BOOL bGotFirstFrame;
		OMX_BOOL bHaveAACPlusLib;
		OMX_BOOL bStreamHeader;
		OMX_BOOL bAlwaysStereo;
		OMX_U32 HeadOffset;
};

#endif
/* File EOF */
