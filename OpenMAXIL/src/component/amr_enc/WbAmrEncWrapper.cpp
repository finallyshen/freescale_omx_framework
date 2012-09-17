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

OMX_ERRORTYPE WbAmrEncWrapper::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 i;

	pAmrEncConfig = (WBAMRE_Encoder_Config *)FSL_MALLOC(sizeof(WBAMRE_Encoder_Config));
	if (pAmrEncConfig == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pAmrEncConfig, 0, sizeof(WBAMRE_Encoder_Config));

	pAmrEncConfig->wbappe_initialized_data_start = NULL;

	WBAMRE_RET_TYPE AmrRet;
	AmrRet = wbamre_query_enc_mem(pAmrEncConfig);
	if (AmrRet != WBAMRE_OK)
	{
		LOG_ERROR("Amr encoder query memory fail.\n");
		return OMX_ErrorUndefined;
	}

	OMX_S32 MemoryCnt = pAmrEncConfig->wbamre_mem_info.wbamre_num_reqs;
	for (i = 0; i < MemoryCnt; i ++)
	{
		OMX_U32 BufferSize;
		OMX_U8 *pBuffer;

		BufferSize = pAmrEncConfig->wbamre_mem_info.mem_info_sub[i].wbamre_size;
		pBuffer = (OMX_U8 *)FSL_MALLOC(BufferSize);
		if (pBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pBuffer, 0, BufferSize);
		pAmrEncConfig->wbamre_mem_info.mem_info_sub[i].wbappe_base_ptr = pBuffer;
	}

	return ret;

}

OMX_ERRORTYPE WbAmrEncWrapper::CodecInit(OMX_AUDIO_PARAM_AMRTYPE *pAmrType)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	nOutputSize = 0;
	mode_in = WBAMR_MODE_23_85;
	pAmrEncConfig->wbappe_dtx_flag = 0;/* disable DTX mode here */
	pAmrEncConfig->wbamre_output_size = (WBAMR_U16*)&nOutputSize;

    switch(pAmrType->eAMRFrameFormat) 
	{
		case OMX_AUDIO_AMRFrameFormatConformance:
			pAmrEncConfig->wbamre_output_format = 1;
			break;
		case OMX_AUDIO_AMRFrameFormatFSF:
			LOG_DEBUG("Amr format MMSIO.\n");
			pAmrEncConfig->wbamre_output_format = 2;
			break;
		case OMX_AUDIO_AMRFrameFormatIF1:
			pAmrEncConfig->wbamre_output_format = 4;
			break;
		case OMX_AUDIO_AMRFrameFormatIF2:
			pAmrEncConfig->wbamre_output_format = 3;
			break;
		default:
			return OMX_ErrorBadParameter;
	}
	switch(pAmrType->eAMRBandMode) 
	{
		case OMX_AUDIO_AMRBandModeUnused:
			mode_in = WBAMR_MODE_23_85;
			pAmrEncConfig->wbappe_mode = (WBAMR_S16 *)&mode_in;/* set encoding mode here */
			break;
		case OMX_AUDIO_AMRBandModeWB0:
			mode_in = WBAMR_MODE_6_60;
			pAmrEncConfig->wbappe_mode = (WBAMR_S16 *)&mode_in;/* set encoding mode here */
			break;
		case OMX_AUDIO_AMRBandModeWB1:
			mode_in = WBAMR_MODE_8_85;
			pAmrEncConfig->wbappe_mode = (WBAMR_S16 *)&mode_in;/* set encoding mode here */
			break;
		case OMX_AUDIO_AMRBandModeWB2:
			mode_in = WBAMR_MODE_12_65;
			pAmrEncConfig->wbappe_mode = (WBAMR_S16 *)&mode_in;/* set encoding mode here */
			break;
		case OMX_AUDIO_AMRBandModeWB3:
			mode_in = WBAMR_MODE_14_25;
			pAmrEncConfig->wbappe_mode = (WBAMR_S16 *)&mode_in;/* set encoding mode here */
			break;
		case OMX_AUDIO_AMRBandModeWB4:
			mode_in = WBAMR_MODE_15_85;
			pAmrEncConfig->wbappe_mode = (WBAMR_S16 *)&mode_in;/* set encoding mode here */
			break;
		case OMX_AUDIO_AMRBandModeWB5:
			mode_in = WBAMR_MODE_18_25;
			pAmrEncConfig->wbappe_mode = (WBAMR_S16 *)&mode_in;/* set encoding mode here */
			break;
		case OMX_AUDIO_AMRBandModeWB6:
			mode_in = WBAMR_MODE_19_85;
			pAmrEncConfig->wbappe_mode = (WBAMR_S16 *)&mode_in;/* set encoding mode here */
			break;
		case OMX_AUDIO_AMRBandModeWB7:
			mode_in = WBAMR_MODE_23_05;
			pAmrEncConfig->wbappe_mode = (WBAMR_S16 *)&mode_in;/* set encoding mode here */
			break;
		case OMX_AUDIO_AMRBandModeWB8:
			mode_in = WBAMR_MODE_23_85;
			pAmrEncConfig->wbappe_mode = (WBAMR_S16 *)&mode_in;/* set encoding mode here */
			break;
		default:
            return OMX_ErrorBadParameter;
    }

	WBAMRE_RET_TYPE AmrRet;
	AmrRet = wbamre_encode_init(pAmrEncConfig);
	if (AmrRet != WBAMRE_OK)
	{
		LOG_ERROR("Amr encoder initialize fail. return: %d\n", AmrRet);
		return OMX_ErrorUndefined;
	}

    return ret;
}

OMX_ERRORTYPE WbAmrEncWrapper::InstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 MemoryCnt = pAmrEncConfig->wbamre_mem_info.wbamre_num_reqs;
	OMX_S32 i;

	for (i = 0; i < MemoryCnt; i ++)
	{
		FSL_FREE(pAmrEncConfig->wbamre_mem_info.mem_info_sub[i].wbappe_base_ptr);
	}

	FSL_FREE(pAmrEncConfig);

    return ret;
}

OMX_U32 WbAmrEncWrapper::getInputBufferPushSize()
{
    return WB_INPUT_BUF_PUSH_SIZE;
}

OMX_ERRORTYPE WbAmrEncWrapper::encodeFrame(
                    OMX_U8 *pOutBuffer, 
                    OMX_U32 *pOutputDataSize,
                    OMX_U8 *pInputBuffer, 
                    OMX_U32 *pInputDataSize)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	wbamre_encode_frame(pAmrEncConfig, (WBAMR_S16*)pInputBuffer, (WBAMR_S16*)pOutBuffer);

	LOG_LOG("Input size: %d\n", *pInputDataSize);
	LOG_LOG("Output size: %d\n", *(pAmrEncConfig->wbamre_output_size));
	*pOutputDataSize = *(pAmrEncConfig->wbamre_output_size);
	*pInputDataSize = (WBAMR_L_FRAME * sizeof(WBAMR_S16));

    return ret;

}

OMX_TICKS WbAmrEncWrapper::getTsPerFrame(OMX_U32 sampleRate)
{
    if(sampleRate > 0)
		return WBAMR_L_FRAME *OMX_TICKS_PER_SECOND/sampleRate;
	else
        return 0;

}


