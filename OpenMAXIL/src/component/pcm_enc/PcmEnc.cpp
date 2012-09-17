/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "PcmEnc.h"

PcmEnc::PcmEnc()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_encoder.pcm.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = "audio_encoder.pcm";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = PCMD_INPUT_BUF_PUSH_SIZE;
	nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE PcmEnc::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = AUDIO_FILTER_INPUT_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
    sPortDef.nBufferSize = 1024;
    ret = ports[AUDIO_FILTER_INPUT_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for port[%d] failed.\n", AUDIO_FILTER_INPUT_PORT);
        return ret;
    }

    sPortDef.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
    sPortDef.eDir = OMX_DirOutput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
    sPortDef.nBufferSize = PCMD_INPUT_BUF_PUSH_SIZE;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	PcmMode.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	OMX_INIT_STRUCT(&PcmModeOut, OMX_AUDIO_PARAM_PCMMODETYPE);

	PcmModeOut.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
	PcmModeOut.nChannels = 2;
	PcmModeOut.nSamplingRate = 44100;
	PcmModeOut.nBitPerSample = 16;
	PcmModeOut.bInterleaved = OMX_TRUE;
	PcmModeOut.eNumData = OMX_NumericalDataSigned;
	PcmModeOut.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmModeOut.eEndian = OMX_EndianLittle;
	PcmModeOut.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	TS_PerFrame = nPushModeInputLen/PcmMode.nChannels/(PcmMode.nBitPerSample>>3)*OMX_TICKS_PER_SECOND/PcmMode.nSamplingRate;

	return ret;
}

OMX_ERRORTYPE PcmEnc::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PcmEnc::AudioFilterInstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return ret;
}

OMX_ERRORTYPE PcmEnc::AudioFilterCodecInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return ret;
}

OMX_ERRORTYPE PcmEnc::AudioFilterInstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return ret;
}

OMX_ERRORTYPE PcmEnc::AudioFilterGetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE PcmEnc::AudioFilterSetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
		default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE PcmEnc::AudioFilterSetParameterPCM()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	LOG_DEBUG("PcmMode.nSamplingRate = %d\n", PcmMode.nSamplingRate);
	if (PcmModeOut.nChannels != PcmMode.nChannels
			|| PcmModeOut.nSamplingRate != PcmMode.nSamplingRate
			|| PcmModeOut.nBitPerSample != PcmMode.nBitPerSample)
	{
		PcmModeOut.nChannels = PcmMode.nChannels;
		PcmModeOut.nSamplingRate = PcmMode.nSamplingRate;
		PcmModeOut.nBitPerSample = PcmMode.nBitPerSample;
		nPushModeInputLen = nPushModeInputLen / ((PcmMode.nBitPerSample>>3) * PcmMode.nChannels) * ((PcmMode.nBitPerSample>>3) * PcmMode.nChannels);
		TS_PerFrame = nPushModeInputLen/PcmMode.nChannels/(PcmMode.nBitPerSample>>3)*OMX_TICKS_PER_SECOND/PcmMode.nSamplingRate;
		SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
	}

    return ret;
}

void PcmEnc::PcmProcess(OMX_U8 *pOutBuffer, OMX_U32 *pOutLen, OMX_U8 *pInBuffer, OMX_U32 nInLen)
{
	OMX_U32 i, j, Len;

	OMX_U32 nChannels = PcmMode.nChannels;

	Len = nInLen / (PcmMode.nBitPerSample>>3) / PcmMode.nChannels;

	switch(PcmMode.nBitPerSample)
	{
		case 8:
			break;
		case 16:
			{
				OMX_S16 *pSrc = (OMX_S16 *)pInBuffer, *pDst = (OMX_S16 *)pOutBuffer;
				OMX_S16 Tmp;
				for (i = 0; i < Len; i ++)
				{
					for (j = 0; j < nChannels; j ++)
					{
						Tmp = pSrc[i*nChannels+j];
						Tmp = ((((OMX_U16)Tmp)&0xFF)<<8) | ((((OMX_U16)Tmp)&0xFF00)>>8);
						pDst[i*nChannels+j] = Tmp;
					}
				}
			}
			break;
		case 24:
			{
				OMX_U8 *pSrc = (OMX_U8 *)pInBuffer, *pDst = (OMX_U8 *)pOutBuffer;
				OMX_S32 Tmp;
				for (i = 0; i < Len; i ++)
				{
					for (j = 0; j < nChannels; j ++)
					{
						Tmp = (((fsl_osal_u32)pSrc[(i*nChannels+j)*3]))|(((fsl_osal_u32)pSrc[(i*nChannels+j)*3+1])<<8)|(((fsl_osal_u32)pSrc[(i*nChannels+j)*3+2])<<16);
						pDst[(i*nChannels+j)*3] = Tmp>>16;
						pDst[(i*nChannels+j)*3+1] = Tmp>>8;
						pDst[(i*nChannels+j)*3+2] = Tmp;
					}
				}
			}
			break;
	}
	*pOutLen = nInLen;
}

AUDIO_FILTERRETURNTYPE PcmEnc::AudioFilterFrame()
{
    AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;

	AudioRingBuffer.BufferGet(&pBuffer, nPushModeInputLen, &nActuralLen);

	if (PcmMode.nBitPerSample == 8)
	{
		OMX_S8 *pDst = (OMX_S8 *)pOutBufferHdr->pBuffer;
		OMX_U8 *pSrc = pBuffer;
		OMX_S32 sTmp, i;
		for (i = 0; i < nActuralLen; i++)
		{
			sTmp = pSrc[i];
			sTmp -= 128;
			pDst[i] = (OMX_S8)sTmp;
		}
	}
	else
	{
		if (PcmMode.eEndian == OMX_EndianBig)
		{
			OMX_U32 OutLen;
			PcmProcess(pOutBufferHdr->pBuffer, &OutLen, pBuffer, nActuralLen);
			LOG_DEBUG("Big endian.\n");
		}
		else
		{
			fsl_osal_memcpy((fsl_osal_ptr)pOutBufferHdr->pBuffer, (fsl_osal_ptr)pBuffer, nActuralLen);
		}
	}
	pOutBufferHdr->nFilledLen = nActuralLen;

	AudioRingBuffer.BufferConsumered(nActuralLen);
	LOG_LOG("PcmEnc process frame len: %d\n", nActuralLen);

	if (nActuralLen > 0)
	{
		LOG_DEBUG("PcmMode.nSamplingRate = %d\n", PcmMode.nSamplingRate);
		if (PcmModeOut.nChannels != PcmMode.nChannels
				|| PcmModeOut.nSamplingRate != PcmMode.nSamplingRate
				|| PcmModeOut.nBitPerSample != PcmMode.nBitPerSample)
		{
			PcmModeOut.nChannels = PcmMode.nChannels;
			PcmModeOut.nSamplingRate = PcmMode.nSamplingRate;
			PcmModeOut.nBitPerSample = PcmMode.nBitPerSample;
			nPushModeInputLen = nPushModeInputLen / ((PcmMode.nBitPerSample>>3) * PcmMode.nChannels) * ((PcmMode.nBitPerSample>>3) * PcmMode.nChannels);
			TS_PerFrame = nPushModeInputLen/PcmMode.nChannels/(PcmMode.nBitPerSample>>3)*OMX_TICKS_PER_SECOND/PcmMode.nSamplingRate;
			SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
		}

		pOutBufferHdr->nOffset = 0;

		LOG_LOG("TS per Frame = %lld\n", TS_PerFrame);

		AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 
	}
	else
	{
		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = 0;

		ret = AUDIO_FILTER_EOS;
	}

	LOG_LOG("Encoder nTimeStamp = %lld\n", pOutBufferHdr->nTimeStamp);

    return ret;
}

OMX_ERRORTYPE PcmEnc::AudioFilterReset()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return ret;
}

OMX_ERRORTYPE PcmEnc::AudioFilterCheckCodecConfig()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	OMX_AUDIO_PARAM_PCMMODETYPE *pPcmType;
	pPcmType = (OMX_AUDIO_PARAM_PCMMODETYPE*)pInBufferHdr->pBuffer;
	OMX_CHECK_STRUCT(pPcmType, OMX_AUDIO_PARAM_PCMMODETYPE, ret);
	if(ret != OMX_ErrorNone)
		return ret;

	PcmMode.nChannels = pPcmType->nChannels;
	PcmMode.nSamplingRate = pPcmType->nSamplingRate;
	PcmMode.nBitPerSample = pPcmType->nBitPerSample;
	nPushModeInputLen = nPushModeInputLen / ((PcmMode.nBitPerSample>>3) * PcmMode.nChannels) * ((PcmMode.nBitPerSample>>3) * PcmMode.nChannels);
	TS_PerFrame = nPushModeInputLen/PcmMode.nChannels/(PcmMode.nBitPerSample>>3)*OMX_TICKS_PER_SECOND/PcmMode.nSamplingRate;

	LOG_DEBUG("Get PCM encoder config. Channel: %d, Sample Rate: %d, BitsPerSample: %d\n", \
			PcmMode.nChannels, PcmMode.nSamplingRate, PcmMode.nBitPerSample);

	pInBufferHdr->nFilledLen = 0;

    return ret;
}
 
/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE PcmEncInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        PcmEnc *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(PcmEnc, ());
        if(obj == NULL)
            return OMX_ErrorInsufficientResources;

        base = (ComponentBase*)obj;
        ret = base->ConstructComponent(pHandle);
        if(ret != OMX_ErrorNone)
            return ret;

        return ret;
    }
}

/* File EOF */
