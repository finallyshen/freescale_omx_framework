/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
 
#include "AudioRender.h"
 
#define AUDIO_INTERVAL_THRESHOLD 1000000

#define CHECK_STRUCT(ps,t,r) \
    do { \
        OMX_CHECK_STRUCT(ps, t, r); \
        if(r != OMX_ErrorNone) \
        break; \
        if(ps->nPortIndex != IN_PORT) { \
            r = OMX_ErrorBadPortIndex; \
            break; \
        } \
    } while(0);

OMX_ERRORTYPE AudioRender::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = IN_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
    sPortDef.nBufferSize = 1024;
    ret = ports[IN_PORT]->SetPortDefinition(&sPortDef);
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

	PcmMode.nPortIndex = IN_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

#ifdef OMX_STEREO_OUTPUT
	OMX_INIT_STRUCT(&PcmModeIn, OMX_AUDIO_PARAM_PCMMODETYPE);

	PcmModeIn.nPortIndex = IN_PORT;
	PcmModeIn.nChannels = 2;
	PcmModeIn.nSamplingRate = 44100;
	PcmModeIn.nBitPerSample = 16;
	PcmModeIn.bInterleaved = OMX_TRUE;
	PcmModeIn.eNumData = OMX_NumericalDataSigned;
	PcmModeIn.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmModeIn.eEndian = OMX_EndianLittle;
	PcmModeIn.eChannelMapping[0] = OMX_AUDIO_ChannelNone;
#endif

	OMX_INIT_STRUCT(&Volume, OMX_AUDIO_CONFIG_VOLUMETYPE);

	Volume.bLinear = OMX_TRUE;
	Volume.sVolume.nValue = 100;
	Volume.sVolume.nMin = 0;
	Volume.sVolume.nMax = 100;

	OMX_INIT_STRUCT(&Mute, OMX_AUDIO_CONFIG_MUTETYPE);

	Mute.bMute = OMX_FALSE;

	OMX_INIT_STRUCT(&StartTime, OMX_TIME_CONFIG_TIMESTAMPTYPE);
	StartTime.nTimestamp = 0;
	OMX_INIT_STRUCT(&ReferTime, OMX_TIME_CONFIG_TIMESTAMPTYPE);
	ReferTime.nTimestamp = 0;
 
	nWriteOutLen = 0;
	nPeriodSize = 0;
	nSampleSize = 0;
	nAudioDataWriteThreshold = 0;
	nFadeInFadeOutProcessLen = 0;
	bClockRuningFlag = OMX_FALSE;
	bHeardwareError = OMX_FALSE;
	bReceivedEOS = OMX_FALSE;
	bSendEOS = OMX_FALSE;
	CurrentTS = 0;
	nDiscardData = 0;
	nAddZeros = 0;
	TotalConsumeredLen = 0;
	nClockScale = Q16_SHIFT;
	nDNSeScale = Q16_SHIFT;

	pInBufferHdr = NULL;
	pInBufferHdrBak = NULL;
       audioIntervalThreshold = AUDIO_INTERVAL_THRESHOLD;

	return ret;
}

OMX_ERRORTYPE AudioRender::DeInitComponent()
{
	if (pInBufferHdrBak)
	{
		FSL_FREE(pInBufferHdrBak->pBuffer);
		FSL_FREE(pInBufferHdrBak);
	}

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioRender::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return ret;
}

OMX_ERRORTYPE AudioRender::InstanceDeInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    return ret;
}

OMX_ERRORTYPE AudioRender::GetParameter(
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
#ifdef OMX_STEREO_OUTPUT
				fsl_osal_memcpy(pPcmMode, &PcmModeIn,	sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
#endif
			}
			break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE AudioRender::SetParameter(
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
				LOG_INFO("Audio render samplerate: %d, channels: %d, bitspersample: %d\n", PcmMode.nSamplingRate, PcmMode.nChannels, PcmMode.nBitPerSample);

#ifdef OMX_STEREO_OUTPUT
				fsl_osal_memcpy(&PcmModeIn, pPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
				if (PcmMode.nChannels > 2)
				{
					PcmMode.nChannels = 2;
				}
				if (PcmMode.nBitPerSample > 16)
				{
					PcmMode.nBitPerSample = 16;
				}
#endif
				LOG_DEBUG("Audio render samplerate: %d\n", PcmMode.nSamplingRate);
				LOG_DEBUG("Audio render channels: %d\n", PcmMode.nChannels);
				LOG_DEBUG("Audio render bitspersample: %d\n", PcmMode.nBitPerSample);
			}
            break;
        case OMX_IndexParamAudioSink:
            SelectDevice(pStructure);
            break;

        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE AudioRender::GetConfig(
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
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE AudioRender::SetConfig(
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
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}
 
OMX_ERRORTYPE AudioRender::ProcessDataBuffer()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(bHeardwareError == OMX_TRUE)
        return OMX_ErrorNoMore;

	LOG_LOG("Input port buffer number = %d\t pInBufferHdr = %p\n", ports[IN_PORT]->BufferNum(), pInBufferHdr);
    if(pInBufferHdr == NULL && ports[IN_PORT]->BufferNum() > 0) {
		ports[IN_PORT]->GetBuffer(&pInBufferHdr);
#ifdef OMX_STEREO_OUTPUT
	if (PcmModeIn.nChannels > 2)
	{
		OMX_U32 i, j, Len;

		Len = pInBufferHdr->nFilledLen / (PcmModeIn.nBitPerSample>>3) / PcmModeIn.nChannels;
		pInBufferHdr->nFilledLen = Len * (PcmModeIn.nBitPerSample>>3) * PcmMode.nChannels;

		switch(PcmModeIn.nBitPerSample)
		{
			case 8:
				{
					OMX_S8 *pSrc = (OMX_S8 *)pInBufferHdr->pBuffer, *pDst = (OMX_S8 *)pInBufferHdr->pBuffer;
					for (i = 0; i < Len; i ++)
					{
						*pDst++ = *pSrc++;
						*pDst++ = *pSrc++;
						pSrc += PcmModeIn.nChannels - 2;
					}
				}
				break;
			case 16:
				{
					OMX_S16 *pSrc = (OMX_S16 *)pInBufferHdr->pBuffer, *pDst = (OMX_S16 *)pInBufferHdr->pBuffer;
					for (i = 0; i < Len; i ++)
					{
						*pDst++ = *pSrc++;
						*pDst++ = *pSrc++;
						pSrc += PcmModeIn.nChannels - 2;
					}
				}
				break;
			case 24:
				{
					OMX_U8 *pSrc = (OMX_U8 *)pInBufferHdr->pBuffer, *pDst = (OMX_U8 *)pInBufferHdr->pBuffer;
					for (i = 0; i < Len; i ++)
					{
						*pDst++ = *pSrc++;
						*pDst++ = *pSrc++;
						*pDst++ = *pSrc++;
						*pDst++ = *pSrc++;
						*pDst++ = *pSrc++;
						*pDst++ = *pSrc++;
						pSrc += (PcmModeIn.nChannels - 2)*3;
					}
				}
				break;
		}
	}
	if(PcmModeIn.nBitPerSample == 24)
	{
		OMX_S32 i,j,Len;
		OMX_U8 *pDst =(OMX_U8 *)pInBufferHdr->pBuffer;
		OMX_U8 *pSrc =(OMX_U8 *)pInBufferHdr->pBuffer;
		Len = pInBufferHdr->nFilledLen / (PcmModeIn.nBitPerSample>>3) / PcmMode.nChannels;
		for(i=0;i<Len;i++)
		{
			for(j=0;j<(OMX_S32)PcmModeIn.nChannels;j++)
			{
				pDst[0] = pSrc[1];
				pDst[1] = pSrc[2];
				pDst+=2;
				pSrc+=3;
			}
		}
		pInBufferHdr->nFilledLen = Len * (PcmMode.nBitPerSample>>3) * PcmMode.nChannels;
	}

#endif
		if(pInBufferHdr != NULL) {
			ret = ProcessInputBufferFlag();
			if (ret != OMX_ErrorNone)
			{
				LOG_ERROR("Process input buffer flag fail.\n");
				return ret;
			}
		}
	}

	if(pInBufferHdr != NULL) 
	{
		ret = ProcessInputDataBuffer();
		if (ret != OMX_ErrorNone)
		{
			LOG_ERROR("Process input data buffer fail.\n");
			return ret;
		}
	}

	LOG_LOG("Audio Ring buffer len: %d\n", AudioRenderRingBuffer.AudioDataLen());
    if((AudioRenderRingBuffer.AudioDataLen() < nAudioDataWriteThreshold 
			&& bReceivedEOS == OMX_FALSE)
			&& (pInBufferHdr == NULL && ports[IN_PORT]->BufferNum() == 0))
        return OMX_ErrorNoMore;

    if((bReceivedEOS == OMX_TRUE && AudioRenderRingBuffer.AudioDataLen() == 0))
    {
        if(ports[IN_PORT]->BufferNum() > 0)
            return OMX_ErrorNone;
        else
		{
			if (bSendEOS == OMX_FALSE)
			{
				LOG_DEBUG("Audio render send EOS\n");
				SendEvent(OMX_EventBufferFlag, IN_PORT, OMX_BUFFERFLAG_EOS, NULL);
				bSendEOS = OMX_TRUE;
			}
			return OMX_ErrorNoMore;
		}
    }

	if((bClockRuningFlag == OMX_TRUE && ports[CLK_PORT]->IsEnabled() == OMX_TRUE)
			|| ports[CLK_PORT]->IsEnabled() == OMX_FALSE) {
		ret = RenderData();
		if (ret != OMX_ErrorNone)
		{
			LOG_ERROR("Render audio data fail.\n");
			return ret;
		}
	}

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioRender::ProcessInputBufferFlag()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	audioIntervalThreshold = AUDIO_INTERVAL_THRESHOLD;

	if(pInBufferHdr->nFlags & OMX_BUFFERFLAG_STARTTIME)
	{
		LOG_DEBUG("Audio render received STARTTIME flag.\n");
		OMX_TIME_CONFIG_CLOCKSTATETYPE ClockState;
		OMX_INIT_STRUCT(&ClockState, OMX_TIME_CONFIG_CLOCKSTATETYPE);
		ClockGetConfig(OMX_IndexConfigTimeClockState, &ClockState);
		AudioRenderFadeInFadeOut.SetMode(FADEIN);
		nWriteOutLen = nPeriodSize * nSampleSize;
		if (ClockState.eState == OMX_TIME_ClockStateRunning)
		{
			OMX_TIME_CONFIG_TIMESTAMPTYPE sCur;
			OMX_INIT_STRUCT(&sCur, OMX_TIME_CONFIG_TIMESTAMPTYPE);
			ClockGetConfig(OMX_IndexConfigTimeCurrentMediaTime, &sCur);

			CurrentTS = sCur.nTimestamp;
			StartTime.nTimestamp = CurrentTS;
			LOG_DEBUG("CurrentTS = %lld\n", CurrentTS);
			audioIntervalThreshold = 0; // fix for a/v sync lost after track select: discard all overdue data in SyncTS() when switching track, prevent data before seek point from coming into ringbuffer
		}
		else
		{
			StartTime.nTimestamp = pInBufferHdr->nTimeStamp;
			CurrentTS = pInBufferHdr->nTimeStamp;
			bClockRuningFlag = OMX_FALSE;
			ClockSetConfig(OMX_IndexConfigTimeClientStartTime, &StartTime);
			LOG_DEBUG("Audio render set start time: %lld\n", StartTime.nTimestamp);
		}
		nAddZeros = 0;
		nDiscardData = 0;
		bReceivedEOS = OMX_FALSE;
		bSendEOS = OMX_FALSE;
		TotalConsumeredLen = 0;
		ResetDevice();
	}

	if(pInBufferHdr->nFlags & OMX_BUFFERFLAG_EOS)
	{
		bReceivedEOS = OMX_TRUE;
		LOG_DEBUG("Audio render received EOS.\n");
	}

	if (pInBufferHdr->nFilledLen != 0 || pInBufferHdr->nTimeStamp != 0)
		SyncTS(pInBufferHdr->nTimeStamp);

	return ret;
}

OMX_ERRORTYPE AudioRender::ClockGetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(ports[CLK_PORT]->IsEnabled() != OMX_TRUE)
        return OMX_ErrorNoMore;

	TUNNEL_INFO TunnelInfo;
    ports[CLK_PORT]->GetTunneledInfo(&TunnelInfo);
	OMX_COMPONENTTYPE *hClockComp = (OMX_COMPONENTTYPE *)TunnelInfo.hTunneledComp;
    if(hClockComp == NULL)
        return OMX_ErrorNoMore;

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

OMX_ERRORTYPE AudioRender::ClockSetConfig(OMX_INDEXTYPE nParamIndex, OMX_TIME_CONFIG_TIMESTAMPTYPE *pTimeStamp)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(ports[CLK_PORT]->IsEnabled() != OMX_TRUE)
        return OMX_ErrorNoMore;

	TUNNEL_INFO TunnelInfo;
    ports[CLK_PORT]->GetTunneledInfo(&TunnelInfo);
	OMX_COMPONENTTYPE *hClockComp = (OMX_COMPONENTTYPE *)TunnelInfo.hTunneledComp;
    if(hClockComp == NULL)
        return OMX_ErrorNoMore;

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

OMX_ERRORTYPE AudioRender::SyncTS(OMX_TICKS nTimeStamp)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	LOG_LOG("CurrentTS = %lld\t nTimeStamp = %lld\n", CurrentTS, nTimeStamp);
	LOG_DEBUG("Input buffer data len: %d\n", pInBufferHdr->nFilledLen);
	if (CurrentTS > nTimeStamp)
	{
		LOG_DEBUG("Audio TS jetter: %lld\n", CurrentTS - nTimeStamp);
		if (CurrentTS - nTimeStamp > audioIntervalThreshold)
		{
			nDiscardData = (CurrentTS - nTimeStamp) * PcmMode.nSamplingRate / OMX_TICKS_PER_SECOND * nSampleSize;
			LOG_DEBUG("CurrentTS = %lld\t nTimeStamp = %lld\n", CurrentTS, nTimeStamp);
			LOG_DEBUG("Need discard audio data.\n");
			return ret;
		}
	}
	else if (CurrentTS < nTimeStamp)
	{
		LOG_DEBUG("Audio TS jetter: %lld\n", nTimeStamp - CurrentTS);
		if (nTimeStamp - CurrentTS > audioIntervalThreshold)
		{
			nAddZeros = (nTimeStamp - CurrentTS) * PcmMode.nSamplingRate / OMX_TICKS_PER_SECOND * nSampleSize;
			LOG_DEBUG("CurrentTS = %lld\t nTimeStamp = %lld\n", CurrentTS, nTimeStamp);
			LOG_DEBUG("Audio render add zeros: %d\n", nAddZeros);
			LOG_DEBUG("Need add zeros in audio render.\n");
			return ret;
		}
	}

	CurrentTS = nTimeStamp + (OMX_TICKS)pInBufferHdr->nFilledLen * OMX_TICKS_PER_SECOND / \
				PcmMode.nChannels / PcmMode.nBitPerSample * 8 / PcmMode.nSamplingRate;
	LOG_LOG("Frame Len = %d\t Channels = %d\t BitPerSample = %d\t Sample Rate = %d\n", \
			pInBufferHdr->nFilledLen, PcmMode.nChannels, PcmMode.nBitPerSample, PcmMode.nSamplingRate);
	return ret;
}

OMX_ERRORTYPE AudioRender::ProcessInputDataBuffer()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 nActuralLen;

	if(pInBufferHdr->nFlags & OMX_BUFFERFLAG_STARTTIME)
	{
		// need return for mark buffer event.
		if (pInBufferHdrBak == NULL)
		{
			OMX_PTR ptr = NULL;
			OMX_U32 nSize;

			nSize = sizeof(OMX_BUFFERHEADERTYPE);
			ptr = FSL_MALLOC(nSize);
			if(ptr == NULL) {
				LOG_ERROR("Allocate memory failed, size: %d\n", nSize);
				return OMX_ErrorInsufficientResources;
			}

			fsl_osal_memset(ptr, 0, nSize);
			pInBufferHdrBak = (OMX_BUFFERHEADERTYPE *)ptr;

			nSize = pInBufferHdr->nAllocLen;
			ptr = FSL_MALLOC(nSize);
			if(ptr == NULL) {
				LOG_ERROR("Allocate memory failed, size: %d\n", nSize);
				return OMX_ErrorInsufficientResources;
			}

			fsl_osal_memset(ptr, 0, nSize);
			pInBufferHdrBak->pBuffer = (OMX_U8 *)ptr;
		}

		fsl_osal_memcpy(pInBufferHdrBak->pBuffer, pInBufferHdr->pBuffer, pInBufferHdr->nFilledLen);

		pInBufferHdrBak->nFilledLen = pInBufferHdr->nFilledLen;
		pInBufferHdrBak->nOffset = pInBufferHdr->nOffset;

		pInBufferHdr->nFilledLen = 0;
		ports[IN_PORT]->SendBuffer(pInBufferHdr);
		pInBufferHdr = NULL;
		LOG_DEBUG("pInBufferHdrBak: %p Len: %d\n", pInBufferHdrBak, pInBufferHdrBak->nFilledLen);
	}

	if (nAddZeros != 0 && pInBufferHdr)
	{
		LOG_DEBUG("Add zeros to ring buffer.\n");
		AudioRenderRingBuffer.BufferAddZeros(nAddZeros, &nActuralLen);
		nAddZeros -= nActuralLen;
		if (nAddZeros != 0)
		{
			return ret;
		}
		CurrentTS = pInBufferHdr->nTimeStamp;
	}

	if (nDiscardData != 0 && pInBufferHdr)
	{ 
		if (nDiscardData > pInBufferHdr->nFilledLen)
		{
			nDiscardData -= pInBufferHdr->nFilledLen;
			LOG_DEBUG("Discard audio data: %d\n", pInBufferHdr->nFilledLen);
			pInBufferHdr->nFilledLen = 0;
		}
		else
		{
			pInBufferHdr->nFilledLen -= nDiscardData;
			LOG_DEBUG("Discard audio data: %d\n", nDiscardData);
			nDiscardData = 0;
		}
	}

	if ((bClockRuningFlag == OMX_TRUE && ports[CLK_PORT]->IsEnabled() == OMX_TRUE)
			|| ports[CLK_PORT]->IsEnabled() == OMX_FALSE)
	{
		/** Process audio data */
		if (pInBufferHdrBak && pInBufferHdrBak->nFilledLen)
		{
			LOG_DEBUG("Process pInBufferHdrBak: %p Len: %d\n", pInBufferHdrBak, pInBufferHdrBak->nFilledLen);
			AudioRenderRingBuffer.BufferAdd(pInBufferHdrBak->pBuffer + pInBufferHdrBak->nOffset, 
					pInBufferHdrBak->nFilledLen,
					&nActuralLen);

			if (nActuralLen < pInBufferHdrBak->nFilledLen)
			{
				pInBufferHdrBak->nOffset += nActuralLen;
				pInBufferHdrBak->nFilledLen -= nActuralLen;
			}
			else
			{
				pInBufferHdrBak->nFilledLen = 0;
			}

		}

		if (pInBufferHdr)
		{
			AudioRenderRingBuffer.BufferAdd(pInBufferHdr->pBuffer + pInBufferHdr->nOffset, 
					pInBufferHdr->nFilledLen,
					&nActuralLen);

			if (nActuralLen < pInBufferHdr->nFilledLen)
			{
				pInBufferHdr->nOffset += nActuralLen;
				pInBufferHdr->nFilledLen -= nActuralLen;
			}
			else
			{
				pInBufferHdr->nFilledLen = 0;
				ports[IN_PORT]->SendBuffer(pInBufferHdr);
				pInBufferHdr = NULL;
			}
		}
	}
	
    return ret;
}

OMX_ERRORTYPE AudioRender::RenderData()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (AudioRenderRingBuffer.AudioDataLen() < nAudioDataWriteThreshold && bReceivedEOS == OMX_FALSE)
	{
		LOG_DEBUG("Not engough audio data for render.\n");
		return OMX_ErrorNone;
	}

	ret = RenderPeriod();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Render period fail.\n");
		return ret;
	}

	if (bReceivedEOS == OMX_TRUE && AudioRenderRingBuffer.AudioDataLen() == 0)
	{
		LOG_DEBUG("Audio render send EOS\n");
		if (bSendEOS == OMX_FALSE)
		{
			SendEvent(OMX_EventBufferFlag, IN_PORT, OMX_BUFFERFLAG_EOS, NULL);
			bSendEOS = OMX_TRUE;
		}
	}

    return OMX_ErrorNone;
} 

OMX_ERRORTYPE AudioRender::RenderPeriod()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;

	AudioRenderRingBuffer.BufferGet(&pBuffer, nWriteOutLen, &nActuralLen);
  
	LOG_LOG("nWriteOutLen = %d\t nActuralLen = %d\n", nWriteOutLen, nActuralLen);
	AudioRenderFadeInFadeOut.Process(pBuffer, nActuralLen);
    /*{
        int ii,jj,kk,ll;
        static int iiCnt=0;
        static FILE *pfTest, *pfTestRead;
        iiCnt ++;
        if (iiCnt==1)
        {
            pfTestRead = fopen("al05_48kHz_64kbps_q0.vorbis", "rb");
            if(pfTestRead == NULL)
            {
                printf("Unable to open test file! \n");
            }
        }
		//kk = fread(pBuffer, sizeof(char), nActuralLen, pfTestRead);

        if (iiCnt==1)
        {
            pfTest = fopen("DumpData.pcm", "wb");
            if(pfTest == NULL)
            {
                printf("Unable to open test file! \n");
            }
        }
        //for (ii = 0; ii < 1; ii++)
        {        
            //for (jj = 0; jj < 6; jj++)
            {        
                //for (kk = 0; kk < 32; kk++)
                {   
                    //fprintf(pfTest,"%d\n",((OMX_U8 *)pInBuffer)[kk]);
                    //fscanf(pfTest,"%d\n",&MP1D_scratch[ll][kk]);
                    kk = fwrite(pBuffer, sizeof(char), nActuralLen, pfTest);
                    fflush(pfTest);
                }
            }
        }
    }*/  

    /*{
        int ii,jj,kk,ll;
        static int iiCnt=0;
        static FILE *pfTest;
        iiCnt ++;
        if (iiCnt==1)
        {
            pfTest = fopen("DumpData.pcm", "wb");
            if(pfTest == NULL)
            {
                printf("Unable to open test file! \n");
            }
        }
        //for (ii = 0; ii < 1; ii++)
        {        
            //for (jj = 0; jj < 6; jj++)
            {        
                //for (kk = 0; kk < 32; kk++)
                {   
                    //fprintf(pfTest,"%d\n",((OMX_U8 *)pInBuffer)[kk]);
                    //fscanf(pfTest,"%d\n",&MP1D_scratch[ll][kk]);
                    kk = fwrite(pBuffer, sizeof(char), nActuralLen, pfTest);
                    fflush(pfTest);
                }
            }
        }
    }*/   
    
	ret = WriteDevice(pBuffer, nActuralLen);
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Can't write audio data to device.\n");
		return ret;
	}

	AudioRenderRingBuffer.BufferConsumered(nActuralLen);

	OMX_U32 nDelayLen;
	OMX_S64 PlayingTime;

	DeviceDelay(&nDelayLen);

	TotalConsumeredLen += nActuralLen;
	PlayingTime = StartTime.nTimestamp + ((TotalConsumeredLen - nDelayLen)/nSampleSize * OMX_TICKS_PER_SECOND) / PcmMode.nSamplingRate * ((float)nDNSeScale / Q16_SHIFT);

	ReferTime.nTimestamp = PlayingTime;
	LOG_LOG("Audio render total render data: %lld\n", TotalConsumeredLen);
	LOG_LOG("Set reference time to clock: %lld\n", PlayingTime);
	ClockSetConfig(OMX_IndexConfigTimeCurrentAudioReference, &ReferTime);
	LOG_LOG("RenderPeriod finished.\n");

    return ret;
}

OMX_ERRORTYPE AudioRender::SelectDevice(OMX_PTR device)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioRender::ProcessClkBuffer()
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
		OMX_ERRORTYPE ret = OMX_ErrorNone;
		bClockRuningFlag = OMX_TRUE;
		LOG_DEBUG("Audio start time: %lld Clock return start time: %lld\n", StartTime.nTimestamp, pTimeBuffer->nMediaTimestamp);
		if (StartTime.nTimestamp > pTimeBuffer->nMediaTimestamp)
		{
			/** Add zeros at ring buffer header */
			nAddZeros = (StartTime.nTimestamp - pTimeBuffer->nMediaTimestamp) * PcmMode.nSamplingRate / OMX_TICKS_PER_SECOND * nSampleSize;
			StartTime.nTimestamp = pTimeBuffer->nMediaTimestamp;
		}
		else if (StartTime.nTimestamp < pTimeBuffer->nMediaTimestamp)
		{
			/** Discard audio data at ring buffer header */

		}
 
		ret = RenderData();
		if (ret != OMX_ErrorNone)
		{
			LOG_ERROR("Render audio data fail.\n");
			return ret;
		}

	}
	else if (pTimeBuffer->eUpdateType == OMX_TIME_UpdateScaleChanged)
	{
		OMX_U32 nClockScalePre = nClockScale;
		nClockScale = pTimeBuffer->xScale;
		LOG_DEBUG("nClockScale = %d\n", nClockScale);
		if (nClockScale <= (OMX_S32)(MAX_RATE * Q16_SHIFT)
				&& nClockScale >= (OMX_S32)(MIN_RATE * Q16_SHIFT))
		{
			OMX_ERRORTYPE ret = OMX_ErrorNone;
			if (nClockScalePre <= (OMX_S32)(MAX_RATE * Q16_SHIFT)
					&& nClockScalePre >= (OMX_S32)(MIN_RATE * Q16_SHIFT))
			{
				ret = DrainDevice();
				if (ret != OMX_ErrorNone)
				{
					LOG_ERROR("Drain fail.\n");
					return ret;
				}
			}

			ret = SetDevice();
			if(ret != OMX_ErrorNone) {
				LOG_WARNING("Set device fail.\n");
				ports[CLK_PORT]->SendBuffer(pClockBufferHdr);
				bHeardwareError = OMX_TRUE;
				SendEvent(OMX_EventError, ret, 0, NULL);
				return ret;
			}

			nWriteOutLen = nPeriodSize * nSampleSize;
			nAudioDataWriteThreshold = nFadeInFadeOutProcessLen + nWriteOutLen;
			AudioRenderFadeInFadeOut.Create(PcmMode.nChannels, PcmMode.nSamplingRate, \
					PcmMode.nBitPerSample, nFadeInFadeOutProcessLen, 2);
		}
	}
	else
	{
		LOG_WARNING("Unknow clock buffer.\n");
	}

	ports[CLK_PORT]->SendBuffer(pClockBufferHdr);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioRender::DoLoaded2Idle()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = OpenDevice();
    if(ret != OMX_ErrorNone) {
        CloseDevice();
		bHeardwareError = OMX_TRUE;
        SendEvent(OMX_EventError, ret, 0, NULL);
        return ret;
    }

    ret = SetDevice();
    if(ret != OMX_ErrorNone) {
        LOG_WARNING("Set device fail.\n");
		bHeardwareError = OMX_TRUE;
		SendEvent(OMX_EventError, ret, 0, NULL);
        return ret;
    }

	nWriteOutLen = nPeriodSize * nSampleSize;
	nAudioDataWriteThreshold = nFadeInFadeOutProcessLen + nWriteOutLen;
	RINGBUFFER_ERRORTYPE BufferRet = RINGBUFFER_SUCCESS;
	/** As speed change, ring buffer should use the largest writen threshold */
	OMX_U32 nRingBufferSize = (OMX_U32)(nAudioDataWriteThreshold * MAX_RATE);
	LOG_DEBUG("Aurio render nRingBufferSize = %d\n", nRingBufferSize);
	BufferRet = AudioRenderRingBuffer.BufferCreate(nRingBufferSize);
	if (BufferRet != RINGBUFFER_SUCCESS)
	{
		LOG_ERROR("Create ring buffer fail.\n");
		return OMX_ErrorInsufficientResources;
	} 

	FADEINFADEOUT_ERRORTYPE FadeRet = FADEINFADEOUT_SUCCESS;
	FadeRet = AudioRenderFadeInFadeOut.Create(PcmMode.nChannels, PcmMode.nSamplingRate, \
			PcmMode.nBitPerSample, nFadeInFadeOutProcessLen, 1);
	if (FadeRet != FADEINFADEOUT_SUCCESS)
	{
		LOG_ERROR("Create fade in fade out process fail.\n");
		return OMX_ErrorInsufficientResources;
	}

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioRender::DoIdle2Loaded()
{
	DoExec2Idle();

 	CloseDevice();

	LOG_DEBUG("Audio render ring buffer free.\n");
	AudioRenderRingBuffer.BufferFree();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioRender::DoExec2Pause()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (AudioRenderRingBuffer.AudioDataLen() == 0 || bHeardwareError == OMX_TRUE)
	{
		LOG_DEBUG("No audio data for Fade out process.\n");
		return OMX_ErrorNone;
	}

	AudioRenderFadeInFadeOut.SetMode(FADEOUT);
	nWriteOutLen = nFadeInFadeOutProcessLen;

	ret = RenderPeriod();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Render period fail(E2P).\n");
		return ret;
	}

	ret = DrainDevice();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Drain fail.\n");
		return ret;
	}

	LOG_DEBUG("Fade out over.\n");

	OMX_S64 PlayingTime;
	PlayingTime = StartTime.nTimestamp + ((TotalConsumeredLen)/nSampleSize * OMX_TICKS_PER_SECOND) / PcmMode.nSamplingRate * ((float)nDNSeScale / Q16_SHIFT);

	ReferTime.nTimestamp = PlayingTime;
	LOG_DEBUG("Set reference time to clock: %lld\n", PlayingTime);
	ClockSetConfig(OMX_IndexConfigTimeCurrentAudioReference, &ReferTime);

    return ret;
}

OMX_ERRORTYPE AudioRender::DoPause2Exec()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	LOG_DEBUG("Fade in begin.\n");
	AudioRenderFadeInFadeOut.SetMode(FADEIN);
	nWriteOutLen = nPeriodSize * nSampleSize;

    return ret;
}

OMX_ERRORTYPE AudioRender::DoExec2Idle()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (AudioRenderRingBuffer.AudioDataLen() == 0 ||bHeardwareError == OMX_TRUE)
	{
		LOG_DEBUG("No audio data for Fade out process.\n");
		return OMX_ErrorNone;
	}

	LOG_DEBUG("Audio render Exec2Idle.\n");

	AudioRenderFadeInFadeOut.SetMode(FADEOUT);
	nWriteOutLen = nFadeInFadeOutProcessLen;

	ret = RenderPeriod();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Render period fail.\n");
		return ret;
	}

	ret = DrainDevice();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Drain fail.\n");
		return ret;
	}

	AudioRenderRingBuffer.BufferReset();

    return ret;
}


OMX_ERRORTYPE AudioRender::ComponentReturnBuffer(
        OMX_U32 nPortIndex)
{
	OMX_STATETYPE eState = OMX_StateInvalid;
	GetState(&eState);

	/* Consume all audio data in input port buffer when disable input port */
	if(nPortIndex == IN_PORT \
			&& ports[IN_PORT]->IsEnabled() != OMX_TRUE \
			&& eState == OMX_StateExecuting) {
		while (ports[IN_PORT]->BufferNum() != 0 || pInBufferHdr != NULL)
		{
			LOG_DEBUG("Audio render process input data.\n");
			ProcessDataBuffer();
			usleep(5000);
		}
	}

    if(nPortIndex == IN_PORT && pInBufferHdr != NULL) {
        ports[IN_PORT]->SendBuffer(pInBufferHdr);
        pInBufferHdr = NULL;
    }

	LOG_DEBUG("Alsa Return Buffer.\n");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AudioRender::FlushComponent(
        OMX_U32 nPortIndex)
{
    if(nPortIndex == IN_PORT && pInBufferHdr != NULL) {
        ports[IN_PORT]->SendBuffer(pInBufferHdr);
        pInBufferHdr = NULL;
    }

	if (AudioRenderRingBuffer.AudioDataLen() == 0 || bHeardwareError == OMX_TRUE)
	{
		LOG_DEBUG("No audio data for Fade out process.\n");
		return OMX_ErrorNone;
	}

	AudioRenderFadeInFadeOut.SetMode(FADEOUT);
	nWriteOutLen = nFadeInFadeOutProcessLen;

	OMX_ERRORTYPE ret = OMX_ErrorNone;
	ret = RenderPeriod();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Render period fail.\n");
		return ret;
	}

	ret = DrainDevice();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Drain fail.\n");
		return ret;
	}

	AudioRenderRingBuffer.BufferReset();

	return OMX_ErrorNone;
}

/* File EOF */
