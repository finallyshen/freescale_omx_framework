/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "AudioProcessor.h"

#define OMX_CHECK_VALUE(value, threshold, err) \
    do { \
		if (value.nValue > threshold.nMax) \
		{ \
			err = OMX_ErrorBadParameter; \
		} \
		else if (value.nValue < threshold.nMin) \
		{ \
			err = OMX_ErrorBadParameter; \
		} \
	} while(0);

#define PEQ_PREMODE_DEFAULT 0
static void Peq_SetDefaultParalist(PEQ_PL *para_list_ptr)
{
	OMX_U32 i, j;

	/* init common parameters */
	para_list_ptr->ppp_inputpara.iptr = PEQ_NULL;
	para_list_ptr->ppp_inputpara.optr = PEQ_NULL;
	para_list_ptr->ppp_inputpara.inputsamples = 256; 		/* input samples need to process */
	para_list_ptr->ppp_inputpara.outputsamples = 256;		/* output samples */
	para_list_ptr->ppp_inputpara.bitwidth = 16;
	para_list_ptr->ppp_inputpara.blockmode = PPP_INTERLEAVE;
	para_list_ptr->ppp_inputpara.decodertype = DecoderTypePCM;
	para_list_ptr->ppp_inputpara.samplerate = 44100;

	for(i=0;i<CHANNUM_MAX;i++)								/* set default VOR as 0 */
	{
		para_list_ptr->ppp_inputpara.VOR[i] = 0;
	}
	para_list_ptr->ppp_inputpara.pppcontrolsize = 0x158;	/* total private parameter size in Bytes */

	/* init PEQ private parameters */
	para_list_ptr->channelnumber = 2;				/* total input channel number is 2*/
	para_list_ptr->peqenablemask = 0x3;				/* all the channel is disable for peq */
	para_list_ptr->chennelfilterselect = 0;			/* do not select filter for every channel */
	para_list_ptr->premode = PEQ_PREMODE_DEFAULT;	/* do not select predetermined scenes */
	para_list_ptr->calbandsperfrm = 4;				/* calculate 4 bands coeff per frame */

	for (i=0;i<NPCMCHANS;i++)						/* init bands number */
	{
		para_list_ptr->bandspergroup[i] = 0; 		/*every group has 5 band */
	}
	para_list_ptr->bandspergroup[0]=BANDSINGRP;
	for (i=0;i<NPCMCHANS;i++)						/* init every filter's parameters */
	{
		for(j=0;j<BANDSINGRP;j++)
		{
			para_list_ptr->group_band[i][j].Gain = 0;			/* gain is 0dB */
			para_list_ptr->group_band[i][j].Q_value = 5;		/* Q is 0.5 */
			para_list_ptr->group_band[i][j].FilterType = 0;		/* Peak Filter */
			para_list_ptr->group_band[i][j].Fc = 320*(j+1);		/* Fc is 32Hz,64,128,256,1024,2048,... */
		}														/* end of band loop */
	}															/* end of group loop */
	para_list_ptr->attenuation = 0;								/* attenuation is 0dB */
	para_list_ptr->zerophaseflag = PEQ_FALSE;

	return;
}

AudioProcessor::AudioProcessor()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_processor.volume.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_processor.volume";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = PROCESSOR_INPUT_FRAME_SIZE;
	nPostProcessLen = nPushModeInputLen;
	nRingBufferScale = 2;
}

OMX_ERRORTYPE AudioProcessor::InitComponent()
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
    sPortDef.nBufferSize = PROCESSOR_OUTPUT_FRAME_SIZE;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	PcmMode.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
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

	OMX_INIT_STRUCT(&Volume, OMX_AUDIO_CONFIG_VOLUMETYPE);

	Volume.bLinear = OMX_TRUE;
	Volume.sVolume.nValue = 100;
	Volume.sVolume.nMin = 0;
	Volume.sVolume.nMax = 100;

	OMX_INIT_STRUCT(&PostProcess, OMX_AUDIO_CONFIG_POSTPROCESSTYPE);

	PostProcess.bEnable = OMX_TRUE;
	PostProcess.sDelay.nValue = 0;
	PostProcess.sDelay.nMin = 0;
	PostProcess.sDelay.nMax = 100;

	OMX_U32 i;
	for(i=0;i<BANDSINGRP;i++)
	{
		OMX_INIT_STRUCT(&PEQ[i], OMX_AUDIO_CONFIG_EQUALIZERTYPE);
		PEQ[i].nPortIndex = AUDIO_FILTER_INPUT_PORT;
		PEQ[i].bEnable = OMX_TRUE;
		PEQ[i].sBandIndex.nValue = i;
		PEQ[i].sBandIndex.nMin = 0;
		PEQ[i].sBandIndex.nMax = BANDSINGRP - 1;
		PEQ[i].sCenterFreq.nValue = 32*(i+1);		/* Fc is 32Hz,64,128,256,1024,2048,... */
		PEQ[i].sCenterFreq.nMin = 0;
		PEQ[i].sCenterFreq.nMax = 48000;
		PEQ[i].sBandLevel.nValue = 0;				/* gain is 0dB */
		PEQ[i].sBandLevel.nMin = -500;
		PEQ[i].sBandLevel.nMax = 500;
	}						

	TS_PerFrame = nPostProcessLen/PcmMode.nChannels/(PcmMode.nBitPerSample>>3)*OMX_TICKS_PER_SECOND/PcmMode.nSamplingRate;

	nFadeInFadeOutProcessLen = PcmMode.nSamplingRate * POSTPROCESSFADEPROCESSTIME / 1000 * (PcmMode.nBitPerSample>>3) * PcmMode.nChannels; 

	nFadeOutProcessLen = 0;
	bSetConfig = OMX_TRUE;
	bPostProcess = OMX_FALSE;
	bNeedChange = OMX_FALSE;
	bPEQEnable = OMX_FALSE;

	return ret;
}

OMX_ERRORTYPE AudioProcessor::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioProcessor::AudioFilterInstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (nPushModeInputLen == 0)
		nPushModeInputLen = PROCESSOR_INPUT_FRAME_SIZE;
	nFadeInFadeOutProcessLen = PcmMode.nSamplingRate * POSTPROCESSFADEPROCESSTIME / 1000 * (PcmMode.nBitPerSample>>3) * PcmMode.nChannels; 
	nPushModeInputLen = (((nPushModeInputLen / ((PcmMode.nBitPerSample>>3) * PcmMode.nChannels))>>1)<<1) * ((PcmMode.nBitPerSample>>3) * PcmMode.nChannels);
	nPostProcessLen = nPushModeInputLen;
	nPushModeInputLen += 2*nFadeInFadeOutProcessLen;
	LOG_DEBUG("Audio PP frame len: %d\n", nPostProcessLen);

	FADEINFADEOUT_ERRORTYPE FadeRet = FADEINFADEOUT_SUCCESS;
	FadeRet = AudioRenderFadeInFadeOut.Create(PcmMode.nChannels, PcmMode.nSamplingRate, \
			PcmMode.nBitPerSample, nFadeInFadeOutProcessLen, 1);
	if (FadeRet != FADEINFADEOUT_SUCCESS)
	{
		LOG_ERROR("Create fade in fade out process fail.\n");
		return OMX_ErrorInsufficientResources;
	}

	pPeqConfig = (PEQ_PPP_Config *)FSL_MALLOC(sizeof(PEQ_PPP_Config));
	if (pPeqConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pPeqConfig, 0, sizeof(PEQ_PPP_Config));

	pPeqInfo = (PEQ_INFO *)FSL_MALLOC(sizeof(PEQ_INFO));
	if (pPeqInfo == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pPeqInfo, 0, sizeof(PEQ_INFO));

	pPeqPl = (PEQ_PL *)FSL_MALLOC(sizeof(PEQ_PL));
	if (pPeqPl == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pPeqPl, 0, sizeof(PEQ_PL));

	PEQ_RET_TYPE PeqRet;
	PeqRet = peq_query_ppp_mem(pPeqConfig);
	if (PeqRet != PEQ_OK)
	{
		LOG_ERROR("Peq query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	OMX_S32 MemoryCnt = pPeqConfig->peq_mem_info.peq_num_reqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pPeqConfig->peq_mem_info.mem_info_sub[i].peq_size;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		pPeqConfig->peq_mem_info.mem_info_sub[i].app_base_ptr = pBuffer;
	}

    return ret;
}

OMX_ERRORTYPE AudioProcessor::AudioFilterInstanceReset()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	LOG_DEBUG("Audio instance reset.\n");
	/* Walk round for component add setting change, we should change ring buffer too */
	nPushModeInputLen = PROCESSOR_INPUT_FRAME_SIZE;
	ret = AudioFilterInstanceDeInit();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Audio decoder instance de-init fail.\n");
		return ret;
	}

	AudioRingBuffer.BufferFree();

	ret = AudioFilterInstanceInit();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Audio decoder instance init fail.\n");
		return ret;
	}

	RINGBUFFER_ERRORTYPE BufferRet = RINGBUFFER_SUCCESS;
	BufferRet = AudioRingBuffer.BufferCreate(nPushModeInputLen, nRingBufferScale);
	if (BufferRet != RINGBUFFER_SUCCESS)
	{
		LOG_ERROR("Create ring buffer fail.\n");
		return OMX_ErrorInsufficientResources;
	}

	return ret;
}

OMX_ERRORTYPE AudioProcessor::AudioFilterCodecInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	AUDIO_POSTPROCESSRETURNTYPE volumeRet = AUDIO_POSTPROCESS_SUCCESS;
	volumeRet = VolumeInit(PcmMode.nChannels, PcmMode.nSamplingRate, PcmMode.nBitPerSample, Volume.sVolume.nValue);
	if (volumeRet != AUDIO_POSTPROCESS_SUCCESS)
	{
		LOG_ERROR("Init audio post process fail.\n");
		return OMX_ErrorInsufficientResources;
	}

	PEQ_RET_TYPE PeqRet;
	PeqRet = peq_ppp_init(pPeqConfig);
	if (PeqRet != PEQ_OK)
	{
		LOG_ERROR("Peq initialize fail.\n");
		return OMX_ErrorUndefined;
	}

   	Peq_SetDefaultParalist(pPeqPl);
	if(pPeqPl->premode == 0)
	{
    		pPeqPl->ppp_inputpara.samplerate = PcmMode.nSamplingRate;
    		pPeqPl->channelnumber = PcmMode.nChannels;
	}
	nSampleSize = (pPeqPl->ppp_inputpara.bitwidth >> 3) * pPeqPl->channelnumber;
	LOG_DEBUG("PEQ sample size: %d\n", nSampleSize);

	return ret;
}

OMX_ERRORTYPE AudioProcessor::AudioFilterInstanceDeInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 MemoryCnt = pPeqConfig->peq_mem_info.peq_num_reqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		FSL_FREE(pPeqConfig->peq_mem_info.mem_info_sub[i].app_base_ptr);
	}

	FSL_FREE(pPeqConfig);
	FSL_FREE(pPeqInfo);
	FSL_FREE(pPeqPl);

    return ret;
}

OMX_ERRORTYPE AudioProcessor::AudioFilterGetParameter(
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

OMX_ERRORTYPE AudioProcessor::AudioFilterSetParameter(
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

OMX_ERRORTYPE AudioProcessor::AudioFilterSetParameterPCM()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	LOG_DEBUG("PcmMode.nSamplingRate = %d\n", PcmMode.nSamplingRate);
	if (PcmModeOut.nChannels != PcmMode.nChannels \
			|| PcmModeOut.nSamplingRate != PcmMode.nSamplingRate \
			|| PcmModeOut.nBitPerSample != PcmMode.nBitPerSample)
	{
		bInstanceReset = OMX_TRUE;
		PcmModeOut.nChannels = PcmMode.nChannels;
		PcmModeOut.nSamplingRate = PcmMode.nSamplingRate;
		PcmModeOut.nBitPerSample = PcmMode.nBitPerSample;
		TS_PerFrame = nPostProcessLen/PcmMode.nChannels/(PcmMode.nBitPerSample>>3)*OMX_TICKS_PER_SECOND/PcmMode.nSamplingRate;
		SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
	}

    return ret;
}
 
OMX_ERRORTYPE AudioProcessor::GetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexConfigAudioVolume:
            {
                OMX_AUDIO_CONFIG_VOLUMETYPE *pVolume;
                pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*)pStructure;
                OMX_CHECK_STRUCT(pVolume, OMX_AUDIO_CONFIG_VOLUMETYPE, ret);
				fsl_osal_memcpy(pVolume, &Volume, sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
            }
            break;
        case OMX_IndexConfigAudioEqualizer:
            {
                OMX_AUDIO_CONFIG_EQUALIZERTYPE *pPEQ;
                pPEQ = (OMX_AUDIO_CONFIG_EQUALIZERTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPEQ, OMX_AUDIO_CONFIG_EQUALIZERTYPE, ret);
				fsl_osal_memcpy(pPEQ, &PEQ[pPEQ->sBandIndex.nValue], sizeof(OMX_AUDIO_CONFIG_EQUALIZERTYPE));
				if (pPEQ->sBandIndex.nValue == 0)
					pPEQ->sBandIndex.nValue = BANDSINGRP;
			}
            break;
        case OMX_IndexConfigAudioPostProcess:
            {
                OMX_AUDIO_CONFIG_POSTPROCESSTYPE *pPostProcess;
                pPostProcess = (OMX_AUDIO_CONFIG_POSTPROCESSTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPostProcess, OMX_AUDIO_CONFIG_POSTPROCESSTYPE, ret);
				fsl_osal_memcpy(pPostProcess, &PostProcess, sizeof(OMX_AUDIO_CONFIG_POSTPROCESSTYPE));
            }
            break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE AudioProcessor::SetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexConfigAudioVolume:
            {
                OMX_AUDIO_CONFIG_VOLUMETYPE *pVolume;
                pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*)pStructure;
                OMX_CHECK_STRUCT(pVolume, OMX_AUDIO_CONFIG_VOLUMETYPE, ret);

				OMX_CHECK_VALUE(pVolume->sVolume, Volume.sVolume, ret);
				if (ret != OMX_ErrorNone)
					return ret;

				if (Volume.sVolume.nValue != pVolume->sVolume.nValue)
				{
					bSetConfig = OMX_TRUE;
				}
				fsl_osal_memcpy(&Volume, pVolume, sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
			}
			break;
        case OMX_IndexConfigAudioEqualizer:
            {
                OMX_AUDIO_CONFIG_EQUALIZERTYPE *pPEQ;
                pPEQ = (OMX_AUDIO_CONFIG_EQUALIZERTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPEQ, OMX_AUDIO_CONFIG_EQUALIZERTYPE, ret);
				if (pPEQ->bEnable == OMX_FALSE
						|| !(PcmMode.nSamplingRate == 32000
							|| PcmMode.nSamplingRate == 44100 
							|| PcmMode.nSamplingRate == 48000))
				{
					bPEQEnable = OMX_FALSE;
					return ret;
				}
				if (!(PcmMode.nBitPerSample == 16))
				{
					bPEQEnable = OMX_FALSE;
					return ret;
				}
				bPEQEnable = OMX_TRUE;

				LOG_DEBUG("PEQ Band Index: %d\n", pPEQ->sBandIndex.nValue);
				OMX_CHECK_VALUE(pPEQ->sBandIndex, PEQ[0].sBandIndex, ret);
				if (ret != OMX_ErrorNone)
					return ret;
				LOG_DEBUG("PEQ Center Frequency: %d\n", pPEQ->sCenterFreq.nValue);
				OMX_CHECK_VALUE(pPEQ->sCenterFreq, PEQ[0].sCenterFreq, ret);
				if (ret != OMX_ErrorNone)
					return ret;
				LOG_DEBUG("PEQ Band Level: %d\n", pPEQ->sBandLevel.nValue);
				OMX_CHECK_VALUE(pPEQ->sBandLevel, PEQ[0].sBandLevel, ret);
				if (ret != OMX_ErrorNone)
					return ret;

				bSetConfig = OMX_TRUE;
				fsl_osal_memcpy(&PEQ[pPEQ->sBandIndex.nValue], pPEQ, \
						sizeof(OMX_AUDIO_CONFIG_EQUALIZERTYPE));
				pPeqPl->group_band[0][pPEQ->sBandIndex.nValue].Fc \
					= PEQ[pPEQ->sBandIndex.nValue].sCenterFreq.nValue * 10;
				pPeqPl->group_band[0][pPEQ->sBandIndex.nValue].Gain \
					= PEQ[pPEQ->sBandIndex.nValue].sBandLevel.nValue;
			}
			break;
        case OMX_IndexConfigAudioPostProcess:
            {
                OMX_AUDIO_CONFIG_POSTPROCESSTYPE *pPostProcess;
                pPostProcess = (OMX_AUDIO_CONFIG_POSTPROCESSTYPE*)pStructure;
				OMX_CHECK_STRUCT(pPostProcess, OMX_AUDIO_CONFIG_POSTPROCESSTYPE, ret);
				fsl_osal_memcpy(&PostProcess, pPostProcess, \
						sizeof(OMX_AUDIO_CONFIG_POSTPROCESSTYPE));
				bSetConfig = OMX_TRUE;
			}
			break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

AUDIO_FILTERRETURNTYPE AudioProcessor::AudioFilterFrame()
{
	AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
	AUDIO_POSTPROCESSRETURNTYPE volumeRet = AUDIO_POSTPROCESS_SUCCESS;
	PEQ_RET_TYPE PeqRet;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;

	CheckVolumeConfig();

	LOG_LOG("PostProcessLenFrame Len:%d\n", nPostProcessLenFrame);
	AudioRingBuffer.BufferGet(&pBuffer, nPostProcessLenFrame, &nActuralLen);
	LOG_LOG("PostProcess Len:%d\n", nActuralLen);

	if (bPostProcess == OMX_TRUE)
	{
		volumeRet = VolumeProcess(pOutBufferHdr->pBuffer, &pOutBufferHdr->nFilledLen, \
				pBuffer, nActuralLen);

		if (bPEQEnable == OMX_TRUE)
		{
			OMX_U32 nSamples;
			pPeqPl->ppp_inputpara.iptr = pOutBufferHdr->pBuffer;
			pPeqPl->ppp_inputpara.optr = pOutBufferHdr->pBuffer;
			nSamples = nActuralLen/nSampleSize;
			if (nSamples & 0x1)
			{
				LOG_WARNING("PEQ process length wrong.\n");
				nSamples = (nSamples >> 1) << 1;
			}
			pPeqPl->ppp_inputpara.inputsamples = nSamples;
			pPeqPl->ppp_inputpara.outputsamples = nSamples;

			LOG_LOG("PEQ input samples: %d\n", pPeqPl->ppp_inputpara.inputsamples);
			if (nSamples > 0)
				PeqRet = peq_ppp_frame(pPeqConfig, pPeqPl, pPeqInfo);
		}
	}
	else
	{
		fsl_osal_memcpy((fsl_osal_ptr)pOutBufferHdr->pBuffer, (fsl_osal_ptr)pBuffer, \
				nActuralLen);
		pOutBufferHdr->nFilledLen = nActuralLen;

		volumeRet = AUDIO_POSTPROCESS_SUCCESS;
	}

	if (nActuralLen == 0)
	{
		volumeRet = AUDIO_POSTPROCESS_EOS;
	}

	LOG_LOG("volumeRet = %d\n", volumeRet);

	AudioRenderFadeInFadeOut.Process(pOutBufferHdr->pBuffer, pOutBufferHdr->nFilledLen);

	AudioRingBuffer.BufferConsumered(nActuralLen);

	LOG_LOG("nActuralLen = %d\n", nActuralLen);

	if (volumeRet == AUDIO_POSTPROCESS_SUCCESS)
	{
		pOutBufferHdr->nOffset = 0;
		LOG_LOG("TS increase: %lld Filled Len: %d\n", TS_PerFrame, \
				pOutBufferHdr->nFilledLen);
		if (nPostProcessLenFrame != nActuralLen)
		{
			OMX_TICKS TS_PerFrameTmp = nActuralLen/PcmMode.nChannels \
									   /(PcmMode.nBitPerSample>>3)*OMX_TICKS_PER_SECOND \
									   /PcmMode.nSamplingRate;
			AudioRingBuffer.TS_SetIncrease(TS_PerFrameTmp); 
		}
		else
		{
			AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 
		}
	}
	else if (volumeRet == AUDIO_POSTPROCESS_EOS)
	{
		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = 0;

		ret = AUDIO_FILTER_EOS;
	}
	else
	{
		ret = AUDIO_FILTER_FAILURE;
	}

	LOG_LOG("Volume nTimeStamp = %lld\n", pOutBufferHdr->nTimeStamp);

	if (bNeedChange == OMX_TRUE)
	{
		if (bPostProcess == OMX_TRUE)
		{
			bPostProcess = OMX_FALSE;
		}
		else if (bPostProcess == OMX_FALSE)
		{
			bPostProcess = OMX_TRUE;
		}
		bNeedChange = OMX_FALSE;
	}

	LOG_LOG("Audio process return = %d\n", ret);

	return ret;
}

OMX_ERRORTYPE AudioProcessor::CheckVolumeConfig()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (bSetConfig == OMX_TRUE)
	{
		VolumeSetConfig(PcmMode.nChannels, PcmMode.nSamplingRate, PcmMode.nBitPerSample, Volume.sVolume.nValue);

		if ((PostProcess.bEnable == OMX_TRUE && bPostProcess == OMX_FALSE)
				|| (PostProcess.bEnable == OMX_FALSE && bPostProcess == OMX_TRUE)
				|| (PostProcess.bEnable == OMX_TRUE && bPostProcess == OMX_TRUE))
		{
			AudioRenderFadeInFadeOut.SetMode(FADEOUT);
			nFadeOutProcessLen = nFadeInFadeOutProcessLen;
			LOG_DEBUG("Fade proces length: %d\n", nFadeOutProcessLen);
		}

		bSetConfig = OMX_FALSE;
	}

	nPostProcessLenFrame = nPostProcessLen;
	if (nFadeOutProcessLen != 0)
	{
		if (nFadeOutProcessLen > nPostProcessLenFrame)
		{
			nFadeOutProcessLen -= nPostProcessLenFrame;
		}
		else
		{
			nPostProcessLenFrame = nFadeOutProcessLen;
			nFadeOutProcessLen -= nPostProcessLenFrame;
			if ((PostProcess.bEnable == OMX_TRUE && bPostProcess == OMX_FALSE)
					|| (PostProcess.bEnable == OMX_FALSE && bPostProcess == OMX_TRUE))
			{
				bNeedChange = OMX_TRUE;
			}
		}
	}

    return ret;
}

AUDIO_POSTPROCESSRETURNTYPE AudioProcessor::VolumeInit(OMX_U32 Channels, OMX_U32 SamplingRate, OMX_U32 BitsPerSample, OMX_U32 nVolume)
{
    AUDIO_POSTPROCESSRETURNTYPE ret = AUDIO_POSTPROCESS_SUCCESS;

	if (BitsPerSample == 0 || Channels == 0)
		return AUDIO_POSTPROCESS_FAILURE;

	Scale = (fsl_osal_float)nVolume / 100;

    return ret;
}

AUDIO_POSTPROCESSRETURNTYPE AudioProcessor::VolumeSetConfig(OMX_U32 Channels, OMX_U32 SamplingRate, OMX_U32 BitsPerSample, OMX_U32 nVolume)
{
    AUDIO_POSTPROCESSRETURNTYPE ret = AUDIO_POSTPROCESS_SUCCESS;

	if (BitsPerSample == 0 || Channels == 0)
		return AUDIO_POSTPROCESS_FAILURE;

	Scale = (fsl_osal_float)nVolume / 100;

    return ret;
}

AUDIO_POSTPROCESSRETURNTYPE AudioProcessor::VolumeProcess(OMX_U8 *pOutBuffer, OMX_U32 *pOutLen, OMX_U8 *pInBuffer, OMX_U32 nInLen)
{
    AUDIO_POSTPROCESSRETURNTYPE ret = AUDIO_POSTPROCESS_SUCCESS;
	OMX_U32 i, j, Len;

	if (Volume.sVolume.nValue == 100)
	{
		fsl_osal_memcpy((fsl_osal_ptr)pOutBuffer, (fsl_osal_ptr)pInBuffer, nInLen);
		*pOutLen = nInLen;
		return ret;
	}
	if (nInLen == 0)
	{
		*pOutLen = nInLen;

		return AUDIO_POSTPROCESS_EOS;
	}

	OMX_U32 nChannels = PcmMode.nChannels;

	Len = nInLen / (PcmMode.nBitPerSample>>3) / PcmMode.nChannels;

	switch(PcmMode.nBitPerSample)
	{
		case 8:
			{
				OMX_S8 *pSrc = (OMX_S8 *)pInBuffer, *pDst = (OMX_S8 *)pOutBuffer;
				OMX_S8 Tmp;
				for (i = 0; i < Len; i ++)
				{
					for (j = 0; j < nChannels; j ++)
					{
						Tmp = pSrc[i*nChannels+j];
						Tmp = (OMX_S8)(Tmp * Scale);
						pDst[i*nChannels+j] = Tmp;
					}
				}
			}
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
						Tmp = (OMX_S16)(Tmp * Scale);
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
						if (Tmp&0x800000) Tmp |= 0xff000000;
						Tmp = (OMX_S32)(Tmp * Scale);
						pDst[(i*nChannels+j)*3] = Tmp;
						pDst[(i*nChannels+j)*3+1] = Tmp>>8;
						pDst[(i*nChannels+j)*3+2] = Tmp>>16;
					}
				}
			}
			break;
	}
	*pOutLen = nInLen;

    return ret;
}

OMX_ERRORTYPE AudioProcessor::AudioFilterReset()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    return ret;
}

OMX_ERRORTYPE AudioProcessor::ComponentReturnBuffer(
        OMX_U32 nPortIndex)
{
	/* Consume all audio data in input port buffer and ring buffer when disable input port */
	bDecoderEOS = OMX_FALSE;
    if(nPortIndex == AUDIO_FILTER_INPUT_PORT \
			&& ports[AUDIO_FILTER_INPUT_PORT]->IsEnabled() != OMX_TRUE \
			&& ports[AUDIO_FILTER_OUTPUT_PORT]->IsEnabled() == OMX_TRUE) {
		bSetConfig = OMX_TRUE;
		PostProcess.bEnable = OMX_FALSE;
		nPushModeInputLen = 0;
		while (ports[AUDIO_FILTER_INPUT_PORT]->BufferNum() != 0 || pInBufferHdr != NULL \
				|| AudioRingBuffer.AudioDataLen() != 0)
		{
			LOG_DEBUG("Buffer num:%d ring buffer len:%d\n", \
					ports[AUDIO_FILTER_INPUT_PORT]->BufferNum(), \
					AudioRingBuffer.AudioDataLen());
			/* Input port disable in Pause state will hang */
			ProcessDataBuffer();
			usleep(5000);
			LOG_WARNING("Input port disable in Pause state will hang.\n");
		}
	}

    if(nPortIndex == AUDIO_FILTER_INPUT_PORT && pInBufferHdr != NULL) {
        ports[AUDIO_FILTER_INPUT_PORT]->SendBuffer(pInBufferHdr);
        pInBufferHdr = NULL;
    }

    if(nPortIndex == AUDIO_FILTER_OUTPUT_PORT && pOutBufferHdr != NULL) {
        ports[AUDIO_FILTER_OUTPUT_PORT]->SendBuffer(pOutBufferHdr);
        pOutBufferHdr = NULL;
    }

    return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE AudioProcessorInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        AudioProcessor *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(AudioProcessor, ());
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
