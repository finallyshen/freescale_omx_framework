/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "GMRecorder.h"

static OMX_BOOL recorder_init(
        OMX_Recorder *h)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->init())
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setAudioSource(
        OMX_Recorder *h,
		OMX_S32 as)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setAudioSource(as))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setVideoSource(
        OMX_Recorder *h,
		video_source_gm vs)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setVideoSource(vs))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setOutputFormat(
        OMX_Recorder *h,
		output_format_gm of)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setOutputFormat(of))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setAudioEncoder(
        OMX_Recorder *h,
		audio_encoder_gm ae)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setAudioEncoder(ae))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setVideoEncoder(
        OMX_Recorder *h,
		video_encoder_gm ve)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setVideoEncoder(ve))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setVideoSize(
        OMX_Recorder *h,
		OMX_S32 width, OMX_S32 height)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setVideoSize(width, height))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setVideoFrameRate(
        OMX_Recorder *h,
		OMX_S32 frames_per_second)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setVideoFrameRate(frames_per_second))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setCamera(
        OMX_Recorder *h,
		OMX_PTR camera,
		OMX_PTR cameraProxy)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setCamera(camera, cameraProxy))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setPreviewSurface(
        OMX_Recorder *h,
		OMX_PTR surface)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setPreviewSurface(surface))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setOutputFile(
        OMX_Recorder *h,
		const OMX_STRING path)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setOutputFile(path))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setOutputFileFD(
        OMX_Recorder *h,
		OMX_S32 fd, OMX_S64 offset, OMX_S64 length)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setOutputFileFD(fd, offset, length))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamMaxFileDurationUs(
        OMX_Recorder *h,
		OMX_TICKS timeUs)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamMaxFileDurationUs(timeUs))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamMaxFileSizeBytes(
		OMX_Recorder *h,
		OMX_S64 bytes)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamMaxFileSizeBytes(bytes))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamInterleaveDuration(
		OMX_Recorder *h,
		OMX_S32 durationUs)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamInterleaveDuration(durationUs))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamMovieTimeScale(
		OMX_Recorder *h,
		OMX_S32 timeScale)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamMovieTimeScale(timeScale))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParam64BitFileOffset(
		OMX_Recorder *h,
		OMX_BOOL use64Bit)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParam64BitFileOffset(use64Bit))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamGeotagLongitude(
		OMX_Recorder *h,
		OMX_S64 longitudex10000)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamGeotagLongitude(longitudex10000))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamGeotagLatitude(
		OMX_Recorder *h,
		OMX_S64 latitudex10000)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamGeotagLatitude(latitudex10000))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamTrackTimeStatus(
		OMX_Recorder *h,
		OMX_S64 timeDurationUs)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamTrackTimeStatus(timeDurationUs))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamAudioSamplingRate(
		OMX_Recorder *h,
		OMX_S32 sampleRate)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamAudioSamplingRate(sampleRate))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamAudioNumberOfChannels(
		OMX_Recorder *h,
		OMX_S32 channels)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamAudioNumberOfChannels(channels))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamAudioEncodingBitRate(
		OMX_Recorder *h,
		OMX_S32 bitRate)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamAudioEncodingBitRate(bitRate))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamAudioTimeScale(
		OMX_Recorder *h,
		OMX_S32 timeScale)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamAudioTimeScale(timeScale))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamVideoEncodingBitRate(
		OMX_Recorder *h,
		OMX_S32 bitRate)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamVideoEncodingBitRate(bitRate))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamVideoRotation(
		OMX_Recorder *h,
		OMX_S32 degrees)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamVideoRotation(degrees))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamVideoIFramesInterval(
		OMX_Recorder *h,
		OMX_S32 seconds)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamVideoIFramesInterval(seconds))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamVideoEncoderProfile(
		OMX_Recorder *h,
		OMX_S32 profile)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamVideoEncoderProfile(profile))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamVideoEncoderLevel(
		OMX_Recorder *h,
		OMX_S32 level)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamVideoEncoderLevel(level))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamVideoCameraId(
		OMX_Recorder *h,
		OMX_S32 cameraId)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamVideoCameraId(cameraId))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamVideoTimeScale(
		OMX_Recorder *h,
		OMX_S32 timeScale)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamVideoTimeScale(timeScale))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamTimeLapseEnable(
		OMX_Recorder *h,
		OMX_S32 timeLapseEnable)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamTimeLapseEnable(timeLapseEnable))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_setParamTimeBetweenTimeLapseFrameCapture(
		OMX_Recorder *h,
		OMX_S64 timeUs)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->setParamTimeBetweenTimeLapseFrameCapture(timeUs))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_registerEventHandler(
		OMX_Recorder *h,
		OMX_PTR context, RECORDER_EVENTHANDLER handler)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->registerEventHandler(context, handler))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_prepare(
		OMX_Recorder *h)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->prepare())
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_start(
		OMX_Recorder *h)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->start())
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_pause(
		OMX_Recorder *h)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->pause())
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_stop(
		OMX_Recorder *h)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->stop())
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_close(
		OMX_Recorder *h)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->close())
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_reset(
		OMX_Recorder *h)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->reset())
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_getMaxAmplitude(
		OMX_Recorder *h,
		OMX_S32 *max)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->getMaxAmplitude(max))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_getMediaTime(
		OMX_Recorder *h,
		OMX_TICKS *pMediaTime)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    if(OMX_ErrorNone != Recorder->getMediaTime(pMediaTime))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL recorder_deleteIt(
        OMX_Recorder* h)
{
    GMRecorder *Recorder = NULL;

    Recorder = (GMRecorder*)h->pData;
	if (Recorder == NULL)
        return OMX_FALSE;

    Recorder->deleteIt();
    FSL_DELETE(Recorder);
    FSL_FREE(h);
    OMX_Deinit();

    return OMX_TRUE;
}

OMX_Recorder* OMX_RecorderCreate()
{
    OMX_Recorder *mRecorder = NULL;
    GMRecorder *Recorder = NULL;

    if (OMX_Init()!=OMX_ErrorNone) {
        LOG_ERROR("can not init OMX core!!!\n");
        return NULL;
    }

    mRecorder = (OMX_Recorder*)FSL_MALLOC(sizeof(OMX_Recorder));
    if(mRecorder == NULL)
        return NULL;

    Recorder = FSL_NEW(GMRecorder, ());
    if(Recorder == NULL) {
        FSL_FREE(mRecorder);
        LOG_ERROR("Can't create player.\n");
        return NULL;
    }

    Recorder->init();
    mRecorder->pData = (OMX_PTR) Recorder;

	mRecorder->init = recorder_init;
	mRecorder->setAudioSource = recorder_setAudioSource;
	mRecorder->setVideoSource = recorder_setVideoSource;
	mRecorder->setOutputFormat = recorder_setOutputFormat;
	mRecorder->setAudioEncoder = recorder_setAudioEncoder;
	mRecorder->setVideoEncoder = recorder_setVideoEncoder;
	mRecorder->setVideoSize = recorder_setVideoSize;
	mRecorder->setVideoFrameRate = recorder_setVideoFrameRate;
	mRecorder->setCamera = recorder_setCamera;
	mRecorder->setPreviewSurface = recorder_setPreviewSurface;
	mRecorder->setOutputFile = recorder_setOutputFile;
	mRecorder->setOutputFileFD = recorder_setOutputFileFD;
	mRecorder->setParamMaxFileDurationUs = recorder_setParamMaxFileDurationUs;
	mRecorder->setParamMaxFileSizeBytes = recorder_setParamMaxFileSizeBytes;
	mRecorder->setParamInterleaveDuration = recorder_setParamInterleaveDuration;
	mRecorder->setParamMovieTimeScale = recorder_setParamMovieTimeScale;
	mRecorder->setParam64BitFileOffset = recorder_setParam64BitFileOffset;
	mRecorder->setParamGeotagLongitude= recorder_setParamGeotagLongitude;
	mRecorder->setParamGeotagLatitude= recorder_setParamGeotagLatitude;
	mRecorder->setParamTrackTimeStatus = recorder_setParamTrackTimeStatus;
	mRecorder->setParamAudioSamplingRate = recorder_setParamAudioSamplingRate;
	mRecorder->setParamAudioNumberOfChannels = recorder_setParamAudioNumberOfChannels;
	mRecorder->setParamAudioEncodingBitRate = recorder_setParamAudioEncodingBitRate;
	mRecorder->setParamAudioTimeScale = recorder_setParamAudioTimeScale;
	mRecorder->setParamVideoEncodingBitRate = recorder_setParamVideoEncodingBitRate;
	mRecorder->setParamVideoRotation = recorder_setParamVideoRotation;
	mRecorder->setParamVideoIFramesInterval = recorder_setParamVideoIFramesInterval;
	mRecorder->setParamVideoEncoderProfile = recorder_setParamVideoEncoderProfile;
	mRecorder->setParamVideoEncoderLevel = recorder_setParamVideoEncoderLevel;
	mRecorder->setParamVideoCameraId = recorder_setParamVideoCameraId;
	mRecorder->setParamVideoTimeScale = recorder_setParamVideoTimeScale;
	mRecorder->setParamTimeLapseEnable = recorder_setParamTimeLapseEnable;
	mRecorder->setParamTimeBetweenTimeLapseFrameCapture = recorder_setParamTimeBetweenTimeLapseFrameCapture;

	mRecorder->registerEventHandler = recorder_registerEventHandler;
	mRecorder->prepare = recorder_prepare;
	mRecorder->start = recorder_start;
	mRecorder->pause = recorder_pause;
	mRecorder->stop = recorder_stop;
	mRecorder->close = recorder_close;
	mRecorder->reset = recorder_reset;
	mRecorder->getMaxAmplitude = recorder_getMaxAmplitude;
	mRecorder->getMediaTime = recorder_getMediaTime;
	mRecorder->deleteIt = recorder_deleteIt;

    return mRecorder;
}


