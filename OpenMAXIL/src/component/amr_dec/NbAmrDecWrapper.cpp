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

#include "AmrDecWrapper.h"
#include "Mem.h"
#include "Log.h"


OMX_ERRORTYPE NbAmrDecWrapper::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	pNbAmrDecConfig = (sAMRDDecoderConfigType *)FSL_MALLOC(sizeof(sAMRDDecoderConfigType));
	if (pNbAmrDecConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pNbAmrDecConfig , 0, sizeof(sAMRDDecoderConfigType));

	eAMRDReturnType AmrRet;
	AmrRet = eAMRDQueryMem(pNbAmrDecConfig );
	if (AmrRet != E_NBAMRD_OK)
	{
		LOG_ERROR("Amr decoder query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	OMX_S32 MemoryCnt = pNbAmrDecConfig ->sAMRDMemInfo.s32AMRDNumMemReqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pNbAmrDecConfig ->sAMRDMemInfo.asMemInfoSub[i].s32AMRDSize;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		pNbAmrDecConfig ->sAMRDMemInfo.asMemInfoSub[i].pvAPPDBasePtr = pBuffer;
	}

	OMX_U32 BufferSize;
	OMX_U8 *pBuffer;

	BufferSize = FRAMESPERLOOP*SERIAL_FRAMESIZE*sizeof (NBAMR_S16);
	pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
	if (pBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pBuffer, 0, BufferSize);
	pInBuf = pBuffer;

	return ret;

}

OMX_ERRORTYPE NbAmrDecWrapper::CodecInit(OMX_AUDIO_PARAM_AMRTYPE *pAmrType)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	eAMRDReturnType AmrRet;

    switch(pAmrType->eAMRFrameFormat) 
	{
        case OMX_AUDIO_AMRFrameFormatConformance:
			pNbAmrDecConfig->u8BitStreamFormat = NBAMR_ETSI;
            break;
        case OMX_AUDIO_AMRFrameFormatFSF:
			pNbAmrDecConfig->u8BitStreamFormat = NBAMR_MMSIO;
            break;
        case OMX_AUDIO_AMRFrameFormatIF1:
			pNbAmrDecConfig->u8BitStreamFormat = NBAMR_IF1IO;
            break;
        case OMX_AUDIO_AMRFrameFormatIF2:
			pNbAmrDecConfig->u8BitStreamFormat = NBAMR_IF2IO;
            break;
        default:
            return OMX_ErrorBadParameter;
    }

    pNbAmrDecConfig->pvAMRDDecodeInfoPtr = NULL;
    pNbAmrDecConfig->pu8APPDInitializedDataStart = NULL;
    pNbAmrDecConfig->u8RXFrameType = 0; 
    pNbAmrDecConfig->u8NumFrameToDecode = FRAMESPERLOOP;

	AmrRet = eAMRDDecodeInit(pNbAmrDecConfig);
	if (AmrRet != E_NBAMRD_OK)
	{
		LOG_ERROR("Amr decoder initialize fail.\n");
		return OMX_ErrorUndefined;
	}

    return ret;
}

OMX_ERRORTYPE NbAmrDecWrapper::InstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 MemoryCnt = pNbAmrDecConfig->sAMRDMemInfo.s32AMRDNumMemReqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		FSL_FREE(pNbAmrDecConfig->sAMRDMemInfo.asMemInfoSub[i].pvAPPDBasePtr);
	}

	FSL_FREE(pNbAmrDecConfig);
	FSL_FREE(pInBuf);

    return ret;
}

OMX_U32 NbAmrDecWrapper::getInputBufferPushSize()
{
    return SERIAL_FRAMESIZE*2*FRAMESPERLOOP;
}

OMX_ERRORTYPE NbAmrDecWrapper::decodeFrame(
                    OMX_U8 *pInputBuffer, 
                    OMX_U8 *pOutBuffer, 
                    OMX_AUDIO_PARAM_AMRTYPE * pAmrType,
                    OMX_U32 inputDataSize,
                    OMX_U32 *pConsumedSize)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U8 *pSrcCur, Header ;

    eAMRDReturnType AmrRet;
    OMX_U32 nInPackSize, nActualLen;

    LOG_DEBUG("decodeFrame: format %d\n", pAmrType->eAMRFrameFormat);

    if (pAmrType->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatConformance)
    {
        nInPackSize = pNbAmrDecConfig->u8NumFrameToDecode * SERIAL_FRAMESIZE * sizeof (NBAMR_S16);
        LOG_DEBUG("Desire packet size %d, %d bytes already!", nInPackSize, inputDataSize);
		if (nInPackSize > inputDataSize)
		{
			pNbAmrDecConfig->u8NumFrameToDecode = inputDataSize / SERIAL_FRAMESIZE / sizeof(NBAMR_S16);
		}
		AmrRet = eAMRDDecodeFrame(pNbAmrDecConfig, (NBAMR_S16 *)pInputBuffer, (NBAMR_S16 *)pOutBuffer);
    }
    else    /* MMS or IF1 or IF2 format */
    {
        // Find the packet Header
		OMX_U32 nFrames = 0, nBytesScaned = 0;
		pSrcCur = pInputBuffer;
        while(nBytesScaned < inputDataSize)
        {
            OMX_U8 u8Mode;
            Header = pSrcCur[0];

			if (nFrames >= pNbAmrDecConfig->u8NumFrameToDecode)
			{
				break;
			}

            nInPackSize = 0;
            if (OMX_AUDIO_AMRFrameFormatFSF == pAmrType->eAMRFrameFormat)
            {
                u8Mode = (NBAMR_U8) (0x0F & (Header >> 3));
                LOG_DEBUG("u8Mode: %d\n", u8Mode);
                if (u8Mode <  NBAMR_MAX_NUM_MODES)
                {
                    nInPackSize = MMSPackedCodedSize[u8Mode];
                }
                else
                {
                    LOG_ERROR("MMS: Invalid mode code %d", u8Mode);
                    break;
                }
            }
            else if (OMX_AUDIO_AMRFrameFormatIF1 == pAmrType->eAMRFrameFormat)
            {
                u8Mode = (NBAMR_U8) (0x0F & (Header >> 4));
                if (u8Mode <  NBAMR_MAX_NUM_MODES)
                {
                    nInPackSize = IF1PackedCodedSize[u8Mode];
                }
                else
                {
                    LOG_ERROR("IF1: Invalid mode code %d", u8Mode);
                    break;
                }
            }
            else if (OMX_AUDIO_AMRFrameFormatIF2 == pAmrType->eAMRFrameFormat)
            {
                u8Mode = (NBAMR_U8) (0x0F & (Header));
                if (u8Mode <  NBAMR_MAX_NUM_MODES)
                {
                    nInPackSize = IF2PackedCodedSize[u8Mode];
                }
                else
                {
                    LOG_ERROR("IF2: Invalid mode code %d", u8Mode);
                    break;
                }
            }
            else
            {
                LOG_ERROR("Unknown Stream type %d", pAmrType->eAMRFrameFormat);
            }

			if (nInPackSize < 1)
				nInPackSize = 1;
			fsl_osal_memcpy(pInBuf + (nFrames*NBAMR_MAX_PACKED_SIZE), pSrcCur, nInPackSize);
			pSrcCur += nInPackSize;
			nBytesScaned += nInPackSize;
			nFrames ++;
                     LOG_DEBUG("nInPackSize is %d, nBytesScaned %d, nFrames %d\n", nInPackSize, nBytesScaned, nFrames);
        }   /* end of while loop */

		if (nBytesScaned > inputDataSize)
			nBytesScaned = inputDataSize;
		nInPackSize = nBytesScaned;
		pNbAmrDecConfig->u8NumFrameToDecode = nFrames;
		AmrRet = eAMRDDecodeFrame(pNbAmrDecConfig, (NBAMR_S16 *)pInBuf, \
					(NBAMR_S16 *)pOutBuffer);
	}

    *pConsumedSize = nInPackSize;

    if(AmrRet == E_NBAMRD_OK){
        ret = OMX_ErrorNone;
    } 
    else
        ret = OMX_ErrorUndefined;

    return ret;

}

OMX_U32 NbAmrDecWrapper::getFrameOutputSize()
{
    return pNbAmrDecConfig->u8NumFrameToDecode * L_FRAME * sizeof(NBAMR_S16);
}

OMX_U32 NbAmrDecWrapper::getBitRate(OMX_U32 InputframeSize)
{
    if(pNbAmrDecConfig->u8NumFrameToDecode > 0)
        return (InputframeSize << 3) * 8000 / L_FRAME / pNbAmrDecConfig->u8NumFrameToDecode;
    else
        return 0;
}

OMX_TICKS NbAmrDecWrapper::getTsPerFrame(OMX_U32 sampleRate)
{
    if(sampleRate > 0)
        return FRAMESPERLOOP * L_FRAME*OMX_TICKS_PER_SECOND/sampleRate;
    else
        return 0;
}

OMX_U32 const NbAmrDecWrapper::MMSPackedCodedSize[16]=
    {
    13, 14, 16, 18, 20, 21, 27, 32,
    6,  0,  0,  0,  0,  0,  0,  1
    };
OMX_U32 const NbAmrDecWrapper::IF1PackedCodedSize[16] = 
    {
    15, 16, 18, 20, 22, 23, 29, 34,
    8,  0,  0,  0,  0,  0,  0,  1
    };

OMX_U32 const NbAmrDecWrapper::IF2PackedCodedSize[16] =
    {
    13, 14, 16, 18, 19, 21, 26, 31,
    6,  0,  0,  0,  0,  0,  0,  1
    };


