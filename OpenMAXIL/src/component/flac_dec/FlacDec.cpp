/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "FlacDec.h"

#define FLACFRAMEHEADERLEN 2

FlacDec::FlacDec()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_decoder.flac.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_decoder.flac";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = FLAC_INPUT_BUF_PUSH_SIZE + 1;
	nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE FlacDec::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = AUDIO_FILTER_INPUT_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = (OMX_AUDIO_CODINGTYPE)OMX_AUDIO_CodingFLAC;
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
    sPortDef.nBufferSize = FLAC_INPUT_BUF_PUSH_SIZE;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&FlacType, OMX_AUDIO_PARAM_FLACTYPE);
	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	FlacType.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	FlacType.nChannels = 2;
	FlacType.nSampleRate = 44100;

	PcmMode.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	pOutBuffer = NULL;
	pInBuffer = NULL;

	return ret;
}

OMX_ERRORTYPE FlacDec::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE FlacDec::AudioFilterInstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	pFlacDecConfig = (FLACD_Decode_Config *)FSL_MALLOC(sizeof(FLACD_Decode_Config));
	if (pFlacDecConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pFlacDecConfig, 0, sizeof(FLACD_Decode_Config));

	FLACD_RET_TYPE FlacRet;
	FlacRet = FSL_FLACD_query_memory(pFlacDecConfig);
	if (FlacRet != FLACD_OK)
	{
		LOG_ERROR("Flac decoder query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	OMX_S32 MemoryCnt = pFlacDecConfig->flacd_mem_info.flacd_num_reqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pFlacDecConfig->flacd_mem_info.mem_info_sub[i].flacd_size;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		pFlacDecConfig->flacd_mem_info.mem_info_sub[i].app_base_ptr = pBuffer;
	}

    return ret;
}

OMX_S32 FlacDec_SwapBuffers(OMX_S8 **new_buf_ptr, OMX_U32 *new_buf_len, void *context)
{
	FlacDec *pFlacDec = (FlacDec *)context;
	RingBuffer *pAudioRingBuffer = (RingBuffer *)&(pFlacDec->AudioRingBuffer);
	*new_buf_ptr = NULL;

	LOG_LOG("Flac need bytes: %d RingBuffer audio data len: %d\n", *new_buf_len, pAudioRingBuffer->AudioDataLen());

	if(pAudioRingBuffer->AudioDataLen()<=0 && pFlacDec->bReceivedEOS == OMX_TRUE)
	{
		LOG_DEBUG("No audio stream any more, send EOS to decoder.\n");
		*new_buf_ptr = NULL;
		*new_buf_len=0;
		return 1;
	}
	if((pAudioRingBuffer->AudioDataLen() - pAudioRingBuffer->nPrevOffset)<*new_buf_len && pFlacDec->bReceivedEOS == OMX_FALSE)
	{
		*new_buf_ptr = NULL;
		*new_buf_len=0;
		return 0;
	}

	pAudioRingBuffer->BufferConsumered(pAudioRingBuffer->nPrevOffset);

	OMX_U32 nActuralLen;
	OMX_U8 *pBuffer = NULL;

	pAudioRingBuffer->BufferGet((fsl_osal_u8**)(&pBuffer), *new_buf_len, &nActuralLen);
	LOG_LOG("Get stream length: %d\n", nActuralLen);

	fsl_osal_memcpy(pFlacDec->pInBuffer, pFlacDec->pInBuffer + *new_buf_len, FLAC_INPUT_PULL_BUFFER_SIZE - *new_buf_len);
	fsl_osal_memcpy(pFlacDec->pInBuffer + FLAC_INPUT_PULL_BUFFER_SIZE - *new_buf_len, pBuffer, nActuralLen);

	*new_buf_ptr = (OMX_S8 *)(pFlacDec->pInBuffer);
	*new_buf_len = nActuralLen;

	pAudioRingBuffer->nPrevOffset = nActuralLen;
	return 0;
}

OMX_ERRORTYPE FlacDec::AudioFilterCodecInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	FLACD_RET_TYPE FlacRet;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;

	AudioRingBuffer.BufferGet(&pBuffer, nPushModeInputLen, &nActuralLen);
	LOG_DEBUG("Get buffer for flac init:%d\n", nActuralLen);

	pFlacDecConfig->read_callback = (FLAC__int32 (*)(FLAC__byte**, FLAC__uint32*, void*))FlacDec_SwapBuffers;
	pFlacDecConfig->context = (void*)(this);
	pFlacDecConfig->bparser_support = OMX_FALSE;
	pFlacDecConfig->channel_no = FlacType.nChannels;
	pFlacDecConfig->bit_per_sample = FlacType.nBitPerSample;
	pFlacDecConfig->sampling_rate = FlacType.nSampleRate;
	pFlacDecConfig->total_sample = FlacType.nTotalSample;
	pFlacDecConfig->block_size = FlacType.nBlockSize;
	LOG_DEBUG("nChannels: %d nBitPerSample: %d nSampleRate: %d nTotalSample: %lld nBlockSize: %d\n", pFlacDecConfig->channel_no, pFlacDecConfig->bit_per_sample, pFlacDecConfig->sampling_rate, pFlacDecConfig->total_sample, pFlacDecConfig->block_size);

	FlacRet = FSL_FLACD_initiate_decoder(pFlacDecConfig, pBuffer);
	if (FlacRet != FLACD_OK)
	{
		LOG_ERROR("Flac decoder initialize fail.\n");
		return OMX_ErrorUnsupportedSetting;
	}

	FSL_FREE(pInBuffer);

	pInBuffer = (OMX_U8*) FSL_MALLOC(FLAC_INPUT_PULL_BUFFER_SIZE);
	if (pInBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pInBuffer, 0, FLAC_INPUT_PULL_BUFFER_SIZE);

	FSL_FREE(pOutBuffer);

	pOutBuffer = (OMX_U8*) FSL_MALLOC(FLAC_INPUT_BUF_PUSH_SIZE);
	if (pOutBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pOutBuffer, 0, FLAC_INPUT_BUF_PUSH_SIZE);

    return ret;
}

OMX_ERRORTYPE FlacDec::AudioFilterInstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 MemoryCnt = pFlacDecConfig->flacd_mem_info.flacd_num_reqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		FSL_FREE(pFlacDecConfig->flacd_mem_info.mem_info_sub[i].app_base_ptr);
	}

	FSL_FREE(pFlacDecConfig);
	FSL_FREE(pInBuffer);
	FSL_FREE(pOutBuffer);

    return ret;
}

OMX_ERRORTYPE FlacDec::AudioFilterGetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioFlac:
            {
                OMX_AUDIO_PARAM_FLACTYPE *pFlacType;
                pFlacType = (OMX_AUDIO_PARAM_FLACTYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pFlacType, OMX_AUDIO_PARAM_FLACTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pFlacType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(pFlacType, &FlacType,	sizeof(OMX_AUDIO_PARAM_FLACTYPE));
            }
			return ret;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE FlacDec::AudioFilterSetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioFlac:
            {
                OMX_AUDIO_PARAM_FLACTYPE *pFlacType;
                pFlacType = (OMX_AUDIO_PARAM_FLACTYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pFlacType, OMX_AUDIO_PARAM_FLACTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pFlacType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(&FlacType, pFlacType, sizeof(OMX_AUDIO_PARAM_FLACTYPE));
			}
			return ret;
		default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE FlacDec::AudioFilterCheckFrameHeader()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	/*
	OMX_U8 *pBuffer;
	OMX_U8 *pBuffer1;
	OMX_U32 nActuralLen;
	OMX_U32 nOffset=0;
	OMX_BOOL bFounded = OMX_FALSE;

	while (1)
	{
		AudioRingBuffer.BufferGet(&pBuffer, CHECKFRAMEHEADERREADLEN, &nActuralLen);
		LOG_LOG("Get stream length: %d\n", nActuralLen);

		if (nActuralLen < FLACFRAMEHEADERLEN)
			break;

		nOffset = 0;
		while(nActuralLen >= FLACFRAMEHEADERLEN)
		{
			if(pBuffer[0] ==0xFF && (pBuffer[1] & 0xFE) == 0xF8)
			{
				LOG_LOG("Frame header founded.\n");
				bFounded = OMX_TRUE;
				break;  
			}
			else
			{
				pBuffer ++;
				nOffset++;
				nActuralLen--;
			}
		}

		printf("Flac decoder skip %d bytes.\n", nOffset);
		AudioRingBuffer.BufferConsumered(nOffset);

		if (bFounded == OMX_TRUE)
			break;
	}
*/
    return ret;
}

AUDIO_FILTERRETURNTYPE FlacDec::AudioFilterFrame()
{
    AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;

	FLACD_RET_TYPE FlacRet;
	OMX_U32 OutputLenPerChannel = 0;
	FlacRet = FSL_FLACD_decode_frame(pFlacDecConfig, (FLAC__uint32*)(&OutputLenPerChannel), pOutBuffer);

	LOG_LOG("FlacRet = %d\n", FlacRet);
	LOG_LOG("OutputLenPerChannel = %d\n", OutputLenPerChannel);

	if (FlacRet == FLACD_OK || FlacRet == FLACD_CONTINUE_DECODING)
	{
		if (PcmMode.nChannels != (OMX_U16)pFlacDecConfig->channel_no
				|| PcmMode.nSamplingRate != (OMX_U32)pFlacDecConfig->sampling_rate
				|| PcmMode.nBitPerSample != pFlacDecConfig->bit_per_sample)
		{
			LOG_DEBUG("FLAC decoder channel: %d sample rate: %d\n", \
					pFlacDecConfig->channel_no, pFlacDecConfig->sampling_rate);
			if (pFlacDecConfig->channel_no == 0 || pFlacDecConfig->sampling_rate == 0)
			{
				return AUDIO_FILTER_FAILURE;
			}
			FlacType.nChannels = pFlacDecConfig->channel_no;
			FlacType.nSampleRate = pFlacDecConfig->sampling_rate;
			PcmMode.nChannels = pFlacDecConfig->channel_no;
			PcmMode.nSamplingRate = pFlacDecConfig->sampling_rate;
			PcmMode.nBitPerSample = pFlacDecConfig->bit_per_sample;
			LOG_DEBUG("Flac nBitPerSample: %d nChannels: %d nSampleRate:%d\n", PcmMode.nBitPerSample, PcmMode.nChannels, PcmMode.nSamplingRate);
			SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
		}

		OMX_U32 i, j, OutputChannel = pFlacDecConfig->channel_no;
		switch(pFlacDecConfig->bit_per_sample)
		{			
			case 8:
				for(i = 0; i < OutputLenPerChannel; i++) 
				{
					for(j=0; j<OutputChannel; j++)
					{
						OMX_U32 ori_index = i+(OutputLenPerChannel)*j;
						OMX_U32 new_index = j+(OutputChannel)*i;
						OMX_S8 sample_value = (OMX_S8)(*((OMX_S32*)pOutBuffer+ori_index));

						*(pOutBufferHdr->pBuffer+new_index*sizeof(OMX_S8)) = sample_value;
					}
				}
				pOutBufferHdr->nFilledLen = OutputLenPerChannel * OutputChannel * sizeof(OMX_S8);
				break;
			case 16:
				for(i = 0; i < OutputLenPerChannel; i++) 
				{
					for(j=0; j<OutputChannel; j++)
					{
						OMX_U32 ori_index = i+(OutputLenPerChannel)*j;
						OMX_U32 new_index = j+(OutputChannel)*i;
						OMX_S16 sample_value = (OMX_S16)(*((OMX_S32*)pOutBuffer+ori_index));
						*(pOutBufferHdr->pBuffer+new_index*sizeof(OMX_U16)) = (OMX_U8)(sample_value&0x00FF);
						*(pOutBufferHdr->pBuffer+new_index*sizeof(OMX_U16)+1) = (OMX_U8)((sample_value>>8)&0x00FF);
					}
				}
				pOutBufferHdr->nFilledLen = OutputLenPerChannel * OutputChannel * sizeof(OMX_S16);
				break;
			case 24:
				for(i = 0; i < OutputLenPerChannel; i++) 
				{
					for(j=0; j<OutputChannel; j++)
					{
						OMX_U32 ori_index = i+(OutputLenPerChannel)*j;
						OMX_U32 new_index = j+(OutputChannel)*i;
						OMX_S32 sample_value = (OMX_S32)(*((OMX_S32*)pOutBuffer+ori_index));
						*(pOutBufferHdr->pBuffer+new_index*3)   = (OMX_U8)(sample_value);
						*(pOutBufferHdr->pBuffer+new_index*3+1) = (OMX_U8)(sample_value>>8);
						*(pOutBufferHdr->pBuffer+new_index*3+2) = (OMX_U8)(sample_value>>16);
					}
				}
				pOutBufferHdr->nFilledLen = OutputLenPerChannel * OutputChannel * 3;
				break;
			default:
				break;
		}

		pOutBufferHdr->nOffset = 0;

		TS_PerFrame = (OMX_U64)OutputLenPerChannel*OMX_TICKS_PER_SECOND/FlacType.nSampleRate;

		LOG_LOG("TS per Frame = %lld\n", TS_PerFrame);

		AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 
	}
	else if (FlacRet == FLACD_END_OF_STREAM || FlacRet == FLACD_COMPLETE_DECODING)
	{
		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = 0;

		ret = AUDIO_FILTER_EOS;
	}
	else
	{
		ret = AUDIO_FILTER_FAILURE;
	}

	LOG_LOG("Decoder nTimeStamp = %lld\n", pOutBufferHdr->nTimeStamp);

    return ret;
}

OMX_ERRORTYPE FlacDec::AudioFilterReset()
{
    return AudioFilterCodecInit();
}

OMX_ERRORTYPE FlacDec::AudioFilterCheckCodecConfig()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_FLACTYPE *pFlacType;
	pFlacType = (OMX_AUDIO_PARAM_FLACTYPE*)pInBufferHdr->pBuffer;
	OMX_CHECK_STRUCT(pFlacType, OMX_AUDIO_PARAM_FLACTYPE, ret);
	if(ret != OMX_ErrorNone)
	{
		pInBufferHdr->nFilledLen = 0;
		return ret;
	}

	fsl_osal_memcpy(&FlacType, pFlacType, sizeof(OMX_AUDIO_PARAM_FLACTYPE));

	pInBufferHdr->nFilledLen = 0;

    return ret;
}

/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE FlacDecInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        FlacDec *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(FlacDec, ());
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
