/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "AmrDec.h"

//#define LOG_DEBUG printf
//#define LOG_ERROR printf

//#define max(a,b) (a)>(b)?(a):(b)


AmrDec::AmrDec()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_decoder.amr.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_decoder.amr";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
    nPushModeInputLen = NB_INPUT_BUF_PUSH_SIZE; 
    nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE AmrDec::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = AUDIO_FILTER_INPUT_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingAMR;
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
    sPortDef.nBufferSize = FRAMESPERLOOP * L_FRAME * sizeof(NBAMR_S16);
    ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
        return ret;
    }

    OMX_INIT_STRUCT(&AmrType, OMX_AUDIO_PARAM_AMRTYPE);
    OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

    AmrType.nPortIndex = AUDIO_FILTER_INPUT_PORT;
    AmrType.nChannels = 1;
    AmrType.nBitRate = 0;
    AmrType.eAMRBandMode = OMX_AUDIO_AMRBandModeUnused; 
    AmrType.eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
    AmrType.eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatConformance;

    PcmMode.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
    PcmMode.nChannels = 1;
    PcmMode.nSamplingRate = 8000;
    PcmMode.nBitPerSample = 16;
    PcmMode.bInterleaved = OMX_TRUE;
    PcmMode.eNumData = OMX_NumericalDataSigned;
    PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
    PcmMode.eEndian = OMX_EndianLittle;
    PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

    AmrBandMode = AMRBandModeNB;
    TS_PerFrame = FRAMESPERLOOP * L_FRAME * OMX_TICKS_PER_SECOND/PcmMode.nSamplingRate;

    pDecWrapper = NULL;

    return ret;
}

OMX_ERRORTYPE AmrDec::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AmrDec::AudioFilterInstanceInit()
{
    if(pDecWrapper != NULL)
        return pDecWrapper->InstanceInit();
    else
        return OMX_ErrorUndefined;
}

OMX_ERRORTYPE AmrDec::AudioFilterCodecInit()
{
    if(pDecWrapper != NULL)
        return pDecWrapper->CodecInit(&AmrType);
    else
        return OMX_ErrorUndefined;
}

OMX_ERRORTYPE AmrDec::AudioFilterInstanceDeInit()
{
    if(pDecWrapper != NULL){
        pDecWrapper->InstanceDeInit();
        FSL_DELETE(pDecWrapper);
        pDecWrapper = NULL;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AmrDec::AudioFilterGetParameter(
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
				if (pAmrType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
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

OMX_ERRORTYPE AmrDec::AudioFilterSetParameter(
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
                if (pAmrType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
		{
			ret = OMX_ErrorBadPortIndex;
			break;
		}

		OMX_U32 nSampleRateIn;
		switch(pAmrType->eAMRBandMode) 
		{
			case OMX_AUDIO_AMRBandModeNB0:             /**< AMRNB Mode 0 =  4750 bps */
			case OMX_AUDIO_AMRBandModeNB1:             /**< AMRNB Mode 1 =  5150 bps */
			case OMX_AUDIO_AMRBandModeNB2:             /**< AMRNB Mode 2 =  5900 bps */ 
			case OMX_AUDIO_AMRBandModeNB3:             /**< AMRNB Mode 3 =  6700 bps */
			case OMX_AUDIO_AMRBandModeNB4:             /**< AMRNB Mode 4 =  7400 bps */
			case OMX_AUDIO_AMRBandModeNB5:             /**< AMRNB Mode 5 =  7950 bps */
			case OMX_AUDIO_AMRBandModeNB6:             /**< AMRNB Mode 6 = 10200 bps */
			case OMX_AUDIO_AMRBandModeNB7:             /**< AMRNB Mode 7 = 12200 bps */
				{
					LOG_DEBUG("AMR NB mode.\n");
					AmrBandMode = AMRBandModeNB;
					nSampleRateIn = 8000;
                                   pDecWrapper = (AmrDecWrapper *)FSL_NEW(NbAmrDecWrapper, ());
   				}
				break;
			case OMX_AUDIO_AMRBandModeWB0:             /**< AMRWB Mode 0 =  6600 bps */
			case OMX_AUDIO_AMRBandModeWB1:             /**< AMRWB Mode 1 =  8850 bps */
			case OMX_AUDIO_AMRBandModeWB2:             /**< AMRWB Mode 2 = 12650 bps */ 
			case OMX_AUDIO_AMRBandModeWB3:             /**< AMRWB Mode 3 = 14250 bps */ 
			case OMX_AUDIO_AMRBandModeWB4:             /**< AMRWB Mode 4 = 15850 bps */
			case OMX_AUDIO_AMRBandModeWB5:             /**< AMRWB Mode 5 = 18250 bps */
			case OMX_AUDIO_AMRBandModeWB6:             /**< AMRWB Mode 6 = 19850 bps */
			case OMX_AUDIO_AMRBandModeWB7:             /**< AMRWB Mode 7 = 23050 bps */
			case OMX_AUDIO_AMRBandModeWB8:             /**< AMRWB Mode 8 = 23850 bps */ 
				{
					LOG_DEBUG("AMR WB mode.\n");
					AmrBandMode = AMRBandModeWB;
					nSampleRateIn = 16000;
                                   pDecWrapper = (AmrDecWrapper *)FSL_NEW(WbAmrDecWrapper, ());
				}
				break;
			default:
				return OMX_ErrorBadParameter;
		}

		if (PcmMode.nSamplingRate != nSampleRateIn)
		{
			PcmMode.nSamplingRate = nSampleRateIn;
			TS_PerFrame = pDecWrapper->getTsPerFrame(PcmMode.nSamplingRate);
			LOG_DEBUG("Decoder Sample Rate = %d\t TS per Frame = %lld\n", \
					PcmMode.nSamplingRate, TS_PerFrame);
			SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
		}

		fsl_osal_memcpy(&AmrType, pAmrType, sizeof(OMX_AUDIO_PARAM_AMRTYPE));

              LOG_DEBUG("AmrType.eAMRFrameFormat %d\n", AmrType.eAMRFrameFormat);

        
		}
		break;

	default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}
 
AUDIO_FILTERRETURNTYPE AmrDec::AudioFilterFrame()
{
    AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
    OMX_ERRORTYPE decRet = OMX_ErrorNone;
    OMX_U8 *pBuffer;
    OMX_U32 nActualLen;
    OMX_U32 consumedSize = 0;

    AudioRingBuffer.BufferGet(&pBuffer, pDecWrapper->getInputBufferPushSize(), &nActualLen);
    LOG_DEBUG("Amr decoder get stream len: %d\n", nActualLen);

    if (nActualLen == 0)
    {
        pOutBufferHdr->nOffset = 0;
        pOutBufferHdr->nFilledLen = 0;

        return AUDIO_FILTER_EOS;
    }

    decRet = pDecWrapper->decodeFrame(pBuffer, pOutBufferHdr->pBuffer, &AmrType, nActualLen, &consumedSize);

    LOG_DEBUG("Audio stream consumed len = %d\n", consumedSize);
    AudioRingBuffer.BufferConsumered(consumedSize);
    
    LOG_DEBUG("AmrRet = %d\n", ret);
    if (decRet == OMX_ErrorNone)
    {
        pOutBufferHdr->nOffset = 0;
        pOutBufferHdr->nFilledLen = pDecWrapper->getFrameOutputSize();

        //LOG_DEBUG("TS per Frame = %lld\n", TS_PerFrame);

        AmrType.nBitRate = pDecWrapper->getBitRate(consumedSize);
        AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 
    }
    else
    {
        ret = AUDIO_FILTER_FAILURE;
    }

    //LOG_DEBUG("Decoder nTimeStamp = %lld\n", pOutBufferHdr->nTimeStamp);

    return ret;
}

OMX_ERRORTYPE AmrDec::AudioFilterReset()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	ret = AudioFilterCodecInit();

    return ret;
}

OMX_ERRORTYPE AmrDec::AudioFilterCheckCodecConfig()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_AMRTYPE *pAmrType;
	pAmrType = (OMX_AUDIO_PARAM_AMRTYPE*)pInBufferHdr->pBuffer;
	OMX_CHECK_STRUCT(pAmrType, OMX_AUDIO_PARAM_AMRTYPE, ret);
	if(ret != OMX_ErrorNone)
	{
		pInBufferHdr->nFilledLen = 0;
		return ret;
	}

	if (pAmrType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
	{
		pInBufferHdr->nFilledLen = 0;
		return ret;
	}

	OMX_U32 nSampleRateIn;
	switch(pAmrType->eAMRBandMode) 
	{
		case OMX_AUDIO_AMRBandModeNB0:             /**< AMRNB Mode 0 =  4750 bps */
		case OMX_AUDIO_AMRBandModeNB1:             /**< AMRNB Mode 1 =  5150 bps */
		case OMX_AUDIO_AMRBandModeNB2:             /**< AMRNB Mode 2 =  5900 bps */ 
		case OMX_AUDIO_AMRBandModeNB3:             /**< AMRNB Mode 3 =  6700 bps */
		case OMX_AUDIO_AMRBandModeNB4:             /**< AMRNB Mode 4 =  7400 bps */
		case OMX_AUDIO_AMRBandModeNB5:             /**< AMRNB Mode 5 =  7950 bps */
		case OMX_AUDIO_AMRBandModeNB6:             /**< AMRNB Mode 6 = 10200 bps */
		case OMX_AUDIO_AMRBandModeNB7:             /**< AMRNB Mode 7 = 12200 bps */
			{
				AmrBandMode = AMRBandModeNB;
				nSampleRateIn = 8000;
			}
			break;
		case OMX_AUDIO_AMRBandModeWB0:             /**< AMRWB Mode 0 =  6600 bps */
		case OMX_AUDIO_AMRBandModeWB1:             /**< AMRWB Mode 1 =  8850 bps */
		case OMX_AUDIO_AMRBandModeWB2:             /**< AMRWB Mode 2 = 12650 bps */ 
		case OMX_AUDIO_AMRBandModeWB3:             /**< AMRWB Mode 3 = 14250 bps */ 
		case OMX_AUDIO_AMRBandModeWB4:             /**< AMRWB Mode 4 = 15850 bps */
		case OMX_AUDIO_AMRBandModeWB5:             /**< AMRWB Mode 5 = 18250 bps */
		case OMX_AUDIO_AMRBandModeWB6:             /**< AMRWB Mode 6 = 19850 bps */
		case OMX_AUDIO_AMRBandModeWB7:             /**< AMRWB Mode 7 = 23050 bps */
		case OMX_AUDIO_AMRBandModeWB8:             /**< AMRWB Mode 8 = 23850 bps */ 
			{
				AmrBandMode = AMRBandModeWB;
				nSampleRateIn = 16000;
			}
			break;
		default:
			return OMX_ErrorBadParameter;
	}

	if (PcmMode.nSamplingRate != nSampleRateIn)
	{
		PcmMode.nSamplingRate = nSampleRateIn;
		TS_PerFrame = pDecWrapper->getTsPerFrame(PcmMode.nSamplingRate); 
		LOG_DEBUG("Decoder Sample Rate = %d\t TS per Frame = %lld\n", \
				PcmMode.nSamplingRate, TS_PerFrame);
		SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
	}

	fsl_osal_memcpy(&AmrType, pAmrType, sizeof(OMX_AUDIO_PARAM_AMRTYPE));

	pInBufferHdr->nFilledLen = 0;

	return ret;
}

/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE AmrDecInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        AmrDec *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(AmrDec, ());
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
