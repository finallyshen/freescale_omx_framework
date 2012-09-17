/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Mp3Dec.h"

#define MP3FRAMEHEADERLEN		4
#define MP3_VERSION_MASK  		0x00180000
#define MP3_LAYER_MASK			0x00060000	
#define MP3_BITRATE_MASK 		0x0000f000
#define MP3_SAMPLERATE_MASK 	0x00000c00
#define MP3_PADDING_MASK 		0x00000200

const static OMX_U32 mp3_bitrate_table[5][15] =
{
  /* MPEG-1 */
  { 0,  32000,  64000,  96000, 128000, 160000, 192000, 224000,  /* Layer I   */
       256000, 288000, 320000, 352000, 384000, 416000, 448000 },
  { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,  /* Layer II  */
       128000, 160000, 192000, 224000, 256000, 320000, 384000 },
  { 0,  32000,  40000,  48000,  56000,  64000,  80000,  96000,  /* Layer III */
       112000, 128000, 160000, 192000, 224000, 256000, 320000 },

  /* MPEG-2 LSF */
  { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,  /* Layer I   */
       128000, 144000, 160000, 176000, 192000, 224000, 256000 },
  { 0,   8000,  16000,  24000,  32000,  40000,  48000,  56000,  /* Layers    */
        64000,  80000,  96000, 112000, 128000, 144000, 160000 } /* II & III  */
};
const static OMX_U32 mp3_samplerate_table[3] =
{
 44100, 48000, 32000
};


Mp3Dec::Mp3Dec()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_decoder.mp3.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_decoder.mp3";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = MP3D_INPUT_BUF_PUSH_SIZE;
	nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE Mp3Dec::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = AUDIO_FILTER_INPUT_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingMP3;
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
    sPortDef.nBufferSize = MP3D_FRAME_SIZE* 2*2;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&Mp3Type, OMX_AUDIO_PARAM_MP3TYPE);
	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	Mp3Type.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	Mp3Type.nChannels = 2;
	Mp3Type.nSampleRate = 44100;
	Mp3Type.eChannelMode = OMX_AUDIO_ChannelModeStereo;
	Mp3Type.eFormat = OMX_AUDIO_MP3StreamFormatMP1Layer3;

	PcmMode.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	TS_PerFrame = MP3D_FRAME_SIZE*OMX_TICKS_PER_SECOND/Mp3Type.nSampleRate;

	return ret;
}

OMX_ERRORTYPE Mp3Dec::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Mp3Dec::AudioFilterInstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	pMp3DecConfig = (MP3D_Decode_Config *)FSL_MALLOC(sizeof(MP3D_Decode_Config));
	if (pMp3DecConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pMp3DecConfig, 0, sizeof(MP3D_Decode_Config));

	pMp3DecParams = (MP3D_Decode_Params *)FSL_MALLOC(sizeof(MP3D_Decode_Params));
	if (pMp3DecParams == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pMp3DecParams, 0, sizeof(MP3D_Decode_Params));

	MP3D_RET_TYPE Mp3Ret;
	Mp3Ret = mp3d_query_dec_mem(pMp3DecConfig);
	if (Mp3Ret != MP3D_OK)
	{
		LOG_ERROR("Mp3 decoder query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	OMX_S32 MemoryCnt = pMp3DecConfig->mp3d_mem_info.mp3d_num_reqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pMp3DecConfig->mp3d_mem_info.mem_info_sub[i].mp3d_size;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		pMp3DecConfig->mp3d_mem_info.mem_info_sub[i].app_base_ptr = pBuffer;
	}

    return ret;
}

OMX_ERRORTYPE Mp3Dec::AudioFilterCodecInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	MP3D_UINT8 Buffer;
	MP3D_RET_TYPE Mp3Ret;
	Mp3Ret = mp3d_decode_init(pMp3DecConfig, &Buffer, 0);
	if (Mp3Ret != MP3D_OK)
	{
		LOG_ERROR("Mp3 decoder initialize fail.\n");
		return OMX_ErrorUndefined;
	}

    return ret;
}

OMX_ERRORTYPE Mp3Dec::AudioFilterInstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 MemoryCnt = pMp3DecConfig->mp3d_mem_info.mp3d_num_reqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		FSL_FREE(pMp3DecConfig->mp3d_mem_info.mem_info_sub[i].app_base_ptr);
	}

	FSL_FREE(pMp3DecConfig);
	FSL_FREE(pMp3DecParams);

    return ret;
}

OMX_ERRORTYPE Mp3Dec::AudioFilterGetParameter(
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
				if (pMp3Type->nPortIndex != AUDIO_FILTER_INPUT_PORT)
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

OMX_ERRORTYPE Mp3Dec::AudioFilterSetParameter(
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
				if (pMp3Type->nPortIndex != AUDIO_FILTER_INPUT_PORT)
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

static OMX_S32 Mp3Dec_CalcFrameLength(OMX_U32 frame_header)
{
	int frame_length,bitrate,samplerate,layer,ver_id,padding,bitrate_idx, samplerate_idx;	
	ver_id = (frame_header & MP3_VERSION_MASK) >> 19;
	ver_id = 4-ver_id; //ver_id, 1:ver1, 2:ver2; 4:ver2.5
	if(ver_id == 3)
		return 0;

	layer = (frame_header & MP3_LAYER_MASK) >> 17;	
	layer = 4-layer;
	if(layer ==4)
		return 0;
	bitrate_idx = (frame_header & MP3_BITRATE_MASK) >>12 ;

	if( bitrate_idx == 15 )
		return 0;
	if(ver_id == 1 )
		bitrate = mp3_bitrate_table[layer -1][bitrate_idx];
	else 
		bitrate = mp3_bitrate_table[3 + (layer >> 1)][bitrate_idx];
	
	samplerate_idx= (frame_header & MP3_SAMPLERATE_MASK) >> 10;
	if( samplerate_idx == 3 )
		return 0;
	
	samplerate = mp3_samplerate_table[samplerate_idx] / ver_id;
	
	padding = (frame_header & MP3_PADDING_MASK) >> 9 ;
	if (layer == 1)
		frame_length= ((12 * bitrate / samplerate) + padding) * 4;
	else {
		OMX_S32 slots_per_frame= (layer == 3 && (ver_id >1)) ? 72 : 144;
		frame_length= (slots_per_frame * bitrate / samplerate) + padding;
	}
 	return frame_length;
}

OMX_BOOL Mp3Dec::IsValidFrameHeader(OMX_U8 * pHeader)
{
    int layer;

    if(pHeader == NULL)
        return OMX_FALSE;
    
    if(pHeader[0] !=0xFF || (pHeader[1] & 0xE0) != 0xE0)
        return OMX_FALSE;

    layer  = 4 - (((int)pHeader[1]&0x06)>>1);

    if (( 3!= layer) && (2 != layer) && (1 != layer))  
        return OMX_FALSE;

    OMX_S32 nFrameHeader = (pHeader[0]<<24) | (pHeader[1]<<16) |(pHeader[2]<<8) |pHeader[3];
    OMX_S32 nFrameLen = Mp3Dec_CalcFrameLength(nFrameHeader);
    if(nFrameLen < MP3FRAMEHEADERLEN)
        return OMX_FALSE;
    
    return OMX_TRUE;
}

OMX_ERRORTYPE Mp3Dec::AudioFilterCheckFrameHeader()
{
    OMX_ERRORTYPE ret = OMX_ErrorUndefined;
    OMX_U8 *pBuffer;
    OMX_U8 *pHeader;
    OMX_U32 nActuralLen;
    OMX_U32 nOffset=0;
    OMX_BOOL bFounded = OMX_FALSE;
    OMX_U8 i = 0, check_more_header = 2;

    while (1)
    {
        AudioRingBuffer.BufferGet(&pBuffer, CHECKFRAMEHEADERREADLEN, &nActuralLen);
        LOG_LOG("Get stream length: %d\n", nActuralLen);

        if (nActuralLen < MP3FRAMEHEADERLEN)
            break;

        nOffset = 0;
        while(nActuralLen - nOffset >= MP3FRAMEHEADERLEN)
        {
                    
            pHeader = pBuffer + nOffset;
            if(!IsValidFrameHeader(pHeader)){
                nOffset++;
                continue;
            }

            LOG_LOG("first header found\n");

            bFounded = OMX_TRUE;

            OMX_U32 required_data_len = nOffset + MP3FRAMEHEADERLEN;
                    
            for(int i = 0; i < check_more_header; i++)
            {

                LOG_LOG("header%d: %X %X %X %X\n",i, pHeader[0],pHeader[1],pHeader[2],pHeader[3]);

                OMX_S32 nFrameHeader = (pHeader[0]<<24) | (pHeader[1]<<16) |(pHeader[2]<<8) |pHeader[3];
                OMX_S32 nFrameLen = Mp3Dec_CalcFrameLength(nFrameHeader);
                LOG_LOG("nFrameLen is %d\n", nFrameLen);
                required_data_len += nFrameLen;

                if(nActuralLen < required_data_len){
                    OMX_U32 header_offset = pHeader - pBuffer;
                    AudioRingBuffer.BufferGet(&pBuffer, required_data_len, &nActuralLen);
                    if(nActuralLen < required_data_len){
                        LOG_LOG("can not read enough data, suppose header found\n");
                        break;
                    }
                    else
                        pHeader = pBuffer + header_offset;
                }

                pHeader += nFrameLen;

                if(!IsValidFrameHeader(pHeader)){
                    bFounded = OMX_FALSE;
                    break;
                }


            }

            if(bFounded == OMX_FALSE){
                nOffset++;
                continue;
            }
            else
                break;

        }

        LOG_LOG("Mp3 decoder skip %d bytes.\n", nOffset);
        AudioRingBuffer.BufferConsumered(nOffset);

        if (bFounded == OMX_TRUE){
            ret = OMX_ErrorNone;
            break;
        }
    }

    return ret;
}

AUDIO_FILTERRETURNTYPE Mp3Dec::AudioFilterFrame()
{
    AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;

	AudioRingBuffer.BufferGet(&pBuffer, MP3D_INPUT_BUF_PUSH_SIZE, &nActuralLen);
	LOG_LOG("Mp3 decoder get stream len: %d\n", nActuralLen);
	pMp3DecConfig->pInBuf = (MP3D_INT8 *)pBuffer;
	pMp3DecConfig->inBufLen = nActuralLen;
	pMp3DecConfig->consumedBufLen = 0;

	MP3D_RET_TYPE Mp3Ret;
	Mp3Ret = mp3d_decode_frame(pMp3DecConfig, pMp3DecParams, (MP3D_INT32 *)pOutBufferHdr->pBuffer);
     
	if (Mp3Ret == MP3D_END_OF_STREAM)
		pMp3DecConfig->consumedBufLen = nActuralLen;
	LOG_LOG("Audio stream frame len = %d, mp3 ret %d\n", pMp3DecConfig->consumedBufLen, Mp3Ret);
	AudioRingBuffer.BufferConsumered(pMp3DecConfig->consumedBufLen);

	LOG_LOG("Mp3Ret = %d\n", Mp3Ret);
	if (Mp3Ret == MP3D_OK || Mp3Ret == MP3D_ERROR_SCALEFACTOR)
	{
		Mp3Type.nBitRate = pMp3DecParams->mp3d_bit_rate*1000;
		LOG_DEBUG("nSampleRate: %d nChannels: %d\n", pMp3DecParams->mp3d_sampling_freq, pMp3DecParams->mp3d_num_channels);
		if (Mp3Type.nChannels != (OMX_U16)pMp3DecParams->mp3d_num_channels
				|| Mp3Type.nSampleRate != (OMX_U32)pMp3DecParams->mp3d_sampling_freq)
		{
			Mp3Type.nChannels = pMp3DecParams->mp3d_num_channels;
			Mp3Type.nSampleRate = pMp3DecParams->mp3d_sampling_freq;
			PcmMode.nChannels = pMp3DecParams->mp3d_num_channels;
			PcmMode.nSamplingRate = pMp3DecParams->mp3d_sampling_freq;
			TS_PerFrame = MP3D_FRAME_SIZE*OMX_TICKS_PER_SECOND/Mp3Type.nSampleRate;
			LOG_DEBUG("Decoder Sample Rate = %d\t TS per Frame = %lld\n", Mp3Type.nSampleRate, \
					TS_PerFrame);
			SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
		}

		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = MP3D_FRAME_SIZE* Mp3Type.nChannels*2;

		LOG_LOG("TS per Frame = %lld\n", TS_PerFrame);

		AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 
	}
	else if (Mp3Ret == MP3D_END_OF_STREAM)
	{
		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = 0;

		ret = AUDIO_FILTER_EOS;
	}
	else
	{
		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = 0;

		ret = AUDIO_FILTER_FAILURE;
	}

	LOG_LOG("Decoder nTimeStamp = %lld\n", pOutBufferHdr->nTimeStamp);

    return ret;
}

OMX_ERRORTYPE Mp3Dec::AudioFilterReset()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	MP3D_RET_TYPE Mp3Ret;
	MP3D_UINT8 Buffer;
	Mp3Ret = mp3d_decode_init(pMp3DecConfig, &Buffer, 0);
	if (Mp3Ret != MP3D_OK)
	{
		LOG_ERROR("Mp3 decoder initialize fail.\n");
		return OMX_ErrorUndefined;
	}

    return ret;
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE Mp3DecInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        Mp3Dec *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(Mp3Dec, ());
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
