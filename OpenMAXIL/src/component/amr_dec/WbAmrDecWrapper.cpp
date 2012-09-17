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

//#define LOG_DEBUG printf

OMX_ERRORTYPE WbAmrDecWrapper::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("WbAmrDecWrapper::InstanceInit()\n");
    
	pWbAmrDecConfig = (WBAMRD_Decoder_Config *)FSL_MALLOC(sizeof(WBAMRD_Decoder_Config));
	if (pWbAmrDecConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pWbAmrDecConfig , 0, sizeof(WBAMRD_Decoder_Config));

	WBAMRD_RET_TYPE AmrRet;
	AmrRet = wbamrd_query_dec_mem(pWbAmrDecConfig );
	if (AmrRet != WBAMRD_OK)
	{
		LOG_ERROR("Amr decoder query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	OMX_S32 MemoryCnt = pWbAmrDecConfig->wbamrd_mem_info.wbamrd_num_reqs;
	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pWbAmrDecConfig->wbamrd_mem_info.mem_info_sub[i].wbamrd_size;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		pWbAmrDecConfig->wbamrd_mem_info.mem_info_sub[i].wbappd_base_ptr = pBuffer;
	}

	OMX_U32 BufferSize;
	OMX_U8 *pBuffer;

	BufferSize = WBAMR_SERIAL_FRAMESIZE*sizeof (WBAMR_S16);
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

OMX_ERRORTYPE WbAmrDecWrapper::CodecInit(OMX_AUDIO_PARAM_AMRTYPE *pAmrType)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	WBAMRD_RET_TYPE AmrRet;

    LOG_DEBUG("WbAmrDecWrapper::CodecInit\n");
    
	//if (AmrBandMode == AMRBandModeWB)
	//	return OMX_ErrorUndefined;

    switch(pAmrType->eAMRFrameFormat) 
	{
        case OMX_AUDIO_AMRFrameFormatConformance:
			pWbAmrDecConfig->bitstreamformat = 0;
            break;
        case OMX_AUDIO_AMRFrameFormatFSF:
			pWbAmrDecConfig->bitstreamformat = 2;
            break;
        case OMX_AUDIO_AMRFrameFormatIF1:
			pWbAmrDecConfig->bitstreamformat = 4;
            break;
        case OMX_AUDIO_AMRFrameFormatIF2:
			pWbAmrDecConfig->bitstreamformat = 3;
            break;
        case OMX_AUDIO_AMRFrameFormatITU:
                     pWbAmrDecConfig->bitstreamformat = 1;;
            break;

        default:
            return OMX_ErrorBadParameter;
    }

    LOG_DEBUG("CodecInit init format to %d\n", pWbAmrDecConfig->bitstreamformat);

    pWbAmrDecConfig->wbamrd_decode_info_struct_ptr = NULL;
    pWbAmrDecConfig->wbappd_initialized_data_start = NULL;

    AmrRet = wbamrd_decode_init(pWbAmrDecConfig);
    if (AmrRet != WBAMRD_OK){
        LOG_ERROR("Amr decoder initialize fail.\n");
        return OMX_ErrorUndefined;
    }

    LOG_DEBUG("CodecInit ok\n");

    return ret;
}

OMX_ERRORTYPE WbAmrDecWrapper::InstanceDeInit()
{
    LOG_DEBUG("WbAmrDecWrapper::InstanceDeInit()\n");

    OMX_ERRORTYPE ret = OMX_ErrorNone;
       OMX_S32 MemoryCnt = pWbAmrDecConfig->wbamrd_mem_info.wbamrd_num_reqs;
 	for (OMX_S32 i = 0; i < MemoryCnt; i ++)
	{
               FSL_FREE(pWbAmrDecConfig->wbamrd_mem_info.mem_info_sub[i].wbappd_base_ptr);
 	}

	FSL_FREE(pWbAmrDecConfig);
	FSL_FREE(pInBuf);

    return ret;
}

OMX_U32 WbAmrDecWrapper::getInputBufferPushSize()
{
    return WBAMR_SERIAL_FRAMESIZE;
}

OMX_ERRORTYPE WbAmrDecWrapper::decodeFrame(
                    OMX_U8 *pInputBuffer, 
                    OMX_U8 *pOutBuffer, 
                    OMX_AUDIO_PARAM_AMRTYPE * pAmrType,
                    OMX_U32 inputDataSize,
                    OMX_U32 *pConsumedSize)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U8 *pSrcCur, Header, *pInBufCur;
    WBAMRD_RET_TYPE AmrRet;

    LOG_DEBUG("WbAmrDecWrapper::decodeFrame()\n");

//    WBAMR_S16 n, n1, type_of_frame_type, coding_mode, datalen, i;
//    WBAMR_U8 toc, q, temp, *packet_ptr, packet[64];
//    WBAMR_S16 frame_type,mode=0,*prms_ptr;

    if (pAmrType->eAMRFrameFormat == OMX_AUDIO_AMRFrameFormatConformance)
    {
		AmrRet = wbamrd_decode_frame(pWbAmrDecConfig, (WBAMR_S16 *)pInputBuffer, (WBAMR_S16 *)pOutBuffer);
             *pConsumedSize = inputDataSize;

    }
    else    /* MMS or IF1 or IF2 format */
    {
        // Find the packet Header
	 OMX_U32 nBytesScaned = 0;
        OMX_U32 frameSize = 0;
	 pSrcCur = pInputBuffer;
        pInBufCur = pInBuf;

        OMX_U8 u8Mode;
        Header = pSrcCur[0];

        if (OMX_AUDIO_AMRFrameFormatFSF == pAmrType->eAMRFrameFormat)
        {
            OMX_U8 q = (Header >> 2) & 0x01;
            u8Mode = (Header >> 3) & 0x0F;
            
            frameSize = MMSPackedCodedSize[u8Mode];
            pSrcCur++;
            *pInBufCur++ = q;
            *pInBufCur++ = u8Mode;
            nBytesScaned += 1;
        }
        else if (OMX_AUDIO_AMRFrameFormatIF1 == pAmrType->eAMRFrameFormat)
        {
            u8Mode = (WBAMR_U8) (0x0F & (Header >> 4));
            //LOG_DEBUG("IF1, mode is %d\n", u8Mode);
            frameSize = IF1PackedCodedSize[u8Mode];
        }
        else if (OMX_AUDIO_AMRFrameFormatIF2 == pAmrType->eAMRFrameFormat)
        {
            u8Mode = (WBAMR_U8) (0x0F & (Header));
            frameSize  = IF2PackedCodedSize[u8Mode];
        }
        else
        {
            LOG_ERROR("Unknown Stream type %d", pAmrType->eAMRFrameFormat);
        }

        //LOG_DEBUG("frameSize  is %d\n", frameSize );

        if(frameSize > 0)
            fsl_osal_memcpy(pInBufCur , pSrcCur, frameSize);

	nBytesScaned += frameSize;

	if (nBytesScaned > inputDataSize)
		nBytesScaned = inputDataSize;
    
	*pConsumedSize = nBytesScaned;

	AmrRet = wbamrd_decode_frame(pWbAmrDecConfig, (WBAMR_S16 *)pInBuf, 	(WBAMR_S16 *)pOutBuffer);
    
    }

    if(AmrRet == WBAMRD_OK){
        ret = OMX_ErrorNone;
    } 
    else
        ret = OMX_ErrorUndefined;

    return ret;

}

OMX_U32 WbAmrDecWrapper::getFrameOutputSize()
{
    return WBAMR_L_FRAME * sizeof(WBAMR_S16);
}

OMX_U32 WbAmrDecWrapper::getBitRate(OMX_U32 InputframeSize)
{
    if(InputframeSize > 0)
        return (InputframeSize << 3) * 8000 / WBAMR_L_FRAME;
    else
        return 0;
}

OMX_TICKS WbAmrDecWrapper::getTsPerFrame(OMX_U32 sampleRate)
{
    return WBAMR_L_FRAME *OMX_TICKS_PER_SECOND/sampleRate;
}

OMX_U32 const WbAmrDecWrapper::MMSPackedCodedSize[16]=
    {
    17, 23, 32, 36, 40, 46, 50, 58,
    60,  5,  0,  0,  0,  0,  0,  0
    };
OMX_U32 const WbAmrDecWrapper::IF1PackedCodedSize[16] = 
    {
    20, 26, 35, 39, 43, 49, 53, 61,
    63,  8,  1,  1,  1,  1,  1,  1
    };

OMX_U32 const WbAmrDecWrapper::IF2PackedCodedSize[16] =
    {
    18, 23, 33, 37, 41, 47, 51, 59,
    61,  6,  1,  1,  1,  1,  1,  1
    };


