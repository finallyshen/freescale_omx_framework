/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Ac3Parser.h"

Ac3Parser::Ac3Parser()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.parser.ac3.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = "parser.ac3";
    bInContext = OMX_FALSE;
	bNeedSendCodecConfig = OMX_FALSE;
}

OMX_ERRORTYPE Ac3Parser::GetCoreParser()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	ParserFileHeader = Ac3ParserFileHeader;
	ParserFrame = Ac3ParserFrame;

	return ret;
}

OMX_AUDIO_CODINGTYPE Ac3Parser::GetAudioCodingType()
{
	return (OMX_AUDIO_CODINGTYPE)OMX_AUDIO_CodingAC3;
}

/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE Ac3ParserInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        Ac3Parser *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(Ac3Parser, ());
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
