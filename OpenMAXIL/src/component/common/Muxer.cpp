/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Muxer.h"

#define MAX_INTERLEAVE (5*OMX_TICKS_PER_SECOND)

FslFileHandle OpenContentCb(
        const uint8 *fileName, 
        const uint8 *mode, 
        void *context)
{
    Muxer *hMuxer = (Muxer*)context;
    if(hMuxer == NULL)
        return NULL;

    return hMuxer->OpenContent(CP_AccessWrite);
}

int32 CloseContentCb(
        FslFileHandle handle, 
        void *context) 
{
    Muxer *hMuxer = (Muxer*)context;
    if(hMuxer == NULL)
        return -1;

    hMuxer->CloseContent(handle);

    return 0;
}

uint32 WriteContentCb(
        FslFileHandle handle, 
        void *buffer, 
        uint32 size, 
        void *context)
{
    Muxer *hMuxer = (Muxer*)context;
    if(hMuxer == NULL)
        return 0;

    return hMuxer->WriteContent(handle, buffer, size);
}

int32 SeekContentCb(
        FslFileHandle handle, 
        int64 offset, 
        int32 whence, 
        void *context)
{
    Muxer *hMuxer = (Muxer*)context;
    if(hMuxer == NULL)
        return -1;

    CP_ORIGINTYPE eOrigin = CP_OriginBegin;
    if(whence == FSL_SEEK_SET)
        eOrigin = CP_OriginBegin;
    else if(whence == FSL_SEEK_CUR)
        eOrigin = CP_OriginCur;
    else if(whence == FSL_SEEK_END)
        eOrigin = CP_OriginEnd;

    if(OMX_ErrorNone != hMuxer->SeekContent(handle, offset, eOrigin))
        return -1;

    return 0;
}

int64 TellContentCb(
        FslFileHandle handle, 
        void * context)
{
    Muxer *hMuxer = (Muxer*)context;
    if(hMuxer == NULL)
        return -1;

    return hMuxer->TellContent(handle);
}

void *CallocCb(uint32 numElements, uint32 size)
{
    OMX_PTR ptr = FSL_MALLOC(numElements * size);
    if(ptr != NULL)
        fsl_osal_memset(ptr, 0, numElements * size);

    return ptr;
}

void *MallocCb(uint32 size)
{
    return FSL_MALLOC(size);
}

void FreeCb(void * ptr)
{
    FSL_FREE(ptr);
}

void *ReAllocCb(void * ptr, uint32 size)
{
    return FSL_REALLOC(ptr, size);
}

Muxer::Muxer()
{
    bInContext = OMX_FALSE;
}

OMX_PTR Muxer::OpenContent(CP_ACCESSTYPE eAccess)
{
    if(pURL == NULL || hPipe == NULL)
        return NULL;

    hContent = NULL;
    if(0 != hPipe->Open(&hContent, (CPstring)pURL, eAccess))
        return NULL;

    return hContent;
}

OMX_ERRORTYPE Muxer::CloseContent(OMX_PTR hContent)
{
    if(hContent == NULL || hPipe == NULL)
        return OMX_ErrorBadParameter;

    hPipe->Close(hContent);

    return OMX_ErrorNone;
}

OMX_U32 Muxer::WriteContent(
        OMX_PTR hContent, 
        OMX_PTR pBuffer, 
        OMX_S32 nSize)
{
    if(hContent == NULL || hPipe == NULL)
        return 0;

	CP_CHECKBYTESRESULTTYPE eResult;
	if(nEosMask == 0) {
		hPipe->CheckAvailableBytes(hContent, nSize, &eResult);
		if (eResult != CP_CheckBytesOk)
			return 0;
	} else {
		hPipe->CheckAvailableBytes(hContent, nSize + nHeadSize, &eResult);
		if (eResult != CP_CheckBytesOk) {
			LOG_WARNING("Not enough memory.\n");
			return 0;
		}
	}

	nTotalWrite += nSize;
	if(hPipe->Write(hContent, (CPbyte*)pBuffer, nSize) != 0)
		return 0;
    else
        return nSize;
}

OMX_ERRORTYPE Muxer::SeekContent(
        OMX_PTR hContent, 
        OMX_S64 nOffset, 
        CP_ORIGINTYPE eOrigin)
{
    if(hContent == NULL || hPipe == NULL)
        return OMX_ErrorBadParameter;

    if(0 != hPipe->SetPosition(hContent, nOffset, eOrigin))
        return OMX_ErrorUndefined;

    return OMX_ErrorNone;
}

OMX_S64 Muxer::TellContent(OMX_PTR hContent)
{
    if(hContent == NULL || hPipe == NULL)
        return -1;

    OMX_S64 nPos = 0;
    if(0 != hPipe->GetPosition(hContent, &nPos))
        return -1;

    return nPos;
}

OMX_ERRORTYPE Muxer::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    fsl_osal_memset(pBufferHdr, 0, sizeof(OMX_BUFFERHEADERTYPE) * MAX_PORTS);
    fsl_osal_memset(Tracks, 0, sizeof(TRACK_INFO) * MAX_PORTS);
    fsl_osal_memcpy(TrackParameter, 0, sizeof(TrackParameter) * MAX_PORTS);
    nMinTrackTs = 0;
    nEosMask = 0;
    nAddSampleMask = 0;
    nGetCodecDataMask = 0;
    nMaxInterleave = MAX_INTERLEAVE;
    bCodecDataAdded = OMX_FALSE;
    hPipe = NULL;
    pURL = NULL;
    nURLLen = 0;
	nTotalWrite = 0;
	nHeadSize = 0;
	nMaxFileSize = MAX_VALUE_S64;
	nLongitudex = -3600000;
	nLatitudex= -3600000;
	bAddSampleDone = OMX_FALSE;

    fsl_osal_memset(&streamOps, 0, sizeof(FslFileStream));
    streamOps.Open = OpenContentCb;
    streamOps.Close = CloseContentCb;
    streamOps.Write = WriteContentCb;
    streamOps.Seek = SeekContentCb;
    streamOps.Tell = TellContentCb;
    
    fsl_osal_memcpy(&memOps, 0, sizeof(MuxerMemoryOps));
    memOps.Calloc = CallocCb;
    memOps.Malloc = MallocCb;
    memOps.ReAlloc = ReAllocCb;
    memOps.Free = FreeCb;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.eDir = OMX_DirInput;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_FALSE;

    OMX_U32 i;
    for(i=0; i<nPorts; i++) {
        sPortDef.nPortIndex = i;
        sPortDef.eDomain = port_info[i].eDomain;
        sPortDef.nBufferCountMin = port_info[i].nBufferCount;
        sPortDef.nBufferCountActual = port_info[i].nBufferCount;
        sPortDef.nBufferSize = port_info[i].nBufferSize;
        ret = ports[i]->SetPortDefinition(&sPortDef);
        if(ret != OMX_ErrorNone) {
            LOG_ERROR("Set port definition for port #%d failed.\n", i);
            return ret;
        }
    }

    ret = InitMuxerComponent();
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("InitMuxerComponent failed, ret = %x\n", ret);
        return ret;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::DeInitComponent()
{
    DeInitMuxerComponent();

    if(pURL != NULL)
        FSL_FREE(pURL);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(hPipe == NULL) {
        ret = OMX_GetContentPipe((void **)&hPipe, (OMX_STRING)"LOCAL_FILE_PIPE_NEW");
        if(hPipe == NULL || ret != OMX_ErrorNone) {
            LOG_ERROR("Get LOCAL_FILE_PIPE_NEW failed.\n");
            return OMX_ErrorInsufficientResources;
        }
    }

    ret = InitMuxer();
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("InitMuxer failed, ret = %x\n", ret);
        return ret;
    }

    ret = InitTracks();
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("InitTracks failed, ret = %x\n", ret);
        return ret;
    }

    ret = InitMetaData();
    if(ret != OMX_ErrorNone)
        LOG_WARNING("InitMetaData failed, ret = %x\n", ret);

    ret = InitDone();
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("InitDone failed, ret = %x\n", ret);
        return ret;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::InstanceDeInit()
{
    DeInitMuxer();

    if(pURL != NULL)
        FSL_FREE(pURL);

    OMX_U32 i = 0;
    for(i=0; i<nPorts; i++) {
        if(Tracks[i].CodecData.pBuffer != NULL)
            FSL_FREE(Tracks[i].CodecData.pBuffer);

        while (Tracks[i].ReadyBuffers.GetNodeCnt() > 0) {
            TRACK_DATA *pData = NULL;
            pData = Tracks[i].ReadyBuffers.GetNode(0);
            if(pData != NULL) {
                Tracks[i].ReadyBuffers.Remove(pData);
                FSL_FREE(pData->pBuffer);
                FSL_FREE(pData);
            }
        }

        while (Tracks[i].FreeBuffers.GetNodeCnt() > 0) {
            TRACK_DATA *pData = NULL;
            pData = Tracks[i].FreeBuffers.GetNode(0);
            if(pData != NULL) {
                Tracks[i].FreeBuffers.Remove(pData);
                FSL_FREE(pData->pBuffer);
                FSL_FREE(pData);
            }
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::ReturnAllBuffer()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i = 0;

    for(i=0; i<nPorts; i++) {
		if (pBufferHdr[i] != NULL) {
			ports[i]->SendBuffer(pBufferHdr[i]);
			pBufferHdr[i] = NULL;
		}
        while (ports[i]->BufferNum()) {
			ports[i]->GetBuffer(&pBufferHdr[i]);
			if(pBufferHdr[i] == NULL) {
                LOG_ERROR("Should not be here, %s:%d\n", __FUNCTION__, __LINE__);
                continue;
            }
			ports[i]->SendBuffer(pBufferHdr[i]);
			pBufferHdr[i] = NULL;
		}
	}

	return ret;
}

OMX_ERRORTYPE Muxer::ProcessDataBuffer()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(nEosMask == 0) 
        return OMX_ErrorNoMore;

    ret = TakeOneBufferFromPort();
    if(ret != OMX_ErrorNone)
        return ret;

    if(nGetCodecDataMask != 0)
        return OMX_ErrorNone;

    if(bCodecDataAdded != OMX_TRUE) {
        AddCodecData();
        AddTracksDone();
        bCodecDataAdded = OMX_TRUE;
    }

    OMX_U32 i;
	for(i=0; i<nPorts; i++) {
		if(Tracks[i].bTrackAdded != OMX_TRUE)
			continue;

        TRACK_DATA *pData = NULL;
        pData = GetOneTrackData(i);
        if(pData != NULL) {
			nHeadSize = GetTrailerSize();
			LOG_DEBUG("nHeadSize: %d nTotalWrite: %lld pData->nFilledLen: %d\n", \
					nHeadSize, nTotalWrite, pData->nFilledLen);
			if(nTotalWrite + pData->nFilledLen + nHeadSize >= nMaxFileSize \
					&& bAddSampleDone == OMX_FALSE) {
				LOG_WARNING("Not enough memory.\n");
				bAddSampleDone = OMX_TRUE;
				SeekContent(hContent, nMaxFileSize, CP_OriginBegin);
				if (nTotalWrite + nHeadSize > nMaxFileSize || nAddSampleMask != 0)
					SendEvent(OMX_EventError, OMX_ErrorStreamCorrupt, 0, NULL);
				else
					SendEvent(OMX_EventBufferFlag, 0, OMX_BUFFERFLAG_MAX_FILESIZE, NULL);

				break;
			}

			if (nMaxFileSize == MAX_VALUE_S64) {
				ret = SeekContent(hContent, nTotalWrite + pData->nFilledLen \
						+ nHeadSize, CP_OriginBegin);
				if (ret != OMX_ErrorNone && bAddSampleDone == OMX_FALSE) {
					LOG_WARNING("Not enough memory.\n");
					bAddSampleDone = OMX_TRUE;
					ret = SeekContent(hContent, nTotalWrite + nHeadSize, CP_OriginBegin);
					if (ret != OMX_ErrorNone || nAddSampleMask != 0)
						SendEvent(OMX_EventError, OMX_ErrorStreamCorrupt, 0, NULL);
					else
						SendEvent(OMX_EventBufferFlag, 0, OMX_BUFFERFLAG_MAX_FILESIZE, NULL);
					break;
				}
			}

			if (bAddSampleDone == OMX_FALSE) {
				if(OMX_ErrorInsufficientResources == AddOneSample(i, pData)) {
					bAddSampleDone = OMX_TRUE;
					SendEvent(OMX_EventBufferFlag, 0, OMX_BUFFERFLAG_MAX_FILESIZE, NULL);
					break;
				}
                nAddSampleMask &= ~(1 << i);
			}

            if(pData->nFlags & OMX_BUFFERFLAG_EOS) {
                nEosMask &= ~(1 << i);
				if(nEosMask == 0)
					AddSampleDone();
				SendEvent(OMX_EventBufferFlag, i, OMX_BUFFERFLAG_EOS, NULL);
			}
            ReturnTrackData(i, pData);
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::CheckEOS(OMX_S32 i)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(nEosMask == 0)
        AddSampleDone();
	SendEvent(OMX_EventBufferFlag, i, OMX_BUFFERFLAG_EOS, NULL);

	return ret;
}

OMX_ERRORTYPE Muxer::HandleSDFull()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	for(OMX_U32 i=0; i<nPorts; i++) {
		nEosMask &= ~(1 << i);
		LOG_DEBUG("Track %d EOS\n");
		CheckEOS(i);
	}

	return ret;
}

OMX_ERRORTYPE Muxer::ComponentReturnBuffer(
        OMX_U32 nPortIndex)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::FlushComponent(
        OMX_U32 nPortIndex)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 trackId = 0;

    switch(nParamIndex) {
        case OMX_IndexParamCustomContentPipe:
            hPipe =(CP_PIPETYPE* ) (((OMX_PARAM_CONTENTPIPETYPE *)pStructure)->hPipe);
            break;
        case OMX_IndexParamContentURI:
            {
                OMX_PARAM_CONTENTURITYPE * pContentURI = (OMX_PARAM_CONTENTURITYPE *)pStructure;

                nURLLen = fsl_osal_strlen((const char *)&(pContentURI->contentURI)) + 1;
                pURL = (OMX_PTR)FSL_MALLOC(nURLLen);
                if (pURL == NULL) {
                    ret = OMX_ErrorInsufficientResources;
                    break;
                }
                fsl_osal_strcpy((char *)pURL, (const char *)&(pContentURI->contentURI));
            }
            break;
        case OMX_IndexParamAudioPcm:
            {
                OMX_AUDIO_PARAM_PCMMODETYPE *pPcmPara = (OMX_AUDIO_PARAM_PCMMODETYPE*)pStructure;
                OMX_CHECK_STRUCT(pPcmPara, OMX_AUDIO_PARAM_PCMMODETYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                trackId = pPcmPara->nPortIndex;
                fsl_osal_memcpy(&TrackParameter[trackId].PcmPara, pPcmPara, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE)); 
            }
            break;
        case OMX_IndexParamAudioMp3:
            {
                OMX_AUDIO_PARAM_MP3TYPE *pMp3Para = (OMX_AUDIO_PARAM_MP3TYPE*)pStructure;
                OMX_CHECK_STRUCT(pMp3Para, OMX_AUDIO_PARAM_MP3TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                trackId = pMp3Para->nPortIndex;
                fsl_osal_memcpy(&TrackParameter[trackId].Mp3Para, pMp3Para, sizeof(*pMp3Para)); 
            }
            break;
        case OMX_IndexParamAudioAmr:
            {
                OMX_AUDIO_PARAM_AMRTYPE *pAmrPara = (OMX_AUDIO_PARAM_AMRTYPE*)pStructure;
                OMX_CHECK_STRUCT(pAmrPara, OMX_AUDIO_PARAM_AMRTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                trackId = pAmrPara->nPortIndex;
                fsl_osal_memcpy(&TrackParameter[trackId].AmrPara, pAmrPara, sizeof(*pAmrPara)); 
            }
            break;
        case OMX_IndexParamAudioAac:
            {
                OMX_AUDIO_PARAM_AACPROFILETYPE *pAacProfile = (OMX_AUDIO_PARAM_AACPROFILETYPE*)pStructure;
                OMX_CHECK_STRUCT(pAacProfile, OMX_AUDIO_PARAM_AACPROFILETYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                trackId = pAacProfile->nPortIndex;
                fsl_osal_memcpy(&TrackParameter[trackId].AacProfile, pAacProfile, sizeof(*pAacProfile)); 
            }
            break;
        case OMX_IndexParamAudioWma:
            {
                OMX_AUDIO_PARAM_WMATYPE *pWmaPara = (OMX_AUDIO_PARAM_WMATYPE*)pStructure;
                OMX_CHECK_STRUCT(pWmaPara, OMX_AUDIO_PARAM_WMATYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                trackId = pWmaPara->nPortIndex;
                fsl_osal_memcpy(&TrackParameter[trackId].WmaPara, pWmaPara, sizeof(*pWmaPara)); 
            }
            break;
        case OMX_IndexParamVideoH263:
            {
                OMX_VIDEO_PARAM_H263TYPE *pH263Para = (OMX_VIDEO_PARAM_H263TYPE*)pStructure;
                OMX_CHECK_STRUCT(pH263Para, OMX_VIDEO_PARAM_H263TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                trackId = pH263Para->nPortIndex;
                fsl_osal_memcpy(&TrackParameter[trackId].H263Para, pH263Para, sizeof(*pH263Para)); 
            }
            break;
        case OMX_IndexParamVideoMpeg4:
            {
                OMX_VIDEO_PARAM_MPEG4TYPE *pMpeg4Para = (OMX_VIDEO_PARAM_MPEG4TYPE*)pStructure;
                OMX_CHECK_STRUCT(pMpeg4Para, OMX_VIDEO_PARAM_MPEG4TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                trackId = pMpeg4Para->nPortIndex;
                fsl_osal_memcpy(&TrackParameter[trackId].Mpeg4Para, pMpeg4Para, sizeof(*pMpeg4Para)); 
            }
            break;
        case OMX_IndexParamVideoAvc:
            {
                OMX_VIDEO_PARAM_AVCTYPE *pAvcPara = (OMX_VIDEO_PARAM_AVCTYPE*)pStructure;
                OMX_CHECK_STRUCT(pAvcPara, OMX_VIDEO_PARAM_AVCTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                trackId = pAvcPara->nPortIndex;
                fsl_osal_memcpy(&TrackParameter[trackId].AvcPara, pAvcPara, sizeof(*pAvcPara)); 
            }
            break;
        case OMX_IndexParamMaxFileSize:
            {
				 nMaxFileSize = *((OMX_TICKS*)pStructure);
            }
            break;
        case OMX_IndexParamLongitudex:
            {
				 nLongitudex = *((OMX_S64*)pStructure);
            }
            break;
        case OMX_IndexParamLatitudex:
            {
				 nLatitudex= *((OMX_S64*)pStructure);
            }
            break;
        default:
            ret = OMX_ErrorNotImplemented;
            break;
    }

    if(ret == OMX_ErrorNone)
        Tracks[trackId].bTrackSetted = OMX_TRUE;

    return ret;
}

OMX_ERRORTYPE Muxer::InitTracks()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_U32 i;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    for(i=0; i<nPorts; i++) {
        if(ports[i]->IsEnabled() == OMX_TRUE) {
            if(port_info[i].eDomain == OMX_PortDomainAudio)
                ret = InitAudioTrack(i);
            else if(port_info[i].eDomain == OMX_PortDomainVideo)
                ret = InitVideoTrack(i);

            if(ret == OMX_ErrorNone) {
                Tracks[i].bTrackAdded = OMX_TRUE;
                nEosMask |= 1 << i;
                nAddSampleMask |= 1 << i;
                nGetCodecDataMask |= 1 << i;
            }
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::InitMetaData()
{
	if (nLongitudex> -3600000 && nLatitudex> -3600000) {
		AddLocationInfo(nLongitudex, nLatitudex);
	}

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::InitDone()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::InitAudioTrack(OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BOOL bParamSetted = Tracks[nPortIndex].bTrackSetted;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    ports[nPortIndex]->GetPortDefinition(&sPortDef);

    switch(sPortDef.format.audio.eEncoding) {
        case OMX_AUDIO_CodingPCM:
            {
                if(bParamSetted != OMX_TRUE) {
                    LOG_ERROR("No parameters for PCM track #%d\n", nPortIndex);
                    ret = OMX_ErrorUndefined;
                    break;
                }
                ret = AddPcmTrack(nPortIndex, &TrackParameter[nPortIndex].PcmPara);
                LOG_DEBUG("Add PCM to track #%d, ret = %x\n", nPortIndex, ret);
            }
            break;
        case OMX_AUDIO_CodingMP3:
            {
                OMX_AUDIO_PARAM_MP3TYPE *pMp3Para = NULL;
                if(bParamSetted == OMX_TRUE)
                    pMp3Para = &TrackParameter[nPortIndex].Mp3Para;
                ret = AddMp3Track(nPortIndex, pMp3Para);
                LOG_DEBUG("Add MP3 to track #%d, ret = %x\n", nPortIndex, ret);
            }
            break;
        case OMX_AUDIO_CodingAMR:
            {
                OMX_AUDIO_PARAM_AMRTYPE *pAmrPara = NULL;
                if(bParamSetted == OMX_TRUE)
                    pAmrPara = &TrackParameter[nPortIndex].AmrPara;
                ret = AddAmrTrack(nPortIndex, pAmrPara);
                LOG_DEBUG("Add AMR to track #%d, ret = %x\n", nPortIndex, ret);
            }
            break;
        case OMX_AUDIO_CodingAAC:
            {
                OMX_AUDIO_PARAM_AACPROFILETYPE *pAacProfile = NULL;
                if(bParamSetted == OMX_TRUE)
                    pAacProfile = &TrackParameter[nPortIndex].AacProfile;
                ret = AddAacTrack(nPortIndex, pAacProfile);
                LOG_DEBUG("Add AAC to track #%d, ret = %x\n", nPortIndex, ret);
            }
            break;
        case OMX_AUDIO_CodingWMA:
            {
                OMX_AUDIO_PARAM_WMATYPE *pWmaPara = NULL;
                if(bParamSetted == OMX_TRUE)
                    pWmaPara = &TrackParameter[nPortIndex].WmaPara;
                    ret = AddWmaTrack(nPortIndex, pWmaPara);
                LOG_DEBUG("Add WMA to track #%d, ret = %x\n", nPortIndex, ret);
            }
            break;
        default:
            ret = OMX_ErrorUndefined;
            break;
    }

    return ret;
}

OMX_ERRORTYPE Muxer::InitVideoTrack(OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BOOL bParamSetted = Tracks[nPortIndex].bTrackSetted;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    ports[nPortIndex]->GetPortDefinition(&sPortDef);

    switch(sPortDef.format.video.eCompressionFormat) {
        case OMX_VIDEO_CodingH263:
            {
                OMX_VIDEO_PARAM_H263TYPE *pH263Para = NULL;
                if(bParamSetted == OMX_TRUE)
                    pH263Para = &TrackParameter[nPortIndex].H263Para;
                ret = AddH263Track(nPortIndex, &sPortDef.format.video, pH263Para);
                LOG_DEBUG("Add H263 to track #%d, ret = %x\n", nPortIndex, ret);
            }
            break;
        case OMX_VIDEO_CodingMPEG4:
            {
                OMX_VIDEO_PARAM_MPEG4TYPE *pMpeg4Para = NULL;
                if(bParamSetted == OMX_TRUE)
                    pMpeg4Para = &TrackParameter[nPortIndex].Mpeg4Para;
                ret = AddMpeg4Track(nPortIndex, &sPortDef.format.video, pMpeg4Para);
                LOG_DEBUG("Add MPEG4 to track #%d, ret = %x\n", nPortIndex, ret);
            }
            break;
        case OMX_VIDEO_CodingAVC:
            {
                OMX_VIDEO_PARAM_AVCTYPE *pAvcPara = NULL;
                if(bParamSetted == OMX_TRUE)
                    pAvcPara = &TrackParameter[nPortIndex].AvcPara;
                ret = AddAvcTrack(nPortIndex, &sPortDef.format.video, pAvcPara);
                LOG_DEBUG("Add AVC to track #%d, ret = %x\n", nPortIndex, ret);
            }
            break;
        default:
            ret = OMX_ErrorUndefined;
            break;
    }

    return ret;
}

OMX_ERRORTYPE Muxer::InitTrackMetaData(OMX_U32 nPortIndex)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::ProcessTrackCodecData(OMX_U32 track)
{
    OMX_U32 i = track;

    LOG_DEBUG("Get Codec config data, len: %d\n", pBufferHdr[i]->nFilledLen);
    if(Tracks[i].CodecData.pBuffer == NULL) {
        Tracks[i].CodecData.pBuffer = FSL_MALLOC(pBufferHdr[i]->nFilledLen);
        if(Tracks[i].CodecData.pBuffer != NULL) {
            fsl_osal_memcpy(Tracks[i].CodecData.pBuffer, pBufferHdr[i]->pBuffer, pBufferHdr[i]->nFilledLen);
            Tracks[i].CodecData.nFilledLen = pBufferHdr[i]->nFilledLen;
        }
    }
    else {
        Tracks[i].CodecData.pBuffer = FSL_REALLOC(Tracks[i].CodecData.pBuffer, 
                Tracks[i].CodecData.nFilledLen + pBufferHdr[i]->nFilledLen);
        if(Tracks[i].CodecData.pBuffer != NULL) {
            fsl_osal_memcpy((OMX_U8*)Tracks[i].CodecData.pBuffer + Tracks[i].CodecData.nFilledLen, 
                    pBufferHdr[i]->pBuffer, pBufferHdr[i]->nFilledLen);
            Tracks[i].CodecData.nFilledLen += pBufferHdr[i]->nFilledLen;
        }
    }
    ports[i]->SendBuffer(pBufferHdr[i]);
    pBufferHdr[i] = NULL;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::AddCodecData()
{
    OMX_U32 i;

    for(i=0; i<nPorts; i++) {
        if(Tracks[i].CodecData.nFilledLen > 0)
            AddTrackCodecData(i, Tracks[i].CodecData.pBuffer, Tracks[i].CodecData.nFilledLen);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::TakeOneBufferFromPort()
{
    OMX_BOOL bNoBuffer = OMX_TRUE;
    OMX_U32 i = 0;

    nMinTrackTs = -1;

    for(i=0; i<nPorts; i++) {
        OMX_U32 nReadyBuffers = Tracks[i].ReadyBuffers.GetNodeCnt();
        OMX_U32 nPortBuffers = ports[i]->BufferNum();

		LOG_LOG("Port: %d buffer num: %d\n", i, nPortBuffers);
        if(nPortBuffers == 0 && nReadyBuffers == 0 && pBufferHdr[i] == NULL)
            continue;

        bNoBuffer = OMX_FALSE;

        if(pBufferHdr[i] == NULL && nPortBuffers > 0) {
            ports[i]->GetBuffer(&pBufferHdr[i]);
			LOG_LOG("Input buffer TS: %lld\n", pBufferHdr[i]->nTimeStamp);
            if(pBufferHdr[i] == NULL) {
                LOG_ERROR("Should not be here, %s:%d\n", __FUNCTION__, __LINE__);
                continue;
            }
        }

        if(pBufferHdr[i] != NULL && pBufferHdr[i]->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
            ProcessTrackCodecData(i);
            continue;
        }

        nGetCodecDataMask &= ~(1 << i);

        if(nReadyBuffers > 0) {
            if(pBufferHdr[i] != NULL)
                AddToReadyBuffers(i);

            TRACK_DATA *pData = Tracks[i].ReadyBuffers.GetNode(0);
            if(pData == NULL) {
                LOG_ERROR("Should not be here, %s:%d\n", __FUNCTION__, __LINE__);
                continue;
            }
            if(!pData->nFlags & OMX_BUFFERFLAG_EOS)
                Tracks[i].nHeadTs = pData->nTimeStamp;
        }
        else {
            if(!pBufferHdr[i]->nFlags & OMX_BUFFERFLAG_EOS)
                Tracks[i].nHeadTs = pBufferHdr[i]->nTimeStamp;
        }

        nMinTrackTs = MIN((OMX_U64)nMinTrackTs, (OMX_U64)(Tracks[i].nHeadTs));
    }

    if(bNoBuffer == OMX_TRUE)
        return OMX_ErrorNoMore;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::AddToReadyBuffers(OMX_U32 track)
{
    OMX_U32 i = track;
    if(pBufferHdr[i] == NULL) {
        LOG_ERROR("No buffer need to add to ready buffer list.\n");
        return OMX_ErrorUndefined;
    }

    TRACK_DATA *pData = GetEmptyTrackData(i, pBufferHdr[i]->nFilledLen);
    if(pData == NULL) {
        LOG_ERROR("Can't get a free track data node, drop this buffer.\n");
        ports[i]->SendBuffer(pBufferHdr[i]);
        pBufferHdr[i] = NULL;
        return OMX_ErrorNone;
    }

    pData->nTimeStamp = pBufferHdr[i]->nTimeStamp;
    pData->nFlags = pBufferHdr[i]->nFlags;
    pData->nFilledLen = pBufferHdr[i]->nFilledLen;
    fsl_osal_memcpy(pData->pBuffer, pBufferHdr[i]->pBuffer, pBufferHdr[i]->nFilledLen);
    Tracks[i].ReadyBuffers.Add(pData);
    ports[i]->SendBuffer(pBufferHdr[i]);
    pBufferHdr[i] = NULL;

    return OMX_ErrorNone;
}

TRACK_DATA * Muxer::GetEmptyTrackData(
        OMX_U32 track, 
        OMX_U32 nSize)
{
    TRACK_DATA *pData = NULL;
    OMX_U32 i = track;

    if(Tracks[i].FreeBuffers.GetNodeCnt() > 0) {
        pData = Tracks[i].FreeBuffers.GetNode(0);
        if(pData == NULL) {
            LOG_ERROR("Should not be here, %s:%d\n", __FUNCTION__, __LINE__);
            return NULL;
        }
        Tracks[i].FreeBuffers.Remove(pData);

        if(pData->nAllocLen >= nSize)
            return pData;

        pData->pBuffer = FSL_REALLOC(pData->pBuffer, nSize);
        if(pData->pBuffer == NULL) {
            LOG_ERROR("Failed to reallocate memory for track data, size changed from %d to %d\n", pData->nAllocLen, nSize);
            FSL_FREE(pData);
            return NULL;
        }
        pData->nAllocLen = nSize;
    }
    else {
        pData = (TRACK_DATA*)FSL_MALLOC(sizeof(TRACK_DATA));
        if(pData == NULL) {
            LOG_ERROR("Failed to allcoate one track data node.\n");
            return NULL;
        }
        fsl_osal_memset(pData, 0, sizeof(TRACK_DATA));
        pData->pBuffer = (OMX_PTR)FSL_MALLOC(nSize);
        if(pData->pBuffer == NULL) {
            LOG_ERROR("Failed to allcoate one track data buffer, size: %d.\n", nSize);
            FSL_FREE(pData);
            return NULL;
        }
        pData->nAllocLen = nSize;
    }

    return pData;
}

TRACK_DATA * Muxer::GetOneTrackData(OMX_U32 track)
{
    OMX_U32 i = track;

    if((Tracks[i].nHeadTs - nMinTrackTs) > nMaxInterleave) {
        LOG_DEBUG("Track %d is ahead of max interleave %lld, head: %lld, min: %lld\n",
                i, nMaxInterleave, Tracks[i].nHeadTs, nMinTrackTs);
        if(pBufferHdr[i] != NULL)
            AddToReadyBuffers(i);
        return NULL;
    }

    if(pBufferHdr[i] != NULL) {
        Tracks[i].TrackBuffer.nTimeStamp = pBufferHdr[i]->nTimeStamp;
        Tracks[i].TrackBuffer.nFlags = pBufferHdr[i]->nFlags; 
        Tracks[i].TrackBuffer.nFilledLen = pBufferHdr[i]->nFilledLen; 
        Tracks[i].TrackBuffer.pBuffer = pBufferHdr[i]->pBuffer; 
        return &(Tracks[i].TrackBuffer);
    }

    if(Tracks[i].ReadyBuffers.GetNodeCnt() == 0)
        return NULL;

    TRACK_DATA *pData = Tracks[i].ReadyBuffers.GetNode(0);
    if(pData == NULL) {
        LOG_ERROR("Should not be here, %s:%d\n", __FUNCTION__, __LINE__);
        return NULL;
    }
    Tracks[i].ReadyBuffers.Remove(pData);

    return pData;
}

OMX_ERRORTYPE Muxer::ReturnTrackData(
        OMX_U32 track,
        TRACK_DATA *pData)
{
    OMX_U32 i = track;

    if(pBufferHdr[i] != NULL) {
        fsl_osal_memset((OMX_PTR)(&(Tracks[i].TrackBuffer)), 0, sizeof(TRACK_DATA));
        ports[i]->SendBuffer(pBufferHdr[i]);
        pBufferHdr[i] = NULL;
        return OMX_ErrorNone;
    }

    pData->nTimeStamp = 0;
    pData->nFlags = 0;
    pData->nFilledLen = 0;

    Tracks[i].FreeBuffers.Add(pData);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::InitMuxerComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::DeInitMuxerComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Muxer::AddLocationInfo(
		OMX_S64 nLongitudex, 
		OMX_S64 nLatitudex)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Muxer::AddPcmTrack(
        OMX_U32 track, 
        OMX_AUDIO_PARAM_PCMMODETYPE *pParameter)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Muxer::AddMp3Track(
        OMX_U32 track, 
        OMX_AUDIO_PARAM_MP3TYPE *pParameter)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Muxer::AddAmrTrack(
        OMX_U32 track, 
        OMX_AUDIO_PARAM_AMRTYPE *pParameter)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Muxer::AddAacTrack(
        OMX_U32 track, 
        OMX_AUDIO_PARAM_AACPROFILETYPE *pParameter)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Muxer::AddWmaTrack(
        OMX_U32 track, 
        OMX_AUDIO_PARAM_WMATYPE *pParameter)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Muxer::AddH263Track(
        OMX_U32 track, 
        OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, 
        OMX_VIDEO_PARAM_H263TYPE *pParameter)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Muxer::AddMpeg4Track(
        OMX_U32 track, 
        OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, 
        OMX_VIDEO_PARAM_MPEG4TYPE *pParameter)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Muxer::AddAvcTrack(
        OMX_U32 track, 
        OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, 
        OMX_VIDEO_PARAM_AVCTYPE *pParameter)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE Muxer::AddTracksDone()
{
    return OMX_ErrorNone;
}

OMX_U32 Muxer::GetTrailerSize()
{
	return 0;
}

OMX_ERRORTYPE Muxer::AddSampleDone()
{
    return OMX_ErrorNone;
}

/* File EOF */
