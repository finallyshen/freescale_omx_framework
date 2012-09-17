/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "VorbisDec.h"

#define VORBIS_RETURN int
#define VORBIS_EOS    100
#define VORBIS10D_MAXDATAREQUESTED 			(8192*2*2)
#define VORBIS10D_OUTBUFF_SIZE 				(8192*2*2)
#define INFORMATION_HEADER  0x01
#define COMMENT_HEADER      0x03
#define CODEC_SETUP_HEADER  0x05

VorbisDec::VorbisDec()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_decoder.vorbis.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_decoder.vorbis";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = VORBIS10D_MAXDATAREQUESTED;
	nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE VorbisDec::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = AUDIO_FILTER_INPUT_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingVORBIS;
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
    sPortDef.nBufferSize = VORBIS10D_OUTBUFF_SIZE;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&VorbisType, OMX_AUDIO_PARAM_VORBISTYPE);
	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	VorbisType.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	VorbisType.nChannels = 2;
	VorbisType.nBitRate = 0;
	VorbisType.nMinBitRate = 0;
	VorbisType.nMaxBitRate = 0;
	VorbisType.nSampleRate = 0;
	VorbisType.nAudioBandWidth = 0;
	VorbisType.nQuality = 3;
	VorbisType.bManaged = OMX_FALSE;
	VorbisType.bDownmix = OMX_FALSE;

	PcmMode.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;


	return ret;
}

OMX_ERRORTYPE VorbisDec::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VorbisDec::AudioFilterInstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	pVorbisDecConfig = (sOggVorbisDecObj *)FSL_MALLOC(sizeof(sOggVorbisDecObj));
	if (pVorbisDecConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pVorbisDecConfig, 0, sizeof(sOggVorbisDecObj));

	pCodecConfigData = NULL;

    return ret;
}

OMX_ERRORTYPE VorbisDec::AudioFilterCodecInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	VORBIS_RETURN VorbisRet;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;
	OMX_U32 nFrameLen;

	pBuffer = pCodecConfigData + nCodecConfigDataOffset;
	nActuralLen = nCodecConfigDataLen - nCodecConfigDataOffset;
	nFrameLen = nActuralLen;
 
    pVorbisDecConfig->datasource = pBuffer;
    pVorbisDecConfig->buffer_length = nFrameLen;

	FSL_FREE(pVorbisDecConfig->pvOggDecObj);
	FSL_FREE(pVorbisDecConfig->decoderbuf);

	OMX_U32 VorbisMemSize = OggVorbisQueryMem(pVorbisDecConfig);

	OMX_U32 BufferSize;

	BufferSize = pVorbisDecConfig->OggDecObjSize;
	pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
	if (pBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pBuffer, 0, BufferSize);
	pVorbisDecConfig->pvOggDecObj = pBuffer;

	BufferSize = VorbisMemSize;
	pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
	if (pBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pBuffer, 0, BufferSize);
	pVorbisDecConfig->decoderbuf = pBuffer;
	pVorbisDecConfig->buf_size = VorbisMemSize;

	VorbisRet = OggVorbisDecoderInit(pVorbisDecConfig);
	if (VorbisRet >= VORBIS_ERROR_MIN)
	{
		LOG_ERROR("Vorbis decoder initialize fail.\n");
		return OMX_ErrorUnsupportedSetting;
	}

    VorbisType.nBitRate = pVorbisDecConfig->ave_bitrate;
    VorbisType.nMaxBitRate= pVorbisDecConfig->max_bitrate;
    VorbisType.nMinBitRate= pVorbisDecConfig->min_bitrate;
    VorbisType.nChannels= pVorbisDecConfig->NoOfChannels;
    VorbisType.nSampleRate= pVorbisDecConfig->SampleRate;
	LOG_DEBUG("Vorbis BitRate: %d MaxBitRate: %d MinBitRate: %d Channel: %d SampleRate: %d\n", VorbisType.nBitRate, VorbisType.nMaxBitRate, VorbisType.nMinBitRate, VorbisType.nChannels, VorbisType.nSampleRate);
 
	AudioRingBuffer.nPrevOffset = 0;

	return ret;
}

OMX_ERRORTYPE VorbisDec::AudioFilterInstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	FSL_FREE(pVorbisDecConfig->pvOggDecObj);
	FSL_FREE(pVorbisDecConfig->decoderbuf);
	FSL_FREE(pVorbisDecConfig);
	FSL_FREE(pCodecConfigData);

    return ret;
}

OMX_ERRORTYPE VorbisDec::AudioFilterGetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioVorbis:
            {
                OMX_AUDIO_PARAM_VORBISTYPE *pVorbisType;
                pVorbisType = (OMX_AUDIO_PARAM_VORBISTYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pVorbisType, OMX_AUDIO_PARAM_VORBISTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pVorbisType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(pVorbisType, &VorbisType,	sizeof(OMX_AUDIO_PARAM_VORBISTYPE));
            }
			return ret;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE VorbisDec::AudioFilterSetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioVorbis:
            {
                OMX_AUDIO_PARAM_VORBISTYPE *pVorbisType;
                pVorbisType = (OMX_AUDIO_PARAM_VORBISTYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pVorbisType, OMX_AUDIO_PARAM_VORBISTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pVorbisType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(&VorbisType, pVorbisType, sizeof(OMX_AUDIO_PARAM_VORBISTYPE));
			}
			return ret;
		default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}
 
AUDIO_FILTERRETURNTYPE VorbisDec::AudioFilterFrame()
{
    AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
	VORBIS_RETURN VorbisRet;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;
    OMX_U32 consumelen = 0;
    OMX_U32 packtype, offset, data_left, i, byte;
    OMX_U8 buffer[6];
	OMX_BOOL bUseCodecConfigData = OMX_FALSE;

    if (pVorbisDecConfig->mPacketCount<3){
		bUseCodecConfigData = OMX_TRUE;
		pBuffer = pCodecConfigData + nCodecConfigDataOffset;
		nActuralLen = nCodecConfigDataLen - nCodecConfigDataOffset;
        offset = 0;
        data_left = nActuralLen;

        while(data_left>0){
            packtype = pBuffer[offset];


            if (packtype == INFORMATION_HEADER ||
                packtype == COMMENT_HEADER ||
                packtype == CODEC_SETUP_HEADER){
                byte = offset+1;
                for (i=0; i<6; i++)
                    buffer[i] = pBuffer[byte++];
                if(fsl_osal_memcmp(buffer,(void *)"vorbis",6)==0){
                    consumelen += offset;
                    break;
                }
            }
            offset++;
            data_left--;
            if(data_left==0){
                LOG_DEBUG("decode head packet error!");
				nCodecConfigDataOffset += consumelen;
				return AUDIO_FILTER_FAILURE;
            }
        }
    } else {
		AudioRingBuffer.BufferGet(&pBuffer, VORBIS10D_MAXDATAREQUESTED, &nActuralLen);
		LOG_DEBUG("Vorbis decoder get stream len: %d\n", nActuralLen);
	}

	pVorbisDecConfig->initial_buffer = (char*)pBuffer + consumelen;
	pVorbisDecConfig->buffer_length = nActuralLen - consumelen;
	pVorbisDecConfig->pcmout = (char*)pOutBufferHdr->pBuffer;
	pVorbisDecConfig->output_length = VORBIS10D_OUTBUFF_SIZE;

	VorbisRet = OggVorbisDecode(pVorbisDecConfig);
	
	if (VorbisRet < VORBIS_ERROR_MIN)
	{
		if (bUseCodecConfigData == OMX_TRUE)
				nCodecConfigDataOffset += pVorbisDecConfig->byteConsumed + consumelen;
		else {
			AudioRingBuffer.BufferConsumered(pVorbisDecConfig->byteConsumed + consumelen);
			LOG_DEBUG("Vorbis decoder consumed len: %d\n", pVorbisDecConfig->byteConsumed + consumelen);
		}
	}
	else
	{
		if (bUseCodecConfigData == OMX_TRUE)
				nCodecConfigDataOffset += nActuralLen;
		else {
			AudioRingBuffer.BufferConsumered(nActuralLen);
			LOG_DEBUG("Vorbis decoder consumed len: %d\n", nActuralLen);
			if (nActuralLen == 0)
				VorbisRet = VORBIS_EOS;
		}
	}

	LOG_LOG("VorbisRet = %d\n", VorbisRet);
	if (VorbisRet < VORBIS_ERROR_MIN)
	{

		LOG_DEBUG("Vorbis channels: %d sample rate: %d\n", VorbisType.nChannels, VorbisType.nSampleRate);
		if (VorbisType.nChannels != PcmMode.nChannels
				|| VorbisType.nSampleRate != PcmMode.nSamplingRate)
		{
			PcmMode.nChannels = VorbisType.nChannels;
			PcmMode.nSamplingRate = VorbisType.nSampleRate;
			SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
		}

		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = pVorbisDecConfig->outNumSamples * \
									VorbisType.nChannels * 2;
 
		TS_PerFrame = (OMX_U64)pVorbisDecConfig->outNumSamples*OMX_TICKS_PER_SECOND/VorbisType.nSampleRate;
		LOG_DEBUG("Decoder output sample: %d\t Sample Rate = %d\t TS per Frame = %lld\n", \
				pVorbisDecConfig->outNumSamples, VorbisType.nSampleRate, TS_PerFrame);

		AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 
	}
	else if (VorbisRet == VORBIS_EOS)
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

OMX_ERRORTYPE VorbisDec::AudioFilterReset()
{
	nCodecConfigDataOffset = 0;
	return OMX_ErrorNone;//AudioFilterCodecInit();
}

OMX_ERRORTYPE VorbisDec::AudioFilterCheckCodecConfig()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_VORBISTYPE *pVorbisType;
	pVorbisType = (OMX_AUDIO_PARAM_VORBISTYPE*)pInBufferHdr->pBuffer;
	OMX_CHECK_STRUCT(pVorbisType, OMX_AUDIO_PARAM_VORBISTYPE, ret);
	if(ret != OMX_ErrorNone)
	{
		LOG_DEBUG("Vorbis codec config len: %d\n", pInBufferHdr->nFilledLen);

		if (pCodecConfigData != NULL) {
			pCodecConfigData = (OMX_U8 *)FSL_REALLOC(pCodecConfigData, \
					nCodecConfigDataLen + pInBufferHdr->nFilledLen);
			if (pCodecConfigData == NULL)
			{
				LOG_ERROR("Can't get memory.\n");
				return OMX_ErrorInsufficientResources;
			}
			fsl_osal_memcpy(pCodecConfigData + nCodecConfigDataLen, pInBufferHdr->pBuffer, pInBufferHdr->nFilledLen);
			nCodecConfigDataOffset = 0;
			nCodecConfigDataLen += pInBufferHdr->nFilledLen;

		} else {
			pCodecConfigData = (OMX_U8 *)FSL_MALLOC(pInBufferHdr->nFilledLen);
			if (pCodecConfigData == NULL)
			{
				LOG_ERROR("Can't get memory.\n");
				return OMX_ErrorInsufficientResources;
			}
			fsl_osal_memcpy(pCodecConfigData, pInBufferHdr->pBuffer, pInBufferHdr->nFilledLen);
			nCodecConfigDataOffset = 0;
			nCodecConfigDataLen = pInBufferHdr->nFilledLen;
		}
		pInBufferHdr->nFilledLen = 0;
		return ret;
	}

	LOG_DEBUG("Vorbis codec type len: %d\n", pInBufferHdr->nFilledLen);
	fsl_osal_memcpy(pVorbisType, &VorbisType,	sizeof(OMX_AUDIO_PARAM_VORBISTYPE));

	pInBufferHdr->nFilledLen = 0;

    return ret;
}
 

/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE VorbisDecInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        VorbisDec *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(VorbisDec, ());
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
