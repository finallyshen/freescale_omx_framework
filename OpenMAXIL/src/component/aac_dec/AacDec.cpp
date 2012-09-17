/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "AacDec.h"

#define FRAME_HEADER_SIZE 7
#define ADIF_FRAME_HEADER_SIZE 7
#define ADIF_FILE 0x41444946
#define ADTS_FILE 0xFFF00000
static const OMX_U32 sampling_frequency[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000};

static const OMX_S32 samplingFrequencyMap[16] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};

enum {
  FLAG_SUCCESS,
  FLAG_NEEDMORE_DATA
};

typedef enum {
    FILETYPEADTS,
    FILETYPEADIF,
    FILETYPERAW
}AAC_FILETYPE;

typedef enum {
    AAC_RETURNSUCESS,
    AAC_RETURNFAIL
}AAC_RETURNTYPE;

typedef struct
{
  OMX_U32 nBitConsumered;
  OMX_U32 nBitTotal;
  BitstreamParam sBitstreamParam;
} BitstreamParamExt;

static OMX_S32 AacDec_Bs_ReadInit(OMX_U8 *buf, OMX_S32 bytes, BitstreamParamExt *pBit)
{
	OMX_U32 	nTemp;

	pBit->nBitConsumered = 0;
	pBit->nBitTotal = bytes << 3;

	pBit->sBitstreamParam.bs_curr = (OMX_U8 *)buf;
	pBit->sBitstreamParam.bs_end = pBit->sBitstreamParam.bs_curr + bytes;
	pBit->sBitstreamParam.bs_eof = 0;
	pBit->sBitstreamParam.bs_seek_needed = 0;

	pBit->sBitstreamParam.bit_register = 0;
	pBit->sBitstreamParam.bit_counter = BIT_COUNTER_INIT;

	while (pBit->sBitstreamParam.bit_counter >= 0)
	{
		if (pBit->sBitstreamParam.bs_curr >= pBit->sBitstreamParam.bs_end)
		{
			return -1;
		}
		nTemp = *pBit->sBitstreamParam.bs_curr++;
		pBit->sBitstreamParam.bit_register = pBit->sBitstreamParam.bit_register | (nTemp << pBit->sBitstreamParam.bit_counter);
		pBit->sBitstreamParam.bit_counter -= 8;
	}

	return 0;
}

static OMX_S32 AacDec_Bs_Byte_Align(BitstreamParamExt *pBit)
{
	OMX_S32             nbits;

	nbits = MIN_REQD_BITS - pBit->sBitstreamParam.bit_counter;
	nbits = nbits & 0x7;    /* LSB 3 bits */

	pBit->sBitstreamParam.bit_register <<= nbits;
	pBit->sBitstreamParam.bit_counter += nbits;
	pBit->nBitConsumered += nbits;

	return (nbits);
}

static OMX_S32 AacDec_Bs_Read_Bits (OMX_S32 nbits, BitstreamParamExt *pBit)
{
	OMX_U32 	nTemp,nTemp1,nBitRegister;
	OMX_S32 		nBitCounter;

	nBitCounter = pBit->sBitstreamParam.bit_counter;
	nBitRegister = pBit->sBitstreamParam.bit_register;

	/* If more than available bits are requested,
	 * return error
	 */
	if ((MIN_REQD_BITS - nBitCounter) < nbits)
		return -1;

	nTemp = nBitRegister >> (32 - nbits);
	nBitRegister <<= nbits;
	nBitCounter += nbits;

	while (nBitCounter >= 0)
	{
		if (pBit->sBitstreamParam.bs_curr >= pBit->sBitstreamParam.bs_end)
		{
			return -1;
		}

		nTemp1 = *pBit->sBitstreamParam.bs_curr++;
		nBitRegister = nBitRegister | (nTemp1 << nBitCounter);
		nBitCounter -= 8;
	}

	pBit->sBitstreamParam.bit_register = nBitRegister;
	pBit->sBitstreamParam.bit_counter = nBitCounter;
	pBit->nBitConsumered += nbits;
	return(nTemp);
}

static void AacDec_Get_Ele_List(AACD_EleList * pList, OMX_S32 enable_cpe, BitstreamParamExt *pBit)
{
	OMX_S32 nCount, nNumElements;

	for (nCount = 0, nNumElements = pList->num_ele; nCount < nNumElements; nCount++)
	{
		if (enable_cpe)
			pList->ele_is_cpe[nCount] = AacDec_Bs_Read_Bits(LEN_ELE_IS_CPE, pBit);
		else
			pList->ele_is_cpe[nCount] = 0;
		pList->ele_tag[nCount] = AacDec_Bs_Read_Bits(LEN_TAG, pBit);
	}
}

static OMX_S32 AacDec_Get_Prog_Config(AACD_ProgConfig * pProgConfig, BitstreamParamExt *pBit)
{
	OMX_S32 nCount, nBits;

	pProgConfig->tag = AacDec_Bs_Read_Bits(LEN_TAG, pBit);

	pProgConfig->profile = AacDec_Bs_Read_Bits(LEN_PROFILE, pBit);
	if (pProgConfig->profile != 1)
	{
		return -1;
	}
	pProgConfig->sampling_rate_idx = AacDec_Bs_Read_Bits(LEN_SAMP_IDX, pBit);
	if (pProgConfig->sampling_rate_idx >= 0xc)
	{
		return -1;
	}
	pProgConfig->front.num_ele = AacDec_Bs_Read_Bits(LEN_NUM_ELE, pBit);
	if (pProgConfig->front.num_ele > FCHANS)
	{
		return -1;
	}
	pProgConfig->side.num_ele = AacDec_Bs_Read_Bits(LEN_NUM_ELE, pBit);
	if (pProgConfig->side.num_ele > SCHANS)
	{
		return -1;
	}
	pProgConfig->back.num_ele = AacDec_Bs_Read_Bits(LEN_NUM_ELE, pBit);
	if (pProgConfig->back.num_ele > BCHANS)
	{
		return -1;
	}
	pProgConfig->lfe.num_ele = AacDec_Bs_Read_Bits(LEN_NUM_LFE, pBit);
	if (pProgConfig->lfe.num_ele > LCHANS)
	{
		return -1;
	}
	pProgConfig->data.num_ele = AacDec_Bs_Read_Bits(LEN_NUM_DAT, pBit);
	pProgConfig->coupling.num_ele = AacDec_Bs_Read_Bits(LEN_NUM_CCE, pBit);
	if (pProgConfig->coupling.num_ele > CCHANS)
	{
		return -1;
	}
	if ((pProgConfig->mono_mix.present = AacDec_Bs_Read_Bits(LEN_MIX_PRES, pBit)) == 1)
		pProgConfig->mono_mix.ele_tag = AacDec_Bs_Read_Bits(LEN_TAG, pBit);
	if ((pProgConfig->stereo_mix.present = AacDec_Bs_Read_Bits(LEN_MIX_PRES, pBit)) == 1)
		pProgConfig->stereo_mix.ele_tag = AacDec_Bs_Read_Bits(LEN_TAG, pBit);
	if ((pProgConfig->matrix_mix.present = AacDec_Bs_Read_Bits(LEN_MIX_PRES, pBit)) == 1)
	{
		pProgConfig->matrix_mix.ele_tag = AacDec_Bs_Read_Bits(LEN_MMIX_IDX, pBit);
		pProgConfig->matrix_mix.pseudo_enab = AacDec_Bs_Read_Bits(LEN_PSUR_ENAB, pBit);
	}

	AacDec_Get_Ele_List(&pProgConfig->front, 1, pBit);
	AacDec_Get_Ele_List(&pProgConfig->side, 1, pBit);
	AacDec_Get_Ele_List(&pProgConfig->back, 1, pBit);
	AacDec_Get_Ele_List(&pProgConfig->lfe, 0, pBit);
	AacDec_Get_Ele_List(&pProgConfig->data, 0, pBit);
	AacDec_Get_Ele_List(&pProgConfig->coupling, 1, pBit);

	AacDec_Bs_Byte_Align(pBit);

	nBits = AacDec_Bs_Read_Bits(LEN_COMMENT_BYTES, pBit);

	for (nCount = 0; nCount < nBits; nCount++)
		pProgConfig->comments[0] = AacDec_Bs_Read_Bits(LEN_BYTE, pBit);
	/* null terminator for string */
	pProgConfig->comments[0] = 0;

	return 0;
}

static FRAME_INFO ParserADTS(AACD_Block_Params *params, OMX_U8 *pBuffer, OMX_U32 nBufferLen, FRAME_INFO *in_info)
{
	ADTS_Header pAdtsHeader;
	ADTS_Header *p = &(pAdtsHeader);
	FRAME_INFO ret;
#ifdef OLD_FORMAT_ADTS_HEADER
	OMX_S32 emphasis;
#endif
	BitstreamParamExt sBitstreamParamExt;
	BitstreamParamExt *pBit = &sBitstreamParamExt;
	AacDec_Bs_ReadInit(pBuffer, nBufferLen, pBit);
       fsl_osal_memset(&ret, 0, sizeof(FRAME_INFO));
       fsl_osal_memset(p, 0, sizeof(ADTS_Header));

	ret.index = nBufferLen;
	while (1)
	{
		/*
		 * If we have used up more than maximum possible frame for finding
		 * the ADTS header, then something is wrong, so exit.
		 */
		if (pBit->nBitTotal - pBit->nBitConsumered < ADTS_FRAME_HEADER_SIZE << 3)
		{
			ret.flags = FLAG_NEEDMORE_DATA;
			return ret;
		}

		/* ADTS header is assumed to be byte aligned */
		AacDec_Bs_Byte_Align(pBit);

#ifdef CRC_CHECK
		/* Add header bits to CRC check */
		UpdateCRCStructBegin();
#endif

		p->syncword = AacDec_Bs_Read_Bits ((LEN_SYNCWORD - LEN_BYTE), pBit);

		/* Search for syncword */
		while (p->syncword != ((1 << LEN_SYNCWORD) - 1))
		{
			p->syncword = ((p->syncword << LEN_BYTE) | AacDec_Bs_Read_Bits(LEN_BYTE, pBit));
			p->syncword &= (1 << LEN_SYNCWORD) - 1;
			/*
			 * If we have used up more than maximum possible frame for finding
			 * the ADTS header, then something is wrong, so exit.
			 */
			if (pBit->nBitTotal - pBit->nBitConsumered < ADTS_FRAME_HEADER_SIZE << 3)
			{
				ret.flags = FLAG_NEEDMORE_DATA;
				return ret;
			}
		}

		p->id = AacDec_Bs_Read_Bits(LEN_ID, pBit);
		p->layer = AacDec_Bs_Read_Bits(LEN_LAYER, pBit);
		if (p->layer != 0)
		{
			continue;
		}
		p->protection_abs = AacDec_Bs_Read_Bits(LEN_PROTECT_ABS, pBit);
		p->profile = AacDec_Bs_Read_Bits(LEN_PROFILE, pBit);
		if (p->profile != 1)
		{
			continue;
		}

		p->sampling_freq_idx = AacDec_Bs_Read_Bits(LEN_SAMP_IDX, pBit);
		if (p->sampling_freq_idx >= 0xc)
		{
			continue;
		}
		p->private_bit = AacDec_Bs_Read_Bits(LEN_PRIVTE_BIT, pBit);
		p->channel_config = AacDec_Bs_Read_Bits(LEN_CHANNEL_CONF, pBit);
		p->original_copy = AacDec_Bs_Read_Bits(LEN_ORIG, pBit);
		p->home = AacDec_Bs_Read_Bits(LEN_HOME, pBit);
#ifdef OLD_FORMAT_ADTS_HEADER
		params->Flush_LEN_EMPHASIS_Bits = 0;
		if (p->id == 0)
		{
			emphasis = AacDec_Bs_Read_Bits(LEN_EMPHASIS, pBit);
			nBitsUsed += LEN_EMPHASIS;
			params->Flush_LEN_EMPHASIS_Bits = 1;
		}
#endif
		p->copyright_id_bit = AacDec_Bs_Read_Bits(LEN_COPYRT_ID_ADTS, pBit);
		p->copyright_id_start = AacDec_Bs_Read_Bits(LEN_COPYRT_START, pBit);
		p->frame_length = AacDec_Bs_Read_Bits(LEN_FRAME_LEN, pBit);
		if (p->frame_length == 0)
			continue;
		p->adts_buffer_fullness = AacDec_Bs_Read_Bits(LEN_ADTS_BUF_FULLNESS, pBit);
		p->num_of_rdb = AacDec_Bs_Read_Bits(LEN_NUM_OF_RDB, pBit);
		/* If this point is reached, then the ADTS header has been found and
		 * CRC structure can be updated.
		 */
#ifdef CRC_CHECK
		/* Finish adding header bits to CRC check. All bits to be CRC
		 * protected.
		 */
		UpdateCRCStructEnd(0);
#endif
		if (p->protection_abs == 0)
		{
			p->crc_check = AacDec_Bs_Read_Bits(LEN_CRC, pBit);
		}

		/* Full header successfully obtained, so get out of the search */
		AacDec_Bs_Byte_Align(pBit);
		if (in_info == NULL)
		{
			if (pBit->nBitTotal - pBit->nBitConsumered < (OMX_U32)p->frame_length << 3 
					|| ((*((short *)(pBuffer + (pBit->nBitConsumered >> 3) + p->frame_length - ADTS_FRAME_HEADER_SIZE)) & 0xF0FF) != 0xF0FF ))
				continue;
		}
		break;
	}

#ifndef OLD_FORMAT_ADTS_HEADER
	AacDec_Bs_Byte_Align(pBit);
#else
	if (p->id != 0) // MPEG-2 style : Emphasis field is absent
		AacDec_Bs_Byte_Align(pBit);
	else //MPEG-4 style : Emphasis field is present; cancel its effect
		pBit->sBitstreamParam.bits_inheader -= LEN_EMPHASIS;
#endif

	/* Fill in the AACD_Block_Params struct now */
	params->num_pce = 0;
	params->ChannelConfig = p->channel_config;
	params->SamplingFreqIndex = p->sampling_freq_idx;
	params->BitstreamType = (p->adts_buffer_fullness == 0x7ff) ? 1 : 0;

	/* The ADTS stream contains the value of buffer fullness in units
	 * of 32-bit words. But the application is supposed to deliver this
	 * in units of bits. Hence we do a left-shift
	 */
	params->BufferFullness = (p->adts_buffer_fullness) << 5;
	params->ProtectionAbsent = p->protection_abs;
	params->CrcCheck = p->crc_check;
    params->frame_length = p->frame_length;

	ret.flags = FLAG_SUCCESS;
	ret.index = pBit->nBitConsumered >> 3;
	if(p->frame_length >= ADTS_FRAME_HEADER_SIZE)
        	ret.frm_size = p->frame_length - ADTS_FRAME_HEADER_SIZE;
       else
              ret.frm_size = 0;
	ret.sampling_rate = sampling_frequency[p->sampling_freq_idx];
	ret.sample_per_fr = AAC_FRAME_SIZE;
	ret.b_rate = (p->frame_length << 3) * ret.sampling_rate / ret.sample_per_fr / 1000;

	params->BitRate = ret.b_rate;

	return ret;
}

static FRAME_INFO ParserADIF(AACD_Block_Params *params, OMX_U8 *pBuffer, OMX_S32 nBufferLen)
{
	OMX_S32             nCount;
	OMX_S32             nBits;
	OMX_S32             nStatus;
	AACD_ProgConfig    *pConfig ;
	ADIF_Header     	pAdifHeader;
	ADIF_Header*    	p 	= &(pAdifHeader);
	FRAME_INFO ret;
	BitstreamParamExt sBitstreamParamExt;
	BitstreamParamExt *pBit = &sBitstreamParamExt;
	AacDec_Bs_ReadInit(pBuffer, nBufferLen, pBit);
       fsl_osal_memset(&ret, 0, sizeof(FRAME_INFO));

	ret.index = nBufferLen;

	if (pBit->nBitTotal - pBit->nBitConsumered < ADIF_FRAME_HEADER_SIZE << 3)
		return ret;

	/* adif header */
	for (nCount = 0; nCount < LEN_ADIF_ID; nCount++)
		p->adif_id[nCount] = AacDec_Bs_Read_Bits(LEN_BYTE, pBit);
	p->adif_id[nCount] = 0;  /* null terminated string */

	/* test for id */
	if (*((OMX_U32 *) p->adif_id) != *((OMX_U32 *) "ADIF"))
		return ret;  /* bad id */

	/* copyright string */
	if ((p->copy_id_present = AacDec_Bs_Read_Bits(LEN_COPYRT_PRES, pBit)) == 1)
	{
		for (nCount = 0; nCount < LEN_COPYRT_ID; nCount++)
			p->copy_id[nCount] = AacDec_Bs_Read_Bits(LEN_BYTE, pBit);

		/* null terminated string */
		p->copy_id[nCount] = 0;
	}
	p->original_copy = AacDec_Bs_Read_Bits(LEN_ORIG, pBit);
	p->home = AacDec_Bs_Read_Bits(LEN_HOME, pBit);
	p->bitstream_type = AacDec_Bs_Read_Bits(LEN_BS_TYPE, pBit);
	p->bitrate = AacDec_Bs_Read_Bits(LEN_BIT_RATE, pBit);

	/* program config elements */
	nStatus = -1;
	nBits = AacDec_Bs_Read_Bits(LEN_NUM_PCE, pBit) + 1;

	pConfig = (AACD_ProgConfig*)malloc(nBits*sizeof(AACD_ProgConfig));
	if(pConfig == NULL)
		return ret;

	for (nCount = 0; nCount < nBits; nCount++)
	{
		pConfig[nCount].buffer_fullness =
			(p->bitstream_type == 0) ? AacDec_Bs_Read_Bits(LEN_ADIF_BF, pBit) : 0;

		if (AacDec_Get_Prog_Config(&pConfig[nCount], pBit))
		{
			return ret;
		}

		nStatus = 1;
	}

	AacDec_Bs_Byte_Align(pBit);

	/* Fill in the AACD_Block_Params struct now */

	params->num_pce = nBits;
	params->pce     = pConfig;
	params->BitstreamType = p->bitstream_type;
	params->BitRate       = p->bitrate;
	params->ProtectionAbsent = 0;

	ret.flags = FLAG_SUCCESS;
	ret.index = pBit->nBitConsumered >> 3;

    return ret;
}

static AAC_RETURNTYPE AacDec_InitRaw(AACD_Block_Params * params,OMX_S32 nChannels,OMX_S32 nSamplingFreq)
{
    ADTS_Header App_adts_header;
    ADTS_Header *pHeader = &(App_adts_header);
    fsl_osal_memset(&App_adts_header, 0, sizeof(ADTS_Header));
    OMX_S32 bits_used = 0;
	OMX_S32 sampling_freq_idx;

	switch( nSamplingFreq )
	{
		case 96000:sampling_freq_idx = 0;break;
		case 88200:sampling_freq_idx = 1;break;
		case 64000:sampling_freq_idx = 2;break;
		case 48000:sampling_freq_idx = 3;break;
		case 44100:sampling_freq_idx = 4;break;
		case 32000:sampling_freq_idx = 5;break;
		case 24000:sampling_freq_idx = 6;break;
		case 22050:sampling_freq_idx = 7;break;
		case 16000:sampling_freq_idx = 8;break;
		case 12000:sampling_freq_idx = 9;break;
		case 11025:sampling_freq_idx = 10;break;
		case  8000:sampling_freq_idx = 11;break;

		default:sampling_freq_idx = -1;break;
	}

	pHeader->id = 0;
	pHeader->layer = 0;
	pHeader->protection_abs = 1;
	pHeader->profile = 1;
	pHeader->sampling_freq_idx = sampling_freq_idx;
	if (pHeader->sampling_freq_idx == -1)
	{
         return AAC_RETURNFAIL;
	}

	pHeader->private_bit = 0;
    pHeader->channel_config = (unsigned)nChannels;
	pHeader->original_copy = 0;
	pHeader->home = 0;
	pHeader->copyright_id_bit = 0;
	pHeader->copyright_id_start = 0;
	pHeader->frame_length = 0;
	pHeader->adts_buffer_fullness = 0;
	pHeader->num_of_rdb = 0;
    pHeader->frame_length += (bits_used / LEN_BYTE) - ADTS_FRAME_HEADER_SIZE;

    /* Fill in the AACD_Block_Params struct now */

    params->num_pce = 0;
    params->ChannelConfig = pHeader->channel_config;
    params->SamplingFreqIndex = pHeader->sampling_freq_idx;
    params->BitstreamType = (pHeader->adts_buffer_fullness == 0x7ff) ? 1 : 0;

    params->BitRate       = 0; /* Caution !*/
    params->BufferFullness = (pHeader->adts_buffer_fullness) << 5;
    params->ProtectionAbsent = pHeader->protection_abs;
    params->CrcCheck = pHeader->crc_check;

    return AAC_RETURNSUCESS;
}

static AAC_FILETYPE Aac_FindFileType(OMX_U8 *pBuffer, OMX_S32 nBufferLen)
{
	OMX_U32	nVal = *pBuffer<<24|*(pBuffer+1)<<16|*(pBuffer+2)<<8|*(pBuffer+3);

	if (nVal == ADIF_FILE)
	{
		return FILETYPEADIF;
	}
	else
	{
		if ((nVal & 0xFFF00000) == ADTS_FILE)
			return FILETYPEADTS;
		else
		{
			FRAME_INFO FrameInfo;
			AACD_Block_Params params;
			fsl_osal_memset(&FrameInfo, 0, sizeof(FRAME_INFO));
			FrameInfo = ParserADTS(&params, pBuffer, nBufferLen, NULL);

			if (FrameInfo.flags == FLAG_NEEDMORE_DATA)
			{
				return FILETYPERAW;
			}
			if (FrameInfo.index >= (OMX_U32)nBufferLen)
			{
				return FILETYPERAW;
			}

			return FILETYPEADTS;
		}
	}
}

AacDec::AacDec()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_decoder.aac.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_decoder.aac";
    bInContext = OMX_FALSE;
    nPorts = AUDIO_FILTER_PORT_NUMBER;
	nPushModeInputLen = AACD_INPUT_BUF_PUSH_SIZE;
	nRingBufferScale = RING_BUFFER_SCALE;
}

OMX_ERRORTYPE AacDec::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = AUDIO_FILTER_INPUT_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingAAC;
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
    sPortDef.nBufferSize = CHANS*AAC_FRAME_SIZE*2*4;
	ret = ports[AUDIO_FILTER_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
	if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
		return ret;
	}

	OMX_INIT_STRUCT(&AacType, OMX_AUDIO_PARAM_AACPROFILETYPE);
	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	AacType.nPortIndex = AUDIO_FILTER_INPUT_PORT;
	AacType.nAudioBandWidth = 0;
	AacType.nAACERtools = OMX_AUDIO_AACERNone;
	AacType.eAACProfile = OMX_AUDIO_AACObjectLC;
	AacType.eChannelMode = OMX_AUDIO_ChannelModeStereo;
	AacType.nSampleRate = 44100;
	AacType.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMax;

	nChannelsRaw = 2;
	nSampleRateRaw = 44100;

	PcmMode.nPortIndex = AUDIO_FILTER_OUTPUT_PORT;
	PcmMode.nChannels = 2;
	PcmMode.nSamplingRate = 44100;
	PcmMode.nBitPerSample = 16;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	TS_PerFrame = AAC_FRAME_SIZE*OMX_TICKS_PER_SECOND/AacType.nSampleRate;
	bStreamHeader = OMX_TRUE;
	bGotFirstFrame = OMX_FALSE;
	bHaveAACPlusLib = OMX_FALSE;
	SbrTypeFlag = NO_SBR_FRAME;

	libMgr = NULL;
	hLib = NULL;
	return ret;
}

OMX_ERRORTYPE AacDec::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AacDec::AudioFilterInstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    libMgr = FSL_NEW(ShareLibarayMgr, ());
    if(libMgr == NULL)
        return OMX_ErrorInsufficientResources;

    hLib = libMgr->load((OMX_STRING)"lib_aacplus_dec_v2_arm11_elinux.so");
    if(hLib == NULL) {
		LOG_WARNING("Can't load library lib_aacplus_dec_v2_arm11_elinux.\n");
        FSL_DELETE(libMgr);
    }

	if (hLib) {
		aacpd_query_dec_mem = (AACD_RET_TYPE (*)( AACD_Decoder_Config *))libMgr->getSymbol(hLib, (OMX_STRING)"aacpd_query_dec_mem");
		SBRD_decoder_init = (AACD_RET_TYPE (*)(AACD_Decoder_Config *))libMgr->getSymbol(hLib, (OMX_STRING)"SBRD_decoder_init");
		SBRD_decode_frame = (AACD_RET_TYPE (*)(
					AACD_Decoder_Config *,
					AACD_Decoder_info *,
					AACD_INT32 *,
					SBR_FRAME_TYPE *))libMgr->getSymbol(hLib,(OMX_STRING)"SBRD_decode_frame");
		if(aacpd_query_dec_mem == NULL
				|| SBRD_decoder_init == NULL
				|| SBRD_decode_frame == NULL) {
			bHaveAACPlusLib = OMX_FALSE;
			libMgr->unload(hLib);
			FSL_DELETE(libMgr);
		} else
			bHaveAACPlusLib = OMX_TRUE;

	}

	pAacDecConfig = (AACD_Decoder_Config *)FSL_MALLOC(sizeof(AACD_Decoder_Config));
	if (pAacDecConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pAacDecConfig, 0, sizeof(AACD_Decoder_Config));

	pAacDecInfo = (AACD_Decoder_info *)FSL_MALLOC(sizeof(AACD_Decoder_info));
	if (pAacDecInfo == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pAacDecInfo, 0, sizeof(AACD_Decoder_info));

	// initialization for SBR decoder
	pAacDecConfig->sbrd_dec_config.sbrd_down_sample = 0;
	pAacDecConfig->sbrd_dec_config.sbrd_stereo_downmix = 0;

	AACD_RET_TYPE AacRet = AACD_ERROR_NO_ERROR;
	if (bHaveAACPlusLib == OMX_TRUE)
		AacRet = aacpd_query_dec_mem(pAacDecConfig);
	else
		AacRet = aacd_query_dec_mem(pAacDecConfig);

	if (AacRet != AACD_ERROR_NO_ERROR)
	{
		LOG_ERROR("Aac decoder query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	OMX_S32 MemoryCnt = pAacDecConfig->aacd_mem_info.aacd_num_reqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pAacDecConfig->aacd_mem_info.mem_info_sub[i].aacd_size;
		if (BufferSize == 0)
			BufferSize = 1;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		pAacDecConfig->aacd_mem_info.mem_info_sub[i].app_base_ptr = pBuffer;
	}

	pAacDecConfig->params = (AACD_Block_Params *)FSL_MALLOC(sizeof(AACD_Block_Params));
	if(pAacDecConfig->params == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}

	fsl_osal_memset(pAacDecConfig->params, 0, sizeof(AACD_Block_Params));

	pAacDecConfig->num_pcm_bits = AACD_16_BIT_OUTPUT;

    return ret;
}

OMX_ERRORTYPE AacDec::AudioFilterCodecInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	AACD_RET_TYPE AacRet = AACD_ERROR_NO_ERROR;

	if (nSampleRateRaw == 0 && nChannelsRaw == 0)
	{
		return OMX_ErrorUndefined;
	}

	AacRet = aacd_decoder_init(pAacDecConfig);
	if (AacRet != AACD_ERROR_NO_ERROR)
	{
		LOG_ERROR("Aac decoder initialize fail.\n");
		return OMX_ErrorUndefined;
	}

	if (bHaveAACPlusLib == OMX_TRUE) {
		AacRet = SBRD_decoder_init(pAacDecConfig);
		if (AacRet != AACD_ERROR_NO_ERROR)
		{
			LOG_ERROR("SBR decoder initialize fail.\n");
			return OMX_ErrorUndefined;
		}
	}

	AacType.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMax;
	bStreamHeader = OMX_TRUE;

    return ret;
}

OMX_ERRORTYPE AacDec::AudioFilterInstanceDeInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (AacType.eAACStreamFormat == OMX_AUDIO_AACStreamFormatADIF)
	{
		FSL_FREE(pAacDecConfig->params->pce);
	}

	OMX_S32 MemoryCnt = pAacDecConfig->aacd_mem_info.aacd_num_reqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		FSL_FREE(pAacDecConfig->aacd_mem_info.mem_info_sub[i].app_base_ptr);
	}

	FSL_FREE(pAacDecConfig->params);
	FSL_FREE(pAacDecConfig);
	FSL_FREE(pAacDecInfo);

	if (libMgr) {
		if (hLib)
			libMgr->unload(hLib);
		FSL_DELETE(libMgr);
	}

    return ret;
}

OMX_ERRORTYPE AacDec::AudioFilterGetParameter(
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
				if (pAacType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
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

OMX_ERRORTYPE AacDec::AudioFilterSetParameter(
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
				if (pAacType->nPortIndex != AUDIO_FILTER_INPUT_PORT)
				{
					ret = OMX_ErrorBadPortIndex;
					break;
				}
				nChannelsRaw = pAacType->nChannels;
				nSampleRateRaw = pAacType->nSampleRate;

				LOG_DEBUG("Set AAC para type. Channel: %d, Sample Rate: %d\n", nChannelsRaw, nSampleRateRaw);
				LOG_DEBUG("AacType.eAACStreamFormat: %d\n", AacType.eAACStreamFormat);
			}
			return ret;
		default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}
 
AUDIO_FILTERRETURNTYPE AacDec::AudioFilterFrame()
{
    AUDIO_FILTERRETURNTYPE ret = AUDIO_FILTER_SUCCESS;
	OMX_U8 *pBuffer;
	OMX_U32 nActuralLen;

	AudioRingBuffer.BufferGet(&pBuffer, AACD_INPUT_BUF_PUSH_SIZE, &nActuralLen);
	LOG_LOG("Get data = %d\n", nActuralLen);
  
    OMX_ERRORTYPE retOMX = OMX_ErrorNone;
	retOMX = CheckFormatType(pBuffer, nActuralLen);
	if (retOMX != OMX_ErrorNone)
	{
		LOG_WARNING("AAC format check fail.\n");
		if (retOMX == OMX_ErrorStreamCorrupt)
		{
			AudioRingBuffer.BufferReset();
			bReceivedEOS = OMX_TRUE;
			return AUDIO_FILTER_EOS;
		}

		if (nActuralLen > ADTS_FRAME_HEADER_SIZE)
		{
			AudioRingBuffer.BufferConsumered(nActuralLen - ADTS_FRAME_HEADER_SIZE);
			return AUDIO_FILTER_FAILURE;
		}
		else
		{
			AudioRingBuffer.BufferConsumered(nActuralLen);
			LOG_DEBUG("AAC decoder return EOS.\n");
			return AUDIO_FILTER_EOS;
		}
	}

	/* AAC RAW format should input one frame by one frame */
	if (AacType.eAACStreamFormat == OMX_AUDIO_AACStreamFormatRAW && nActuralLen != 0)
	{
		OMX_U32 nFrameLen;

		nFrameLen = AudioRingBuffer.GetFrameLen();
		if (nFrameLen == 0)
			nFrameLen = nActuralLen;

		AudioRingBuffer.BufferGet(&pBuffer, nFrameLen, &nActuralLen);
		LOG_LOG("Get data = %d\n", nActuralLen);
	}

	AACD_RET_TYPE AacRet = AACD_ERROR_NO_ERROR;
    AACD_RET_TYPE SbrRet = AACD_ERROR_NO_ERROR;
	SBR_FRAME_TYPE SbrFrameType = NO_SBR_FRAME;

	/* Walkround for ADIF can't return EOS */
	if ((nActuralLen == 0) \
			&& (AacType.eAACStreamFormat == OMX_AUDIO_AACStreamFormatADIF \
				|| AacType.eAACStreamFormat == OMX_AUDIO_AACStreamFormatRAW))
	{
		AacRet = AACD_ERROR_EOF;
	}
	else
	{
		AacRet = aacd_decode_frame(pAacDecConfig, pAacDecInfo, (AACD_OutputFmtType *)pOutBufferHdr->pBuffer, (AACD_INT8 *)(pBuffer + HeadOffset), (nActuralLen - HeadOffset));

		if (bHaveAACPlusLib == OMX_TRUE) {
			SbrRet = SBRD_decode_frame(pAacDecConfig, pAacDecInfo, (AACD_INT32 *)pOutBufferHdr->pBuffer, &SbrFrameType);
		}

		OMX_U32 nComsumeredLen = HeadOffset + pAacDecInfo->BitsInBlock/8;
		if (nComsumeredLen < 2)
			nComsumeredLen = nActuralLen;

		if (AacType.eAACStreamFormat == OMX_AUDIO_AACStreamFormatRAW && AacRet != AACD_ERROR_NO_ERROR)
		    nComsumeredLen = nActuralLen;

		LOG_LOG("Consumered data = %d\n", nComsumeredLen);

		if (nComsumeredLen > nActuralLen)
		{
			AudioRingBuffer.BufferConsumered(nActuralLen);
		}
		else
		{
			AudioRingBuffer.BufferConsumered(nComsumeredLen);
			LOG_DEBUG("Header Consumered = %d\n", HeadOffset);
			LOG_LOG("Consumered data2 = %d\n", nComsumeredLen);
		}
	}

	bAlwaysStereo = OMX_FALSE;
	if (bHaveAACPlusLib == OMX_TRUE) {
		if (SbrFrameType!=NO_SBR_FRAME && SbrRet!=AACD_ERROR_NO_ERROR) {
			AacRet = AACD_ERROR_INVALID_HEADER;
			LOG_ERROR("SBR decode fail.\n");
		}
		if (SbrFrameType == SBR_FRAME) {
			LOG_LOG("Is AAC Plus stream.\n");
			if (pAacDecInfo->aacd_num_channels == 1) {
				pAacDecInfo->aacd_num_channels = 2;
				bAlwaysStereo = OMX_TRUE;
			}
		}
	}

	LOG_LOG("AacRet = %d\n", AacRet);
	if (AacRet == AACD_ERROR_NO_ERROR)
	{
		AacType.nBitRate = pAacDecInfo->aacd_bit_rate*1000;
		if (AacType.nChannels != (OMX_U16)pAacDecInfo->aacd_num_channels
				|| AacType.nSampleRate != (OMX_U32)pAacDecInfo->aacd_sampling_frequency
				|| SbrTypeFlag != SbrFrameType)
		{
			SbrTypeFlag = SbrFrameType;
			LOG_DEBUG("AAC decoder channel: %d sample rate: %d\n", \
					pAacDecInfo->aacd_num_channels, pAacDecInfo->aacd_sampling_frequency);
			if (pAacDecInfo->aacd_num_channels == 0 || pAacDecInfo->aacd_sampling_frequency == 0)
			{
				return AUDIO_FILTER_FAILURE;
			}
			AacType.nChannels = pAacDecInfo->aacd_num_channels;
			AacType.nSampleRate = pAacDecInfo->aacd_sampling_frequency;
			PcmMode.nChannels = pAacDecInfo->aacd_num_channels;
			PcmMode.nSamplingRate = pAacDecInfo->aacd_sampling_frequency;
			LOG_DEBUG("Aac decoder samplerate: %d\n", PcmMode.nSamplingRate);
			TS_PerFrame = pAacDecInfo->aacd_len*OMX_TICKS_PER_SECOND/AacType.nSampleRate;
			SendEvent(OMX_EventPortSettingsChanged, AUDIO_FILTER_OUTPUT_PORT, 0, NULL);
		}

		OMX_S32 *pS32 = (OMX_S32 *)pOutBufferHdr->pBuffer;
		OMX_S16 *pS16 = (OMX_S16 *)pOutBufferHdr->pBuffer;
		if (bAlwaysStereo == OMX_FALSE) {
			for (OMX_U32 i = 0; i < pAacDecInfo->aacd_len * AacType.nChannels; i ++) {
				*pS16++ = *pS32++;
			}
#ifdef OMX_STEREO_OUTPUT
			if (AacType.nChannels > 2) {
				OMX_S16 *pS16_src = (OMX_S16 *)pOutBufferHdr->pBuffer;
				OMX_S16 *pS16_dst = (OMX_S16 *)pOutBufferHdr->pBuffer;
				pS16_dst ++;
				for (OMX_U32 i = 0; i < pAacDecInfo->aacd_len; i ++) {
					*pS16_dst = *pS16_src;
					pS16_src += AacType.nChannels;
					pS16_dst += AacType.nChannels;
				}
			}
#endif
		}
		else {
			for (OMX_U32 i = 0; i < pAacDecInfo->aacd_len; i ++) {
				*pS16++ = *pS32;
				*pS16++ = *pS32++;
			}
		}
		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = pAacDecInfo->aacd_len * AacType.nChannels*2;
 
		LOG_LOG("TS_PerFrame: %lld\n", TS_PerFrame);
		AudioRingBuffer.TS_SetIncrease(TS_PerFrame); 
	}
	else if (AacRet == AACD_ERROR_EOF)
	{
		pOutBufferHdr->nOffset = 0;
		pOutBufferHdr->nFilledLen = 0;

		ret = AUDIO_FILTER_EOS;
	}
	else
	{
		ret = AUDIO_FILTER_FAILURE;
	}

	LOG_LOG("Decoder output TS: %lld\n", pOutBufferHdr->nTimeStamp);

	return ret;
}

OMX_ERRORTYPE AacDec::CheckFormatType(OMX_U8 *pBuffer, OMX_U32 nBufferLen)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (!(AacType.eAACProfile == OMX_AUDIO_AACObjectLC
	    || AacType.eAACProfile == OMX_AUDIO_AACObjectHE))
	{
		LOG_ERROR("Unimplement AAC profile type: %d\n", AacType.eAACProfile);
		return OMX_ErrorNotImplemented;
	}

	if (AacType.eAACStreamFormat == OMX_AUDIO_AACStreamFormatMax)
	{
		AAC_FILETYPE FileType;
		FileType = Aac_FindFileType(pBuffer, nBufferLen);
		LOG_DEBUG("AAC format: %d\n", FileType);
		if (FileType == FILETYPEADTS)
		{
			AacType.eAACStreamFormat = OMX_AUDIO_AACStreamFormatMP2ADTS;
		}
		else if (FileType == FILETYPEADIF)
		{
			AacType.eAACStreamFormat = OMX_AUDIO_AACStreamFormatADIF;
		}
		else
		{
			AacType.eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;
		}
	}

	HeadOffset = 0;
	if (AacType.eAACStreamFormat == OMX_AUDIO_AACStreamFormatMP2ADTS
			|| AacType.eAACStreamFormat == OMX_AUDIO_AACStreamFormatMP4ADTS)
	{
		LOG_DEBUG("AAC ADTS format.\n");
		if (bGotFirstFrame == OMX_FALSE) 
		{
			FrameInfo = ParserADTS(pAacDecConfig->params, pBuffer, nBufferLen, NULL);
			if (FrameInfo.flags == FLAG_NEEDMORE_DATA
					||FrameInfo.index >= nBufferLen)
			{
				LOG_DEBUG("ADTS parser error.\n");
				return OMX_ErrorStreamCorrupt;
			}

			HeadOffset = FrameInfo.index;
			bGotFirstFrame = OMX_TRUE;
		}
		else 
		{

			FRAME_INFO FrameInfoTmp;
			FrameInfoTmp = ParserADTS(pAacDecConfig->params, pBuffer, nBufferLen, &FrameInfo);
			if (FrameInfoTmp.flags == FLAG_NEEDMORE_DATA
					||FrameInfoTmp.index >= nBufferLen)
			{
				LOG_DEBUG("ADTS parser error.\n");
				return OMX_ErrorStreamCorrupt;
			}

			HeadOffset = FrameInfoTmp.index;
		}

	}
	else if (AacType.eAACStreamFormat == OMX_AUDIO_AACStreamFormatADIF)
	{
		if (bStreamHeader == OMX_TRUE)
		{
			LOG_DEBUG("AAC ADIF format.\n");
			FRAME_INFO FrameInfoTmp;
			FrameInfoTmp = ParserADIF(pAacDecConfig->params, pBuffer, nBufferLen);
			if (FrameInfoTmp.flags == FLAG_NEEDMORE_DATA
					||FrameInfoTmp.index >= nBufferLen)
			{
				LOG_DEBUG("ADTS parser error.\n");
				return OMX_ErrorUndefined;
			}

			HeadOffset = FrameInfoTmp.index;
			bStreamHeader = OMX_FALSE;
		}
	}
	else if (AacType.eAACStreamFormat == OMX_AUDIO_AACStreamFormatRAW)
	{
		if (bStreamHeader == OMX_TRUE)
		{
			LOG_DEBUG("AAC RAW format.\n");
			LOG_DEBUG("Get AAC raw init. Channel: %d, Sample Rate: %d\n", nChannelsRaw, nSampleRateRaw);
			AacDec_InitRaw(pAacDecConfig->params,nChannelsRaw,nSampleRateRaw);

			bStreamHeader = OMX_FALSE;
		}
	}
	else
	{
		LOG_ERROR("Unimplement AAC format type: %d\n", AacType.eAACStreamFormat);
		return OMX_ErrorNotImplemented;
	}

    return ret;
}

OMX_ERRORTYPE AacDec::AudioFilterReset()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	AACD_RET_TYPE AacRet = AACD_ERROR_NO_ERROR;

	OMX_S32 MemoryCnt = pAacDecConfig->aacd_mem_info.aacd_num_reqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pAacDecConfig->aacd_mem_info.mem_info_sub[i].aacd_size;
		if (BufferSize == 0)
			BufferSize = 1;
		pBuffer = (OMX_U8 *)pAacDecConfig->aacd_mem_info.mem_info_sub[i].app_base_ptr;
		fsl_osal_memset(pBuffer, 0, BufferSize);
	}

	AacRet = aacd_decoder_init(pAacDecConfig);
	if (AacRet != AACD_ERROR_NO_ERROR)
	{
		LOG_ERROR("Aac decoder initialize fail.\n");
		return OMX_ErrorUndefined;
	}

	if (bHaveAACPlusLib == OMX_TRUE) {
		AacRet = SBRD_decoder_init(pAacDecConfig);
		if (AacRet != AACD_ERROR_NO_ERROR)
		{
			LOG_ERROR("SBR decoder initialize fail.\n");
			return OMX_ErrorUndefined;
		}
	}

    return ret;
}

OMX_ERRORTYPE AacDec::AudioFilterCheckCodecConfig()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_AACPROFILETYPE *pAacType;
	pAacType = (OMX_AUDIO_PARAM_AACPROFILETYPE*)pInBufferHdr->pBuffer;
	OMX_CHECK_STRUCT(pAacType, OMX_AUDIO_PARAM_AACPROFILETYPE, ret);
	if(ret != OMX_ErrorNone)
	{
		BitstreamParamExt sBitstreamParamExt;
		BitstreamParamExt *pBit = &sBitstreamParamExt;
		AacDec_Bs_ReadInit(pInBufferHdr->pBuffer, pInBufferHdr->nFilledLen+MIN_REQD_BITS, pBit);

		OMX_S32 audioObjectType = AacDec_Bs_Read_Bits(5, pBit);
		LOG_DEBUG("pInBufferHdr->nFilledLen: %d\n", pInBufferHdr->nFilledLen);
		LOG_DEBUG("audioObjectType: %d\n", audioObjectType);

		if (audioObjectType == 5 || audioObjectType == 29)
			AacType.eAACProfile = OMX_AUDIO_AACObjectHE;
		if (audioObjectType == 2 || audioObjectType == 5 || audioObjectType == 29)
		{
			OMX_S32 samplingFrequencyIndex = AacDec_Bs_Read_Bits(4, pBit);
			nSampleRateRaw = samplingFrequencyMap[samplingFrequencyIndex];
			if (samplingFrequencyIndex == 0xF)
				nSampleRateRaw = AacDec_Bs_Read_Bits(24, pBit);
		}

		pInBufferHdr->nFilledLen = 0;
		return ret;
	}

	nChannelsRaw = pAacType->nChannels;
	nSampleRateRaw = pAacType->nSampleRate;
	LOG_DEBUG("Get AAC decoder config. Channel: %d, Sample Rate: %d\n", nChannelsRaw, nSampleRateRaw);

	pInBufferHdr->nFilledLen = 0;

    return ret;
}
 
/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE AacDecInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        AacDec *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(AacDec, ());
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
