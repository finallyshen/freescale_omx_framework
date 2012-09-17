/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "WavParser.h"

WavParser::WavParser()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.parser.wav.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"parser.wav";
    bInContext = OMX_FALSE;
	bNeedSendCodecConfig = OMX_TRUE;
}

OMX_ERRORTYPE WavParser::GetCoreParser()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	ParserFileHeader = WavParserFileHeader;
	ParserFrame = WavParserFrame;

	return ret;
}

OMX_AUDIO_CODINGTYPE WavParser::GetAudioCodingType()
{
	if (FileInfo.ePCMMode == PCM_MODE_LINEAR)
		return OMX_AUDIO_CodingPCM;
	return OMX_AUDIO_CodingUnused;
}

OMX_ERRORTYPE WavParser::AudioParserSetCodecConfig(OMX_BUFFERHEADERTYPE *pOutBuffer)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PCMMODETYPE PcmMode; 

	OMX_INIT_STRUCT(&PcmMode, OMX_AUDIO_PARAM_PCMMODETYPE);

	PcmMode.nPortIndex = AUDIO_OUTPUT_PORT;
	PcmMode.nChannels = FileInfo.nChannels;
	PcmMode.nSamplingRate = FileInfo.nSamplingRate;
	PcmMode.nBitPerSample = FileInfo.nBitPerSample;
	PcmMode.bInterleaved = OMX_TRUE;
	PcmMode.eNumData = OMX_NumericalDataSigned;
	PcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
	PcmMode.eEndian = OMX_EndianLittle;
	PcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelNone;

	fsl_osal_memcpy(pOutBuffer->pBuffer, &PcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
	pOutBuffer->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;

    return ret;
}

OMX_U64 WavParser::FrameBound(OMX_U64 nSkip)
{
	OMX_U32 nSampleLen = FileInfo.nChannels * (FileInfo.nBitPerSample>>3);
	return nSkip/nSampleLen*nSampleLen;
}

/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE WavParserInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        WavParser *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(WavParser, ());
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
