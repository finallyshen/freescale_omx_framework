/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "AmrEnc.h"

AmrEnc::AmrEnc()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_encoder.amr.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_encoder.amr";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = MAX(NB_INPUT_BUF_PUSH_SIZE, WB_INPUT_BUF_PUSH_SIZE);
	nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE AmrEnc::InitComponent()
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
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingAMR;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
    sPortDef.nBufferSize = nPushModeInputLen;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&AmrType, OMX_AUDIO_PARAM_AMRTYPE);
	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	AmrType.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
	AmrType.nChannels = 1;
    AmrType.nBitRate = 4750;
    AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeNB0; 
    AmrType.eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
    AmrType.eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;

	PcmMode.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	PcmMode.nChannels = 1;
	PcmMode.nSamplingRate = 8000;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

    AmrBandMode = AMRBandModeNB;
    pEncWrapper = NULL;

	return ret;
}

OMX_ERRORTYPE AmrEnc::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AmrEnc::AudioFilterInstanceInit()
{
	TS_PerFrame = pEncWrapper->getTsPerFrame(PcmMode.nSamplingRate);

    if(pEncWrapper != NULL)
        return pEncWrapper->InstanceInit();
    else
        return OMX_ErrorUndefined;
}

OMX_ERRORTYPE AmrEnc::AudioFilterCodecInit()
{
    if(pEncWrapper != NULL)
        return pEncWrapper->CodecInit(&AmrType);
    else
        return OMX_ErrorUndefined;
}

OMX_ERRORTYPE AmrEnc::AudioFilterInstanceDeInit()
{
    if(pEncWrapper != NULL){
        pEncWrapper->InstanceDeInit();
        FSL_DELETE(pEncWrapper);
        pEncWrapper = NULL;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AmrEnc::AudioFilterGetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioAmr:
            {
                OMX_AUDIO_PARAM_AMRTYPE *pAmrType;
                pAmrType = (OMX_AUDIO_PARAM_AMRTYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pAmrType, OMX_AUDIO_PARAM_AMRTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pAmrType->nPortIndex != AUDIO_FILTER_OUTPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(pAmrType, &AmrType,	sizeof(OMX_AUDIO_PARAM_AMRTYPE));
            }
			return ret;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE AmrEnc::AudioFilterSetParameterPCM()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (AmrType.nChannels != PcmMode.nChannels
			|| PcmMode.nSamplingRate != PcmMode.nSamplingRate)
	{
		AmrType.nChannels = PcmMode.nChannels;
		PcmMode.nSamplingRate = PcmMode.nSamplingRate;
		SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
	}

    return ret;
}

OMX_ERRORTYPE AmrEnc::AudioFilterSetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioAmr:
            {
                OMX_AUDIO_PARAM_AMRTYPE *pAmrType;
                pAmrType = (OMX_AUDIO_PARAM_AMRTYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pAmrType, OMX_AUDIO_PARAM_AMRTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pAmrType->nPortIndex != AUDIO_FILTER_OUTPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(&AmrType, pAmrType, sizeof(OMX_AUDIO_PARAM_AMRTYPE));

				AmrBandMode = AMRBandModeNB;
				if (AmrType.eAMRBandMode >= OMX_AUDIO_AMRBandModeWB0) 
					AmrBandMode = AMRBandModeWB;

				if (AmrBandMode == AMRBandModeNB) {
					pEncWrapper = (AmrEncWrapper *)FSL_NEW(NbAmrEncWrapper, ());
					if (AmrType.nBitRate >= 12200) {
						AmrType.nBitRate = 12200;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeNB7;
					} else if (AmrType.nBitRate >= 10200) {
						AmrType.nBitRate = 10200;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeNB6;
					} else if (AmrType.nBitRate >= 7950) {
						AmrType.nBitRate = 7950;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeNB5;
					} else if (AmrType.nBitRate >= 7400) {
						AmrType.nBitRate = 7400;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeNB4;
					} else if (AmrType.nBitRate >= 6700) {
						AmrType.nBitRate = 6700;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeNB3;
					} else if (AmrType.nBitRate >= 5900) {
						AmrType.nBitRate = 5900;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeNB2;
					} else if (AmrType.nBitRate >= 5150) {
						AmrType.nBitRate = 5150;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeNB1;
					} else {
						AmrType.nBitRate = 4750;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeNB0;
					}
				}
				else {
					pEncWrapper = (AmrEncWrapper *)FSL_NEW(WbAmrEncWrapper, ());
					if (AmrType.nBitRate >= 23850) {
						AmrType.nBitRate = 23850;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeWB8;
					} else if (AmrType.nBitRate >= 23050) {
						AmrType.nBitRate = 23050;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeWB7;
					} else if (AmrType.nBitRate >= 19850) {
						AmrType.nBitRate = 19850;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeWB6;
					} else if (AmrType.nBitRate >= 18250) {
						AmrType.nBitRate = 18250;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeWB5;
					} else if (AmrType.nBitRate >= 15850) {
						AmrType.nBitRate = 15850;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeWB4;
					} else if (AmrType.nBitRate >= 14250) {
						AmrType.nBitRate = 14250;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeWB3;
					} else if (AmrType.nBitRate >= 12650) {
						AmrType.nBitRate = 12650;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeWB2;
					} else if (AmrType.nBitRate >= 8850) {
						AmrType.nBitRate = 8850;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeWB1;
					} else {
						AmrType.nBitRate = 6600;
						AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeWB0;
					}
				}
			}
			return ret;
		default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

AUDIO_FILTERRETURNTYPE AmrEnc::AudioFilterFrame()
{
    AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
	OMX_U32 nEncoderFrameLen = pEncWrapper->getInputBufferPushSize();
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;

	AudioRingBuffer.BufferGet(&pBuffer, nEncoderFrameLen, &nActuralLen);
	LOG_LOG("Amr encoder get stream len: %d\n", nActuralLen);
 
	if (nActuralLen == 0) {
		ret = AUDIO_FILTER_EOS;
		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = 0;
		return ret;
	} else {
		if (nActuralLen < nEncoderFrameLen) {
			OMX_U32 nActuralLenZero;
			AudioRingBuffer.BufferAddZeros(nEncoderFrameLen - nActuralLen, &nActuralLenZero);
			AudioRingBuffer.BufferGet(&pBuffer, nEncoderFrameLen, &nActuralLen);
		}
	}
    pEncWrapper->encodeFrame(pOutBufferHdr->pBuffer, &(pOutBufferHdr->nFilledLen), pBuffer, &nActuralLen);

	LOG_LOG("Amr consumed len: %d\n", nActuralLen);
	AudioRingBuffer.BufferConsumered(nActuralLen);

	pOutBufferHdr->nOffset = 0;
		
	LOG_LOG("Amr stream len: %d\n", pOutBufferHdr->nFilledLen);
	LOG_LOG("nSampleRate: %d nChannels: %d\n", PcmMode.nSamplingRate, PcmMode.nChannels);

	LOG_LOG("TS per Frame = %lld\n", TS_PerFrame);

	AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 

	LOG_LOG("Encoder nTimeStamp = %lld\n", pOutBufferHdr->nTimeStamp);

    return ret;
}

OMX_ERRORTYPE AmrEnc::AudioFilterReset()
{
    return AudioFilterCodecInit();
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE AmrEncInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        AmrEnc *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(AmrEnc, ());
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
