/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "AudioFilter.h"
 
OMX_ERRORTYPE AudioFilter::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
		case OMX_IndexParamAudioPcm:
            {
				OMX_BOOL bOutputPort = OMX_FALSE;
                OMX_AUDIO_PARAM_PCMMODETYPE *pPcmMode;
                pPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pPcmMode, OMX_AUDIO_PARAM_PCMMODETYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pPcmMode->nPortIndex == AUDIO_FILTER_OUTPUT_PORT)
				{
					bOutputPort = OMX_TRUE;
				}

				fsl_osal_memcpy(pPcmMode, &PcmMode,	sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
			}
			LOG_DEBUG("PcmMode.nSamplingRate = %d\n", PcmMode.nSamplingRate);
			break;
		default:
			ret = AudioFilterGetParameter(nParamIndex, pComponentParameterStructure);
			break;
	}

	return ret;
}

OMX_ERRORTYPE AudioFilter::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioPortFormat:
            {
                OMX_AUDIO_PARAM_PORTFORMATTYPE *pPortFormat;
                pPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pPortFormat, OMX_AUDIO_PARAM_PORTFORMATTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pPortFormat->nPortIndex > AUDIO_FILTER_PORT_NUMBER - 1)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(&PortFormat[pPortFormat->nPortIndex], pPortFormat, \
						sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
            }
			break;
        case OMX_IndexParamAudioPcm:
            {
                OMX_AUDIO_PARAM_PCMMODETYPE *pPcmMode;
                pPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pPcmMode, OMX_AUDIO_PARAM_PCMMODETYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				fsl_osal_memcpy(&PcmMode, pPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
			}
			AudioFilterSetParameterPCM();
			break;
		default:
			ret = AudioFilterSetParameter(nParamIndex, pComponentParameterStructure);
			break;
	}

    return ret;
}

OMX_ERRORTYPE AudioFilter::AudioFilterSetParameterPCM()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return ret;
}
 
OMX_ERRORTYPE AudioFilter::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

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
	LOG_DEBUG("Ring buffer push mode input len: %d\n", nPushModeInputLen);

	pInBufferHdr = pOutBufferHdr = NULL; 
	bReceivedEOS = OMX_FALSE;
	bFirstFrame = OMX_FALSE;
	bCodecInit = OMX_FALSE;
	bInstanceReset = OMX_FALSE;
	bDecoderEOS = OMX_FALSE;
	bDecoderInitFail = OMX_FALSE;

	return ret;
}

OMX_ERRORTYPE AudioFilter::InstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	ret = AudioFilterInstanceDeInit();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Audio decoder instance de-init fail.\n");
		return ret;
	}

	LOG_DEBUG("Audio decoder instance de-init.\n");
	AudioRingBuffer.BufferFree();

    return ret;
}

OMX_ERRORTYPE AudioFilter::AudioFilterCheckCodecConfig()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	pInBufferHdr->nFilledLen = 0;
	return ret;
}
 
OMX_ERRORTYPE AudioFilter::ProcessDataBuffer()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    LOG_LOG("Audio filter In: %d, Out: %d\n", ports[AUDIO_FILTER_INPUT_PORT]->BufferNum(), ports[AUDIO_FILTER_OUTPUT_PORT]->BufferNum());

    if(((ports[AUDIO_FILTER_INPUT_PORT]->BufferNum() == 0 && pInBufferHdr == NULL )
				|| pInBufferHdr != NULL) /**< Ring buffer full */
            && (ports[AUDIO_FILTER_OUTPUT_PORT]->BufferNum() == 0  && pOutBufferHdr == NULL))
        return OMX_ErrorNoMore;
	if(pOutBufferHdr == NULL && ports[AUDIO_FILTER_OUTPUT_PORT]->BufferNum() > 0) {
		ports[AUDIO_FILTER_OUTPUT_PORT]->GetBuffer(&pOutBufferHdr);
		pOutBufferHdr->nFlags = 0;
	}

    if(pInBufferHdr == NULL && ports[AUDIO_FILTER_INPUT_PORT]->BufferNum() > 0) {
		ports[AUDIO_FILTER_INPUT_PORT]->GetBuffer(&pInBufferHdr);
		if(pInBufferHdr != NULL) {
			ret = ProcessInputBufferFlag();
			if (ret != OMX_ErrorNone)
			{
				LOG_ERROR("Process input buffer flag fail.\n");
				return ret;
			}
		}
	}

	if(pInBufferHdr != NULL) {
		ret = ProcessInputDataBuffer();
		if (ret != OMX_ErrorNone)
		{
			LOG_ERROR("Process input data buffer fail.\n");
			return ret;
		}
	}

	if (bInstanceReset == OMX_TRUE)
	{
		bInstanceReset = OMX_FALSE;
		ret = AudioFilterInstanceReset();
		if (ret != OMX_ErrorNone)
		{
			LOG_ERROR("Audio filter instance reset fail.\n");
			bDecoderEOS = OMX_TRUE;
			SendEvent(OMX_EventError, ret, 0, NULL);
			return ret;
		}
	} 

	if (bFirstFrame == OMX_TRUE)
	{
		ret = AudioFilterCheckFrameHeader();
		if (ret != OMX_ErrorNone)
		{
			LOG_ERROR("AudioFilterCheckFrameHeader fail.\n");
		}
            
	}

    	LOG_LOG("Audio Filter Ringbuffer data len: %d\n", AudioRingBuffer.AudioDataLen());
	if ((AudioRingBuffer.AudioDataLen() < nPushModeInputLen && bReceivedEOS == OMX_FALSE)
			|| bDecoderEOS == OMX_TRUE)
    {
        LOG_DEBUG("Input buffer is not enough for filter.\n");
        if(ports[AUDIO_FILTER_INPUT_PORT]->BufferNum() > 0)
			return OMX_ErrorNone;
		else
			return OMX_ErrorNoMore;
	}

	if (bCodecInit == OMX_FALSE)
	{
		bCodecInit = OMX_TRUE;
		ret = AudioFilterCodecInit();
		if (ret != OMX_ErrorNone)
		{
			LOG_ERROR("Audio decoder codec init fail.\n");
			bDecoderInitFail = OMX_TRUE;
		}
	}  

	if(pOutBufferHdr != NULL) 
	{
		ret = ProcessOutputDataBuffer();
		if (ret != OMX_ErrorNone)
			LOG_ERROR("Process Output data buffer fail.\n");
	}

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioFilter::AudioFilterInstanceReset()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	return ret;
}

OMX_ERRORTYPE AudioFilter::ProcessInputBufferFlag()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if(pInBufferHdr->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
        {
            LOG_DEBUG("received audio codec config data.\n");
            AudioFilterCheckCodecConfig();
            return OMX_ErrorNone;
        }

	if(pInBufferHdr->nFlags & OMX_BUFFERFLAG_STARTTIME)
	{
		AudioRingBuffer.BufferReset();
		AudioFilterReset();
		bReceivedEOS = OMX_FALSE;
		bDecoderEOS = OMX_FALSE;
		bFirstFrame = OMX_TRUE; 
        	LOG_DEBUG("Audio Filter received STARTTIME.\n");
	}

	if(pInBufferHdr->nFlags & OMX_BUFFERFLAG_EOS)
	{
		bReceivedEOS = OMX_TRUE;
		LOG_DEBUG("Audio Filter %s received EOS.\n", role[0]);
	}
 
	LOG_DEBUG_INS("FilledLen = %d, TimeStamp = %lld\n", \
			pInBufferHdr->nFilledLen, pInBufferHdr->nTimeStamp);
	AudioRingBuffer.TS_Add(pInBufferHdr->nTimeStamp);

	return ret;
}


OMX_ERRORTYPE AudioFilter::ProcessInputDataBuffer()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 nActuralLen;

	/** Process audio data */
	AudioRingBuffer.BufferAdd(pInBufferHdr->pBuffer + pInBufferHdr->nOffset, 
                pInBufferHdr->nFilledLen,
				&nActuralLen);

	if (nActuralLen < pInBufferHdr->nFilledLen)
	{
        pInBufferHdr->nOffset += nActuralLen;
        pInBufferHdr->nFilledLen -= nActuralLen;
	}
	else
	{
        pInBufferHdr->nOffset = 0;
        pInBufferHdr->nFilledLen = 0;
        ports[AUDIO_FILTER_INPUT_PORT]->SendBuffer(pInBufferHdr);
        pInBufferHdr = NULL;
	}
	
    return ret;
}

OMX_ERRORTYPE AudioFilter::AudioFilterCheckFrameHeader()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	
    return ret;
}


OMX_ERRORTYPE AudioFilter::ProcessOutputDataBuffer()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	AudioRingBuffer.TS_Get(&pOutBufferHdr->nTimeStamp);

	AUDIO_FILTERRETURNTYPE DecodeRet;

	if (bDecoderInitFail == OMX_TRUE)
	{
		AudioRingBuffer.BufferReset();
		bReceivedEOS = OMX_TRUE;
		DecodeRet = AUDIO_FILTER_EOS;
	}
	else
	{
		DecodeRet = AudioFilterFrame();
		if (DecodeRet == AUDIO_FILTER_FAILURE)
		{
			LOG_WARNING("Decode frame fail.\n");
                     fsl_osal_sleep(4000);
                     return OMX_ErrorNone;
		}
	}

	if (DecodeRet == AUDIO_FILTER_EOS && bReceivedEOS == OMX_TRUE && AudioRingBuffer.AudioDataLen() == 0)
	{
		LOG_DEBUG("Audio Filter %s send EOS, len %d\n", role[0], pOutBufferHdr->nFilledLen);
		pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_EOS;
		bDecoderEOS = OMX_TRUE;
		SendEvent(OMX_EventBufferFlag, AUDIO_FILTER_OUTPUT_PORT, OMX_BUFFERFLAG_EOS, NULL);
	}

	if (bFirstFrame == OMX_TRUE)
	{
		bFirstFrame = OMX_FALSE;
		pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_STARTTIME;
	}
	pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
	pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

    	LOG_LOG("audio filter pOutBufferHdr->nTimeStamp = %lld, \n", pOutBufferHdr->nTimeStamp);
    ports[AUDIO_FILTER_OUTPUT_PORT]->SendBuffer(pOutBufferHdr);
    pOutBufferHdr = NULL;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioFilter::ComponentReturnBuffer(
        OMX_U32 nPortIndex)
{
	bDecoderEOS = OMX_FALSE;
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

OMX_ERRORTYPE AudioFilter::FlushComponent(
        OMX_U32 nPortIndex)
{
    if(nPortIndex == AUDIO_FILTER_INPUT_PORT && pInBufferHdr != NULL) {
        ports[AUDIO_FILTER_INPUT_PORT]->SendBuffer(pInBufferHdr);
        pInBufferHdr = NULL;
    }

    if(nPortIndex == AUDIO_FILTER_OUTPUT_PORT && pOutBufferHdr != NULL) {
        ports[AUDIO_FILTER_OUTPUT_PORT]->SendBuffer(pOutBufferHdr);
        pOutBufferHdr = NULL;
    }

	bReceivedEOS = OMX_FALSE;
	AudioRingBuffer.BufferReset();
	LOG_DEBUG("Clear ring buffer.\n");

    return OMX_ErrorNone;
}


/* File EOF */
