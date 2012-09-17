/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
 
#include "AudioSource.h"
 
#define CHECK_STRUCT(ps,t,r) \
    do { \
        OMX_CHECK_STRUCT(ps, t, r); \
        if(r != OMX_ErrorNone) \
        break; \
        if(ps->nPortIndex != OUTPUT_PORT) { \
            r = OMX_ErrorBadPortIndex; \
            break; \
        } \
    } while(0);

AudioSource::AudioSource()
{
    bInContext = OMX_FALSE;
    nPorts = NUM_PORTS;
}

OMX_ERRORTYPE AudioSource::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = OUTPUT_PORT;
    sPortDef.eDir = OMX_DirOutput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
	sPortDef.nBufferSize = 1024;
	ret = ports[OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for in port failed.\n");
        return ret;
    }

    sPortDef.nPortIndex = CLK_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainOther;
    sPortDef.format.other.eFormat = OMX_OTHER_FormatTime;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_FALSE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 1;
    sPortDef.nBufferSize = sizeof(OMX_TIME_MEDIATIMETYPE);
    ret = ports[CLK_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for clk port failed.\n");
        return ret;
    }

	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	PcmMode.nPortIndex = OUTPUT_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	OMX_INIT_STRUCT(&Volume, OMX_AUDIO_CONFIG_VOLUMETYPE);

	Volume.bLinear = OMX_TRUE;
	Volume.sVolume.nValue = 100;
	Volume.sVolume.nMin = 0;
	Volume.sVolume.nMax = 100;

	OMX_INIT_STRUCT(&EOS, OMX_CONFIG_BOOLEANTYPE);

	EOS.bEnabled = OMX_FALSE;

	OMX_INIT_STRUCT(&Mute, OMX_AUDIO_CONFIG_MUTETYPE);

	Mute.bMute = OMX_FALSE;

	OMX_INIT_STRUCT(&StartTime, OMX_TIME_CONFIG_TIMESTAMPTYPE);
	StartTime.nTimestamp = 0;

	bFirstFrame = OMX_TRUE;
	pOutBufferHdr = NULL;
	TotalRecordedLen = 0;
	nDeviceReadLen = 0;
	nDeviceReadOffset = 0;
	bAddZeros = OMX_FALSE;
	pDeviceReadBuffer = NULL;
	nMaxDuration = MAX_VALUE_S64;
	nAudioSource = 0;
	bSendEOS = OMX_FALSE;

    ret = InitSourceComponent();
    if(ret != OMX_ErrorNone)
        return ret;

	return ret;
}

OMX_ERRORTYPE AudioSource::DeInitComponent()
{
	AudioRenderFadeInFadeOut.SetMode(FADEOUT);

    DeInitSourceComponent();

	return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioSource::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	return ret;
}

OMX_ERRORTYPE AudioSource::InstanceDeInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	StopDevice();

    return ret;
}

OMX_ERRORTYPE AudioSource::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
		case OMX_IndexParamAudioPcm:
            {
                OMX_AUDIO_PARAM_PCMMODETYPE *pPcmMode;
                pPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)pStructure;
                CHECK_STRUCT(pPcmMode, OMX_AUDIO_PARAM_PCMMODETYPE, ret);
				fsl_osal_memcpy(pPcmMode, &PcmMode,	sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
			}
			break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE AudioSource::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioPcm:
            {
                OMX_AUDIO_PARAM_PCMMODETYPE *pPcmMode;
                pPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)pStructure;
                CHECK_STRUCT(pPcmMode, OMX_AUDIO_PARAM_PCMMODETYPE, ret);
				fsl_osal_memcpy(&PcmMode, pPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
				LOG_DEBUG("Audio source samplerate: %d\n", PcmMode.nSamplingRate);
				LOG_DEBUG("Audio source channels: %d\n", PcmMode.nChannels);
				LOG_DEBUG("Audio source bitspersample: %d\n", PcmMode.nBitPerSample);
			}
            break;
		case OMX_IndexParamMaxFileDuration:
			 {
				 nMaxDuration = *((OMX_TICKS*)pStructure);
			 }
			 break;
		case OMX_IndexParamAudioSource:
			{
				 nAudioSource = *((OMX_S32*)pStructure);
			}
			break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE AudioSource::GetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexConfigAudioVolume:
            {
                OMX_AUDIO_CONFIG_VOLUMETYPE *pVolume;
                pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*)pStructure;
                CHECK_STRUCT(pVolume, OMX_AUDIO_CONFIG_VOLUMETYPE, ret);
				fsl_osal_memcpy(pVolume, &Volume, sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
            }
            break;
        case OMX_IndexConfigAudioMute:
             {
                OMX_AUDIO_CONFIG_MUTETYPE *pMute;
                pMute = (OMX_AUDIO_CONFIG_MUTETYPE*)pStructure;
                CHECK_STRUCT(pMute, OMX_AUDIO_CONFIG_MUTETYPE, ret);
				fsl_osal_memcpy(pMute, &Mute, sizeof(OMX_AUDIO_CONFIG_MUTETYPE));
            }
            break;
        case OMX_IndexConfigMaxAmplitude:
            {
				 *((OMX_S32*)pStructure) = getMaxAmplitude();
            }
            break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE AudioSource::SetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexConfigAudioVolume:
            {
                OMX_AUDIO_CONFIG_VOLUMETYPE *pVolume;
                pVolume = (OMX_AUDIO_CONFIG_VOLUMETYPE*)pStructure;
                CHECK_STRUCT(pVolume, OMX_AUDIO_CONFIG_VOLUMETYPE, ret);
				fsl_osal_memcpy(&Volume, pVolume, sizeof(OMX_AUDIO_CONFIG_VOLUMETYPE));
            }
            break;
        case OMX_IndexConfigAudioMute:
             {
                OMX_AUDIO_CONFIG_MUTETYPE *pMute;
                pMute = (OMX_AUDIO_CONFIG_MUTETYPE*)pStructure;
                CHECK_STRUCT(pMute, OMX_AUDIO_CONFIG_MUTETYPE, ret);
				fsl_osal_memcpy(&Mute, pMute, sizeof(OMX_AUDIO_CONFIG_MUTETYPE));
            }
            break;
		case OMX_IndexConfigEOS:
			 {
				 OMX_CONFIG_BOOLEANTYPE *pEOS;
				 pEOS = (OMX_CONFIG_BOOLEANTYPE*)pStructure;
				 OMX_CHECK_STRUCT(pEOS, OMX_CONFIG_BOOLEANTYPE, ret);
				 fsl_osal_memcpy(&EOS, pEOS, sizeof(OMX_CONFIG_BOOLEANTYPE));
			 }
			 break;
		default:
			 ret = OMX_ErrorUnsupportedIndex;
			 break;
    }

    return ret;
}
 
OMX_ERRORTYPE AudioSource::ProcessDataBuffer()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 nActuralLen;
	
	if (bSendEOS == OMX_TRUE)
		return OMX_ErrorNoMore;

	if (AudioRenderRingBuffer.AudioDataLen() <= nFadeInFadeOutProcessLen) {
		if (nDeviceReadLen == 0) {
			LOG_DEBUG("Ringbuffer data len: %d\n", AudioRenderRingBuffer.AudioDataLen());
			ret = GetOneFrameFromDevice();
			if (ret == OMX_ErrorUndefined) {
				OMX_U8 *pBuffer;

				AudioRenderRingBuffer.BufferGet(&pBuffer, nFadeInFadeOutProcessLen, &nActuralLen);
				AudioRenderFadeInFadeOut.SetMode(FADEOUT);
				AudioRenderFadeInFadeOut.Process(pBuffer, nActuralLen);
				AudioRenderRingBuffer.BufferConsumered(nActuralLen);
				AudioRenderRingBuffer.BufferAdd(pBuffer, nFadeInFadeOutProcessLen, &nActuralLen);

				nDeviceReadOffset = 0;
				AudioRenderRingBuffer.BufferAddZeros(nDeviceReadLen, &nActuralLen);
				LOG_LOG("Ringbuffer add len: %d\n", nActuralLen);
				nDeviceReadLen -= nActuralLen;
				nDeviceReadOffset += nActuralLen;
				bAddZeros = OMX_TRUE;
				if (nDeviceReadLen == 0)
					AudioRenderFadeInFadeOut.SetMode(FADEIN);

			} else {
				if (ret != OMX_ErrorNone)
					return ret;

				bAddZeros = OMX_FALSE;
				nDeviceReadOffset = 0;
				AudioRenderFadeInFadeOut.Process(pDeviceReadBuffer, nDeviceReadLen);
				AudioRenderRingBuffer.BufferAdd(pDeviceReadBuffer, nDeviceReadLen, &nActuralLen);
				LOG_LOG("Ringbuffer add len: %d\n", nActuralLen);
				nDeviceReadLen -= nActuralLen;
				nDeviceReadOffset += nActuralLen;
			}
		} else {
			if (bAddZeros == OMX_TRUE) {
				AudioRenderRingBuffer.BufferAddZeros(nDeviceReadLen, &nActuralLen);
				nDeviceReadLen -= nActuralLen;
				nDeviceReadOffset += nActuralLen;
				if (nDeviceReadLen == 0)
					AudioRenderFadeInFadeOut.SetMode(FADEIN);
			} else {
				AudioRenderFadeInFadeOut.Process(pDeviceReadBuffer + nDeviceReadOffset, nDeviceReadLen);
				AudioRenderRingBuffer.BufferAdd(pDeviceReadBuffer + nDeviceReadOffset, nDeviceReadLen, &nActuralLen);
				nDeviceReadLen -= nActuralLen;
				nDeviceReadOffset += nActuralLen;
			}
		}
	}

	LOG_LOG("Ringbuffer data len: %d\n", AudioRenderRingBuffer.AudioDataLen());
	while(AudioRenderRingBuffer.AudioDataLen() > nFadeInFadeOutProcessLen) {
		if (ports[OUTPUT_PORT]->BufferNum() == 0 && pOutBufferHdr == NULL ) {
			fsl_osal_sleep(5000);
			continue;
		}

		if (ports[OUTPUT_PORT]->BufferNum() > 0 && pOutBufferHdr == NULL) {
			ports[OUTPUT_PORT]->GetBuffer(&pOutBufferHdr);
		}

		ret = ProcessOutputBufferData();
		if (ret != OMX_ErrorNone)
			return ret;
		ret = ProcessOutputBufferFlag();
		if (ret != OMX_ErrorNone)
			return ret;
		ret = SendOutputBuffer();
		if (ret != OMX_ErrorNone)
			return ret;
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioSource::ProcessOutputBufferData()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;
	OMX_U32 nSendDataLen = pOutBufferHdr->nAllocLen;
	if (nSendDataLen > AudioRenderRingBuffer.AudioDataLen() - nFadeInFadeOutProcessLen)
		nSendDataLen = AudioRenderRingBuffer.AudioDataLen() - nFadeInFadeOutProcessLen;

	AudioRenderRingBuffer.BufferGet(&pBuffer, nSendDataLen, &nActuralLen);
 
	fsl_osal_memcpy(pOutBufferHdr->pBuffer, pBuffer, nActuralLen);
	pOutBufferHdr->nFilledLen = nActuralLen;

	LOG_LOG("pOutBufferHdr->nAllocLen = %d\t pOutBufferHdr->nFilledLen = %d\n", pOutBufferHdr->nAllocLen, pOutBufferHdr->nFilledLen);
	AudioRenderRingBuffer.BufferConsumered(nActuralLen);

	return ret;
}

OMX_ERRORTYPE AudioSource::ProcessOutputBufferFlag()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (bFirstFrame == OMX_TRUE) {
		pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_STARTTIME;
		bFirstFrame = OMX_FALSE;
	}
	pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

	return ret;
}

OMX_ERRORTYPE AudioSource::SendOutputBuffer()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_TIME_CONFIG_TIMESTAMPTYPE ReferTime;
	OMX_U32 nDelayLen;
	OMX_S64 MediaTime;

	OMX_INIT_STRUCT(&ReferTime, OMX_TIME_CONFIG_TIMESTAMPTYPE);

	GetDeviceDelay(&nDelayLen);

	TotalRecordedLen += pOutBufferHdr->nFilledLen;
	MediaTime = StartTime.nTimestamp + ((TotalRecordedLen + nDelayLen)/nSampleSize * OMX_TICKS_PER_SECOND) / PcmMode.nSamplingRate;
	pOutBufferHdr->nTimeStamp = StartTime.nTimestamp + ((TotalRecordedLen)/nSampleSize * OMX_TICKS_PER_SECOND) / PcmMode.nSamplingRate;

	ReferTime.nTimestamp = MediaTime;
	LOG_DEBUG("Audio source total recorded data: %lld\n", TotalRecordedLen);
	LOG_DEBUG("Set reference time to clock: %lld\n", MediaTime);
	//ClockSetConfig(OMX_IndexConfigTimeCurrentAudioReference, &ReferTime);
   
	if (EOS.bEnabled == OMX_TRUE || pOutBufferHdr->nTimeStamp > nMaxDuration) {
		pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_EOS;
		pOutBufferHdr->nFilledLen = 0;
		bSendEOS = OMX_TRUE;
		SendEvent(OMX_EventBufferFlag, 0, OMX_BUFFERFLAG_MAX_DURATION, NULL);
	}

	LOG_DEBUG("Audio source send buffer len: %d, flags: %p offset: %d time stamp: %lld\n", \
			pOutBufferHdr->nFilledLen, pOutBufferHdr->nFlags, pOutBufferHdr->nOffset, \
			pOutBufferHdr->nTimeStamp);
	ports[OUTPUT_PORT]->SendBuffer(pOutBufferHdr);
	pOutBufferHdr = NULL;
	
	return ret;
}

OMX_ERRORTYPE AudioSource::ClockGetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(ports[CLK_PORT]->IsEnabled() != OMX_TRUE)
	{
		return ret;
	}

	TUNNEL_INFO TunnelInfo;
    ports[CLK_PORT]->GetTunneledInfo(&TunnelInfo);
	OMX_COMPONENTTYPE *hClockComp = (OMX_COMPONENTTYPE *)TunnelInfo.hTunneledComp;
    if(hClockComp == NULL)
	{
				return ret;
	}

	switch (nParamIndex) {
		case OMX_IndexConfigTimeClockState:
			{
				OMX_TIME_CONFIG_CLOCKSTATETYPE *pClockState = (OMX_TIME_CONFIG_CLOCKSTATETYPE *)pStructure;
				ret = OMX_GetConfig(hClockComp, OMX_IndexConfigTimeClockState, pClockState);
			}
			break;
		case OMX_IndexConfigTimeCurrentMediaTime:
			{
				OMX_TIME_CONFIG_TIMESTAMPTYPE *pCur = (OMX_TIME_CONFIG_TIMESTAMPTYPE *)pStructure;
				pCur->nPortIndex = TunnelInfo.nTunneledPort;
				ret = OMX_GetConfig(hClockComp, OMX_IndexConfigTimeCurrentMediaTime, pCur);
			}
			break;
		default :
			ret = OMX_ErrorUnsupportedIndex;
			break;
	}

	return ret;
}

OMX_ERRORTYPE AudioSource::ClockSetConfig(OMX_INDEXTYPE nParamIndex, OMX_TIME_CONFIG_TIMESTAMPTYPE *pTimeStamp)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(ports[CLK_PORT]->IsEnabled() != OMX_TRUE)
		return ret;

	TUNNEL_INFO TunnelInfo;
    ports[CLK_PORT]->GetTunneledInfo(&TunnelInfo);
	OMX_COMPONENTTYPE *hClockComp = (OMX_COMPONENTTYPE *)TunnelInfo.hTunneledComp;
    if(hClockComp == NULL)
		return ret;

	pTimeStamp->nPortIndex = TunnelInfo.nTunneledPort;

    switch (nParamIndex) {
        case OMX_IndexConfigTimeClientStartTime:
			ret = OMX_SetConfig(hClockComp, OMX_IndexConfigTimeClientStartTime,pTimeStamp);
            break;
        case OMX_IndexConfigTimeCurrentAudioReference:
			ret = OMX_SetConfig(hClockComp, OMX_IndexConfigTimeCurrentAudioReference,pTimeStamp);
            break;
        default :
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

	return ret;
}

OMX_ERRORTYPE AudioSource::ProcessClkBuffer()
{
    if(ports[CLK_PORT]->IsEnabled() != OMX_TRUE)
        return OMX_ErrorNoMore;

	TUNNEL_INFO TunnelInfo;
    ports[CLK_PORT]->GetTunneledInfo(&TunnelInfo);
	OMX_COMPONENTTYPE *hClockComp = (OMX_COMPONENTTYPE *)TunnelInfo.hTunneledComp;
    if(hClockComp == NULL)
        return OMX_ErrorNoMore;

    if(ports[CLK_PORT]->BufferNum() == 0)
		return OMX_ErrorNoMore;

	OMX_BUFFERHEADERTYPE *pClockBufferHdr;
	ports[CLK_PORT]->GetBuffer(&pClockBufferHdr);
	OMX_TIME_MEDIATIMETYPE *pTimeBuffer = (OMX_TIME_MEDIATIMETYPE*) pClockBufferHdr->pBuffer;

	if ((pTimeBuffer->eUpdateType == OMX_TIME_UpdateClockStateChanged)
		&& (pTimeBuffer->eState == OMX_TIME_ClockStateRunning))
	{
	}
	else if (pTimeBuffer->eUpdateType == OMX_TIME_UpdateScaleChanged)
	{
	}
	else
	{
		LOG_WARNING("Unknow clock buffer.\n");
	}

	ports[CLK_PORT]->SendBuffer(pClockBufferHdr);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioSource::DoLoaded2Idle()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = OpenDevice();
    if(ret != OMX_ErrorNone) {
        CloseDevice();
        SendEvent(OMX_EventError, ret, 0, NULL);
        return ret;
	}

	nDeviceReadLen = nPeriodSize * nSampleSize;
	OMX_U32 BufferSize;
	OMX_U8 *pBuffer;

	BufferSize = nDeviceReadLen;
	pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
	if (pBuffer == NULL) {
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pBuffer, 0, BufferSize);
	pDeviceReadBuffer = pBuffer;

	OMX_U32 nRingBufferSize = nFadeInFadeOutProcessLen + nDeviceReadLen;
	RINGBUFFER_ERRORTYPE BufferRet = RINGBUFFER_SUCCESS;
	LOG_DEBUG("Audio source nRingBufferSize = %d\n", nRingBufferSize);
	BufferRet = AudioRenderRingBuffer.BufferCreate(nRingBufferSize);
	if (BufferRet != RINGBUFFER_SUCCESS) {
		LOG_ERROR("Create ring buffer fail.\n");
		return OMX_ErrorInsufficientResources;
	} 

	FADEINFADEOUT_ERRORTYPE FadeRet = FADEINFADEOUT_SUCCESS;
	FadeRet = AudioRenderFadeInFadeOut.Create(PcmMode.nChannels, PcmMode.nSamplingRate, \
			PcmMode.nBitPerSample, nFadeInFadeOutProcessLen, 0);
	if (FadeRet != FADEINFADEOUT_SUCCESS) {
		LOG_ERROR("Create fade in fade out process fail.\n");
		return OMX_ErrorInsufficientResources;
	}

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioSource::DoIdle2Loaded()
{
 	CloseDevice();

	LOG_DEBUG("Audio source ring buffer free.\n");
	AudioRenderRingBuffer.BufferFree();

	FSL_FREE(pDeviceReadBuffer);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioSource::DoPause2Exec()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	AudioRenderFadeInFadeOut.SetMode(FADEIN);

	printf("Set AudioSource start time.\n");
	ClockSetConfig(OMX_IndexConfigTimeClientStartTime, &StartTime);

	StartDevice();

    return ret;
}

OMX_ERRORTYPE AudioSource::ComponentReturnBuffer(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	StopDevice();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioSource::FlushComponent(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	StopDevice();

	return OMX_ErrorNone;
}

/* File EOF */
