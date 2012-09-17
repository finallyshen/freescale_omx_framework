/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Mp3Parser.h"

Mp3Parser::Mp3Parser()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.parser.mp3.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"parser.mp3";
    bInContext = OMX_FALSE;
	bNeedSendCodecConfig = OMX_FALSE;
}

OMX_ERRORTYPE Mp3Parser::GetCoreParser()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	ParserFileHeader = Mp3ParserFileHeader;
	ParserFrame = Mp3ParserFrame;

	return ret;
}

OMX_AUDIO_CODINGTYPE Mp3Parser::GetAudioCodingType()
{
	return OMX_AUDIO_CodingMP3;
}

OMX_ERRORTYPE Mp3Parser::SetSource(OMX_PARAM_CONTENTURITYPE *Content, OMX_PARAM_CONTENTPIPETYPE *Pipe)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	hPipe =(CP_PIPETYPE* ) (((OMX_PARAM_CONTENTPIPETYPE *)Pipe)->hPipe);

	OMX_PARAM_CONTENTURITYPE * pContentURI = (OMX_PARAM_CONTENTURITYPE *)Content;
	nMediaNameLen = fsl_osal_strlen((const char *)&(pContentURI->contentURI)) + 1;

	pMediaName = (OMX_S8 *)FSL_MALLOC(nMediaNameLen);
	if (!pMediaName)
	{
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_strcpy((char *)pMediaName, (const char *)&(pContentURI->contentURI));
	if(fsl_osal_strncmp((fsl_osal_char*)pMediaName, "http://", 7) == 0
			|| fsl_osal_strncmp((fsl_osal_char*)pMediaName, "rtsp://", 7) == 0)
		isStreamingSource = OMX_TRUE;

	bGetMetadata = OMX_TRUE;
	fsl_osal_strcpy((fsl_osal_char*)sCompRole.cRole, role[0]);

	return ret;
}

OMX_ERRORTYPE Mp3Parser::UnloadParserMetadata()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (pMediaName)
    {
        FSL_FREE(pMediaName);
        pMediaName = NULL;
    }

	for (OMX_U32 i = 0; i < MAX_MATADATA_NUM; i ++)
	{
		FSL_FREE(psMatadataItem[i]);;
	}

	return ret;
}

OMX_U32 Mp3Parser::GetMetadataNum()
{
	return sMatadataItemCount.nMetadataItemCount;
}

OMX_U32 Mp3Parser::GetMetadataSize(OMX_U32 index)
{
	return psMatadataItem[index]->nValueMaxSize;
}

OMX_ERRORTYPE Mp3Parser::GetMetadata(OMX_CONFIG_METADATAITEMTYPE *pMatadataItem)
{
	fsl_osal_memcpy(pMatadataItem->nKey, psMatadataItem[pMatadataItem->nMetadataItemIndex]->nKey, 128);
	pMatadataItem->nKeySizeUsed = 128;
	fsl_osal_memcpy(pMatadataItem->nValue, psMatadataItem[pMatadataItem->nMetadataItemIndex]->nValue, psMatadataItem[pMatadataItem->nMetadataItemIndex]->nValueSizeUsed);
	pMatadataItem->nValueSizeUsed = psMatadataItem[pMatadataItem->nMetadataItemIndex]->nValueSizeUsed;

	return OMX_ErrorNone;
}

/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE Mp3ParserInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        Mp3Parser *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(Mp3Parser, ());
        if(obj == NULL)
            return OMX_ErrorInsufficientResources;

        base = (ComponentBase*)obj;
        ret = base->ConstructComponent(pHandle);
        if(ret != OMX_ErrorNone)
            return ret;

        return ret;
    }

	OMX_ERRORTYPE ParserMetadata(OMX_PTR *handle, OMX_PARAM_CONTENTURITYPE *Content, OMX_PARAM_CONTENTPIPETYPE *Pipe)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        Mp3Parser *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(Mp3Parser, ());
        if(obj == NULL)
            return OMX_ErrorInsufficientResources;

		ret = obj->SetSource(Content, Pipe);
		if(ret != OMX_ErrorNone)
			return ret;

		ret = obj->InstanceInit();
		if(ret != OMX_ErrorNone)
			obj->InstanceDeInit();

		*handle = obj;

		return ret;
	}

	OMX_ERRORTYPE UnloadParserMetadata(OMX_PTR handle)
	{
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        Mp3Parser *obj = (Mp3Parser *)handle;

		obj->InstanceDeInit();
		obj->UnloadParserMetadata();
		FSL_DELETE(obj);

		return ret;
	}

	OMX_U32 GetMetadataNum(OMX_PTR handle)
	{
        Mp3Parser *obj = (Mp3Parser *)handle;
		return obj->GetMetadataNum();
	}

	OMX_U32 GetMetadataSize(OMX_PTR handle, OMX_U32 index)
	{
        Mp3Parser *obj = (Mp3Parser *)handle;
		return obj->GetMetadataSize(index);
	}

	OMX_ERRORTYPE GetMetadata(OMX_PTR handle, OMX_CONFIG_METADATAITEMTYPE *pMatadataItem)
	{
        Mp3Parser *obj = (Mp3Parser *)handle;
		return obj->GetMetadata(pMatadataItem);
	}
}

/* File EOF */
