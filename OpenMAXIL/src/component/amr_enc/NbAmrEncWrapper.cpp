/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Audio.h"

#include "AmrEncWrapper.h"
#include "Mem.h"
#include "Log.h"


OMX_ERRORTYPE NbAmrEncWrapper::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 i;

	pAmrEncConfig = (sAMREEncoderConfigType *)FSL_MALLOC(sizeof(sAMREEncoderConfigType));
	if (pAmrEncConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pAmrEncConfig, 0, sizeof(sAMREEncoderConfigType));

	/* allocate memory for encoder to use */
	pAmrEncConfig->pvAMREEncodeInfoPtr = NULL;
	/* Not used */
	pAmrEncConfig->pu8APPEInitializedDataStart = NULL;

	/* Set DTX flag */
	pAmrEncConfig->s16APPEDtxFlag = NBAMR_NONDTX;

	pAmrEncConfig->u8NumFrameToEncode= ENCORD_FRAME;    /* number of frame to be encoded */

	/* encoded data size */
	pAmrEncConfig->pu32AMREPackedSize = (NBAMR_U32 *)FSL_MALLOC(ENCORD_FRAME*sizeof(NBAMR_U32));

	/* user requested mode */
	pAmrEncConfig->pps8APPEModeStr = (NBAMR_S8**)FSL_MALLOC(ENCORD_FRAME * sizeof(NBAMR_S8 *));

	/* used mode by encoder */
	pAmrEncConfig->pps8AMREUsedModeStr = (NBAMR_S8**)FSL_MALLOC(ENCORD_FRAME * sizeof(NBAMR_S8*));

	/* initialize packed data variable for all the frames */
	for (i=0; i<ENCORD_FRAME; i++)
	{
		*(pAmrEncConfig->pu32AMREPackedSize+i) = 0;
	}

	eAMREReturnType AmrRet;
	AmrRet = eAMREQueryMem(pAmrEncConfig);
	if (AmrRet != E_NBAMRE_OK)
	{
		LOG_ERROR("Amr encoder query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	OMX_S32 MemoryCnt = pAmrEncConfig->sAMREMemInfo.s32AMRENumMemReqs;
	for (i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pAmrEncConfig->sAMREMemInfo.asMemInfoSub[i].s32AMRESize;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		pAmrEncConfig->sAMREMemInfo.asMemInfoSub[i].pvAPPEBasePtr = pBuffer;
	}

	return ret;

}

OMX_ERRORTYPE NbAmrEncWrapper::CodecInit(OMX_AUDIO_PARAM_AMRTYPE *pAmrType)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 i;

    switch(pAmrType->eAMRFrameFormat) 
	{
        case OMX_AUDIO_AMRFrameFormatConformance:
			pAmrEncConfig->u8BitStreamFormat = NBAMR_ETSI;
            break;
        case OMX_AUDIO_AMRFrameFormatFSF:
			LOG_DEBUG("Amr format MMSIO.\n");
			pAmrEncConfig->u8BitStreamFormat = NBAMR_MMSIO;
            break;
        case OMX_AUDIO_AMRFrameFormatIF1:
			pAmrEncConfig->u8BitStreamFormat = NBAMR_IF1IO;
            break;
        case OMX_AUDIO_AMRFrameFormatIF2:
			pAmrEncConfig->u8BitStreamFormat = NBAMR_IF2IO;
            break;
        default:
            return OMX_ErrorBadParameter;
    }

    switch(pAmrType->eAMRBandMode) 
	{
		case OMX_AUDIO_AMRBandModeUnused:
			for (i =0; i<ENCORD_FRAME; i++)
			{
				/* set user requested encoding mode */
				pAmrEncConfig->pps8APPEModeStr[i] = (NBAMR_S8*)("MR122");
			}
            break;
		case OMX_AUDIO_AMRBandModeNB0:
			for (i =0; i<ENCORD_FRAME; i++)
			{
				/* set user requested encoding mode */
				pAmrEncConfig->pps8APPEModeStr[i] = (NBAMR_S8*)("MR475");
			}
            break;
		case OMX_AUDIO_AMRBandModeNB1:
			for (i =0; i<ENCORD_FRAME; i++)
			{
				/* set user requested encoding mode */
				pAmrEncConfig->pps8APPEModeStr[i] = (NBAMR_S8*)("MR515");
			}
            break;
		case OMX_AUDIO_AMRBandModeNB2:
			for (i =0; i<ENCORD_FRAME; i++)
			{
				/* set user requested encoding mode */
				pAmrEncConfig->pps8APPEModeStr[i] = (NBAMR_S8*)("MR59");
			}
            break;
		case OMX_AUDIO_AMRBandModeNB3:
			for (i =0; i<ENCORD_FRAME; i++)
			{
				/* set user requested encoding mode */
				pAmrEncConfig->pps8APPEModeStr[i] = (NBAMR_S8*)("MR67");
			}
            break;
		case OMX_AUDIO_AMRBandModeNB4:
			for (i =0; i<ENCORD_FRAME; i++)
			{
				/* set user requested encoding mode */
				pAmrEncConfig->pps8APPEModeStr[i] = (NBAMR_S8*)("MR74");
			}
            break;
		case OMX_AUDIO_AMRBandModeNB5:
			for (i =0; i<ENCORD_FRAME; i++)
			{
				/* set user requested encoding mode */
				pAmrEncConfig->pps8APPEModeStr[i] = (NBAMR_S8*)("MR795");
			}
            break;
		case OMX_AUDIO_AMRBandModeNB6:
			for (i =0; i<ENCORD_FRAME; i++)
			{
				/* set user requested encoding mode */
				pAmrEncConfig->pps8APPEModeStr[i] = (NBAMR_S8*)("MR102");
			}
            break;
		case OMX_AUDIO_AMRBandModeNB7:
			for (i =0; i<ENCORD_FRAME; i++)
			{
				/* set user requested encoding mode */
				pAmrEncConfig->pps8APPEModeStr[i] = (NBAMR_S8*)("MR122");
			}
            break;
        default:
            return OMX_ErrorBadParameter;
    }

	eAMREReturnType AmrRet;
	AmrRet = eAMREEncodeInit(pAmrEncConfig);
	if (AmrRet != E_NBAMRE_OK)
	{
		LOG_ERROR("Amr encoder initialize fail. return: %d\n", AmrRet);
		return OMX_ErrorUndefined;
	}

    return ret;
}

OMX_ERRORTYPE NbAmrEncWrapper::InstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 MemoryCnt = pAmrEncConfig->sAMREMemInfo.s32AMRENumMemReqs;
	OMX_S32 i;

	for (i = 0; i < MemoryCnt; i ++)
	{
		FSL_FREE(pAmrEncConfig->sAMREMemInfo.asMemInfoSub[i].pvAPPEBasePtr);
	}

	FSL_FREE(pAmrEncConfig->pps8APPEModeStr);

	FSL_FREE(pAmrEncConfig->pu32AMREPackedSize);

	FSL_FREE(pAmrEncConfig->pps8APPEModeStr);

	FSL_FREE(pAmrEncConfig->pps8AMREUsedModeStr);

	FSL_FREE(pAmrEncConfig);

    return ret;
}

OMX_U32 NbAmrEncWrapper::getInputBufferPushSize()
{
    return NB_INPUT_BUF_PUSH_SIZE;
}

OMX_ERRORTYPE NbAmrEncWrapper::encodeFrame(
                    OMX_U8 *pOutBuffer, 
                    OMX_U32 *pOutputDataSize,
                    OMX_U8 *pInputBuffer, 
                    OMX_U32 *pInputDataSize)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U8 *pSrc = pOutBuffer;
	OMX_U8 *pDst = pOutBuffer;

	eAMREEncodeFrame(pAmrEncConfig, (NBAMR_S16*)pInputBuffer, (NBAMR_S16*)pOutBuffer);

	*pOutputDataSize = 0;
	for (OMX_U32 i = 0; i < pAmrEncConfig->u8NumFrameToEncode; i ++) {
		pSrc = pOutBuffer + i*((NBAMR_MAX_PACKED_SIZE/2)+(NBAMR_MAX_PACKED_SIZE%2))*2;
		OMX_U8 Header;
		OMX_U8 u8Mode;
		Header = pSrc[0];
		u8Mode = (NBAMR_U8) (0x0F & (Header >> 3));
		LOG_DEBUG("u8Mode: %d\n", u8Mode);

		for (OMX_U32 j = 0; j < pAmrEncConfig->pu32AMREPackedSize[i]; j ++) {
			*pDst++ = *pSrc++;
		}
		LOG_LOG("Stream frame len: %d\n", pAmrEncConfig->pu32AMREPackedSize[i]);
		*pOutputDataSize += pAmrEncConfig->pu32AMREPackedSize[i];
	}
    *pInputDataSize = (ENCORD_FRAME * L_FRAME) * sizeof (NBAMR_S16);

    return ret;

}

OMX_TICKS NbAmrEncWrapper::getTsPerFrame(OMX_U32 sampleRate)
{
    if(sampleRate > 0)
        return (ENCORD_FRAME * L_FRAME) *OMX_TICKS_PER_SECOND/sampleRate;
    else
        return 0;
}


