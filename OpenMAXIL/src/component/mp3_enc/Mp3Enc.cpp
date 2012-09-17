/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Mp3Enc.h"

Mp3Enc::Mp3Enc()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_encoder.mp3.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_encoder.mp3";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = MP3E_INPUT_BUFFER_SIZE * 4;
	nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE Mp3Enc::InitComponent()
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
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingMP3;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
    sPortDef.nBufferSize = MP3E_INPUT_BUFFER_SIZE * 4;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&Mp3Type, OMX_AUDIO_PARAM_MP3TYPE);
	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	Mp3Type.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
	Mp3Type.nChannels = 2;
	Mp3Type.nSampleRate = 44100;
	Mp3Type.nBitRate = 128000;
	Mp3Type.eChannelMode = OMX_AUDIO_ChannelModeStereo;
	Mp3Type.eFormat = OMX_AUDIO_MP3StreamFormatMP1Layer3;

	PcmMode.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	TS_PerFrame = MP3E_INPUT_BUFFER_SIZE*OMX_TICKS_PER_SECOND/Mp3Type.nSampleRate;

	return ret;
}

OMX_ERRORTYPE Mp3Enc::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Mp3Enc::AudioFilterInstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	pMp3EncConfig = (MP3E_Encoder_Config *)FSL_MALLOC(sizeof(MP3E_Encoder_Config));
	if (pMp3EncConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pMp3EncConfig, 0, sizeof(MP3E_Encoder_Config));

	pMp3EncParams = (MP3E_Encoder_Parameter *)FSL_MALLOC(sizeof(MP3E_Encoder_Parameter));
	if (pMp3EncParams == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pMp3EncParams, 0, sizeof(MP3E_Encoder_Parameter));

	MP3E_RET_VAL Mp3Ret;
	Mp3Ret = mp3e_query_mem(pMp3EncConfig);
	if (Mp3Ret != MP3E_SUCCESS)
	{
		LOG_ERROR("Mp3 encoder query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	OMX_S32 MemoryCnt = ENC_NUM_MEM_BLOCKS;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pMp3EncConfig->mem_info[i].size;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		MemPtr[i] = pBuffer;
		pMp3EncConfig->mem_info[i].ptr = (int*)((unsigned int )(pBuffer + \
			pMp3EncConfig->mem_info[i].align - 1 ) \
			& (0xffffffff ^ (pMp3EncConfig->mem_info[i].align - 1 )));
	}

    return ret;
}

OMX_ERRORTYPE Mp3Enc::AudioFilterCodecInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 app_stereo_mode = 0;
	OMX_S32 app_input_format = 0;
	OMX_S32 app_quality = 0;
	OMX_S32 app_mode = 0;

	pMp3EncParams->app_bit_rate = (Mp3Type.nBitRate/1000);
	if(Mp3Type.nChannels == 2 )
	{
		app_stereo_mode = 0;
	}
	else if(Mp3Type.nChannels == 1 )
	{
		app_stereo_mode = 1;
	}
	else
	{
		app_stereo_mode = 0;
	}
	//Interleaved 
	app_input_format = 0;

	app_quality = 1;
	app_mode = app_stereo_mode | (app_input_format<<8) | (app_quality<<16);
	pMp3EncParams->app_mode = app_mode;
	pMp3EncParams->app_sampling_rate = Mp3Type.nSampleRate;
	LOG_DEBUG("Mp3 encoder bitrate: %d mode: %d samplerate: %d\n", pMp3EncParams->app_bit_rate, pMp3EncParams->app_mode, pMp3EncParams->app_sampling_rate);
	
	MP3E_RET_VAL Mp3Ret;
	Mp3Ret = mp3e_encode_init(pMp3EncParams, pMp3EncConfig);
	if (Mp3Ret != MP3E_SUCCESS)
	{
		LOG_ERROR("Mp3 encoder initialize fail. return: %d\n", Mp3Ret);
		return OMX_ErrorUndefined;
	}

	bNeedFlush = OMX_FALSE;

	return ret;
}

OMX_ERRORTYPE Mp3Enc::AudioFilterInstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 MemoryCnt = ENC_NUM_MEM_BLOCKS;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		FSL_FREE(MemPtr[i]);
	}

	FSL_FREE(pMp3EncConfig);
	FSL_FREE(pMp3EncParams);

    return ret;
}

OMX_ERRORTYPE Mp3Enc::AudioFilterGetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioMp3:
            {
                OMX_AUDIO_PARAM_MP3TYPE *pMp3Type;
                pMp3Type = (OMX_AUDIO_PARAM_MP3TYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pMp3Type, OMX_AUDIO_PARAM_MP3TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pMp3Type->nPortIndex != AUDIO_FILTER_OUTPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(pMp3Type, &Mp3Type,	sizeof(OMX_AUDIO_PARAM_MP3TYPE));
            }
			return ret;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE Mp3Enc::AudioFilterSetParameterPCM()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (Mp3Type.nChannels != PcmMode.nChannels
			|| Mp3Type.nSampleRate != PcmMode.nSamplingRate)
	{
		Mp3Type.nChannels = PcmMode.nChannels;
		Mp3Type.nSampleRate = PcmMode.nSamplingRate;
		TS_PerFrame = MP3E_INPUT_BUFFER_SIZE*OMX_TICKS_PER_SECOND/Mp3Type.nSampleRate;
		nPushModeInputLen = MP3E_INPUT_BUFFER_SIZE * 2 * Mp3Type.nChannels;
		LOG_DEBUG("Encoder Sample Rate = %d\t channels: %d\t TS per Frame = %lld\n", \
				Mp3Type.nSampleRate, Mp3Type.nChannels, TS_PerFrame);
		SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
	}

    return ret;
}

OMX_ERRORTYPE Mp3Enc::AudioFilterSetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioMp3:
            {
                OMX_AUDIO_PARAM_MP3TYPE *pMp3Type;
                pMp3Type = (OMX_AUDIO_PARAM_MP3TYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pMp3Type, OMX_AUDIO_PARAM_MP3TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pMp3Type->nPortIndex != AUDIO_FILTER_OUTPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(&Mp3Type, pMp3Type, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
			}
			return ret;
		default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

AUDIO_FILTERRETURNTYPE Mp3Enc::AudioFilterFrame()
{
    AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;

	AudioRingBuffer.BufferGet(&pBuffer, nPushModeInputLen, &nActuralLen);
	LOG_LOG("Mp3 encoder get stream len: %d\n", nActuralLen);

	if (nActuralLen == 0) {
		if (bNeedFlush == OMX_TRUE)
			mp3e_flush_bitstream(pMp3EncConfig, (MP3E_INT8*)pOutBufferHdr->pBuffer);
	} else {
		if (nActuralLen < nPushModeInputLen) {
			OMX_U32 nActuralLenZero;
			AudioRingBuffer.BufferAddZeros(nPushModeInputLen - nActuralLen, &nActuralLenZero);
			AudioRingBuffer.BufferGet(&pBuffer, nPushModeInputLen, &nActuralLen);
		}
		mp3e_encode_frame((MP3E_INT16*)pBuffer, pMp3EncConfig, (MP3E_INT8*)pOutBufferHdr->pBuffer);
		bNeedFlush = OMX_TRUE;
	}

	AudioRingBuffer.BufferConsumered(nActuralLen);

	pOutBufferHdr->nOffset = 0;
	pOutBufferHdr->nFilledLen = pMp3EncConfig->num_bytes;

	LOG_LOG("Stream frame len: %d\n", pMp3EncConfig->num_bytes);
	LOG_LOG("nSampleRate: %d nChannels: %d\n", PcmMode.nSamplingRate, PcmMode.nChannels);

	LOG_LOG("TS per Frame = %lld\n", TS_PerFrame);

	AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 

	if (nActuralLen == 0)
	{
		ret = AUDIO_FILTER_EOS;
	}


	LOG_LOG("Encoder nTimeStamp = %lld\n", pOutBufferHdr->nTimeStamp);

    return ret;
}

OMX_ERRORTYPE Mp3Enc::AudioFilterReset()
{
    return AudioFilterCodecInit();
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE Mp3EncInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        Mp3Enc *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(Mp3Enc, ());
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
