/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "WmaDec.h"

#define WMA10D_MAXDATAREQUESTED 			1024
#define WMA10D_OUTBUFF_SIZE 				50000

#define DECOPT_CHANNEL_DOWNMIXING      0x00000001
#define DECOPT_DRC                     0x00000002
#define DECOPT_INTERPOLATED_DOWNSAMPLE 0x00000004
#define DECOPT_HALF_TRANSFORM          0x00000008
#define DECOPT_HALF_UP_TRANSFORM       0x00000010
#define DECOPT_2X_TRANSFORM            0x00000020
#define DECOPT_REQUANTTO16             0x00000040
#define DECOPT_DOWNSAMPLETO44OR48      0x00000080
#define DECOPT_LTRTDOWNMIX             0x00000100
#define DECOPT_FLOAT_OUT               0x00000200
#define DECOPT_PCM24_OUT               0x00000400
#define DECOPT_PCM32_OUT               0x00000800
#define DECOPT_IGNOREFREQEX            0x00001000
#define DECOPT_IGNORECX                0x00002000

WmaDec::WmaDec()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_decoder.wma.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_decoder.wma";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = WMA10D_MAXDATAREQUESTED;
	nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE WmaDec::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    hLib = NULL;
    pWmaDecConfig = NULL;
    libMgr = NULL;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = AUDIO_FILTER_INPUT_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingWMA;
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
    sPortDef.nBufferSize = WMA10D_OUTBUFF_SIZE;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&WmaType, OMX_AUDIO_PARAM_WMATYPE);
	OMX_INIT_STRUCT(&WmaTypeExt, OMX_AUDIO_PARAM_WMATYPE_EXT);
	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	WmaTypeExt.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	WmaTypeExt.nBitsPerSample = 16;

	WmaType.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	WmaType.nChannels = 2;
	WmaType.nSamplingRate = 44100;
	WmaType.nBitRate = 64000;
	WmaType.eFormat = OMX_AUDIO_WMAFormat9;

	PcmMode.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	dwChannelMask = 0;    
	nAdvancedEncodeOpt = 0;
	nAdvancedEncodeOpt2 = 0;

	libMgr = FSL_NEW(ShareLibarayMgr, ());
	if(libMgr == NULL)
		return OMX_ErrorInsufficientResources;

	hLib = libMgr->load((OMX_STRING)"lib_wma10_dec_v2_arm12_elinux.so");
	if(hLib == NULL) {
		LOG_WARNING("Can't load library lib_wma10_dec_v2_arm12_elinux.\n");
		FSL_DELETE(libMgr);
		return OMX_ErrorComponentNotFound;
	}

	pWMADQueryMem = (tWMAFileStatus (*)( WMADDecoderConfig *))libMgr->getSymbol(hLib, (OMX_STRING)"eWMADQueryMem");
	pInitWMADecoder = (tWMAFileStatus (*)(WMADDecoderConfig *, WMADDecoderParams *, WMAD_UINT8 *, WMAD_INT32 ))libMgr->getSymbol(hLib, (OMX_STRING)"eInitWMADecoder");
	pWMADecodeFrame = (tWMAFileStatus (*)(WMADDecoderConfig *,
						WMADDecoderParams *,
						WMAD_INT16 *,
						WMAD_INT32))libMgr->getSymbol(hLib,(OMX_STRING)"eWMADecodeFrame");

	if(pWMADQueryMem == NULL || pInitWMADecoder == NULL || pWMADecodeFrame == NULL) {
		libMgr->unload(hLib);
		FSL_DELETE(libMgr);
		return OMX_ErrorComponentNotFound;
	}

	return ret;
}

OMX_ERRORTYPE WmaDec::DeInitComponent()
{
    if (libMgr) {
        if (hLib)
            libMgr->unload(hLib);
        FSL_DELETE(libMgr);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmaDec::AudioFilterInstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	pWmaDecConfig = (WMADDecoderConfig *)FSL_MALLOC(sizeof(WMADDecoderConfig));
	if (pWmaDecConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pWmaDecConfig, 0, sizeof(WMADDecoderConfig));

	pWmaDecParams = (WMADDecoderParams *)FSL_MALLOC(sizeof(WMADDecoderParams));
	if (pWmaDecParams == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pWmaDecParams, 0, sizeof(WMADDecoderParams));

	if (nPushModeInputLen < WmaType.nBlockAlign * 2)
		nPushModeInputLen = WmaType.nBlockAlign * 2;

    return ret;
}

tWMAFileStatus WMA10FileCBGetNewPayload(OMX_PTR *state, OMX_U64 offset, OMX_U32 *pnum_bytes, \
			     OMX_U8 **ppData, OMX_PTR pAppContext, OMX_U32 *pbIsCompressedPayload)
{
    tWMAFileStatus ret = cWMA_NoErr;
	WmaDec *pWmaDec = (WmaDec *)pAppContext;
	RingBuffer *pAudioRingBuffer = (RingBuffer *)&(pWmaDec->AudioRingBuffer);
	*ppData = NULL;

	LOG_LOG("Wma need bytes: %d RingBuffer audio data len: %d\n", *pnum_bytes, pAudioRingBuffer->AudioDataLen());

	if(pAudioRingBuffer->AudioDataLen()<=0 && pWmaDec->bReceivedEOS == OMX_TRUE)
	{
		return cWMA_NoMoreFrames;
	}
	if((pAudioRingBuffer->AudioDataLen() - pAudioRingBuffer->nPrevOffset)<*pnum_bytes && pWmaDec->bReceivedEOS == OMX_FALSE)
	{
		*ppData = NULL;
		*pnum_bytes=0;
		return cWMA_NoMoreDataThisTime;
	}

	pAudioRingBuffer->BufferConsumered(pAudioRingBuffer->nPrevOffset);

	OMX_U32 nActuralLen;

	pAudioRingBuffer->BufferGet(ppData, *pnum_bytes, &nActuralLen);
	LOG_DEBUG("Get stream length: %d\n", nActuralLen);

	if(nActuralLen==0 && pWmaDec->bReceivedEOS == OMX_TRUE)
	{
		return cWMA_NoMoreFrames;
	}

	pAudioRingBuffer->nPrevOffset = nActuralLen;
	return ret;
}

OMX_ERRORTYPE WmaDec::AudioFilterCodecInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	tWMAFileStatus WmaRet;

	pWmaDecConfig->psWMADecodeInfoStructPtr = NULL;
	pWmaDecParams->bDropPacket = 0;
	pWmaDecParams->nDRCSetting = 0;
	pWmaDecParams->nDecoderFlags =0;
	pWmaDecParams->nDstChannelMask = 0;
	pWmaDecParams->nInterpResampRate = 0;
	pWmaDecParams->nMBRTargetStream = 1;
	pWmaDecParams->bIsRawDecoder = WMAD_TRUE;
	pWmaDecParams->us16Version = WmaType.eFormat;
	pWmaDecParams->us16Channels = WmaType.nChannels;
	pWmaDecParams->us32SamplesPerSec = WmaType.nSamplingRate;
	pWmaDecParams->us32AvgBytesPerSec = WmaType.nBitRate >> 3;
	pWmaDecParams->us16EncodeOpt = WmaType.nEncodeOptions;
	if (WmaType.eFormat == OMX_AUDIO_WMAFormat7)
		pWmaDecParams->us16wFormatTag = 0x160;
	else if (WmaType.eFormat == OMX_AUDIO_WMAFormat8)
		pWmaDecParams->us16wFormatTag = 0x161;
	else if (WmaType.eFormat == OMX_AUDIO_WMAFormat9)
		pWmaDecParams->us16wFormatTag = 0x162;

	pWmaDecParams->us32nBlockAlign = WmaType.nBlockAlign;
	pWmaDecParams->us32ChannelMask = dwChannelMask;
	pWmaDecParams->us16AdvancedEncodeOpt = nAdvancedEncodeOpt;
	pWmaDecParams->us32AdvancedEncodeOpt2 = nAdvancedEncodeOpt2;
	pWmaDecParams->us32ValidBitsPerSample = WmaTypeExt.nBitsPerSample;
	if((pWmaDecParams->us32ValidBitsPerSample!=16)&&(pWmaDecParams->us32ValidBitsPerSample!=24))
	{
		pWmaDecParams->us32ValidBitsPerSample = 16;
	}
	if(pWmaDecParams->us16Channels > 2)
	{
		pWmaDecParams->nDecoderFlags |= DECOPT_CHANNEL_DOWNMIXING;
		pWmaDecParams->nDstChannelMask = 0x03;
	}

	pWmaDecConfig->app_swap_buf = (tWMAFileStatus (*)(void*, WMAD_UINT64, WMAD_UINT32*, WMAD_UINT8**, void*, WMAD_UINT32*))WMA10FileCBGetNewPayload;
	pWmaDecConfig->pContext = (OMX_PTR)(this);
	pWmaDecConfig->sDecodeParams = pWmaDecParams;
	fsl_osal_memset(&sWfx,0,sizeof(WAVEFORMATEXTENSIBLE));
    pWmaDecParams->pWfx = &sWfx;

	OMX_S32 MemoryCnt = pWmaDecConfig->sWMADMemInfo.s32NumReqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		FSL_FREE(pWmaDecConfig->sWMADMemInfo.sMemInfoSub[i].app_base_ptr);
	}

	LOG_DEBUG("nVersion=%d,wFormatTag=%p,nSamplesPerSec=%d,nAvgBytesPerSec=%d,nBlockAlign=%d,nChannels=%d,nEncodeOpt=%p,wBitsPerSample=%d,dwChannelMask=%p,nAdvancedEncodeOpt=%p,nAdvancedEncodeOpt2=%p\n",pWmaDecConfig->sDecodeParams->us16Version,pWmaDecConfig->sDecodeParams->us16wFormatTag,pWmaDecConfig->sDecodeParams->us32SamplesPerSec,pWmaDecConfig->sDecodeParams->us32AvgBytesPerSec,pWmaDecConfig->sDecodeParams->us32nBlockAlign,pWmaDecConfig->sDecodeParams->us16Channels,pWmaDecConfig->sDecodeParams->us16EncodeOpt,pWmaDecConfig->sDecodeParams->us32ValidBitsPerSample,pWmaDecConfig->sDecodeParams->us32ChannelMask,pWmaDecConfig->sDecodeParams->us16AdvancedEncodeOpt,pWmaDecConfig->sDecodeParams->us32AdvancedEncodeOpt2);

	WmaRet = pWMADQueryMem(pWmaDecConfig);
	if (WmaRet != cWMA_NoErr)
	{
		LOG_ERROR("Wma decoder query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	MemoryCnt = pWmaDecConfig->sWMADMemInfo.s32NumReqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pWmaDecConfig->sWMADMemInfo.sMemInfoSub[i].s32WMADSize;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		pWmaDecConfig->sWMADMemInfo.sMemInfoSub[i].app_base_ptr = pBuffer;
	}

	WmaRet = pInitWMADecoder(pWmaDecConfig, pWmaDecParams, NULL, 0);
	if (WmaRet != cWMA_NoErr)
	{
		LOG_ERROR("Wma decoder initialize fail.\n");
		return OMX_ErrorUnsupportedSetting;
	}

	AudioRingBuffer.nPrevOffset = 0;

	return ret;
}

OMX_ERRORTYPE WmaDec::AudioFilterInstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    if(pWmaDecConfig){
	OMX_S32 MemoryCnt = pWmaDecConfig->sWMADMemInfo.s32NumReqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		FSL_FREE(pWmaDecConfig->sWMADMemInfo.sMemInfoSub[i].app_base_ptr);
	}

	FSL_FREE(pWmaDecConfig);
	FSL_FREE(pWmaDecParams);
    }

    return ret;
}

OMX_ERRORTYPE WmaDec::AudioFilterGetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioWma:
            {
                OMX_AUDIO_PARAM_WMATYPE *pWmaType;
                pWmaType = (OMX_AUDIO_PARAM_WMATYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pWmaType, OMX_AUDIO_PARAM_WMATYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pWmaType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(pWmaType, &WmaType,	sizeof(OMX_AUDIO_PARAM_WMATYPE));
            }
			return ret;
        case OMX_IndexParamAudioWmaExt:
            {
                OMX_AUDIO_PARAM_WMATYPE_EXT *pWmaTypeExt;
                pWmaTypeExt = (OMX_AUDIO_PARAM_WMATYPE_EXT*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pWmaTypeExt, OMX_AUDIO_PARAM_WMATYPE_EXT, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pWmaTypeExt->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(pWmaTypeExt, &WmaTypeExt,	sizeof(OMX_AUDIO_PARAM_WMATYPE_EXT));
            }
			return ret;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE WmaDec::AudioFilterSetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioWma:
            {
                OMX_AUDIO_PARAM_WMATYPE *pWmaType;
                pWmaType = (OMX_AUDIO_PARAM_WMATYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pWmaType, OMX_AUDIO_PARAM_WMATYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pWmaType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(&WmaType, pWmaType, sizeof(OMX_AUDIO_PARAM_WMATYPE));
			}
			return ret;
        case OMX_IndexParamAudioWmaExt:
            {
                OMX_AUDIO_PARAM_WMATYPE_EXT *pWmaTypeExt;
                pWmaTypeExt = (OMX_AUDIO_PARAM_WMATYPE_EXT*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pWmaTypeExt, OMX_AUDIO_PARAM_WMATYPE_EXT, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pWmaTypeExt->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(&WmaTypeExt, pWmaTypeExt, sizeof(OMX_AUDIO_PARAM_WMATYPE_EXT));
			}
			return ret;
		default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}
 
AUDIO_FILTERRETURNTYPE WmaDec::AudioFilterFrame()
{
    AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
	tWMAFileStatus WmaRet;
	WmaRet = pWMADecodeFrame(pWmaDecConfig, pWmaDecParams, (WMAD_INT16 *)(pOutBufferHdr->pBuffer), pWmaDecParams->us32OutputBufSize);
	
	LOG_LOG("WmaRet = %d\n", WmaRet);
	if (WmaRet == cWMA_NoErr)
	{
		sWfx.Format.wBitsPerSample = pWmaDecParams->us32ValidBitsPerSample;

		LOG_DEBUG("Wma channels: %d sample rate: %d bispersample: %d\n", sWfx.Format.nChannels, sWfx.Format.nSamplesPerSec, sWfx.Format.wBitsPerSample);
		if (sWfx.Format.nChannels != PcmMode.nChannels
				|| sWfx.Format.nSamplesPerSec != PcmMode.nSamplingRate
				|| sWfx.Format.wBitsPerSample != PcmMode.nBitPerSample)
		{
			PcmMode.nChannels = sWfx.Format.nChannels;
			PcmMode.nSamplingRate = sWfx.Format.nSamplesPerSec;
			PcmMode.nBitPerSample = sWfx.Format.wBitsPerSample;
			SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
		}

		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = pWmaDecParams->us16NumSamples * \
									sWfx.Format.nChannels * sWfx.Format.wBitsPerSample/8;

		TS_PerFrame = (OMX_U64)pWmaDecParams->us16NumSamples*OMX_TICKS_PER_SECOND/sWfx.Format.nSamplesPerSec;
		LOG_LOG("Decoder output sample: %d\t Sample Rate = %d\t TS per Frame = %lld\n", \
				pWmaDecParams->us16NumSamples, sWfx.Format.nSamplesPerSec, TS_PerFrame);

		AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 
	}
	else if (WmaRet == cWMA_NoMoreFrames)
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

OMX_ERRORTYPE WmaDec::AudioFilterReset()
{
	return AudioFilterCodecInit();
}

OMX_ERRORTYPE WmaDec::AudioFilterCheckCodecConfig()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_WMATYPE *pWmaType;
	pWmaType = (OMX_AUDIO_PARAM_WMATYPE*)pInBufferHdr->pBuffer;
	OMX_CHECK_STRUCT(pWmaType, OMX_AUDIO_PARAM_WMATYPE, ret);
	if(ret != OMX_ErrorNone)
	{
		LOG_DEBUG("Wma codec config len: %d\n", pInBufferHdr->nFilledLen);
		switch (WmaType.eFormat)
		{
			case OMX_AUDIO_WMAFormat7:        
				LOG_DEBUG("WMA1 audio\n");            
				if (pInBufferHdr->nFilledLen >= 4)
					WmaType.nEncodeOptions = *(OMX_U16*)(pInBufferHdr->pBuffer + 2);
				break;
			case OMX_AUDIO_WMAFormat8:        
				LOG_DEBUG("WMA2 audio\n");            
				if (pInBufferHdr->nFilledLen >= 6)
					WmaType.nEncodeOptions = *(OMX_U16*)(pInBufferHdr->pBuffer + 4);
				break;
			case OMX_AUDIO_WMAFormat9:        
				LOG_DEBUG("WMA3 audio\n");            
				if (pInBufferHdr->nFilledLen >= 18)
				{
					WmaType.eProfile = OMX_AUDIO_WMAProfileL1;
					dwChannelMask = *(OMX_U32*)(pInBufferHdr->pBuffer + 2);
					nAdvancedEncodeOpt2 = *(OMX_U16 *)(pInBufferHdr->pBuffer + 10);
					WmaType.nEncodeOptions = *(OMX_U16 *)(pInBufferHdr->pBuffer + 14);
					nAdvancedEncodeOpt = *(OMX_U16 *)(pInBufferHdr->pBuffer + 16);
				}
				break;
                      default:
                            break;
		}

		pInBufferHdr->nFilledLen = 0;
		return ret;
	}

	LOG_DEBUG("Wma codec type len: %d\n", pInBufferHdr->nFilledLen);
	fsl_osal_memcpy(pWmaType, &WmaType,	sizeof(OMX_AUDIO_PARAM_WMATYPE));

	pInBufferHdr->nFilledLen = 0;

    return ret;
}
 

/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE WmaDecInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        WmaDec *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(WmaDec, ());
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
