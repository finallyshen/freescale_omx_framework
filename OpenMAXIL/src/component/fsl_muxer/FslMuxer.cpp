/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "FslMuxer.h"
#include "fsl_media_types.h"

#define NUM_PORTS 2
#define AUDIO_PORT 0
#define VIDEO_PORT 1

#define AUDIO_BUFFER_SIZE 1024
#define VIDEO_BUFFER_SIZE (512*1024)

FslMuxer::FslMuxer()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.muxer.fsl.sw-based");
    ComponentVersion.s.nVersionMajor = 0x0;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x0;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"muxer.mp4";
    nPorts = NUM_PORTS;
    port_info[AUDIO_PORT].eDomain = OMX_PortDomainAudio;
	// workround for write has very big peak time.
    port_info[AUDIO_PORT].nBufferCount = 30;
    port_info[AUDIO_PORT].nBufferSize = AUDIO_BUFFER_SIZE;
    port_info[VIDEO_PORT].eDomain = OMX_PortDomainVideo;
    port_info[VIDEO_PORT].nBufferCount = 30;
    port_info[VIDEO_PORT].nBufferSize = VIDEO_BUFFER_SIZE;

    hMuxerLib = NULL;
    hMuxer = NULL;
    fsl_osal_memset(&hItf, 0, sizeof(FSL_MUXER_ITF));
}

OMX_ERRORTYPE FslMuxer::InitMuxerComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = LoadCoreMuxer();
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::DeInitMuxerComponent()
{
    UnLoadCoreMuxer();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::InitMuxer()
{
    uint32 err = MUXER_SUCCESS;

    err = hItf.Create(false, &streamOps, &memOps, this, &hMuxer);
    if(err != MUXER_SUCCESS || hMuxer == NULL) {
        LOG_ERROR("Create muxer faile.\n");
        return OMX_ErrorUndefined;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::DeInitMuxer()
{
    if(hMuxer != NULL)
        hItf.Delete(hMuxer);

    return OMX_ErrorNone;
}

// Written in +/-DD.DDDD format
void writeLatitude(OMX_U8 *pBuffer, int degreex10000) {
    bool isNegative = (degreex10000 < 0);
    char sign = isNegative? '-': '+';

    // Handle the whole part
    char str[9];
    int wholePart = degreex10000 / 10000;
    if (wholePart == 0) {
        snprintf(str, 5, "%c%.2d.", sign, wholePart);
    } else {
        snprintf(str, 5, "%+.2d.", wholePart);
    }

    // Handle the fractional part
    int fractionalPart = degreex10000 - (wholePart * 10000);
    if (fractionalPart < 0) {
        fractionalPart = -fractionalPart;
    }
    snprintf(&str[4], 5, "%.4d", fractionalPart);

    // Do not write the null terminator
    fsl_osal_memcpy(pBuffer, str, 8);
}

// Written in +/- DDD.DDDD format
void writeLongitude(OMX_U8 *pBuffer, int degreex10000) {
    bool isNegative = (degreex10000 < 0);
    char sign = isNegative? '-': '+';

    // Handle the whole part
    char str[10];
    int wholePart = degreex10000 / 10000;
    if (wholePart == 0) {
        snprintf(str, 6, "%c%.3d.", sign, wholePart);
    } else {
        snprintf(str, 6, "%+.3d.", wholePart);
    }

    // Handle the fractional part
    int fractionalPart = degreex10000 - (wholePart * 10000);
    if (fractionalPart < 0) {
        fractionalPart = -fractionalPart;
    }
    snprintf(&str[5], 5, "%.4d", fractionalPart);

    // Do not write the null terminator
    fsl_osal_memcpy(pBuffer, str, 9);
}

OMX_ERRORTYPE FslMuxer::AddLocationInfo(
		OMX_S64 nLongitudex, 
		OMX_S64 nLatitudex)
{
	static uint8 metaData[20] = {0};
	uint32 metaDataSize = 17;
	DataFormat userDataFormat;
	OMX_S32 nLatitudexInt = nLatitudex, nLongitudexInt = nLongitudex;
	userDataFormat = DATA_FORMAT_UTF8;

	writeLatitude(metaData, nLatitudexInt);
	writeLongitude(&metaData[7], nLongitudexInt);
	LOG_DEBUG("location: %s\n", metaData);
	if(MUXER_SUCCESS != hItf.SetMovieMetadata(hMuxer, METADATA_LOCATION, \
				userDataFormat,  metaData, metaDataSize)) {
		LOG_ERROR("Can't set location information.\n");
		return OMX_ErrorUndefined;
	}

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::AddMp3Track(
        OMX_U32 trackId, 
        OMX_AUDIO_PARAM_MP3TYPE *pParameter)
{
    if(MUXER_SUCCESS != hItf.AddTrack(hMuxer, MEDIA_AUDIO, AUDIO_MP3, 0, &trackId))
        return OMX_ErrorUndefined;

    if(pParameter != NULL) {
        hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_AVERAGE_BIT_RATE, DATA_FORMAT_UINT32, 
                &pParameter->nBitRate, sizeof(OMX_U32));

		LOG_INFO("Muxer MP3 channel: %d sample rate: %d\n", pParameter->nChannels, pParameter->nSampleRate);
        hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_CHANNEL_NUMBER, DATA_FORMAT_UINT32, 
                &pParameter->nChannels, sizeof(OMX_U32));

        hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_SAMPLE_RATE, DATA_FORMAT_UINT32, 
                &pParameter->nSampleRate, sizeof(OMX_U32));
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::AddAacTrack(
        OMX_U32 trackId, 
        OMX_AUDIO_PARAM_AACPROFILETYPE *pParameter)
{
    if(MUXER_SUCCESS != hItf.AddTrack(hMuxer, MEDIA_AUDIO, AUDIO_AAC, 0, &trackId))
        return OMX_ErrorUndefined;

    if(pParameter != NULL) {
        hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_AVERAGE_BIT_RATE, DATA_FORMAT_UINT32, 
                &pParameter->nBitRate, sizeof(OMX_U32));

		LOG_INFO("Muxer AAC channel: %d sample rate: %d\n", pParameter->nChannels, pParameter->nSampleRate);
        hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_CHANNEL_NUMBER, DATA_FORMAT_UINT32, 
                &pParameter->nChannels, sizeof(OMX_U32));

        hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_SAMPLE_RATE, DATA_FORMAT_UINT32, 
                &pParameter->nSampleRate, sizeof(OMX_U32));
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::AddAmrTrack(OMX_U32 trackId, OMX_AUDIO_PARAM_AMRTYPE *pParameter)
{
    AmrAudioTypes subType;
    OMX_U32 nSampleRate;

    if(pParameter->eAMRBandMode >= OMX_AUDIO_AMRBandModeNB0 && pParameter->eAMRBandMode <= OMX_AUDIO_AMRBandModeNB7) {
        subType = AUDIO_AMR_NB;
		nSampleRate = 8000;
	}
    if(pParameter->eAMRBandMode >= OMX_AUDIO_AMRBandModeWB0 && pParameter->eAMRBandMode <= OMX_AUDIO_AMRBandModeWB8) {
        subType = AUDIO_AMR_WB;
		nSampleRate = 16000;
	}

	printf("Amr subType: %d bit rate: %d channel: %d\n", subType, pParameter->nBitRate, pParameter->nChannels);
    if(MUXER_SUCCESS != hItf.AddTrack(hMuxer, MEDIA_AUDIO, AUDIO_AMR, subType, &trackId))
        return OMX_ErrorUndefined;

    if(pParameter != NULL) {
        hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_AVERAGE_BIT_RATE, DATA_FORMAT_UINT32, 
                &pParameter->nBitRate, sizeof(OMX_U32));

        hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_CHANNEL_NUMBER, DATA_FORMAT_UINT32, 
                &pParameter->nChannels, sizeof(OMX_U32));

        hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_SAMPLE_RATE, DATA_FORMAT_UINT32, 
                &nSampleRate, sizeof(OMX_U32));
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::AddAvcTrack(
        OMX_U32 trackId, 
        OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, 
        OMX_VIDEO_PARAM_AVCTYPE *pParameter)
{
    if(MUXER_SUCCESS != hItf.AddTrack(hMuxer, MEDIA_VIDEO, VIDEO_H264, 0, &trackId))
        return OMX_ErrorUndefined;

	LOG_INFO("Avc width: %d height: %d\n", pFormat->nFrameWidth, pFormat->nFrameHeight);
    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_AVERAGE_BIT_RATE, DATA_FORMAT_UINT32, 
            &pFormat->nBitrate, sizeof(OMX_U32));

    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_FRAME_WIDTH, DATA_FORMAT_UINT32, 
            &pFormat->nFrameWidth, sizeof(OMX_U32));

    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_FRAME_HEIGHT, DATA_FORMAT_UINT32, 
            &pFormat->nFrameHeight, sizeof(OMX_U32));

    FractionData fps;
    fps.numerator = pFormat->xFramerate;
    fps.denominator = Q16_SHIFT;
    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_FRAME_RATE, DATA_FORMAT_FRACTION, 
            &fps, sizeof(FractionData));

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::AddH263Track(
        OMX_U32 trackId, 
        OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, 
        OMX_VIDEO_PARAM_H263TYPE *pParameter)
{
    if(MUXER_SUCCESS != hItf.AddTrack(hMuxer, MEDIA_VIDEO, VIDEO_H263, 0, &trackId))
        return OMX_ErrorUndefined;

    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_AVERAGE_BIT_RATE, DATA_FORMAT_UINT32, 
            &pFormat->nBitrate, sizeof(OMX_U32));

    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_FRAME_WIDTH, DATA_FORMAT_UINT32, 
            &pFormat->nFrameWidth, sizeof(OMX_U32));

    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_FRAME_HEIGHT, DATA_FORMAT_UINT32, 
            &pFormat->nFrameHeight, sizeof(OMX_U32));

    FractionData fps;
    fps.numerator = pFormat->xFramerate;
    fps.denominator = Q16_SHIFT;
    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_FRAME_RATE, DATA_FORMAT_FRACTION, 
            &fps, sizeof(FractionData));

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::AddMpeg4Track(
        OMX_U32 trackId, 
        OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, 
        OMX_VIDEO_PARAM_MPEG4TYPE *pParameter)
{
    if(MUXER_SUCCESS != hItf.AddTrack(hMuxer, MEDIA_VIDEO, VIDEO_MPEG4, 0, &trackId))
        return OMX_ErrorUndefined;

    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_AVERAGE_BIT_RATE, DATA_FORMAT_UINT32, 
            &pFormat->nBitrate, sizeof(OMX_U32));

    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_FRAME_WIDTH, DATA_FORMAT_UINT32, 
            &pFormat->nFrameWidth, sizeof(OMX_U32));

    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_FRAME_HEIGHT, DATA_FORMAT_UINT32, 
            &pFormat->nFrameHeight, sizeof(OMX_U32));

    FractionData fps;
    fps.numerator = pFormat->xFramerate;
    fps.denominator = Q16_SHIFT;
    hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_FRAME_RATE, DATA_FORMAT_FRACTION, 
            &fps, sizeof(FractionData));

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::AddTrackCodecData(OMX_U32 trackId, OMX_PTR pData, OMX_U32 nLen)
{
    if(MUXER_SUCCESS != hItf.SetTrackProperty(hMuxer, trackId, PROPERTY_CODEC_SPECIFIC_INFO, DATA_FORMAT_GENERAL_BINARY, 
            pData, nLen)) {
        LOG_ERROR("Track %d set codec data failed.\n", trackId);
        return OMX_ErrorUndefined;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::AddTracksDone()
{
    uint32 err = MUXER_SUCCESS;

    LOG_DEBUG("Write mp4 header.\n");

    if(MUXER_SUCCESS != hItf.WriteHeader(hMuxer)) {
        LOG_ERROR("Write muxer header failed.\n");
        return OMX_ErrorUndefined;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::AddOneSample(
        OMX_U32 trackId, 
        TRACK_DATA *pData)
{
    uint32 flags = 0;
    int32 err = MUXER_SUCCESS;

    if(pData->nFlags & OMX_BUFFERFLAG_SYNCFRAME)
        flags |= FLAG_SYNC_SAMPLE;
    if(!(pData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME))
        flags |= FLAG_SAMPLE_NOT_FINISHED;

	LOG_DEBUG("AddSample nFilledLen: %d trackId: %d TS: %lld flags: %p\n", pData->nFilledLen, trackId, pData->nTimeStamp, flags);
    err = hItf.AddSample(hMuxer, trackId, (uint8*)pData->pBuffer, pData->nFilledLen, 
            -1, pData->nTimeStamp, 2, flags);
	LOG_DEBUG("AddSample ret: %d.\n", err);
	if(err == MUXER_WRITE_ERROR) {
		return OMX_ErrorInsufficientResources;
	}

    return OMX_ErrorNone;
}

OMX_U32 FslMuxer::GetTrailerSize()
{
    uint32 tailerSize = 0;

    if(MUXER_SUCCESS != hItf.GetTrailerSize(hMuxer, &tailerSize)) {
        LOG_ERROR("Can't get trailer size.\n");
		return 0;
	}

    return tailerSize;
}

OMX_ERRORTYPE FslMuxer::AddSampleDone()
{
	LOG_DEBUG("WriteTrailer.\n");
    if(MUXER_SUCCESS != hItf.WriteTrailer(hMuxer))
        LOG_ERROR("Write muxer trailer failed.\n");

    return OMX_ErrorNone;
}

#define GET_MUX_API(id, api, mandotory) \
    do { \
        if(MUXER_SUCCESS != QueryItf(id, (void **)(api))) { \
            LOG_ERROR("Query Muxer API %d failed\n", id); \
            LibMgr.unload(hMuxerLib); \
            hMuxerLib = NULL; \
            return OMX_ErrorBadParameter; \
        } \
        if(mandotory != OMX_FALSE && (api) == NULL) { \
            LOG_ERROR("Not found Muxer mandotory API %d\n", id); \
            LibMgr.unload(hMuxerLib); \
            hMuxerLib = NULL; \
            return OMX_ErrorBadParameter; \
        } \
    } while(0);

OMX_ERRORTYPE FslMuxer::LoadCoreMuxer()
{
    OMX_STRING lib_name = (OMX_STRING)"lib_mp4_muxer_arm11_elinux.so";
    OMX_STRING symbol_name = (OMX_STRING)"FslMuxerQueryInterface";
    tFslMuxerQueryInterface QueryItf = NULL;

    hMuxerLib = LibMgr.load(lib_name);
    if(hMuxerLib == NULL) {
        LOG_ERROR("Unable to load %s\n", lib_name);
        return OMX_ErrorBadParameter;
    }

    QueryItf = (tFslMuxerQueryInterface)LibMgr.getSymbol(hMuxerLib, symbol_name);
    if(QueryItf == NULL) {
        LibMgr.unload(hMuxerLib);
        hMuxerLib = NULL;
        LOG_ERROR("Get symbol %s from %s failed.\n", symbol_name, lib_name);
        return OMX_ErrorBadParameter;
    }

    GET_MUX_API(MUXER_API_VERSION, &hItf.GetApiVersion, OMX_TRUE);

    /* creation & deletion */
    GET_MUX_API(MUXER_API_GET_VERSION_INFO, &hItf.GetMuxerVersion, OMX_TRUE);
    GET_MUX_API(MUXER_API_CREATE_MUXER, &hItf.Create, OMX_TRUE);
    GET_MUX_API(MUXER_API_DELETE_MUXER, &hItf.Delete, OMX_TRUE);

    /* movie */
    GET_MUX_API(MUXER_API_SET_MOVIE_PROPERTY, &hItf.SetMovieProperty, OMX_TRUE);
    GET_MUX_API(MUXER_API_SET_MOVIE_METADATA, &hItf.SetMovieMetadata, OMX_FALSE);
    GET_MUX_API(MUXER_API_WRITE_MOVIE_HEADER, &hItf.WriteHeader, OMX_TRUE);
    GET_MUX_API(MUXER_API_WRITE_MOVIE_TRAILER, &hItf.WriteTrailer, OMX_TRUE);
    GET_MUX_API(MUXER_API_GET_MOVIE_TAILER_SIZE, &hItf.GetTrailerSize, OMX_TRUE);

    /* track */
    GET_MUX_API(MUXER_API_ADD_TRACK, &hItf.AddTrack, OMX_TRUE);
    GET_MUX_API(MUXER_API_SET_TRACK_PROPERTY, &hItf.SetTrackProperty, OMX_TRUE);
    GET_MUX_API(MUXER_API_SET_TRACK_METADATA, &hItf.SetTrackMetadata, OMX_FALSE);
    GET_MUX_API(MUXER_API_ADD_SAMPLE, &hItf.AddSample, OMX_TRUE);

    LOG_INFO("Muxer version: %s\n", hItf.GetMuxerVersion());

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FslMuxer::UnLoadCoreMuxer()
{
    if(hMuxerLib != NULL) {
        LibMgr.unload(hMuxerLib);
        hMuxerLib = NULL;
    }

    fsl_osal_memset(&hItf, 0, sizeof(FSL_MUXER_ITF));

    return OMX_ErrorNone;
}

/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE FslMuxerInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        FslMuxer *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(FslMuxer, ());
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
