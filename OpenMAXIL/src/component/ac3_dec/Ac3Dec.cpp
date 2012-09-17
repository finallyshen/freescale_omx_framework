/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Ac3Dec.h"
#define	NCHANS			        6			/* max channels */
#define KCAPABLE                1			/* Enables karaoke capable code */
#define	N_AC3                   256			/* block size */
#define NBLOCKS	                6			/* number of time blocks per frame */

#define AC3D_OUTPUT_BUF_SIZE 	N_AC3 * NBLOCKS * NCHANS * (sizeof(AC3D_INT32))
#define AC3D_MIN_INPUT_SIZE     AC3D_INPUT_BUF_SIZE

const static OMX_S32 channel_map[8] = {2,1,2,3,3,4,4,5};

Ac3Dec::Ac3Dec()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_decoder.ac3.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_decoder.ac3";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = AC3D_MIN_INPUT_SIZE;
	nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE Ac3Dec::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    hLib = NULL;
    pAc3DecConfig = NULL;
    libMgr = NULL;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = AUDIO_FILTER_INPUT_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = (OMX_AUDIO_CODINGTYPE)OMX_AUDIO_CodingAC3;
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
    sPortDef.nBufferSize = AC3D_OUTPUT_BUF_SIZE;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&Ac3Type, OMX_AUDIO_PARAM_AC3TYPE);
	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	Ac3Type.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	Ac3Type.nChannels = 2;
	Ac3Type.nSampleRate = 44100;

	PcmMode.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	nSkip = 0;

	libMgr = FSL_NEW(ShareLibarayMgr, ());
	if(libMgr == NULL)
		return OMX_ErrorInsufficientResources;

	hLib = libMgr->load((OMX_STRING)"lib_ac3_dec_v2_arm11_elinux.so");
	if(hLib == NULL) {
		LOG_WARNING("Can't load library lib_ac3_dec_v2_arm11_elinux.\n");
		FSL_DELETE(libMgr);
		return OMX_ErrorComponentNotFound;
	}

	pAC3D_QueryMem = (AC3D_RET_TYPE (*)( AC3DDecoderConfig *))libMgr->getSymbol(hLib, (OMX_STRING)"AC3D_QueryMem");
	pAC3D_dec_init = (AC3D_RET_TYPE (*)(AC3DDecoderConfig *, AC3D_UINT8 *, AC3D_INT32))libMgr->getSymbol(hLib, (OMX_STRING)"AC3D_dec_init");
	pAC3D_dec_Frame = (AC3D_RET_TYPE (*)(
				AC3DDecoderConfig *,
				AC3D_INT32 *,
				AC3D_INT16 *,
				AC3D_INT32))libMgr->getSymbol(hLib,(OMX_STRING)"AC3D_dec_Frame");

	if(pAC3D_QueryMem == NULL || pAC3D_dec_init == NULL || pAC3D_dec_Frame == NULL) {
		libMgr->unload(hLib);
		FSL_DELETE(libMgr);
		return OMX_ErrorComponentNotFound;
	}

	return ret;
}

OMX_ERRORTYPE Ac3Dec::DeInitComponent()
{
    if (libMgr) {
        if (hLib)
            libMgr->unload(hLib);
        FSL_DELETE(libMgr);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Ac3Dec::AudioFilterInstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	pAc3DecConfig = (AC3DDecoderConfig *)FSL_MALLOC(sizeof(AC3DDecoderConfig));
	if (pAc3DecConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pAc3DecConfig, 0, sizeof(AC3DDecoderConfig));

	AC3D_PARAM *pAc3DecPara = (AC3D_PARAM *)FSL_MALLOC(sizeof(AC3D_PARAM));
	if (pAc3DecPara == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pAc3DecPara, 0, sizeof(AC3D_PARAM));
	pAc3DecConfig->pAC3param  = pAc3DecPara;

	AC3D_RET_TYPE Ac3Ret;
	Ac3Ret = pAC3D_QueryMem(pAc3DecConfig);
	if (Ac3Ret != AC3D_OK)
	{
		LOG_ERROR("Ac3 decoder query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	OMX_S32 MemoryCnt = pAc3DecConfig->sAC3DMemInfo.s32NumReqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pAc3DecConfig->sAC3DMemInfo.sMemInfoSub[i].s32AC3DSize;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		pAc3DecConfig->sAC3DMemInfo.sMemInfoSub[i].app_base_ptr = pBuffer;
	}

	TS_PerFrame = AC3D_FRAME_SIZE*OMX_TICKS_PER_SECOND/Ac3Type.nSampleRate;

    return ret;
}

OMX_ERRORTYPE Ac3Dec::AudioFilterCodecInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	AC3D_RET_TYPE Ac3Ret;
	AC3D_PARAM *pAc3DecPara = pAc3DecConfig->pAC3param;

#ifndef DOWNMIX 
#define DOWNMIX
#endif
#ifdef DOWNMIX
	pAc3DecPara->numchans = 2;
    pAc3DecPara->chanptr[0] = 0;
    pAc3DecPara->chanptr[1] = 2;
    pAc3DecPara->chanptr[2] = 1;
    pAc3DecPara->chanptr[3] = 3;
    pAc3DecPara->chanptr[4] = 4;
    pAc3DecPara->chanptr[5] = 5;  
    //pAc3DecPara->wordsize = ac3dec_info->wordsize; 
    //pAc3DecPara->dynrngscalelow = (AC3D_INT32)(ac3dec_info->dynrngscalelow * 2147483647.0);
    //pAc3DecPara->dynrngscalehi = (AC3D_INT32)(ac3dec_info->dynrngscalehi * 2147483647.0);   
    //pAc3DecPara->pcmscalefac = (AC3D_INT32)(ac3dec_info->pcmscalefac * 2147483647.0);
    //pAc3DecPara->compmode = ac3dec_info->compmode;
    //pAc3DecPara->stereomode = ac3dec_info->stereomode;        
    //pAc3DecPara->dualmonomode = ac3dec_info->dualmonomode;
    //pAc3DecPara->outputmode = ac3dec_info->outputmode;
    //pAc3DecPara->outlfeon = ac3dec_info->outlfeon;
    pAc3DecPara->outputflg = 1;               /* enable output file flag */
    pAc3DecPara->framecount = 0;          /* frame counter */
    pAc3DecPara->blockcount = 0;          /* block counter */
    pAc3DecPara->framestart = 0;          /* starting frame */
    pAc3DecPara->frameend = -1;           /* ending frame */
    pAc3DecPara->useverbose = 0;          /* verbose messages flag */
    //pAc3DecPara->debug_arg = ac3dec_info->debug_arg;
#ifdef KCAPABLE
    //pAc3DecPara->kcapablemode = ac3dec_info->kcapablemode;
#endif
    pAc3DecPara->wordsize=0;                  /* output word size code */
    pAc3DecPara->dynrngscalelow=0x7FFFFFFF ;         /* dynamic range scale factor (low) */
    pAc3DecPara->dynrngscalehi=0x7FFFFFFF ;           /* dynamic range scale factor (high) */
    pAc3DecPara->pcmscalefac=0x7FFFFFFF ;         /* PCM scale factor */
    pAc3DecPara->compmode=2;                  /* compression mode */
    pAc3DecPara->stereomode=2;                /* stereo downmix mode */
    pAc3DecPara->dualmonomode=0;              /* dual mono reproduction mode */
    pAc3DecPara->outputmode=2;                /* output channel configuration */
    pAc3DecPara->outlfeon=0;                  /* output subwoofer present flag */
    pAc3DecPara->outputflg=1;                 /* enable output file flag */
    pAc3DecPara->framecount=0;                /* frame counter */
    pAc3DecPara->blockcount=0;                /* block counter */
    pAc3DecPara->framestart=0;                /* starting frame */
    pAc3DecPara->frameend=-1;                 /* ending frame */
    pAc3DecPara->useverbose=0;                /* verbose messages flag */
    pAc3DecPara->debug_arg=0;                 /* debug argument */
#ifdef KCAPABLE
    pAc3DecPara->kcapablemode = 3;                /* karaoke capable mode */
#endif
    /*output param  init*/
    pAc3DecPara->ac3d_endianmode = 2;     //endiamode checked by decoeder
    pAc3DecPara->ac3d_sampling_freq = 0;
    pAc3DecPara->ac3d_num_channels = 0;
    pAc3DecPara->ac3d_frame_size = 0;
    pAc3DecPara->ac3d_acmod = 0;
    pAc3DecPara->ac3d_bitrate = 0;
    pAc3DecPara->ac3d_outputmask = 0;
#else
	pAc3DecPara->numchans = 6;
    pAc3DecPara->chanptr[0] = 0;
    pAc3DecPara->chanptr[1] = 1;
    pAc3DecPara->chanptr[2] = 2;
    pAc3DecPara->chanptr[3] = 3;
    pAc3DecPara->chanptr[4] = 4;
    pAc3DecPara->chanptr[5] = 5;  
    //pAc3DecPara->wordsize = ac3dec_info->wordsize; 
    //pAc3DecPara->dynrngscalelow = (AC3D_INT32)(ac3dec_info->dynrngscalelow * 2147483647.0);
    //pAc3DecPara->dynrngscalehi = (AC3D_INT32)(ac3dec_info->dynrngscalehi * 2147483647.0);   
    //pAc3DecPara->pcmscalefac = (AC3D_INT32)(ac3dec_info->pcmscalefac * 2147483647.0);
    //pAc3DecPara->compmode = ac3dec_info->compmode;
    //pAc3DecPara->stereomode = ac3dec_info->stereomode;        
    //pAc3DecPara->dualmonomode = ac3dec_info->dualmonomode;
    //pAc3DecPara->outputmode = ac3dec_info->outputmode;
    //pAc3DecPara->outlfeon = ac3dec_info->outlfeon;
    pAc3DecPara->outputflg = 1;               /* enable output file flag */
    pAc3DecPara->framecount = 0;          /* frame counter */
    pAc3DecPara->blockcount = 0;          /* block counter */
    pAc3DecPara->framestart = 0;          /* starting frame */
    pAc3DecPara->frameend = -1;           /* ending frame */
    pAc3DecPara->useverbose = 0;          /* verbose messages flag */
    //pAc3DecPara->debug_arg = ac3dec_info->debug_arg;
#ifdef KCAPABLE
    //pAc3DecPara->kcapablemode = ac3dec_info->kcapablemode;
#endif
    pAc3DecPara->wordsize=0;                  /* output word size code */
    pAc3DecPara->dynrngscalelow=0x7FFFFFFF ;         /* dynamic range scale factor (low) */
    pAc3DecPara->dynrngscalehi=0x7FFFFFFF ;           /* dynamic range scale factor (high) */
    pAc3DecPara->pcmscalefac=0x7FFFFFFF ;         /* PCM scale factor */
    pAc3DecPara->compmode=2;                  /* compression mode */
    pAc3DecPara->stereomode=0;                /* stereo downmix mode */
    pAc3DecPara->dualmonomode=0;              /* dual mono reproduction mode */
    pAc3DecPara->outputmode=7;                /* output channel configuration */
    pAc3DecPara->outlfeon=1;                  /* output subwoofer present flag */
    pAc3DecPara->outputflg=1;                 /* enable output file flag */
    pAc3DecPara->framecount=0;                /* frame counter */
    pAc3DecPara->blockcount=0;                /* block counter */
    pAc3DecPara->framestart=0;                /* starting frame */
    pAc3DecPara->frameend=-1;                 /* ending frame */
    pAc3DecPara->useverbose=0;                /* verbose messages flag */
    pAc3DecPara->debug_arg=0;                 /* debug argument */
#ifdef KCAPABLE
    pAc3DecPara->kcapablemode = 3;                /* karaoke capable mode */
#endif
    /*output param  init*/
    pAc3DecPara->ac3d_endianmode = 2;     //endiamode checked by decoeder
    pAc3DecPara->ac3d_sampling_freq = 0;
    pAc3DecPara->ac3d_num_channels = 0;
    pAc3DecPara->ac3d_frame_size = 0;
    pAc3DecPara->ac3d_acmod = 0;
    pAc3DecPara->ac3d_bitrate = 0;
    pAc3DecPara->ac3d_outputmask = 0;
#endif
 
	pAc3DecConfig->pContext  = NULL;

	Ac3Ret = pAC3D_dec_init(pAc3DecConfig, NULL, 0);
	if (Ac3Ret != AC3D_OK)
	{
		LOG_ERROR("Ac3 decoder initialize fail.\n");
		return OMX_ErrorUnsupportedSetting;
	}

    return ret;
}

OMX_ERRORTYPE Ac3Dec::AudioFilterInstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    if(pAc3DecConfig){
        OMX_S32 MemoryCnt = pAc3DecConfig->sAC3DMemInfo.s32NumReqs;
        for (OMX_S32 i = 0; i < MemoryCnt; i ++)
        {
            FSL_FREE(pAc3DecConfig->sAC3DMemInfo.sMemInfoSub[i].app_base_ptr);
        }

        FSL_FREE(pAc3DecConfig->pAC3param);
        FSL_FREE(pAc3DecConfig);
    }

    return ret;
}

OMX_ERRORTYPE Ac3Dec::AudioFilterGetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioAc3:
            {
                OMX_AUDIO_PARAM_AC3TYPE *pAc3Type;
                pAc3Type = (OMX_AUDIO_PARAM_AC3TYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pAc3Type, OMX_AUDIO_PARAM_AC3TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pAc3Type->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				fsl_osal_memcpy(pAc3Type, &Ac3Type,	sizeof(OMX_AUDIO_PARAM_AC3TYPE));
#ifdef DOWNMIX
				AC3D_PARAM *pAc3DecPara = pAc3DecConfig->pAC3param;
				pAc3Type->nChannels = channel_map[pAc3DecPara->ac3d_acmod]+pAc3DecPara->ac3d_lfeon;
#endif
            }
			return ret;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE Ac3Dec::AudioFilterSetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioAc3:
            {
                OMX_AUDIO_PARAM_AC3TYPE *pAc3Type;
                pAc3Type = (OMX_AUDIO_PARAM_AC3TYPE*)pComponentParameterStructure;
                OMX_CHECK_STRUCT(pAc3Type, OMX_AUDIO_PARAM_AC3TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
				if (pAc3Type->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				//fsl_osal_memcpy(&Ac3Type, pAc3Type, sizeof(OMX_AUDIO_PARAM_AC3TYPE));
				Ac3Type.nBitRate = pAc3Type->nBitRate;
			}
			return ret;
		default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}
 
OMX_ERRORTYPE Ac3Dec::AudioFilterCheckFrameHeader()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;
	OMX_U32 nOffset=0;
	OMX_BOOL bFounded = OMX_FALSE;

	while (1)
	{
		AudioRingBuffer.BufferGet(&pBuffer, CHECKFRAMEHEADERREADLEN, &nActuralLen);
		LOG_LOG("Get stream length: %d\n", nActuralLen);

		if (nActuralLen < 2)
			break;

		nOffset = 0;
		while(nActuralLen >= 2)
		{
			if((pBuffer[0] ==0x0B && pBuffer[1] == 0x77) \
					|| (pBuffer[1] ==0x0B && pBuffer[0] == 0x77))
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

		LOG_DEBUG("Ac3 decoder skip %d bytes.\n", nOffset);
		AudioRingBuffer.BufferConsumered(nOffset);
		nSkip += nOffset;

		if (bFounded == OMX_TRUE)
		{
			if (Ac3Type.nBitRate)
				AudioRingBuffer.TS_SetIncrease(nSkip * 8 * OMX_TICKS_PER_SECOND / Ac3Type.nBitRate);
			nSkip = 0;
			break;
		}
	}

    return ret;
}

AUDIO_FILTERRETURNTYPE Ac3Dec::AudioFilterFrame()
{
    AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;
	AC3D_PARAM *pAc3DecPara = pAc3DecConfig->pAC3param;

	AudioRingBuffer.BufferGet(&pBuffer, nPushModeInputLen, &nActuralLen);
	LOG_LOG("Get stream length: %d\n", nActuralLen);

	AC3D_RET_TYPE Ac3Ret;

	Ac3Ret = pAC3D_dec_Frame(pAc3DecConfig, (AC3D_INT32 *)pOutBufferHdr->pBuffer, \
			(AC3D_INT16 *)pBuffer, nActuralLen);
	if (pAc3DecPara->ac3d_frame_size > nActuralLen)
	{
		AudioRingBuffer.BufferConsumered(nActuralLen);
	}
	else
	{
		LOG_LOG("Consumed: %d\n", pAc3DecPara->ac3d_frame_size);
		AudioRingBuffer.BufferConsumered(pAc3DecPara->ac3d_frame_size);
	}

	if (nActuralLen < 2)
		Ac3Ret = AC3D_END_OF_STREAM;

	LOG_LOG("Ac3Ret = %d\n", Ac3Ret);
	if (Ac3Ret == AC3D_OK)
	{
		Ac3Type.nBitRate = pAc3DecPara->ac3d_bitrate;
		LOG_DEBUG("bit rate: %d\n", Ac3Type.nBitRate);

		if (Ac3Type.nChannels != (OMX_U16)pAc3DecPara->ac3d_num_channels
				|| Ac3Type.nSampleRate != (OMX_U32)pAc3DecPara->ac3d_sampling_freq)
		{
			LOG_DEBUG("AC3 decoder channel: %d sample rate: %d\n", \
					pAc3DecPara->ac3d_num_channels, pAc3DecPara->ac3d_sampling_freq);
			if (pAc3DecPara->ac3d_num_channels == 0 || pAc3DecPara->ac3d_sampling_freq == 0)
			{
				return AUDIO_FILTER_FAILURE;
			}
			Ac3Type.nChannels = pAc3DecPara->ac3d_num_channels;
			Ac3Type.nSampleRate = pAc3DecPara->ac3d_sampling_freq;
			PcmMode.nChannels = pAc3DecPara->ac3d_num_channels;
			PcmMode.nSamplingRate = pAc3DecPara->ac3d_sampling_freq;
			TS_PerFrame = AC3D_FRAME_SIZE*OMX_TICKS_PER_SECOND/Ac3Type.nSampleRate;
			SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
		}

        AC3D_INT16 outshortval;
        AC3D_INT16 *outshortptr, count, chan;
        AC3D_INT32 dblval,block;
        AC3D_INT16 *pcmbufptr;
        AC3D_INT32 outbuf[NCHANS * N_AC3 * NBLOCKS];  

		outshortptr = (AC3D_INT16 *)outbuf;
        pcmbufptr = (AC3D_INT16 *)pOutBufferHdr->pBuffer;
        for (block = 0; block < NBLOCKS; block++)
        {
            for (count = 0; count < N_AC3; count++)
            {
                for (chan = 0; chan < pAc3DecPara->ac3d_num_channels; chan++)
                {
                    dblval = *pcmbufptr++ ;
                    outshortval = (AC3D_INT16)(dblval);
                    *outshortptr++ = outshortval;
                }
                pcmbufptr += NCHANS - pAc3DecPara->ac3d_num_channels;
            }
        }

		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = sizeof(AC3D_INT16) * pAc3DecPara->ac3d_num_channels * N_AC3 * NBLOCKS;

		fsl_osal_memcpy(pOutBufferHdr->pBuffer, outbuf, pOutBufferHdr->nFilledLen);

		LOG_LOG("TS per Frame = %lld\n", TS_PerFrame);

		AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 
	}
	else if (Ac3Ret == AC3D_END_OF_STREAM)
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

OMX_ERRORTYPE Ac3Dec::AudioFilterReset()
{
    return AudioFilterCodecInit();
}

OMX_ERRORTYPE Ac3Dec::AudioFilterCheckCodecConfig()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	pInBufferHdr->nFilledLen = 0;

    return ret;
}
 

/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE Ac3DecInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        Ac3Dec *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(Ac3Dec, ());
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
