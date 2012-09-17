/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
#include "StreamingParser.h"
#include "avcodec.h"

#define PACKET_CACHE_SIZE (2*OMX_TICKS_PER_SECOND)
#define FREE_PKT(pkt) \
    do { \
        if (pkt->destruct) \
           pkt->destruct(pkt); \
        FSL_FREE(pkt); \
    }while(0);

extern AVInputFormat ff_rtsp_demuxer;
extern AVInputFormat ff_applehttp_demuxer;

static OMX_BOOL bAbortStreaming = OMX_FALSE;

static int abort_cb(void)
{
    if(bAbortStreaming == OMX_TRUE) {
        return 1;
    }

    return 0;
}

static void *DoReadPacket(void *arg)
{
    StreamingParser *h = (StreamingParser*) arg;
    while(OMX_ErrorNone == h->ReadPacketFromStream());
    return NULL;
}

StreamingParser::StreamingParser()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.parser.streaming.sw-based");
    ComponentVersion.s.nVersionMajor = 0x0;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x0;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"parser.rtsp";
    fsl_osal_strcpy((fsl_osal_char*)sCompRole.cRole, (fsl_osal_char*)role[0]);

    ic = NULL;
    pThread = NULL;
    bStopThread = OMX_FALSE;
    Lock = NULL;
    bStreamEos = OMX_FALSE;
    bPaused = OMX_FALSE;
    eStreamFmt = RTSP;

    bAbortStreaming = OMX_FALSE;

    OMX_S32 i;
    for(i=0; i<PORT_NUM; i++) {
        bSpecificDataSent[i] = OMX_FALSE;
        bNewSegment[i] = OMX_FALSE;
    }
}

OMX_ERRORTYPE StreamingParser::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = InitStreaming();
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("InitStreaming failed.\n");
        return ret;
    }

    ret = SetupPortMediaFormat();
    if(ret != OMX_ErrorNone) {
        DeInitStreaming();
        LOG_ERROR("SetupPortMediaFormat failed.\n");
        return ret;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StreamingParser::InstanceDeInit()
{
    if(pThread != NULL) {
        bStopThread = OMX_TRUE;
        fsl_osal_thread_destroy(pThread);
        pThread = NULL;
    }

    if(Lock != NULL) {
        fsl_osal_mutex_destroy(Lock);
        Lock = NULL;
    }

    if(bAudioActived == OMX_TRUE)
        pktCache[AUDIO_OUTPUT_PORT].DeInitCache();
    if(bVideoActived == OMX_TRUE)
        pktCache[VIDEO_OUTPUT_PORT].DeInitCache();

    if(eStreamFmt == RTSP)
        FreeAssemblingPackets();

    DeInitStreaming();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StreamingParser::GetNextSample(
        Port *pPort,
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    if(pThread == NULL) {
        if(bAudioActived == OMX_TRUE)
            pktCache[AUDIO_OUTPUT_PORT].InitCache();
        if(bVideoActived == OMX_TRUE)
            pktCache[VIDEO_OUTPUT_PORT].InitCache();

        if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&Lock, fsl_osal_mutex_normal)) {
            SendEvent(OMX_EventError, OMX_ErrorInsufficientResources, OMX_ALL, NULL);
            return OMX_ErrorInsufficientResources;
        }

        if(E_FSL_OSAL_SUCCESS != fsl_osal_thread_create(&pThread, NULL, DoReadPacket, this)) {
            SendEvent(OMX_EventError, OMX_ErrorInsufficientResources, OMX_ALL, NULL);
            return OMX_ErrorInsufficientResources;
        }
    }

    OMX_U32 nPortIndex = pBufferHdr->nOutputPortIndex;

    if(bSpecificDataSent[nPortIndex] != OMX_TRUE) {
        bSpecificDataSent[nPortIndex] = OMX_TRUE;
        if(OMX_ErrorNone == GetSpecificData(nPortIndex, pBufferHdr))
            return OMX_ErrorNone;
    }

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    if(OMX_FALSE != PacketCacheEmpty() && bStreamEos != OMX_TRUE) {
        SendEvent(OMX_EventBufferingData, 0, 0, NULL);
        while(OMX_FALSE != PacketCacheFree()) {
            fsl_osal_sleep(50000);
            if(bStreamEos == OMX_TRUE)
                break;
            if(OMX_TRUE == bHasCmdToProcess()) {
                ret = OMX_ErrorUndefined;
                break;
            }
            if(bAbortStreaming == OMX_TRUE) {
                ret = OMX_ErrorUndefined;
                break;
            }
        }
        SendEvent(OMX_EventBufferingDone, 0, 0, NULL);
    }

    if(ret != OMX_ErrorNone)
        return ret;

    AVPacket *pkt = pktCache[nPortIndex].GetOnePkt();
    DumpDataToOMXBuffer(pkt, pBufferHdr);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StreamingParser::DoSeek(
        OMX_U32 nPortIndex, 
        OMX_U32 nSeekFlag)
{
    if(nPortIndex == AUDIO_OUTPUT_PORT && bVideoActived == OMX_TRUE)
        return OMX_ErrorNone;

    fsl_osal_mutex_lock(Lock);

    LOG_DEBUG("DoSeek to %lld\n", sVideoSeekPos);

    if(bVideoActived == OMX_TRUE) {
        pktCache[VIDEO_OUTPUT_PORT].ResetCache();
        if(bAudioActived == OMX_TRUE)
            pktCache[AUDIO_OUTPUT_PORT].ResetCache();
        if(eStreamFmt == HTTPLIVE)
            ic->iformat->read_seek(ic, -1, sVideoSeekPos, 0);
        else if(eStreamFmt == RTSP)
            ic->iformat->read_seek(ic, nActiveVideoNum, OMXTs2Stream(sVideoSeekPos, nActiveVideoNum), 0);
    }
    else {
        pktCache[AUDIO_OUTPUT_PORT].ResetCache();
        if(eStreamFmt == HTTPLIVE)
            ic->iformat->read_seek(ic, -1, sAudioSeekPos, 0);
        else if(eStreamFmt == RTSP)
            ic->iformat->read_seek(ic, nActiveAudioNum, OMXTs2Stream(sAudioSeekPos, nActiveAudioNum), 0);
    }

    if(eStreamFmt == RTSP)
        FreeAssemblingPackets();

    bStreamEos = OMX_FALSE;

    fsl_osal_mutex_unlock(Lock);

    return OMX_ErrorNone;
}

void StreamingParser::SetAudioCodecType(
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDef, 
        uint32 stream_id)
{
    AVCodecContext *ffcodec = ic->streams[stream_id]->codec;

    switch(ffcodec->codec_id) {
        case CODEC_ID_MP2:
        case CODEC_ID_MP3:
            pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingMP3;
            break;
        case CODEC_ID_AAC:
            pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingAAC;
            break;
        case CODEC_ID_AMR_NB:
        case CODEC_ID_AMR_WB:
            pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingAMR;
            break;
        case CODEC_ID_PCM_U8:
        case CODEC_ID_PCM_S16LE:
        case CODEC_ID_PCM_S16BE:
        case CODEC_ID_PCM_S24LE:
        case CODEC_ID_PCM_S24BE:
        case CODEC_ID_PCM_S32LE:
        case CODEC_ID_PCM_S32BE:
            pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingPCM;
            break;
        default:
            pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingUnused;
            break;
    }

    nAudioDuration = StreamTs2OMX(ic->streams[stream_id]->duration, stream_id);
    LOG_DEBUG("Select audio stream %d, format is %d, duration: %lld.\n", 
            stream_id, pPortDef->format.audio.eEncoding, nAudioDuration);

    return;
}

void StreamingParser::SetVideoCodecType(
        OMX_PARAM_PORTDEFINITIONTYPE *pPortDef, 
        uint32 stream_id)
{
    AVCodecContext *ffcodec = ic->streams[stream_id]->codec;

    pPortDef->format.video.nFrameWidth = ffcodec->width;
    pPortDef->format.video.nFrameWidth = ffcodec->height;
    pPortDef->format.video.nBitrate = ffcodec->bit_rate;

    switch(ffcodec->codec_id) {
        case CODEC_ID_H264:
            pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
            break;
        case CODEC_ID_MPEG4:
            pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
            break;
        case CODEC_ID_H263:
            pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
            break;
        default:
            pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
            break;
    }

    nVideoDuration = StreamTs2OMX(ic->streams[stream_id]->duration, stream_id);
    LOG_DEBUG("Select video stream %d, format is %d, duration: %lld.\n", 
            stream_id, pPortDef->format.video.eCompressionFormat, nVideoDuration);

    return;
}

OMX_ERRORTYPE StreamingParser::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexParamAudioAac:
            {
                OMX_AUDIO_PARAM_AACPROFILETYPE *pParam = (OMX_AUDIO_PARAM_AACPROFILETYPE*)pComponentParameterStructure;
                OMX_S32 id = nActiveAudioNum;
                AVCodecContext *ffcodec = ic->streams[id]->codec;
                if(ffcodec->channels == 0 || ffcodec->sample_rate == 0)
                    return OMX_ErrorBadParameter;
                pParam->nChannels = ffcodec->channels;
                pParam->nSampleRate = ffcodec->sample_rate;
                pParam->eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;
                LOG_DEBUG("AAC, channel: %d, bitrate: %d, samplerate: %d\n", 
                        pParam->nChannels, pParam->nBitRate, pParam->nSampleRate);
            }
            break;	
        case OMX_IndexParamAudioAmr:
            {
                OMX_AUDIO_PARAM_AMRTYPE *pParam = (OMX_AUDIO_PARAM_AMRTYPE*)pComponentParameterStructure;
                OMX_S32 id = nActiveAudioNum;
                AVCodecContext *ffcodec = ic->streams[id]->codec;
                pParam->nChannels = ffcodec->channels;
                pParam->nBitRate = ffcodec->bit_rate;
                pParam->eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;
                if(ffcodec->codec_id == CODEC_ID_AMR_NB) {
                    pParam->eAMRBandMode = OMX_AUDIO_AMRBandModeNB0;
                    pParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
                    LOG_DEBUG("AMR-NB, "); 
                }
                if(ffcodec->codec_id == CODEC_ID_AMR_WB) {
                    pParam->eAMRBandMode = OMX_AUDIO_AMRBandModeWB0;
                    pParam->eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
                    LOG_DEBUG("AMR-WB, "); 
                }
                LOG_DEBUG("channel: %d, bitrate: %d\n", pParam->nChannels, pParam->nBitRate);
            }
            break;
        case OMX_IndexParamAudioPcm:
            {
                OMX_AUDIO_PARAM_PCMMODETYPE *pParam = (OMX_AUDIO_PARAM_PCMMODETYPE *)pComponentParameterStructure;
                OMX_S32 id = nActiveAudioNum;
                AVCodecContext *ffcodec = ic->streams[id]->codec;
                pParam->nChannels = ffcodec->channels;
                pParam->nSamplingRate = ffcodec->sample_rate;
                pParam->bInterleaved = OMX_TRUE;
                pParam->ePCMMode = OMX_AUDIO_PCMModeLinear;
                switch(ffcodec->codec_id) {
                    case CODEC_ID_PCM_U8:
                        pParam->nBitPerSample = 8;
                        pParam->eEndian = OMX_EndianLittle;
                        break;
                    case CODEC_ID_PCM_S16LE:
                        pParam->nBitPerSample = 16;
                        pParam->eEndian = OMX_EndianLittle;
                        break;
                    case CODEC_ID_PCM_S16BE:
                        pParam->nBitPerSample = 16;
                        pParam->eEndian = OMX_EndianBig;
                        break;
                    case CODEC_ID_PCM_S24LE:
                        pParam->nBitPerSample = 24;
                        pParam->eEndian = OMX_EndianLittle;
                        break;
                    case CODEC_ID_PCM_S24BE:
                        pParam->nBitPerSample = 24;
                        pParam->eEndian = OMX_EndianBig;
                        break;
                    case CODEC_ID_PCM_S32LE:
                        pParam->nBitPerSample = 32;
                        pParam->eEndian = OMX_EndianLittle;
                        break;
                    case CODEC_ID_PCM_S32BE:
                        pParam->nBitPerSample = 32;
                        pParam->eEndian = OMX_EndianBig;
                        break;
                    default:
                        break;
                }
                LOG_DEBUG("PCM, channel: %d, sample_rate: %d, bit: %d, endian: %d\n", 
                        pParam->nChannels, pParam->nSamplingRate, pParam->nBitPerSample, pParam->eEndian);
            }
            break;
        default:
            ret = Parser::GetParameter(nParamIndex, pComponentParameterStructure);
            break;
    }

    return ret;
}

void StreamingParser::AbortReadSource(OMX_BOOL bAbort)
{
    bAbortStreaming = bAbort;
}

OMX_ERRORTYPE StreamingParser::DoExec2Pause()
{
#if 0
    if(ic && ic->iformat && ic->iformat->read_pause) {
        SendEvent(OMX_EventBufferingData, 0, 0, NULL);
        fsl_osal_mutex_lock(Lock);
        ic->iformat->read_pause(ic);
        bPaused = OMX_TRUE;
        printf("Streaming is paused.\n");
        fsl_osal_mutex_unlock(Lock);
        SendEvent(OMX_EventBufferingDone, 0, 0, NULL);
    }
#endif
    return OMX_ErrorNone;
}

OMX_ERRORTYPE StreamingParser::DoPause2Exec()
{
#if 0
    if(ic && ic->iformat && ic->iformat->read_play) {
        SendEvent(OMX_EventBufferingData, 0, 0, NULL);
        fsl_osal_mutex_lock(Lock);
        ic->iformat->read_play(ic);
        bPaused = OMX_FALSE;
        printf("Streaming is resumed.\n");
        fsl_osal_mutex_unlock(Lock);
        SendEvent(OMX_EventBufferingDone, 0, 0, NULL);
    }
#endif
    return OMX_ErrorNone;
}

OMX_U32 StreamingParser::GetPortIndexByStreamId(OMX_S32 stream_id)
{
    OMX_U32 nPortIndex = -1;

    if((OMX_U32)stream_id == nActiveAudioNum)
        nPortIndex = AUDIO_OUTPUT_PORT;
    if((OMX_U32)stream_id == nActiveVideoNum)
        nPortIndex = VIDEO_OUTPUT_PORT;

    return nPortIndex;
}

OMX_S32 StreamingParser::GetStreamIdByPortIndex(OMX_U32 nPortIndex)
{
    OMX_S32 stream_id = -1;
    if(nPortIndex == AUDIO_OUTPUT_PORT)
        stream_id = nActiveAudioNum;
    if(nPortIndex == VIDEO_OUTPUT_PORT)
        stream_id = nActiveVideoNum;

    return stream_id;
}

OMX_ERRORTYPE StreamingParser::InitStreaming()
{
    AVFormatParameters *ap = &params;
    OMX_STRING role = (OMX_STRING)sCompRole.cRole;
    AVInputFormat *fmt = NULL;

    if(!strncmp(role, "parser.httplive", strlen("parser.httplive"))) {
        fmt = &ff_applehttp_demuxer;
        eStreamFmt = HTTPLIVE;
    }
    else if(!strncmp(role, "parser.rtsp", strlen("parser.rtsp"))) {
        fmt = &ff_rtsp_demuxer;
        eStreamFmt = RTSP;
    }
    else {
        fmt = NULL;
        eStreamFmt = NONE;
    }

    if(fmt == NULL) {
        LOG_ERROR("Unsupported streaming parser role.\n");
        return OMX_ErrorUndefined;
    }

    av_register_all();

    fsl_osal_memset(ap, 0, sizeof(*ap));

    ic = avformat_alloc_context();
    if(ic == NULL)
        return OMX_ErrorInsufficientResources;

    ic->iformat = fmt;

    fsl_osal_strcpy(ic->filename, (char*)pMediaName);
    OMX_S32 len = fsl_osal_strlen(ic->filename) + 1;
    for(OMX_S32 i=0; i<len; i++) {
        if(ic->filename[i] == '\n') {
            ic->filename[i] = '\0';
            break;
        }
    }

    if (fmt->priv_data_size > 0) {
        ic->priv_data = FSL_MALLOC(fmt->priv_data_size);
        if (!ic->priv_data) {
            FSL_FREE(ic);
            return OMX_ErrorInsufficientResources;
        }
        fsl_osal_memset(ic->priv_data, 0, fmt->priv_data_size);
    }
    else {
        ic->priv_data = NULL;
    }

    avio_set_interrupt_cb(abort_cb);

    int err = 0;
    SendEvent(OMX_EventBufferingData, 0, 0, NULL);
    err = ic->iformat->read_header(ic, ap);
    SendEvent(OMX_EventBufferingDone, 0, 0, NULL);
    if (err < 0) {
        FSL_FREE(ic->priv_data);
        FSL_FREE(ic);
        avio_set_interrupt_cb(NULL);
        return OMX_ErrorFormatNotDetected;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StreamingParser::DeInitStreaming()
{
    if(ic) {
        ic->iformat->read_close(ic);
        if(ic->priv_data)
            FSL_FREE(ic->priv_data);
        FSL_FREE(ic);
        avio_set_interrupt_cb(NULL);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StreamingParser::SetupPortMediaFormat()
{
    numTracks = ic->nb_streams;

    OMX_S32 i = 0;
    for(i=0; i<(OMX_S32)numTracks; i++) {
        AVStream *stream = ic->streams[i];
        LOG_DEBUG("stream %d codec is %x\n", i, stream->codec->codec_id);

        if(stream->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            nAudioTrackNum[nNumAvailAudioStream] = i;
            nNumAvailAudioStream ++; 
        }
        if(stream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            nVideoTrackNum[nNumAvailVideoStream] = i;
            nNumAvailVideoStream ++;
        }
    }

    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);

    nActiveAudioNum = -1;
    sPortDef.nPortIndex = AUDIO_OUTPUT_PORT;
    ports[AUDIO_OUTPUT_PORT]->GetPortDefinition(&sPortDef);
    if(nNumAvailAudioStream > 0) {
        nActiveAudioNum = nAudioTrackNum[0];
        SetAudioCodecType(&sPortDef, nActiveAudioNum);
        LOG_DEBUG("Audio port format is detected, have #%d streams.\n", nNumAvailAudioStream);
    }
    else {
        sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingMax;
        LOG_DEBUG("Audio port format is not detected.\n");
    }
    ports[AUDIO_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
    SendEvent(OMX_EventPortFormatDetected,  AUDIO_OUTPUT_PORT, 0, NULL);
    SendEvent(OMX_EventPortSettingsChanged, AUDIO_OUTPUT_PORT, 0, NULL);

    nActiveVideoNum = -1;
    sPortDef.nPortIndex = VIDEO_OUTPUT_PORT;
    ports[VIDEO_OUTPUT_PORT]->GetPortDefinition(&sPortDef);
    if(nNumAvailVideoStream > 0) {
        nActiveVideoNum = nVideoTrackNum[0];
        SetVideoCodecType(&sPortDef,nActiveVideoNum);
        LOG_DEBUG("Video port format is detected, have #%d streams.\n", nNumAvailVideoStream);
    }
    else {
        sPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingMax;
        LOG_DEBUG("Video port format is not detected.\n");
    }
    ports[VIDEO_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
    SendEvent(OMX_EventPortFormatDetected,  VIDEO_OUTPUT_PORT, 0, NULL);
    SendEvent(OMX_EventPortSettingsChanged, VIDEO_OUTPUT_PORT, 0, NULL);

    usDuration = ic->duration;
    if(usDuration < 0)
        usDuration = 0;
    bSeekable = OMX_TRUE;
    LOG_DEBUG("Stream duration is: %lld\n", usDuration);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StreamingParser::GetSpecificData(
        OMX_U32 nPortIndex, 
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_S32 stream_id = GetStreamIdByPortIndex(nPortIndex);

    AVCodecContext *codec = ic->streams[stream_id]->codec;
    if(codec->extradata_size > 0) {
        LOG_DEBUG("stream %d send codec specific data %d\n", stream_id, codec->extradata_size);
        fsl_osal_memcpy(pBufferHdr->pBuffer, codec->extradata, codec->extradata_size);
        pBufferHdr->nFilledLen = codec->extradata_size;
        pBufferHdr->nFlags = OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;
        return OMX_ErrorNone;
    }
    else
        return OMX_ErrorUndefined;
}

OMX_ERRORTYPE StreamingParser::ReadPacketFromStream()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(bStopThread == OMX_TRUE)
        return OMX_ErrorUndefined;

    if((OMX_TRUE != PacketCacheFree() && eStreamFmt != RTSP)
            || bStreamEos != OMX_FALSE) {
        fsl_osal_sleep(100000);
        return OMX_ErrorNone;
    }

    fsl_osal_mutex_lock(Lock);

#if 0
    if(bPaused == OMX_TRUE) {
        fsl_osal_sleep(100000);
        fsl_osal_mutex_unlock(Lock);
        fsl_osal_sleep(0);
        return OMX_ErrorNone;
    }
#endif

    AVPacket *pkt = (AVPacket*)FSL_MALLOC(sizeof(AVPacket));
    if(pkt == NULL) {
        fsl_osal_mutex_unlock(Lock);
        fsl_osal_sleep(0);
        return OMX_ErrorInsufficientResources;
    }

    while(1) {
        av_init_packet(pkt);
        int err = 0;
        err = ic->iformat->read_packet(ic, pkt);
        if(err < 0) {
            if(eStreamFmt == RTSP)
                err = AVERROR_EOF;
            if(err == AVERROR_EOF) {
                LOG_DEBUG("read stream EOS.\n");
                bStreamEos = OMX_TRUE;
                break;
            }
            if(bStopThread == OMX_TRUE) {
                ret = OMX_ErrorUndefined;
                break;
            }
            LOG_DEBUG("read packet err: %d, try again.\n", err);
            continue;
        }

#if 0
        OMX_U8 *pTemp = (OMX_U8*)pkt->data;
        printf("read stream[%d] data: %x %x %x %x %x %x %x %x\n",
                pkt->stream_index, pTemp[0], pTemp[1], pTemp[2], pTemp[3], pTemp[4], pTemp[5], pTemp[6], pTemp[7]);
#endif

        if(OMX_TRUE != IsStreamEnabled(pkt->stream_index)) {
            LOG_DEBUG("stream %d is not enabled, discard data: %d\n", pkt->stream_index, pkt->size);
            if(pkt->destruct) pkt->destruct(pkt);
            if(bStopThread == OMX_TRUE) {
                ret = OMX_ErrorUndefined;
                break;
            }
            continue;
        }

        break;
    }

    if(ret != OMX_ErrorNone || bStreamEos != OMX_FALSE) {
        FREE_PKT(pkt);
        fsl_osal_mutex_unlock(Lock);
        fsl_osal_sleep(0);
        return ret;
    }

    if(pkt->flags & AV_PKT_FLAG_SKIPPED) {
        HandleStreamingSkip();
        FREE_PKT(pkt);
        fsl_osal_mutex_unlock(Lock);
        fsl_osal_sleep(0);
        return OMX_ErrorNone;
    }

    if(pkt->flags & AV_PKT_FLAG_DROP) {
        FREE_PKT(pkt);
        fsl_osal_mutex_unlock(Lock);
        fsl_osal_sleep(0);
        return OMX_ErrorNone;
    }

    if(eStreamFmt == RTSP) {
        pkt = AssemblePacket(pkt);
        if(pkt == NULL) {
            fsl_osal_mutex_unlock(Lock);
            fsl_osal_sleep(0);
            return OMX_ErrorNone;
        }
    }

    pkt->pts = StreamTs2OMX(pkt->pts, pkt->stream_index);
    LOG_DEBUG("read stream: %d, len: %d, pts: %lld, flag: %x\n", pkt->stream_index, pkt->size, pkt->pts, pkt->flags);

    OMX_U32 nPortIndex = GetPortIndexByStreamId(pkt->stream_index);
    pktCache[nPortIndex].AddOnePkt(pkt);

    fsl_osal_mutex_unlock(Lock);
    fsl_osal_sleep(0);

    return OMX_ErrorNone;
}

OMX_BOOL StreamingParser::IsStreamEnabled(OMX_S32 stream_id)
{
    OMX_U32 nPortIndex = GetPortIndexByStreamId(stream_id);
    if(nPortIndex == (OMX_U32)-1)
        return OMX_FALSE;
    if(ports[nPortIndex]->IsEnabled() != OMX_TRUE)
        return OMX_FALSE;
    else
        return OMX_TRUE;
}

OMX_BOOL StreamingParser::PacketCacheFree()
{
    OMX_TICKS ACacheDepth = pktCache[AUDIO_OUTPUT_PORT].GetCacheDepth();
    OMX_TICKS VCacheDepth = pktCache[VIDEO_OUTPUT_PORT].GetCacheDepth();

    LOG_DEBUG("ACacheDepth: %lld, VCacheDepth: %lld\n", ACacheDepth/1000, VCacheDepth/1000);

    if(nNumAvailVideoStream > 0 && ports[VIDEO_OUTPUT_PORT]->IsEnabled() != OMX_FALSE) { 
        if(VCacheDepth < PACKET_CACHE_SIZE)
            return OMX_TRUE;
    }

    if(nNumAvailAudioStream > 0 && ports[AUDIO_OUTPUT_PORT]->IsEnabled() != OMX_FALSE) { 
        if(ACacheDepth < PACKET_CACHE_SIZE)
            return OMX_TRUE;
    }

    return OMX_FALSE;
}

OMX_BOOL StreamingParser::PacketCacheEmpty()
{
    OMX_TICKS ACacheDepth = pktCache[AUDIO_OUTPUT_PORT].GetCacheDepth();
    OMX_TICKS VCacheDepth = pktCache[VIDEO_OUTPUT_PORT].GetCacheDepth();

    if(nNumAvailVideoStream > 0 && ports[VIDEO_OUTPUT_PORT]->IsEnabled() != OMX_FALSE) { 
        if(VCacheDepth == 0)
            return OMX_TRUE;
    }

    if(nNumAvailAudioStream > 0 && ports[AUDIO_OUTPUT_PORT]->IsEnabled() != OMX_FALSE) { 
        if(ACacheDepth == 0)
            return OMX_TRUE;
    }

    return OMX_FALSE;
}

OMX_ERRORTYPE StreamingParser::DumpDataToOMXBuffer(
        AVPacket *pkt, 
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    if(pkt == NULL) {
        pBufferHdr->nFilledLen = 0;
        pBufferHdr->nOffset = 0;
        pBufferHdr->nTimeStamp = 0;
        pBufferHdr->nFlags = OMX_BUFFERFLAG_EOS;
        return OMX_ErrorNone;
    }

    fsl_osal_memcpy(pBufferHdr->pBuffer, pkt->data, pkt->size);
    pBufferHdr->nFilledLen = pkt->size;
    pBufferHdr->nOffset = 0;
    pBufferHdr->nTimeStamp = pkt->pts;
    pBufferHdr->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;

    OMX_U32 nPortIndex = pBufferHdr->nOutputPortIndex;
    if(bNewSegment[nPortIndex] == OMX_TRUE) {
        pBufferHdr->nFlags |= OMX_BUFFERFLAG_STARTTIME;
        bNewSegment[nPortIndex] = OMX_FALSE;
        LOG_DEBUG("First buffer is sent after stream skipped.\n");
    }

#if 0
    OMX_U8 *pTemp = (OMX_U8*)pkt->data;
    printf("Send [%d] buffer data: %x %x %x %x %x %x %x %x\n",
            pkt->stream_index, pTemp[0], pTemp[1], pTemp[2], pTemp[3], pTemp[4], pTemp[5], pTemp[6], pTemp[7]);
#endif

    FREE_PKT(pkt);

    return OMX_ErrorNone;
}

OMX_TICKS StreamingParser::StreamTs2OMX(
        int64_t ts, 
        OMX_S32 stream_id)
{
    int num = ic->streams[stream_id]->time_base.num;
    int den = ic->streams[stream_id]->time_base.den;
    return ts * OMX_TICKS_PER_SECOND * num / den;
}

int64_t StreamingParser::OMXTs2Stream(
        OMX_TICKS ts, 
        OMX_S32 stream_id)
{
    int num = ic->streams[stream_id]->time_base.num;
    int den = ic->streams[stream_id]->time_base.den;
    return ts * den / (OMX_TICKS_PER_SECOND * num);
}

OMX_ERRORTYPE StreamingParser::HandleStreamingSkip()
{
    LOG_DEBUG("handle streaming skip.\n");

    if(bAudioActived == OMX_TRUE) {
        pktCache[AUDIO_OUTPUT_PORT].ResetCache();
        bNewSegment[AUDIO_OUTPUT_PORT] = OMX_TRUE;
    }

    if(bVideoActived == OMX_TRUE) {
        pktCache[VIDEO_OUTPUT_PORT].ResetCache();
        bNewSegment[VIDEO_OUTPUT_PORT] = OMX_TRUE;
    }

    SendEvent(OMX_EventStreamSkipped, 0, 0, NULL);

    return OMX_ErrorNone;
}

AVPacket *StreamingParser::AssemblePacket(AVPacket *pkt)
{
    if((OMX_U32)pkt->stream_index != nActiveVideoNum)
        return pkt;

    OMX_U32 nCnt = AssemblePktList.GetNodeCnt();
    if(pkt->pts < 0) {
        FREE_PKT(pkt);
        if(nCnt == 0)
            return NULL;
        else
            return AssembledPacket();
    }

    if(nCnt == 0) {
        AssemblePktList.Add(pkt);
        return NULL;
    }

    AVPacket *ret_pkt = NULL;
    AVPacket *last_pkt = AssemblePktList.GetNode(nCnt - 1);
    if(last_pkt->pts != pkt->pts) {
        ret_pkt = AssembledPacket();
        AssemblePktList.Add(pkt);
        return ret_pkt;
    }
    else {
        AssemblePktList.Add(pkt);
        return NULL;
    }

    return NULL;
}

static void pkt_destruct(struct AVPacket *pkt)
{
    FSL_FREE(pkt->data);
    pkt->data = NULL; pkt->size = 0;
}

AVPacket *StreamingParser::AssembledPacket()
{
    AVPacket *pkt = NULL;

    OMX_U32 nCnt = AssemblePktList.GetNodeCnt();
    if(nCnt == 0) {
        LOG_ERROR("Should not be here, %s:%d\n", __FILE__, __LINE__);
        return NULL;
    }

    if(nCnt == 1) {
        pkt = AssemblePktList.GetNode(0);
        AssemblePktList.Remove(pkt);
        return pkt;
    }

    pkt = (AVPacket*)FSL_MALLOC(sizeof(AVPacket));
    if(pkt == NULL) {
        LOG_ERROR("Failed to allocate AVPacket in AssembledPacket.\n");
        return NULL;
    }

    av_init_packet(pkt);
    OMX_U32 nSize = 0;
    OMX_S32 i;
    for(i=0; i<(OMX_S32)nCnt; i++) {
        AVPacket *pkt1 = AssemblePktList.GetNode(i);
        nSize += pkt1->size;
    }

    pkt->data = (uint8_t*)FSL_MALLOC(nSize);
    if(pkt->data == NULL) {
        LOG_ERROR("Failed to allocate data size: %d\n", nSize);
        return NULL;
    }
    pkt->size = nSize;
    AVPacket *pkt1 = AssemblePktList.GetNode(0);
    pkt->pts = pkt1->pts;
    pkt->stream_index = pkt1->stream_index;
    pkt->flags = pkt1->flags;
    pkt->destruct = pkt_destruct;

    uint8_t *ptr = pkt->data;
    for(i=0; i<(OMX_S32)nCnt; i++) {
        AVPacket *pkt1 = AssemblePktList.GetNode(0);
        fsl_osal_memcpy(ptr, pkt1->data, pkt1->size);
        ptr += pkt1->size;

        AssemblePktList.Remove(pkt1);
        FREE_PKT(pkt1);
    }

    return pkt;
}

OMX_ERRORTYPE StreamingParser::FreeAssemblingPackets()
{
    OMX_S32 nCnt = AssemblePktList.GetNodeCnt();
    if(nCnt == 0)
        return OMX_ErrorNone;

    OMX_S32 i;
    for(i=0; i<nCnt; i++) {
        AVPacket *pkt = AssemblePktList.GetNode(0);
        AssemblePktList.Remove(pkt);
        FREE_PKT(pkt);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StreamingParser::PktCache::InitCache()
{
    if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&Lock, fsl_osal_mutex_normal))
        return OMX_ErrorInsufficientResources;

    bInit = OMX_TRUE;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StreamingParser::PktCache::DeInitCache()
{
    AVPacket *pkt = NULL;

    if(bInit != OMX_TRUE)
        return OMX_ErrorNone;

    ResetCache();

    if(Lock != NULL) {
        fsl_osal_mutex_destroy(Lock);
        Lock = NULL;
    }

    bInit = OMX_FALSE;

    return OMX_ErrorNone;
}

OMX_TICKS StreamingParser::PktCache::GetCacheDepth()
{
    return ABS(LastPktTs - FirstPktTs);
}

OMX_ERRORTYPE StreamingParser::PktCache::AddOnePkt(AVPacket *pkt)
{
    fsl_osal_mutex_lock(Lock);
    if(PktList.GetNodeCnt() == 0) {
        LOG_DEBUG("No packet in cache, reset first pkt ts and last pkt ts to %lld\n", pkt->pts);
        FirstPktTs = LastPktTs = pkt->pts;
    }
    PktList.Add(pkt);
    if(pkt->pts > LastPktTs)
        LastPktTs = pkt->pts;
    fsl_osal_mutex_unlock(Lock);
    return OMX_ErrorNone;
}

AVPacket * StreamingParser::PktCache::GetOnePkt()
{
    AVPacket *pkt = NULL;
    fsl_osal_mutex_lock(Lock);
    if(PktList.GetNodeCnt() == 0) {
        fsl_osal_mutex_unlock(Lock);
        return NULL;
    }

    pkt = PktList.GetNode(0);
    PktList.Remove(pkt);
    AVPacket *pkt1 = PktList.GetNode(0);
    if(pkt1)
        FirstPktTs = pkt1->pts;
    fsl_osal_mutex_unlock(Lock);
    return pkt;
}

OMX_ERRORTYPE StreamingParser::PktCache::ResetCache()
{
    AVPacket *pkt = NULL;
    fsl_osal_mutex_lock(Lock);
    if(PktList.GetNodeCnt() == 0) {
        fsl_osal_mutex_unlock(Lock);
        return OMX_ErrorNone;
    }

    while((pkt = PktList.GetNode(0))) {
        PktList.Remove(pkt);
        FREE_PKT(pkt);
        pkt = NULL;
    }

    FirstPktTs = LastPktTs = 0;
    fsl_osal_mutex_unlock(Lock);
    return OMX_ErrorNone;
}

/**< C style functions to expose entry point for the shared library */
    extern "C" {
        OMX_ERRORTYPE StreamingParserInit(OMX_IN OMX_HANDLETYPE pHandle)
        {
            OMX_ERRORTYPE ret = OMX_ErrorNone;
            StreamingParser *obj = NULL;
            ComponentBase *base = NULL;

            obj = FSL_NEW(StreamingParser, ());
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
