/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "FlacParser.h"

#define FLAC_TMP_BUFFER_SIZE (1024*1024)

FlacParser::FlacParser()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.parser.flac.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"parser.flac";
    bInContext = OMX_FALSE;
	bNeedSendCodecConfig = OMX_TRUE;
}

OMX_ERRORTYPE FlacParser::GetCoreParser()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	ParserFileHeader = FlacParserFileHeader;
	ParserFrame = FlacParserFrame;

	return ret;
}

OMX_AUDIO_CODINGTYPE FlacParser::GetAudioCodingType()
{
	return (OMX_AUDIO_CODINGTYPE)OMX_AUDIO_CodingFLAC;
}

OMX_ERRORTYPE FlacParser::AudioParserSetCodecConfig(OMX_BUFFERHEADERTYPE *pOutBuffer)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_FLACTYPE FlacType; 

	OMX_INIT_STRUCT(&FlacType, OMX_AUDIO_PARAM_FLACTYPE);

	FlacType.nPortIndex = AUDIO_OUTPUT_PORT;
	FlacType.nChannels = FileInfo.nChannels;
	FlacType.nSampleRate = FileInfo.nSamplingRate;
	FlacType.nBitPerSample = FileInfo.nBitPerSample;
	if (nAudioDuration)
		FlacType.nBitRate = ((OMX_U64)nFileSize << 3) * OMX_TICKS_PER_SECOND / nAudioDuration;
	FlacType.nTotalSample = FileInfo.nTotalSample;
	FlacType.nBlockSize = FileInfo.nBlockSize;

	fsl_osal_memcpy(pOutBuffer->pBuffer, &FlacType, sizeof(OMX_AUDIO_PARAM_FLACTYPE));
	pOutBuffer->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;

    return ret;
}

OMX_S64 FlacParser::nGetTimeStamp(OMX_S64 *pSeekPoint)
{
	OMX_U32 nActuralRead;
	OMX_U32 nReadLen = FLAC_TMP_BUFFER_SIZE;
	FRAME_INFO FrameInfo;
       fsl_osal_memset(&FrameInfo, 0, sizeof(FRAME_INFO));
	OMX_U8 *pTmpBuffer = (OMX_U8 *)FSL_MALLOC(FLAC_TMP_BUFFER_SIZE);
	if (pTmpBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return sAudioSeekPos;
	}

	fileOps.Seek(sourceFileHandle, *pSeekPoint, SEEK_SET, this);

	if ((OMX_U64)*pSeekPoint + nReadLen > nEndPoint)
	{
		nReadLen = nEndPoint - *pSeekPoint;
	}

	nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);

	LOG_DEBUG("Actural read len: %d\n", nActuralRead);
	FrameInfo = DoFlacParserFrame(pTmpBuffer, nActuralRead, &FileInfo);
	if (FrameInfo.flags == FLAG_SUCCESS)
	{
		*pSeekPoint += FrameInfo.index;
		if (FileInfo.nSamplingRate && FrameInfo.samples)
		{
			sAudioSeekPos = (OMX_S64)FrameInfo.samples * OMX_TICKS_PER_SECOND / FileInfo.nSamplingRate;
			LOG_DEBUG("After adjust Seek Position: %lld\t Audio Duration: %lld\n", sAudioSeekPos, nAudioDuration);
		}
	}
	else
	{
		LOG_ERROR("Flac parser can't find frame.\n");
	}

	FSL_FREE(pTmpBuffer);

	return sAudioSeekPos;
}

/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE FlacParserInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        FlacParser *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(FlacParser, ());
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
