/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file FadeInFadeOut.h
 *  @brief Class definition of fade in fade out.
 *  @ingroup FadeInFadeOut
 */


#ifndef FadeInFadeOut_h
#define FadeInFadeOut_h

#include "fsl_osal.h"

typedef enum {
    FADEINFADEOUT_SUCCESS,
    FADEINFADEOUT_FAILURE,
    FADEINFADEOUT_INSUFFICIENT_RESOURCES
}FADEINFADEOUT_ERRORTYPE;

typedef enum {
    FADENONE,
    FADEIN,
	FADEOUT,
    FADEOUTALL
}FADE_MODE;

class FadeInFadeOut {
	public:
		FADEINFADEOUT_ERRORTYPE Create(fsl_osal_u32 Channels, fsl_osal_u32 SamplingRate, \
				fsl_osal_u32 BitsPerSample, fsl_osal_u32 ProcessLen, fsl_osal_u32 FadeScale);
		FADEINFADEOUT_ERRORTYPE SetMode(FADE_MODE FadeMode);
		FADEINFADEOUT_ERRORTYPE SetAudioDataLen(fsl_osal_u32 AudioDataLen);
		FADEINFADEOUT_ERRORTYPE Process(fsl_osal_u8 *pBuffer, fsl_osal_u32 BufferLen);
		FADEINFADEOUT_ERRORTYPE FadeIn(fsl_osal_u8 *pBuffer, fsl_osal_u32 BufferLen);
		FADEINFADEOUT_ERRORTYPE FadeOut(fsl_osal_u8 *pBuffer, fsl_osal_u32 BufferLen);
	private:
		fsl_osal_float nFadeScale;
		fsl_osal_float nFadeScaleStep;
		FADE_MODE eFadeMode;
		fsl_osal_u32 nChannels;
		fsl_osal_u32 nSamplingRate;
		fsl_osal_u32 nBitsPerSample;
		fsl_osal_u32 nAudioDataLen;
		fsl_osal_u32 nProcessLen;
};

#endif
/* File EOF */
