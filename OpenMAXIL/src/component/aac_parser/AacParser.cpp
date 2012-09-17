/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "AacParser.h"

AacParser::AacParser()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.parser.aac.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"parser.aac";
    bInContext = OMX_FALSE;
	bNeedSendCodecConfig = OMX_FALSE;
}

OMX_ERRORTYPE AacParser::GetCoreParser()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	ParserFileHeader = AacParserFileHeader;
	ParserFrame = AacParserFrame;

	return ret;
}

OMX_AUDIO_CODINGTYPE AacParser::GetAudioCodingType()
{
	return OMX_AUDIO_CodingAAC;
}

/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE AacParserInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        AacParser *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(AacParser, ());
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
