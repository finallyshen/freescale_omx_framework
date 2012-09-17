/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Mem.h"
#include "Log.h"
#include "FadeInFadeOut.h"

FADEINFADEOUT_ERRORTYPE FadeInFadeOut::Create(fsl_osal_u32 Channels, fsl_osal_u32 \
        SamplingRate, fsl_osal_u32 BitsPerSample, fsl_osal_u32 ProcessLen, \
        fsl_osal_u32 FadeScale)
{
	FADEINFADEOUT_ERRORTYPE ret = FADEINFADEOUT_SUCCESS;

	if (FadeScale <= 1)
	{
		nFadeScale = FadeScale;
	}
	nFadeScaleStep = (fsl_osal_float)(1)/(ProcessLen/Channels/(BitsPerSample>>3));
	eFadeMode = FADENONE;
	nChannels = Channels;
	nSamplingRate = SamplingRate;
	nBitsPerSample = BitsPerSample;
	nAudioDataLen = 0;
	nProcessLen = ProcessLen;

	return ret;
}
FADEINFADEOUT_ERRORTYPE FadeInFadeOut::SetMode(FADE_MODE FadeMode)
{
	FADEINFADEOUT_ERRORTYPE ret = FADEINFADEOUT_SUCCESS;

	eFadeMode = FadeMode;

	return ret;
}

FADEINFADEOUT_ERRORTYPE FadeInFadeOut::SetAudioDataLen(fsl_osal_u32 AudioDataLen)
{
	FADEINFADEOUT_ERRORTYPE ret = FADEINFADEOUT_SUCCESS;

	nAudioDataLen = AudioDataLen;

	return ret;
}

FADEINFADEOUT_ERRORTYPE FadeInFadeOut::Process(fsl_osal_u8 *pBuffer, fsl_osal_u32 BufferLen)
{
	FADEINFADEOUT_ERRORTYPE ret = FADEINFADEOUT_SUCCESS;

	if(nProcessLen == 0)
		return FADEINFADEOUT_SUCCESS;

	switch(eFadeMode)
	{
	case FADENONE:
		break;
	case FADEIN:
		ret = FadeIn(pBuffer, BufferLen);
		LOG_DEBUG("Fade in scale = %f\n", nFadeScale);
		break;
	case FADEOUT:
		ret = FadeOut(pBuffer, BufferLen);
		LOG_DEBUG("Fade out scale = %f\n", nFadeScale);
		break;
	case FADEOUTALL:
		if (nAudioDataLen == 0)
			return FADEINFADEOUT_FAILURE;

		nAudioDataLen -= BufferLen;
		if (nAudioDataLen < nProcessLen)
		{
			eFadeMode = FADEOUT;
			ret = FadeOut(pBuffer + BufferLen - (nProcessLen - nAudioDataLen), \
			              nProcessLen - nAudioDataLen);
			nAudioDataLen = 0;
		}
		break;
	default:
		break;
	}

	return ret;
}

FADEINFADEOUT_ERRORTYPE FadeInFadeOut::FadeIn(fsl_osal_u8 *pBuffer, fsl_osal_u32 BufferLen)
{
	FADEINFADEOUT_ERRORTYPE ret = FADEINFADEOUT_SUCCESS;
	fsl_osal_u32 i, j, Len;
	if (nBitsPerSample == 0 || nChannels == 0)
		return FADEINFADEOUT_FAILURE;

	if (nFadeScale >= 1)
	{
		nFadeScale = 1;
		eFadeMode = FADENONE;
		return ret;
	}

	Len = BufferLen / (nBitsPerSample>>3) / nChannels;

	switch(nBitsPerSample)
	{
	case 8:
	{
		fsl_osal_s8 *pSrc = (fsl_osal_s8 *)pBuffer, *pDst = (fsl_osal_s8 *)pBuffer;
		fsl_osal_s8 Tmp;
		for (i = 0; i < Len; i ++)
		{
			for (j = 0; j < nChannels; j ++)
			{
				Tmp = pSrc[i*nChannels+j];
				Tmp = (fsl_osal_s8)(Tmp * nFadeScale);
				pDst[i*nChannels+j] = Tmp;
			}
			nFadeScale += nFadeScaleStep;
			if (nFadeScale > 1)
			{
				nFadeScale = 1;
				eFadeMode = FADENONE;
				break;
			}
		}
	}
	break;
	case 16:
	{
		fsl_osal_s16 *pSrc = (fsl_osal_s16 *)pBuffer, *pDst = (fsl_osal_s16 *)pBuffer;
		fsl_osal_s16 Tmp;
		for (i = 0; i < Len; i ++)
		{
			for (j = 0; j < nChannels; j ++)
			{
				Tmp = pSrc[i*nChannels+j];
				LOG_LOG("Fade in scale = %f\n", nFadeScale);
				Tmp = (fsl_osal_s16)(Tmp * nFadeScale);
				pDst[i*nChannels+j] = Tmp;
			}
			nFadeScale += nFadeScaleStep;
			if (nFadeScale > 1)
			{
				nFadeScale = 1;
				eFadeMode = FADENONE;
				break;
			}
		}
	}
	break;
	case 24:
	{
		fsl_osal_u8 *pSrc = (fsl_osal_u8 *)pBuffer, *pDst = (fsl_osal_u8 *)pBuffer;
		fsl_osal_s32 Tmp;
		for (i = 0; i < Len; i ++)
		{
			for (j = 0; j < nChannels; j ++)
			{
				Tmp = (((fsl_osal_u32)pSrc[(i*nChannels+j)*3]))|(((fsl_osal_u32)pSrc[(i*nChannels+j)*3+1])<<8)|(((fsl_osal_u32)pSrc[(i*nChannels+j)*3+2])<<16);
				if (Tmp&0x800000) Tmp |= 0xff000000;
				Tmp = (fsl_osal_s32)(Tmp * nFadeScale);
				pDst[(i*nChannels+j)*3] = Tmp;
				pDst[(i*nChannels+j)*3+1] = Tmp>>8;
				pDst[(i*nChannels+j)*3+2] = Tmp>>16;
			}
			nFadeScale += nFadeScaleStep;
			if (nFadeScale > 1)
			{
				nFadeScale = 1;
				eFadeMode = FADENONE;
				break;
			}
		}
	}
	break;
	}

	return ret;
}

FADEINFADEOUT_ERRORTYPE FadeInFadeOut::FadeOut(fsl_osal_u8 *pBuffer, fsl_osal_u32 BufferLen)
{
	FADEINFADEOUT_ERRORTYPE ret = FADEINFADEOUT_SUCCESS;
	fsl_osal_u32 i, j, Len;
	if (nBitsPerSample == 0 || nChannels == 0)
		return FADEINFADEOUT_FAILURE;

	Len = BufferLen / (nBitsPerSample>>3) / nChannels;

	switch(nBitsPerSample)
	{
	case 8:
	{
		fsl_osal_s8 *pSrc = (fsl_osal_s8 *)pBuffer, *pDst = (fsl_osal_s8 *)pBuffer;
		fsl_osal_s8 Tmp;
		for (i = 0; i < Len; i ++)
		{
			for (j = 0; j < nChannels; j ++)
			{
				Tmp = pSrc[i*nChannels+j];
				Tmp = (fsl_osal_s8)(Tmp * nFadeScale);
				pDst[i*nChannels+j] = Tmp;
			}
			nFadeScale -= nFadeScaleStep;
			if (nFadeScale < 0)
			{
				nFadeScale = 0;
				eFadeMode = FADEIN;
				break;
			}
		}
	}
	break;
	case 16:
	{
		fsl_osal_s16 *pSrc = (fsl_osal_s16 *)pBuffer, *pDst = (fsl_osal_s16 *)pBuffer;
		fsl_osal_s16 Tmp;
		for (i = 0; i < Len; i ++)
		{
			for (j = 0; j < nChannels; j ++)
			{
				Tmp = pSrc[i*nChannels+j];
				LOG_LOG("Fade out scale = %f\n", nFadeScale);
				LOG_LOG("Fade process len = %d\n", i);
				Tmp = (fsl_osal_s16)(Tmp * nFadeScale);
				pDst[i*nChannels+j] = Tmp;
			}
			nFadeScale -= nFadeScaleStep;
			if (nFadeScale < 0)
			{
				nFadeScale = 0;
				eFadeMode = FADEIN;
				break;
			}
		}
	}
	break;
	case 24:
	{
		fsl_osal_u8 *pSrc = (fsl_osal_u8 *)pBuffer, *pDst = (fsl_osal_u8 *)pBuffer;
		fsl_osal_s32 Tmp;
		for (i = 0; i < Len; i ++)
		{
			for (j = 0; j < nChannels; j ++)
			{
				Tmp = (((fsl_osal_u32)pSrc[(i*nChannels+j)*3]))|(((fsl_osal_u32)pSrc[(i*nChannels+j)*3+1])<<8)|(((fsl_osal_u32)pSrc[(i*nChannels+j)*3+2])<<16);
				if (Tmp&0x800000) Tmp |= 0xff000000;
				Tmp = (fsl_osal_s32)(Tmp * nFadeScale);
				pDst[(i*nChannels+j)*3] = Tmp;
				pDst[(i*nChannels+j)*3+1] = Tmp>>8;
				pDst[(i*nChannels+j)*3+2] = Tmp>>16;
			}
			nFadeScale -= nFadeScaleStep;
			if (nFadeScale < 0)
			{
				nFadeScale = 0;
				eFadeMode = FADEIN;
				break;
			}
		}
	}
	break;
	}

	return ret;
}


/* File EOF */
