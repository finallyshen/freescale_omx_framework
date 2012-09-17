/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "AacEnc.h"

#define AACE_INPUT_BUFFER_SIZE (1024)
#define STAGEFRIGHT_LIBRARY "libstagefright.so"
#define AAC_ENC_API "voGetAACEncAPI"

static const OMX_U32 samplingFrequencyMap[16] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};

VO_U32 cmnMemAlloc (VO_S32 uID,  VO_MEM_INFO * pMemInfo)
{
	if (!pMemInfo)
		return VO_ERR_INVALID_ARG;

	pMemInfo->VBuffer = FSL_MALLOC(pMemInfo->Size);
	return 0;
}

VO_U32 cmnMemFree (VO_S32 uID, VO_PTR pMem)
{
	FSL_FREE (pMem);
	return 0;
}

VO_U32	cmnMemSet (VO_S32 uID, VO_PTR pBuff, VO_U8 uValue, VO_U32 uSize)
{
	fsl_osal_memset (pBuff, uValue, uSize);
	return 0;
}

VO_U32	cmnMemCopy (VO_S32 uID, VO_PTR pDest, VO_PTR pSource, VO_U32 uSize)
{
	fsl_osal_memcpy (pDest, pSource, uSize);
	return 0;
}

VO_U32	cmnMemCheck (VO_S32 uID, VO_PTR pBuffer, VO_U32 uSize)
{
	return 0;
}

AacEnc::AacEnc()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_encoder.aac.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_encoder.aac";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = AACE_INPUT_BUFFER_SIZE * 4;
	nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE AacEnc::InitComponent()
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
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingAAC;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
    sPortDef.nBufferSize = AACE_INPUT_BUFFER_SIZE * 4;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&AacType, OMX_AUDIO_PARAM_AACPROFILETYPE);
	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	AacType.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
	AacType.nAudioBandWidth = 0;
	AacType.nSampleRate = 44100;
	AacType.nChannels = 2;
	AacType.nBitRate = 128000;
	AacType.nAACERtools = OMX_AUDIO_AACERNone;
	AacType.eAACProfile = OMX_AUDIO_AACObjectLC;
	AacType.eChannelMode = OMX_AUDIO_ChannelModeStereo;
	AacType.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMax;

	PcmMode.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	libMgr = NULL;
	hLibStagefright = NULL;
	mEncoderHandle = NULL;
	mApiHandle = NULL;
	mMemOperator = NULL;
	bFirstFrame = OMX_TRUE;

	TS_PerFrame = AACE_INPUT_BUFFER_SIZE*OMX_TICKS_PER_SECOND/AacType.nSampleRate;

	return ret;
}

OMX_ERRORTYPE AacEnc::DeInitComponent()
{
    return OMX_ErrorNone;
}

typedef int (VO_API * VOGETAUDIODECAPI) (VO_AUDIO_CODECAPI * pDecHandle);

OMX_ERRORTYPE AacEnc::AudioFilterInstanceInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_PTR pfunc = NULL;
	VOGETAUDIODECAPI voGetAACEncAPI;

    libMgr = FSL_NEW(ShareLibarayMgr, ());
    if (libMgr == NULL)
        return OMX_ErrorInsufficientResources;

    hLibStagefright = libMgr->load((OMX_STRING)STAGEFRIGHT_LIBRARY);
    if (hLibStagefright == NULL) {
        LOG_ERROR("Can't load library libstagefright.so");
        return OMX_ErrorUndefined;
    }

    pfunc = libMgr->getSymbol(hLibStagefright, (OMX_STRING)AAC_ENC_API);
    if (pfunc == NULL) {
        LOG_ERROR("Can't get sysble voGetAACEncAPI from libstagefright.so");
        return OMX_ErrorUndefined;
    }

	voGetAACEncAPI = (VOGETAUDIODECAPI)pfunc;

    mApiHandle = FSL_NEW(VO_AUDIO_CODECAPI, ());
    if (mApiHandle == NULL) {
        return OMX_ErrorInsufficientResources;
	}

    if (VO_ERR_NONE != voGetAACEncAPI(mApiHandle)) {
        LOG_ERROR("Failed to get api handle");
        return OMX_ErrorUndefined;
    }

    mMemOperator = FSL_NEW(VO_MEM_OPERATOR, ());
    if (mMemOperator == NULL) {
        return OMX_ErrorInsufficientResources;
	}

    mMemOperator->Alloc = cmnMemAlloc;
    mMemOperator->Copy = cmnMemCopy;
    mMemOperator->Free = cmnMemFree;
    mMemOperator->Set = cmnMemSet;
    mMemOperator->Check = cmnMemCheck;

    return ret;
}

OMX_ERRORTYPE AacEnc::AudioFilterInstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (mEncoderHandle) {
        mApiHandle->Uninit(mEncoderHandle);
        mEncoderHandle = NULL;
    }
    FSL_DELETE(mApiHandle);
	FSL_DELETE(mMemOperator);

	if (libMgr) {
		if (hLibStagefright)
			libMgr->unload(hLibStagefright);
		hLibStagefright = NULL;
        FSL_DELETE(libMgr);
	}

    return ret;
}

OMX_ERRORTYPE AacEnc::AudioFilterCodecInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    VO_CODEC_INIT_USERDATA userData;
    fsl_osal_memset(&userData, 0, sizeof(userData));
    userData.memflag = VO_IMF_USERMEMOPERATOR;
    userData.memData = (VO_PTR) mMemOperator;
    if (VO_ERR_NONE != mApiHandle->Init(&mEncoderHandle, VO_AUDIO_CodingAAC, &userData)) {
        LOG_ERROR("Failed to init AAC encoder");
        return OMX_ErrorUndefined;
    }
 
    // Configure AAC encoder$
    AACENC_PARAM params;
    fsl_osal_memset(&params, 0, sizeof(params));
    params.sampleRate = AacType.nSampleRate;
    params.bitRate = AacType.nBitRate;
    params.nChannels = AacType.nChannels;
    params.adtsUsed = 0;  // For MP4 file, don't use adts format$
	LOG_DEBUG("Encoder Sample Rate = %d\t channels: %d\t bitRate = %d\n", \
			AacType.nSampleRate, AacType.nChannels, AacType.nBitRate);

    if (VO_ERR_NONE != mApiHandle->SetParam(mEncoderHandle, VO_PID_AAC_ENCPARAM,  &params)) {
        LOG_ERROR("Failed to set AAC encoder parameters");
        return OMX_ErrorUndefined;
    }

	return ret;
}

OMX_ERRORTYPE AacEnc::AudioFilterGetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioAac:
            {
                OMX_AUDIO_PARAM_AACPROFILETYPE *pAacType;
                pAacType = (OMX_AUDIO_PARAM_AACPROFILETYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pAacType, OMX_AUDIO_PARAM_AACPROFILETYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pAacType->nPortIndex != AUDIO_FILTER_OUTPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(pAacType, &AacType,	sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
            }
			return ret;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE AacEnc::AudioFilterSetParameterPCM()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (AacType.nChannels != PcmMode.nChannels
			|| AacType.nSampleRate != PcmMode.nSamplingRate)
	{
		AacType.nChannels = PcmMode.nChannels;
		AacType.nSampleRate = PcmMode.nSamplingRate;
		TS_PerFrame = AACE_INPUT_BUFFER_SIZE*OMX_TICKS_PER_SECOND/AacType.nSampleRate;
		nPushModeInputLen = AACE_INPUT_BUFFER_SIZE * 2 * AacType.nChannels;
		LOG_DEBUG("Encoder Sample Rate = %d\t channels: %d\t TS per Frame = %lld\n", \
				AacType.nSampleRate, AacType.nChannels, TS_PerFrame);
		SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
	}

    return ret;
}

OMX_ERRORTYPE AacEnc::AudioFilterSetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioAac:
            {
                OMX_AUDIO_PARAM_AACPROFILETYPE *pAacType;
                pAacType = (OMX_AUDIO_PARAM_AACPROFILETYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pAacType, OMX_AUDIO_PARAM_AACPROFILETYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pAacType->nPortIndex != AUDIO_FILTER_OUTPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(&AacType, pAacType, sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
			}
			return ret;
		default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

AUDIO_FILTERRETURNTYPE AacEnc::AudioFilterFrame()
{
	AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;

	if (bFirstFrame == OMX_TRUE) {
/*
		put_bits(&pb, 5, 2); //object type - AAC-LC
		put_bits(&pb, 4, s->samplerate_index); //sample rate index
		put_bits(&pb, 4, avctx->channels);
		//GASpecificConfig
		put_bits(&pb, 1, 0); //frame length - 1024 samples
		put_bits(&pb, 1, 0); //does not depend on core coder
		put_bits(&pb, 1, 0); //is not extension

		//Explicitly Mark SBR absent
		put_bits(&pb, 11, 0x2b7); //sync extension
		put_bits(&pb, 5,  5);
		put_bits(&pb, 1,  0);
*/
		OMX_U8 csd[5] = {0x10, 0x00, 0x56, 0xE5, 0x00};

		for (OMX_U32 i = 0; i < 16; i ++) {
			if (PcmMode.nSamplingRate == samplingFrequencyMap[i]) {
				csd[0] |= 0x7 & (i>>1);
				csd[1] |= (0x1 & i) << 7;
				printf("csd[0]: 0x%x, csd[1]: 0x%x\n", csd[0], csd[1]);
				break;
			}
		}
		csd[1] |= (0xF & PcmMode.nChannels) << 3;
		printf("csd[1]: 0x%x\n", csd[1]);

		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = 5;
		pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
		fsl_osal_memcpy(pOutBufferHdr->pBuffer, csd, 5);

		bFirstFrame = OMX_FALSE;
		return ret;
	}

	AudioRingBuffer.BufferGet(&pBuffer, nPushModeInputLen, &nActuralLen);
	LOG_LOG("Aac encoder get stream len: %d\n", nActuralLen);

	pOutBufferHdr->nOffset = 0;
	pOutBufferHdr->nFilledLen = 0;

	if (nActuralLen == 0) {
	} else {
		if (nActuralLen < nPushModeInputLen) {
			OMX_U32 nActuralLenZero;
			AudioRingBuffer.BufferAddZeros(nPushModeInputLen - nActuralLen, &nActuralLenZero);
			AudioRingBuffer.BufferGet(&pBuffer, nPushModeInputLen, &nActuralLen);
		}

		VO_CODECBUFFER inputData;
		fsl_osal_memset(&inputData, 0, sizeof(inputData));
		inputData.Buffer = (unsigned char*) pBuffer;
		inputData.Length = nActuralLen;
		mApiHandle->SetInputData(mEncoderHandle,&inputData);

		VO_CODECBUFFER outputData;
		fsl_osal_memset(&outputData, 0, sizeof(outputData));
		VO_AUDIO_OUTPUTINFO outputInfo;
		fsl_osal_memset(&outputInfo, 0, sizeof(outputInfo));

		VO_U32 ret = VO_ERR_NONE;
		outputData.Buffer = pOutBufferHdr->pBuffer;
		outputData.Length = pOutBufferHdr->nAllocLen;
		ret = mApiHandle->GetOutputData(mEncoderHandle, &outputData, &outputInfo);
		//CHECK(ret == VO_ERR_NONE || ret == VO_ERR_INPUT_BUFFER_SMALL);
		//CHECK(outputData.Length != 0);
		pOutBufferHdr->nFilledLen = outputData.Length;
	}

	AudioRingBuffer.BufferConsumered(nActuralLen);

	LOG_LOG("Stream frame len: %d\n", pOutBufferHdr->nFilledLen);
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

OMX_ERRORTYPE AacEnc::AudioFilterReset()
{
    return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE AacEncInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        AacEnc *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(AacEnc, ());
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
