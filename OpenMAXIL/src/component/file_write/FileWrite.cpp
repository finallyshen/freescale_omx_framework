/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <string.h>
#include "FileWrite.h"

static void MakeDumpFileName(OMX_STRING media_file_name,
        OMX_STRING dump_file_path,OMX_S32 port_num)
{
    OMX_STRING p = strrchr(media_file_name, '.');
    if (p == NULL) {
        return;
    }
    OMX_S32 suffix_len = strlen(p);
    OMX_S32 len_org = strlen(media_file_name);
    strncpy(dump_file_path,media_file_name, (len_org-suffix_len));
    dump_file_path[len_org-suffix_len] = '\0';
    OMX_S8 tmp[8];
    sprintf((char *)tmp, "_port%d", port_num);
    strcat(dump_file_path, (char *)tmp);
    //strcat(dump_file_path, ".yuv");
    printf("%s, port dump file name %s.\n",__FUNCTION__,dump_file_path);
    return;
}

static void write_data(FILE *pOutFile0, FILE *pOutFile1, OMX_BUFFERHEADERTYPE *pBufferHdr, OMX_S32 nPortIndex)
{
    if (nPortIndex == AUDIO_PORT)
        fwrite(pBufferHdr->pBuffer, 1, pBufferHdr->nFilledLen, pOutFile0);
    else if (nPortIndex == VIDEO_PORT)
        fwrite(pBufferHdr->pBuffer, 1, pBufferHdr->nFilledLen, pOutFile1);
    fflush(pOutFile0);
    fflush(pOutFile1);
    pBufferHdr->nFilledLen = 0;
    pBufferHdr->nOffset = 0;
    return;
}

FileWrite::FileWrite()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.component.file_write.sw-based");
    ComponentVersion.s.nVersionMajor = 0x0;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x0;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = "component.file_write";
    bInContext = OMX_FALSE;
    nPorts = 2;
    pAudioBufferHdr = pVideoBufferHdr = NULL; 
}

OMX_ERRORTYPE FileWrite::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = AUDIO_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingMP3;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
    sPortDef.nBufferSize = 1024;
    ret = ports[AUDIO_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for port[%d] failed.\n", AUDIO_PORT);
        return ret;
    }

    sPortDef.nPortIndex = VIDEO_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainVideo;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
    sPortDef.nBufferSize = 1024;
	sPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    ret = ports[VIDEO_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
        return ret;
    }

    return ret;
}

OMX_ERRORTYPE FileWrite::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE FileWrite::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	OMX_S8 dump_file_path0[128];
	OMX_S8 dump_file_path1[128];

	printf("Output file name: %s\n", pMediaName);
	MakeDumpFileName((char *)pMediaName,(char *)dump_file_path0,AUDIO_PORT);
    MakeDumpFileName((char *)pMediaName,(char *)dump_file_path1,VIDEO_PORT);

    pOutFile0 = fopen((char *)dump_file_path0, "wb");
    if(pOutFile0 == NULL) {
        printf("Failed to open file: %s\n", dump_file_path0);
        return OMX_ErrorInsufficientResources;
    }
    pOutFile1 = fopen((char *)dump_file_path1, "wb");
    if(pOutFile1 == NULL) {
        printf("Failed to open file: %s\n", dump_file_path1);
        return OMX_ErrorInsufficientResources;
    }

	return ret;
}

OMX_ERRORTYPE FileWrite::InstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	fclose(pOutFile0);
	fclose(pOutFile1);

    return ret;
}
OMX_ERRORTYPE FileWrite::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE FileWrite::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
		case OMX_IndexParamCustomContentPipe:
			hPipe =(CP_PIPETYPE* ) (((OMX_PARAM_CONTENTPIPETYPE *)pComponentParameterStructure)->hPipe);
			break;
		case OMX_IndexParamContentURI:
		{
			OMX_PARAM_CONTENTURITYPE * pContentURI = (OMX_PARAM_CONTENTURITYPE *)pComponentParameterStructure;
			OMX_S32 nMediaNameLen = strlen((const char *)&(pContentURI->contentURI)) + 1;
				
            pMediaName = (OMX_S8 *)fsl_osal_malloc_new(nMediaNameLen);
            if (!pMediaName)
            {
                ret = OMX_ErrorInsufficientResources;
                break;
            }
			strcpy((char *)pMediaName, (const char *)&(pContentURI->contentURI));

			break;
		}
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE FileWrite::ProcessDataBuffer()
{
    LOG_DEBUG("In: %d, Out: %d\n", ports[AUDIO_PORT]->BufferNum(), ports[VIDEO_PORT]->BufferNum());

    if(ports[AUDIO_PORT]->BufferNum() == 0
            && ports[VIDEO_PORT]->BufferNum() == 0)
        return OMX_ErrorNoMore;

	if(pVideoBufferHdr == NULL && ports[VIDEO_PORT]->BufferNum() > 0) {
		ports[VIDEO_PORT]->GetBuffer(&pVideoBufferHdr);
		if(pVideoBufferHdr == NULL)
			return OMX_ErrorUnderflow;
	}

    if(pAudioBufferHdr == NULL && ports[AUDIO_PORT]->BufferNum() > 0) {
        ports[AUDIO_PORT]->GetBuffer(&pAudioBufferHdr);
        if(pAudioBufferHdr == NULL)
			return OMX_ErrorUnderflow;
	}

	if (pAudioBufferHdr != NULL) {
		write_data(pOutFile0, pOutFile1, pAudioBufferHdr, AUDIO_PORT);
		LOG_DEBUG("Audio TS: %lld\n", pAudioBufferHdr->nTimeStamp);
		ports[AUDIO_PORT]->SendBuffer(pAudioBufferHdr);
		pAudioBufferHdr = NULL;
	}

	if (pVideoBufferHdr != NULL) {
		write_data(pOutFile0, pOutFile1, pVideoBufferHdr, VIDEO_PORT);
		LOG_DEBUG("Video TS: %lld\n", pVideoBufferHdr->nTimeStamp);
		ports[VIDEO_PORT]->SendBuffer(pVideoBufferHdr);
		pVideoBufferHdr = NULL;
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE FileWrite::ComponentReturnBuffer(
        OMX_U32 nPortIndex)
{
    if(nPortIndex == AUDIO_PORT && pAudioBufferHdr != NULL) {
        ports[AUDIO_PORT]->SendBuffer(pAudioBufferHdr);
        pAudioBufferHdr = NULL;
    }

    if(nPortIndex == VIDEO_PORT && pVideoBufferHdr != NULL) {
        ports[VIDEO_PORT]->SendBuffer(pVideoBufferHdr);
        pVideoBufferHdr = NULL;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FileWrite::FlushComponent(
        OMX_U32 nPortIndex)
{
    if(nPortIndex == AUDIO_PORT && pAudioBufferHdr != NULL) {
        ports[AUDIO_PORT]->SendBuffer(pAudioBufferHdr);
        pAudioBufferHdr = NULL;
    }

    if(nPortIndex == VIDEO_PORT && pVideoBufferHdr != NULL) {
        ports[VIDEO_PORT]->SendBuffer(pVideoBufferHdr);
        pVideoBufferHdr = NULL;
    }

    return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE FileWriteInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        FileWrite *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(FileWrite, ());
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
