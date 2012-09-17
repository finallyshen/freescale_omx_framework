/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <ctype.h>
#include "GMRecorder.h"

#ifdef ANDROID_BUILD
#include "PMemory.h"
#include "PlatformResourceMgrItf.h"
#endif

#define AUDIO_CLOCK_PORT 0
#define VIDEO_CLOCK_PORT 1
#define FILTER_INPUT_PORT 0
#define FILTER_OUT_PORT 1

#define VIDEO_WIDTH_DEFAULT 640
#define VIDEO_HEIGTH_DEFAULT 480
#define VIDEO_FRAMERATE_DEFAULT 30
#define INTERLEAVE_DEFAULT 5000000
#define AUDIO_SAMPLERATE_DEFAULT 44100
#define AUDIO_CHANNELS_DEFAULT 1
#define AUDIO_BITRATE_DEFAULT 0

#define AUDIO_TIMESCALE_DEFAULT 1
#define VIDEO_BITRATE_DEFAULT 1024000
#define VIDEO_DEGREES_DEFAULT 0
#define VIDEO_IFRAMEINTERVAL_DEFAULT 1
#define VIDEO_PROFILE_DEFAULT 1
#define VIDEO_LEVEL_DEFAULT 1
#define VIDEO_CAMERAID_DEFAULT 1
#define VIDEO_TIMESCALE_FAULT 1

#ifdef ANDROID_BUILD
#define AUDIO_SOURCE "audio_source.android"
#else
#define AUDIO_SOURCE "audio_source.alsa"
#endif

#ifdef ANDROID_BUILD
#define VIDEO_SOURCE "video_source.camera"
#else
#define VIDEO_SOURCE "video_source.v4l"
#endif

#define GM_QUIT_EVENT ((OMX_EVENTTYPE)(OMX_EventMax - 1))

#define WAIT_MARKBUFFER(to) \
    do { \
        OMX_U32 WaitCnt = 0; \
        OMX_U32 ToCnt = to / 10000; \
        while(1) { \
            if(bAudioMarkDone == OMX_TRUE && bVideoMarkDone == OMX_TRUE) \
            break; \
            if(bError == OMX_TRUE) \
            break; \
            fsl_osal_sleep(10000); \
            if(bBuffering == OMX_TRUE) { \
                WaitCnt = 0; \
            } \
            WaitCnt ++; \
            if(WaitCnt >= ToCnt) { \
                ret = OMX_ErrorTimeout; \
                break; \
            } \
        } \
        bAudioMarkDone = bVideoMarkDone = OMX_FALSE; \
    }while(0);

#define swap_value(a,b) \
    do { \
        OMX_U32 temp; \
        temp = a; \
        a = b; \
        b = temp; \
    } while(0);

static OMX_BOOL closeFD(int mSharedFd)
{

	close(mSharedFd);

    return OMX_TRUE;
}

static OMX_BOOL char_tolower(OMX_STRING string)
{
    while(*string != '\0')
    {
        *string = tolower(*string);
        string++;
    }

    return OMX_TRUE;
}

static OMX_ERRORTYPE SysEventCallback(
        GMComponent *pComponent,
        OMX_PTR pAppData,
        OMX_EVENTTYPE eEvent,
        OMX_U32 nData1,
        OMX_U32 nData2,
        OMX_PTR pEventData)
{
    GMRecorder *Recorder = NULL;
    GM_SYSEVENT SysEvent;

    Recorder = (GMRecorder*)pAppData;

    SysEvent.pComponent = pComponent;
    SysEvent.eEvent = eEvent;
    SysEvent.nData1 = nData1;
    SysEvent.nData2 = nData2;
    SysEvent.pEventData = pEventData;
    if(eEvent == OMX_EventMark)
        Recorder->SysEventHandler(&SysEvent);
    else
        Recorder->AddSysEvent(&SysEvent);

    return OMX_ErrorNone;
}

static fsl_osal_ptr SysEventThreadFunc(fsl_osal_ptr arg)
{
    GMRecorder *Recorder = (GMRecorder*)arg;
    GM_SYSEVENT SysEvent;

    while(1) {
        Recorder->GetSysEvent(&SysEvent);
        if(SysEvent.eEvent == GM_QUIT_EVENT)
            break;
        Recorder->SysEventHandler(&SysEvent);
    }

    return NULL;
}

static OMX_ERRORTYPE GMGetBuffer(
        OMX_HANDLETYPE hComponent,
        OMX_U32 nPortIndex,
        OMX_PTR *pBuffer,
        GMBUFFERINFO *pInfo,
        OMX_PTR pAppData)
{
    GMRecorder *player = (GMRecorder*)pAppData;

    *pBuffer = NULL;

    switch(pInfo->type) {
        case GM_VIRTUAL:
            break;
        case GM_PHYSIC:
#ifdef ANDROID_BUILD
            PMEMADDR sMemAddr;
            sMemAddr = GetPMBuffer(player->PmContext, pInfo->nSize, pInfo->nNum);
            if(sMemAddr.pVirtualAddr == NULL)
                return OMX_ErrorInsufficientResources;
            AddHwBuffer(sMemAddr.pPhysicAddr, sMemAddr.pVirtualAddr);
            *pBuffer = sMemAddr.pVirtualAddr;
#endif
            break;
        default:
            break;
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE GMFreeBuffer(
        OMX_HANDLETYPE hComponent,
        OMX_U32 nPortIndex,
        OMX_PTR pBuffer,
        GMBUFFERINFO *pInfo,
        OMX_PTR pAppData)
{
    GMRecorder *player = (GMRecorder*)pAppData;

    switch(pInfo->type) {
        case GM_VIRTUAL:
            break;
        case GM_PHYSIC:
#ifdef ANDROID_BUILD
            RemoveHwBuffer(pBuffer);
            FreePMBuffer(player->PmContext, pBuffer);
#endif
            break;
        default:
            break;
    }

    return OMX_ErrorNone;
}

static GM_BUFFERCB gBufferCb = {
    GMGetBuffer,
    GMFreeBuffer
};

OMX_ERRORTYPE GMRecorder::reset()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    bNeedAudio = OMX_TRUE;
    bNeedVideo = OMX_TRUE;
	bSetAudioSource = OMX_FALSE;
	bSetVideoSource = OMX_FALSE;
    Muxer = NULL;
    AudioEncoder = NULL;
    VideoEncoder = NULL;
    AudioSource = NULL;
    VideoSource = NULL;
    Clock = NULL;
    bHasAudio = OMX_TRUE;
    bHasVideo = OMX_TRUE;
    AudioFmt = OMX_AUDIO_CodingMax;
    VideoFmt = OMX_VIDEO_CodingMax;

    bAudioMarkDone = OMX_FALSE;
    bVideoMarkDone = OMX_FALSE;
    bAudioEos = OMX_FALSE;
    bVideoEos = OMX_FALSE;
    bMediaEos = OMX_FALSE;
	bMaxDuration = OMX_FALSE;
	bError = OMX_FALSE;
    State = RECORDER_STATE_STOP;
	gm_as = 0;
	gm_vs = VIDEO_SOURCE_DEFAULT;
	gm_ae = AUDIO_ENCODER_DEFAULT;
	gm_ve = VIDEO_ENCODER_DEFAULT;
	gm_of = OUTPUT_FORMAT_DEFAULT;
	gm_width = VIDEO_WIDTH_DEFAULT;
	gm_height = VIDEO_HEIGTH_DEFAULT;
	gm_frames_per_second = VIDEO_FRAMERATE_DEFAULT;
	gm_camera = NULL;
	gm_camera_proxy = NULL;
	gm_surface = NULL;
	gm_timeUs = MAX_VALUE_S64;
	gm_bytes = MAX_VALUE_S64;
	gm_durationUs = INTERLEAVE_DEFAULT;
	gm_movie_timeScale = 0;
	gm_use64Bit = OMX_TRUE;
	gm_Latitudex10000= -3600000;
    gm_Longitudex10000 = -3600000;
	gm_timeDurationUs = MAX_VALUE_S64;
	gm_sampleRate = AUDIO_SAMPLERATE_DEFAULT;
	gm_channels = AUDIO_CHANNELS_DEFAULT;
	gm_audio_bitRate = AUDIO_BITRATE_DEFAULT;
	gm_audio_timeScale = AUDIO_TIMESCALE_DEFAULT;
	gm_video_bitRate = VIDEO_BITRATE_DEFAULT;
	gm_degrees = VIDEO_DEGREES_DEFAULT;
	gm_seconds = VIDEO_IFRAMEINTERVAL_DEFAULT;
	gm_profile = VIDEO_PROFILE_DEFAULT;
	gm_level = VIDEO_LEVEL_DEFAULT;
	gm_cameraId = VIDEO_CAMERAID_DEFAULT;
	gm_video_timeScale = VIDEO_TIMESCALE_FAULT;
	gm_capture_time_lapse = OMX_FALSE;
	gm_timeBetweenTimeLapseFrameCaptureUs = 0;

    if (mSharedFd >= 0) {
        closeFD(mSharedFd);
        mSharedFd = -1;
    }

    if(Content != NULL)
        FSL_FREE(Content);

    if(gm_path != NULL)
        FSL_FREE(gm_path);

    while(SysEventQueue->Size() > 0) {
        GM_SYSEVENT SysEvent;
        SysEventQueue->Get(&SysEvent);
    }

	return ret;
}

OMX_ERRORTYPE GMRecorder::init()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
    Content = NULL;
	gm_path = NULL;
    Pipeline = NULL;
    SysEventQueue = NULL;
    SysEventThread = NULL;
    Lock = NULL;
	mSharedFd = -1;
    hPipe = NULL;

    SysEventQueue = FSL_NEW(Queue, ());
    if(SysEventQueue == NULL)
        return OMX_ErrorInsufficientResources;

    if(QUEUE_SUCCESS != SysEventQueue->Create(128, sizeof(GM_SYSEVENT), E_FSL_OSAL_TRUE)) {
        ret = OMX_ErrorInsufficientResources;
        goto err;
    }

    if(E_FSL_OSAL_SUCCESS != fsl_osal_thread_create(&SysEventThread, NULL, SysEventThreadFunc, this)) {
        ret = OMX_ErrorInsufficientResources;
        goto err;
    }

    if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&Lock, fsl_osal_mutex_normal)) {
        ret = OMX_ErrorInsufficientResources;
        goto err;
    }

    Pipeline = FSL_NEW(List<GMComponent>, ());
    if(Pipeline == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto err;
    }

    reset();

    return OMX_ErrorNone;

err:
    deleteIt();
 
	return ret;
}

OMX_ERRORTYPE GMRecorder::deleteIt()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	stop();

    if(SysEventThread != NULL) {
        GM_SYSEVENT SysEvent;
        SysEvent.eEvent = GM_QUIT_EVENT;
        AddSysEvent(&SysEvent);
        fsl_osal_thread_destroy(SysEventThread);
    }

    if(SysEventQueue != NULL) {
        SysEventQueue->Free();
        FSL_DELETE(SysEventQueue);
    }

    if(Lock != NULL)
        fsl_osal_mutex_destroy(Lock);

    if(Pipeline != NULL)
        FSL_DELETE(Pipeline);

	return ret;
}

OMX_ERRORTYPE GMRecorder::setAudioSource(OMX_S32 as)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setAudioSource: %d", as);

	gm_as = as;
	bSetAudioSource = OMX_TRUE;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setVideoSource(video_source_gm vs)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setVideoSource: %d", vs);
    if (vs < VIDEO_SOURCE_DEFAULT ||
        vs >= VIDEO_SOURCE_LIST_END) {
        LOG_ERROR("Invalid video source: %d", vs);
        return OMX_ErrorBadParameter;
    }

	gm_vs = vs;
	bSetVideoSource = OMX_TRUE;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setOutputFormat(output_format_gm of)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setOutputFormat: %d", of);
    if (of < OUTPUT_FORMAT_DEFAULT ||
        of >= OUTPUT_FORMAT_LIST_END) {
        LOG_ERROR("Invalid output format: %d", of);
        return OMX_ErrorBadParameter;
    }

	gm_of = of;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setAudioEncoder(audio_encoder_gm ae)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setAudioEncoder: %d", ae);
    if (ae < AUDIO_ENCODER_DEFAULT ||
        ae >= AUDIO_ENCODER_LIST_END) {
        LOG_ERROR("Invalid audio encoder: %d", ae);
        return OMX_ErrorBadParameter;
    }

	gm_ae = ae;

	// check sampleRate of audio.
	switch(gm_ae) {
		case AUDIO_ENCODER_DEFAULT:
		case AUDIO_ENCODER_AMR_NB:
			{
				gm_sampleRate = 8000;
			}
			break;
		case AUDIO_ENCODER_AMR_WB:
			{
				gm_sampleRate = 16000;
			}
			break;
		default:
			break;
	}

	return ret;
}

OMX_ERRORTYPE GMRecorder::setVideoEncoder(video_encoder_gm ve)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setVideoEncoder: %d", ve);
    if (ve < VIDEO_ENCODER_DEFAULT ||
        ve >= VIDEO_ENCODER_LIST_END) {
        LOG_ERROR("Invalid video encoder: %d", ve);
        return OMX_ErrorBadParameter;
    }

	gm_ve = ve;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setVideoSize(OMX_S32 width, OMX_S32 height)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setVideoSize: %dx%d", width, height);
    if (width <= 0 || height <= 0) {
        LOG_ERROR("Invalid video size: %dx%d", width, height);
        return OMX_ErrorBadParameter;
    }


	gm_width = width;
	gm_height = height;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setVideoFrameRate(OMX_S32 frames_per_second)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setVideoFrameRate: %d", frames_per_second);
    if (frames_per_second <= 0 || frames_per_second > 30) {
        LOG_ERROR("Invalid video frame rate: %d", frames_per_second);
        return OMX_ErrorBadParameter;
    }

	gm_frames_per_second = frames_per_second;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setCamera(OMX_PTR camera, OMX_PTR cameraProxy)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setCamera: %p\n", camera);
    if (camera == NULL) {
        LOG_ERROR("camera is NULL");
        return OMX_ErrorBadParameter;
    }

	gm_camera = camera;
	gm_camera_proxy = cameraProxy;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setPreviewSurface(OMX_PTR surface)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setPreviewSurface");
    if (surface == NULL) {
        LOG_ERROR("surface is NULL");
        return OMX_ErrorBadParameter;
    }

	gm_surface = surface;

	return ret;
}

OMX_ERRORTYPE GMRecorder::MediaTypeInspectBySubfix(OMX_STRING path)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_STRING p = NULL;
    fsl_osal_char subfix[16];

    p = fsl_osal_strrchr((fsl_osal_char*)(path), '.');
    if (p == NULL || fsl_osal_strlen(p)>15)
        return OMX_ErrorBadParameter;

    fsl_osal_strcpy(subfix, p);
    char_tolower(subfix);
    if (fsl_osal_strcmp(subfix, ".mp3")==0)
		gm_of = OUTPUT_FORMAT_MP3;
    else if (fsl_osal_strcmp(subfix, ".aac")==0)
		gm_of = OUTPUT_FORMAT_AAC_ADTS;
    else if (fsl_osal_strcmp(subfix, ".3gp")==0 || fsl_osal_strcmp(subfix, ".3g2")==0)
		gm_of = OUTPUT_FORMAT_THREE_GPP;
    else if (fsl_osal_strcmp(subfix, ".mp4")==0 || fsl_osal_strcmp(subfix, ".mov")==0)
		gm_of = OUTPUT_FORMAT_MPEG_4;

    LOG_DEBUG("%s, gm_of: %d\n", __FUNCTION__, gm_of);

	return ret;
}

OMX_ERRORTYPE GMRecorder::setOutputFile(const OMX_STRING path)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("OMXRecorder set output file %s\n", path);

	if (path == NULL)
		return OMX_ErrorBadParameter;

    if (mSharedFd >= 0) {
        closeFD(mSharedFd);
        mSharedFd = -1;
    }

    OMX_S32 len = fsl_osal_strlen(path) + 1;
    gm_path = (OMX_STRING)FSL_MALLOC(len);
    if(gm_path == NULL)
        return OMX_ErrorInsufficientResources;

    fsl_osal_strcpy(gm_path, path);
    LOG_DEBUG("OMXRecorder gm_path: %s\n", gm_path);

	ret = MediaTypeInspectBySubfix(gm_path);
	if (ret != OMX_ErrorNone) {
		LOG_ERROR("Can't support file format.\n");
		return ret;
	}

	return ret;
}

OMX_ERRORTYPE GMRecorder::setOutputFileFD(OMX_S32 fd, OMX_S64 offset, OMX_S64 length)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("OMXRecorder set output file fd, fd:%d, offset: %lld, length: %lld.\n",
            fd, offset, length);

    if (mSharedFd >= 0) {
        closeFD(mSharedFd);
        mSharedFd = -1;
    }

    gm_path = (char*)FSL_MALLOC(128);
    if(gm_path == NULL)
        return OMX_ErrorInsufficientResources;

    mSharedFd = dup(fd);
    sprintf(gm_path, "sharedfd://%d:%lld:%lld",  mSharedFd, offset, length);
    LOG_DEBUG("OMXRecorder gm_path: %s\n", gm_path);

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamMaxFileDurationUs(OMX_TICKS timeUs)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamMaxFileDurationUs: %lld us", timeUs);
    if (timeUs <= 0) {
        LOG_WARNING("Max file duration is not positive: %lld us. Disabling duration limit.", timeUs);
        timeUs = 0; // Disable the duration limit for zero or negative values.
		return ret;
	} else if (timeUs <= 100000LL) {  // XXX: 100 milli-seconds
        LOG_ERROR("Max file duration is too short: %lld us", timeUs);
		bError = OMX_TRUE;
        return OMX_ErrorBadParameter;
    }

	gm_timeUs = timeUs;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamMaxFileSizeBytes(OMX_S64 bytes)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	LOG_DEBUG("setParamMaxFileSizeBytes: %lld bytes", bytes);
	if (bytes <= 0) {
		LOG_WARNING("Max file size is not positive: %lld bytes. "
				"Disabling file size limit.", bytes);
		bytes = 0; // Disable the file size limit for zero or negative values.
		return ret;
	} else if (bytes <= 1024) {  // XXX: 1 kB
        LOG_ERROR("Max file size is too small: %lld bytes", bytes);
		bError = OMX_TRUE;
		return OMX_ErrorBadParameter;
    }

	gm_bytes = bytes;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamInterleaveDuration(OMX_S32 durationUs)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamInterleaveDuration: %d", durationUs);
    if (durationUs <= 500000) {           //  500 ms
        // If interleave duration is too small, it is very inefficient to do
        // interleaving since the metadata overhead will count for a significant
        // portion of the saved contents
        LOG_ERROR("Audio/video interleave duration is too small: %d us", durationUs);
        return OMX_ErrorBadParameter;
    } else if (durationUs >= 10000000) {  // 10 seconds
        // If interleaving duration is too large, it can cause the recording
        // session to use too much memory since we have to save the output
        // data before we write them out
        LOG_ERROR("Audio/video interleave duration is too large: %d us", durationUs);
        return OMX_ErrorBadParameter;
    }

	gm_durationUs = durationUs;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamMovieTimeScale(OMX_S32 timeScale)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamMovieTimeScale: %d", timeScale);

    // The range is set to be the same as the audio's time scale range
    // since audio's time scale has a wider range.
    if (timeScale < 600 || timeScale > 96000) {
        LOG_ERROR("Time scale (%d) for movie is out of range [600, 96000]", timeScale);
        return OMX_ErrorBadParameter;
    }

	gm_movie_timeScale = timeScale;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParam64BitFileOffset(OMX_BOOL use64Bit)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParam64BitFileOffset: %s",
        use64Bit? "use 64 bit file offset": "use 32 bit file offset");
 
	gm_use64Bit = use64Bit;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamGeotagLongitude(OMX_S64 longitudex10000)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	
    LOG_DEBUG("setParamGeotagLongitude: %lld", longitudex10000);
    if (longitudex10000 > 1800000 || longitudex10000 < -1800000) {
        LOG_ERROR("longitudex10000 wrong: %lld us", longitudex10000);
        return OMX_ErrorBadParameter;
    }
    gm_Longitudex10000 = longitudex10000;
 
	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamGeotagLatitude(OMX_S64 latitudex10000)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	
    LOG_DEBUG("setParamGeotagLatitude: %lld", latitudex10000);
    if (latitudex10000 > 900000 || latitudex10000 < -900000) {
        LOG_ERROR("latitudex10000 wrong: %lld us", latitudex10000);
        return OMX_ErrorBadParameter;
    }
    gm_Latitudex10000= latitudex10000;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamTrackTimeStatus(OMX_S64 timeDurationUs)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	
    LOG_DEBUG("setParamTrackTimeStatus: %lld", timeDurationUs);
    if (timeDurationUs < 20000) {  // Infeasible if shorter than 20 ms?
        LOG_ERROR("Tracking time duration too short: %lld us", timeDurationUs);
        return OMX_ErrorBadParameter;
    }

	gm_timeDurationUs = timeDurationUs;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamAudioSamplingRate(OMX_S32 sampleRate)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamAudioSamplingRate: %d", sampleRate);
    if (sampleRate <= 0) {
        LOG_ERROR("Invalid audio sampling rate: %d", sampleRate);
        return OMX_ErrorBadParameter;
    }

	gm_sampleRate = sampleRate;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamAudioNumberOfChannels(OMX_S32 channels)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamAudioNumberOfChannels: %d", channels);
    if (channels <= 0 || channels >= 3) {
        LOG_ERROR("Invalid number of audio channels: %d", channels);
        return OMX_ErrorBadParameter;
    }

	gm_channels = channels;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamAudioEncodingBitRate(OMX_S32 bitRate)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamAudioEncodingBitRate: %d", bitRate);
    if (bitRate <= 0) {
        LOG_ERROR("Invalid audio encoding bit rate: %d", bitRate);
        return OMX_ErrorBadParameter;
    }

    // The target bit rate may not be exactly the same as the requested.
    // It depends on many factors, such as rate control, and the bit rate
    // range that a specific encoder supports. The mismatch between the
    // the target and requested bit rate will NOT be treated as an error.
 
	gm_audio_bitRate = bitRate;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamAudioTimeScale(OMX_S32 timeScale)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamAudioTimeScale: %d", timeScale);

    // 96000 Hz is the highest sampling rate support in AAC.
    if (timeScale < 600 || timeScale > 96000) {
        LOG_ERROR("Time scale (%d) for audio is out of range [600, 96000]", timeScale);
        return OMX_ErrorBadParameter;
    }

	gm_audio_timeScale = timeScale;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamVideoEncodingBitRate(OMX_S32 bitRate)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	
    LOG_DEBUG("setParamVideoEncodingBitRate: %d", bitRate);
    if (bitRate <= 0) {
        LOG_ERROR("Invalid video encoding bit rate: %d", bitRate);
        return OMX_ErrorBadParameter;
    }

    // The target bit rate may not be exactly the same as the requested.
    // It depends on many factors, such as rate control, and the bit rate
    // range that a specific encoder supports. The mismatch between the
    // the target and requested bit rate will NOT be treated as an error.
 
	gm_video_bitRate = bitRate;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamVideoRotation(OMX_S32 degrees)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamVideoRotation: %d", degrees);
    if (degrees < 0 || degrees % 90 != 0) {
        LOG_ERROR("Unsupported video rotation angle: %d", degrees);
        return OMX_ErrorBadParameter;
    }

	gm_degrees = degrees;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamVideoIFramesInterval(OMX_S32 seconds)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamVideoIFramesInterval: %d seconds", seconds);

	gm_seconds = seconds;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamVideoEncoderProfile(OMX_S32 profile)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamVideoEncoderProfile: %d", profile);

	gm_profile = profile;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamVideoEncoderLevel(OMX_S32 level)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamVideoEncoderLevel: %d", level);

	gm_level = level;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamVideoCameraId(OMX_S32 cameraId)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamVideoCameraId: %d", cameraId);
    if (cameraId < 0) {
        return OMX_ErrorBadParameter;
    }

	gm_cameraId = cameraId;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamVideoTimeScale(OMX_S32 timeScale)

{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("setParamVideoTimeScale: %d", timeScale);

    // 60000 is chosen to make sure that each video frame from a 60-fps
    // video has 1000 ticks.
    if (timeScale < 600 || timeScale > 60000) {
        LOG_ERROR("Time scale (%d) for video is out of range [600, 60000]", timeScale);
        return OMX_ErrorBadParameter;
    }
 
	gm_video_timeScale = timeScale;

	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamTimeLapseEnable(OMX_S32 timeLapseEnable) {
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	LOG_DEBUG("setParamTimeLapseEnable: %d", timeLapseEnable);

	if(timeLapseEnable == 0) {
		gm_capture_time_lapse = OMX_FALSE;
	} else if (timeLapseEnable == 1) {
		gm_capture_time_lapse = OMX_TRUE;
	} else {
        return OMX_ErrorBadParameter;
	}
	return ret;
}

OMX_ERRORTYPE GMRecorder::setParamTimeBetweenTimeLapseFrameCapture(OMX_S64 timeUs) {
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	LOG_DEBUG("setParamTimeBetweenTimeLapseFrameCapture: %lld us", timeUs);

	// Not allowing time more than a day
	if (timeUs <= 0 || timeUs > 86400*1E6) {
		LOG_ERROR("Time between time lapse frame capture (%lld) is out of range [0, 1 Day]", timeUs);
        return OMX_ErrorBadParameter;
	}

	gm_timeBetweenTimeLapseFrameCaptureUs = timeUs;
	return ret;
}

OMX_ERRORTYPE GMRecorder::AddSysEvent(
        GM_SYSEVENT *pSysEvent)
{
    SysEventQueue->Add((fsl_osal_ptr)pSysEvent);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMRecorder::GetSysEvent(
        GM_SYSEVENT *pSysEvent)
{
    SysEventQueue->Get((fsl_osal_ptr)pSysEvent);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMRecorder::SysEventHandler(
        GM_SYSEVENT *pSysEvent)
{
    GMComponent *pComponent = pSysEvent->pComponent;
    OMX_EVENTTYPE eEvent = pSysEvent->eEvent;
    OMX_U32 nData1 = pSysEvent->nData1;
    OMX_U32 nData2 = pSysEvent->nData2;

    switch(eEvent) {
        case OMX_EventBufferFlag:
            {
                if(nData2 == OMX_BUFFERFLAG_EOS) {
                    if(pComponent == Muxer) { 
                        if(bHasAudio == OMX_TRUE && nData1 == \
								Muxer->PortPara[AUDIO_PORT_PARA].nStartPortNumber) {
                            printf("Muxer get Audio EOS event.\n");
                            bAudioEos = OMX_TRUE;
                        }

                        if(bHasVideo == OMX_TRUE && nData1 == \
								Muxer->PortPara[VIDEO_PORT_PARA].nStartPortNumber) {
                            printf("Muxer get Video EOS event.\n");
                            bVideoEos = OMX_TRUE;
                        }
                    }
 
					if(((bHasAudio == OMX_TRUE && bHasVideo == OMX_TRUE) \
							&& (bAudioEos == OMX_TRUE && bVideoEos == OMX_TRUE)) \
							|| ((bHasAudio == OMX_TRUE && bHasVideo != OMX_TRUE) \
							&& bAudioEos == OMX_TRUE) \
							|| ((bHasAudio != OMX_TRUE && bHasVideo == OMX_TRUE) \
							&& bVideoEos == OMX_TRUE)) {
                        bMediaEos = OMX_TRUE;
                        if(pAppCallback != NULL) {
                            (*pAppCallback)(pAppData, RECORDER_EVENT_COMPLETION_STATUS, 0);
							if (bMaxDuration == OMX_TRUE)
								(*pAppCallback)(pAppData, RECORDER_EVENT_MAX_DURATION_REACHED, 0);
						}
                    }
                }
				if(nData2 == OMX_BUFFERFLAG_MAX_FILESIZE) {
					if(pComponent == Muxer) { 
						if(pAppCallback != NULL) {
							(*pAppCallback)(pAppData, RECORDER_EVENT_MAX_FILESIZE_REACHED, 0);
						}
					}
				}
				if(nData2 == OMX_BUFFERFLAG_MAX_DURATION) {
					bMaxDuration = OMX_TRUE;
				}
            }
            break;
        case OMX_EventMark:
            {
                if(pComponent == AudioSource) {
                    bAudioMarkDone = OMX_TRUE;
                    LOG_DEBUG("Recorder get Audio mark done event.\n");
                }
                if(pComponent == VideoSource) {
                    bVideoMarkDone = OMX_TRUE;
                    LOG_DEBUG("Recorder get Video mark done event.\n");
                }
            }
            break;
        case OMX_EventError:
            {
                LOG_ERROR("%s report Error %x.\n", pComponent->name, nData1);
                bError = OMX_TRUE;
				if((OMX_S32)nData1 == OMX_ErrorStreamCorrupt) {
					bError = OMX_FALSE;
                    if(pAppCallback != NULL)
                        (*pAppCallback)(pAppData, RECORDER_EVENT_ERROR_UNKNOWN, 0);
                }
                if((OMX_S32)nData1 == OMX_ErrorHardware) {
                    if(pAppCallback != NULL)
                        (*pAppCallback)(pAppData, RECORDER_EVENT_UNKNOWN, 0);
                }
            }
            break;
        default:
            break;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMRecorder::registerEventHandler(OMX_PTR context, RECORDER_EVENTHANDLER handler)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    pAppData = context;
    pAppCallback = handler;
 
	return ret;
}

OMX_ERRORTYPE GMRecorder::prepare()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    fsl_osal_mutex_lock(Lock);

    if(State != RECORDER_STATE_STOP) {
        fsl_osal_mutex_unlock(Lock);
        return OMX_ErrorNone;
    }

	if (bError == OMX_TRUE) {
        fsl_osal_mutex_unlock(Lock);
        return OMX_ErrorNone;
	}

	if (gm_path == NULL) {
		LOG_ERROR("Haven't set output file.\n");
        fsl_osal_mutex_unlock(Lock);
		return OMX_ErrorIncorrectStateOperation;
	}

    State = RECORDER_STATE_LOADING;

    ret = LoadRecorderComponent();
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("LoadRecorderComponent failed\n");
        fsl_osal_mutex_unlock(Lock);
        return ret;
    }

    ret = SetupPipeline();
    if(ret != OMX_ErrorNone)
        goto err;

    LoadClock();

    ret = StartPipeline();
    if(ret != OMX_ErrorNone)
        goto err;

    State = RECORDER_STATE_LOADED;
    fsl_osal_mutex_unlock(Lock);

    return OMX_ErrorNone;

err:
    fsl_osal_mutex_unlock(Lock);
    close();

	return ret;
}

OMX_ERRORTYPE GMRecorder::DecidePipeLine()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (gm_of == OUTPUT_FORMAT_DEFAULT \
    || gm_of == OUTPUT_FORMAT_THREE_GPP \
    || gm_of == OUTPUT_FORMAT_MPEG_4) {
		bNeedAudio = OMX_TRUE;
		bNeedVideo = OMX_TRUE;
	}

    if (gm_of == OUTPUT_FORMAT_AUDIO_ONLY_START \
    || gm_of == OUTPUT_FORMAT_RAW_AMR \
    || gm_of == OUTPUT_FORMAT_AMR_NB \
    || gm_of == OUTPUT_FORMAT_AMR_WB \
    || gm_of == OUTPUT_FORMAT_AAC_ADIF \
    || gm_of == OUTPUT_FORMAT_AAC_ADTS \
    || gm_of == OUTPUT_FORMAT_RTP_AVP \
    || gm_of == OUTPUT_FORMAT_MPEG2TS \
    || gm_of == OUTPUT_FORMAT_MP3 \
    || gm_of == OUTPUT_FORMAT_PCM16) {
		bNeedAudio = OMX_TRUE;
		bNeedVideo = OMX_FALSE;
	}

	if (bSetVideoSource == OMX_FALSE)
		bNeedVideo = OMX_FALSE;

	if (gm_capture_time_lapse == OMX_TRUE \
			|| bSetAudioSource == OMX_FALSE)
		bNeedAudio = OMX_FALSE;

	return ret;
}

OMX_ERRORTYPE GMRecorder::LoadComponent(
        OMX_STRING role,
        GMComponent **ppComponent)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GMComponent *pComponent = NULL;

    LOG_DEBUG("Loading component %s ...\n", role);

    *ppComponent = NULL;
    pComponent = FSL_NEW(GMComponent, ());
    if(pComponent == NULL)
        return OMX_ErrorInsufficientResources;

    ret = pComponent->Load(role);
    if(ret != OMX_ErrorNone) {
        FSL_DELETE(pComponent);
        return ret;
    }

    pComponent->RegistSysCallback(SysEventCallback, (OMX_PTR)this);
    pComponent->RegistBufferCallback(&gBufferCb, this);

    *ppComponent = pComponent;

    LOG_DEBUG("Load component %s done.\n", pComponent->name);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE GMRecorder::LoadASComponent()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	ret = LoadComponent((OMX_STRING)AUDIO_SOURCE, &AudioSource);
	if(ret != OMX_ErrorNone) {
		return OMX_ErrorInvalidComponentName;
	}
	return ret;
}

OMX_ERRORTYPE GMRecorder::LoadAEComponent()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 j;

	struct KeyMap {
		int nKey;
		OMX_STRING tag;
	};

	static const KeyMap kKeyMap[] = {
		{ AUDIO_ENCODER_DEFAULT, (OMX_STRING)"audio_encoder.amr" },
		{ AUDIO_ENCODER_AMR_NB, (OMX_STRING)"audio_encoder.amr" },
		{ AUDIO_ENCODER_AMR_WB, (OMX_STRING)"audio_encoder.amr" },
		{ AUDIO_ENCODER_AAC, (OMX_STRING)"audio_encoder.aac" },
		{ AUDIO_ENCODER_AAC_PLUS, (OMX_STRING)"audio_encoder.aacplus" },
		{ AUDIO_ENCODER_EAAC_PLUS, (OMX_STRING)"audio_encoder.eaacplus" },
		{ AUDIO_ENCODER_MP3, (OMX_STRING)"audio_encoder.mp3" },
		{ AUDIO_ENCODER_PCM16, (OMX_STRING)"audio_encoder.pcm" },
	};

	static const size_t kNumMapEntries = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

	for (j = 0; j < kNumMapEntries; j ++) {
		if (gm_ae == kKeyMap[j].nKey) {
			ret = LoadComponent(kKeyMap[j].tag, &AudioEncoder);
			if(ret != OMX_ErrorNone) {
				LOG_ERROR("Can't load component with role: %s\n", kKeyMap[j].tag);
				return OMX_ErrorInvalidComponentName;
			}
			return ret;
		}
	}

       if(j < kNumMapEntries){
           LOG_ERROR("Can't load component with role: %s\n", kKeyMap[j].tag);
       }
	return OMX_ErrorInvalidComponentName;
}

OMX_ERRORTYPE GMRecorder::LoadVSComponent()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 j;

	struct KeyMap {
		int nKey;
		OMX_STRING tag;
	};
	static const KeyMap kKeyMap[] = {
		{ VIDEO_SOURCE_DEFAULT, (OMX_STRING)VIDEO_SOURCE },
		{ VIDEO_SOURCE_CAMERA, (OMX_STRING)VIDEO_SOURCE },
	};

	static const size_t kNumMapEntries = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

	for (j = 0; j < kNumMapEntries; j ++) {
		if (gm_vs == kKeyMap[j].nKey) {
			ret = LoadComponent(kKeyMap[j].tag, &VideoSource);
			if(ret != OMX_ErrorNone) {
				LOG_ERROR("Can't load component with role: %s\n", kKeyMap[j].tag);
				return OMX_ErrorInvalidComponentName;
			}
			return ret;
		}
	}

	LOG_ERROR("Can't load component with role: %s\n", kKeyMap[j].tag);
	return OMX_ErrorInvalidComponentName;
}

OMX_ERRORTYPE GMRecorder::LoadVEComponent()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 j;

	struct KeyMap {
		int nKey;
		OMX_STRING tag;
	};

	static const KeyMap kKeyMap[] = {
		{ VIDEO_ENCODER_DEFAULT, (OMX_STRING)"video_encoder.avc" },
		{ VIDEO_ENCODER_H263, (OMX_STRING)"video_encoder.h263" },
		{ VIDEO_ENCODER_H264, (OMX_STRING)"video_encoder.avc" },
		{ VIDEO_ENCODER_MPEG_4_SP, (OMX_STRING)"video_encoder.mpeg4" },
	};

	static const size_t kNumMapEntries = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

	for (j = 0; j < kNumMapEntries; j ++) {
		if (gm_ve == kKeyMap[j].nKey) {
			ret = LoadComponent(kKeyMap[j].tag, &VideoEncoder);
			if(ret != OMX_ErrorNone) {
				LOG_ERROR("Can't load component with role: %s\n", kKeyMap[j].tag);
				return OMX_ErrorInvalidComponentName;
			}
			return ret;
		}
	}

       if(j < kNumMapEntries){
	    LOG_ERROR("Can't load component with role: %s\n", kKeyMap[j].tag);
       }
	return OMX_ErrorInvalidComponentName;
}

OMX_ERRORTYPE GMRecorder::LoadMuxerComponent()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 j;

	struct KeyMap {
		int nKey;
		OMX_STRING tag;
	};

	static const KeyMap kKeyMap[] = {
		{ OUTPUT_FORMAT_DEFAULT, (OMX_STRING)"muxer.mp4" },
		{ OUTPUT_FORMAT_THREE_GPP, (OMX_STRING)"muxer.3gp" },
		{ OUTPUT_FORMAT_MPEG_4, (OMX_STRING)"muxer.mp4" },
		{ OUTPUT_FORMAT_RAW_AMR, (OMX_STRING)"muxer.amr" },
		{ OUTPUT_FORMAT_AMR_NB, (OMX_STRING)"muxer.amrnb" },
		{ OUTPUT_FORMAT_AMR_WB, (OMX_STRING)"muxer.amrwb" },
		{ OUTPUT_FORMAT_AAC_ADIF, (OMX_STRING)"muxer.aacadif" },
		{ OUTPUT_FORMAT_AAC_ADTS, (OMX_STRING)"muxer.aacadts" },
		{ OUTPUT_FORMAT_RTP_AVP, (OMX_STRING)"muxer.rtp" },
		{ OUTPUT_FORMAT_MPEG2TS, (OMX_STRING)"muxer.mpg" },
		{ OUTPUT_FORMAT_MP3, (OMX_STRING)"muxer.mp3" },
		{ OUTPUT_FORMAT_PCM16, (OMX_STRING)"muxer.pcm" },
	};

	static const size_t kNumMapEntries = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

	gm_of = OUTPUT_FORMAT_DEFAULT;

	for (j = 0; j < kNumMapEntries; j ++) {
		if (gm_of == kKeyMap[j].nKey) {
			ret = LoadComponent(kKeyMap[j].tag, &Muxer);
			if(ret != OMX_ErrorNone) {
				LOG_ERROR("Can't load component with role: %s\n", kKeyMap[j].tag);
				return OMX_ErrorInvalidComponentName;
			}
			return ret;
		}
	}

       if(j <kNumMapEntries){
           LOG_ERROR("Can't load component with role: %s\n", kKeyMap[j].tag);
       }
	return OMX_ErrorInvalidComponentName;
}

OMX_ERRORTYPE GMRecorder::LoadRecorderComponent()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	bNeedAudio = OMX_FALSE;
	bNeedVideo = OMX_FALSE;

	DecidePipeLine();

	if (bNeedAudio == OMX_TRUE) {
		ret = LoadASComponent();
		if(ret != OMX_ErrorNone) {
			LOG_WARNING("LoadASComponent failed.\n");
			bHasAudio = OMX_FALSE;
		}
		if (bHasAudio == OMX_TRUE) {
			ret = LoadAEComponent();
			if(ret != OMX_ErrorNone) {
				AudioSource->UnLoad();
                                FSL_DELETE(AudioSource);
				LOG_WARNING("LoadAEComponent failed.\n");
				bHasAudio = OMX_FALSE;
			}
		}
	} else {
		bHasAudio = OMX_FALSE;
	}

	if (bNeedVideo == OMX_TRUE) {
		ret = LoadVSComponent();
		if(ret != OMX_ErrorNone) {
			LOG_WARNING("LoadVSComponent failed.\n");
			bHasVideo = OMX_FALSE;
		}
		if (bHasVideo == OMX_TRUE) {
			ret = LoadVEComponent();
			if(ret != OMX_ErrorNone) {
				VideoSource->UnLoad();
                                FSL_DELETE(VideoSource);
				LOG_WARNING("LoadVEComponent failed.\n");
				bHasVideo = OMX_FALSE;
			}
		}
	} else {
		bHasVideo = OMX_FALSE;
	}

	if (bHasAudio == OMX_FALSE && bHasVideo == OMX_FALSE) {
		LOG_ERROR("Can't load recorder component.\n");
		return OMX_ErrorComponentNotFound;
	}

	ret = LoadMuxerComponent();
	if(ret != OMX_ErrorNone) {
		if (bHasAudio == OMX_TRUE) {
			AudioSource->UnLoad();
                        FSL_DELETE(AudioSource);
			AudioEncoder->UnLoad();
                        FSL_DELETE(AudioEncoder);
		}
		if (bHasVideo == OMX_TRUE) {
			VideoSource->UnLoad();
                        FSL_DELETE(VideoSource);
			VideoEncoder->UnLoad();
                        FSL_DELETE(VideoEncoder);
		}
		LOG_ERROR("LoadMuxerComponent failed.\n");
		return ret;
	}

	return ret;
}

OMX_ERRORTYPE GMRecorder::SetASParameter()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_AUDIO_PARAM_PCMMODETYPE sParam;

	OMX_INIT_STRUCT(&sParam, OMX_AUDIO_PARAM_PCMMODETYPE);
	sParam.nPortIndex= 0;
	OMX_GetParameter(AudioSource->hComponent, OMX_IndexParamAudioPcm, &sParam);
	sParam.nSamplingRate = gm_sampleRate;
	sParam.nChannels = gm_channels;
	OMX_SetParameter(AudioSource->hComponent, OMX_IndexParamAudioPcm, &sParam);

	if (gm_as) {
		OMX_SetParameter(AudioSource->hComponent, OMX_IndexParamAudioSource, &gm_as);
	}

	OMX_SetParameter(AudioSource->hComponent, OMX_IndexParamMaxFileDuration, &gm_timeUs);

	return ret;
}

OMX_ERRORTYPE GMRecorder::SetAEParameter()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

	OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	sPortDef.nPortIndex = 1;

	OMX_GetParameter(AudioEncoder->hComponent, OMX_IndexParamPortDefinition, &sPortDef);

	switch(sPortDef.format.audio.eEncoding) {
		case OMX_AUDIO_CodingPCM:
			{
				OMX_AUDIO_PARAM_PCMMODETYPE sParam;
				OMX_INIT_STRUCT(&sParam, OMX_AUDIO_PARAM_PCMMODETYPE);
				sParam.nPortIndex = 1;
				OMX_GetParameter(AudioEncoder->hComponent, OMX_IndexParamAudioPcm, &sParam);

				OMX_SetParameter(AudioEncoder->hComponent, OMX_IndexParamAudioPcm, &sParam);
			}
			break;
		case OMX_AUDIO_CodingMP3:
			{
				OMX_AUDIO_PARAM_MP3TYPE sParam;
				OMX_INIT_STRUCT(&sParam, OMX_AUDIO_PARAM_MP3TYPE);
				sParam.nPortIndex = 1;
				OMX_GetParameter(AudioEncoder->hComponent, OMX_IndexParamAudioMp3, &sParam);
				if (gm_audio_bitRate)
					sParam.nBitRate = gm_audio_bitRate;
				OMX_GetParameter(AudioEncoder->hComponent, OMX_IndexParamAudioMp3, &sParam);
			}
			break;
		case OMX_AUDIO_CodingAMR:
			{
				OMX_AUDIO_PARAM_AMRTYPE sParam;
				OMX_INIT_STRUCT(&sParam, OMX_AUDIO_PARAM_AMRTYPE);
				sParam.nPortIndex = 1;
				OMX_GetParameter(AudioEncoder->hComponent, OMX_IndexParamAudioAmr, &sParam);
				if (gm_audio_bitRate)
					sParam.nBitRate = gm_audio_bitRate;
				if (gm_ae == AUDIO_ENCODER_AMR_WB)
					sParam.eAMRBandMode = OMX_AUDIO_AMRBandModeWB0;
				else
					sParam.eAMRBandMode = OMX_AUDIO_AMRBandModeNB0;
				OMX_SetParameter(AudioEncoder->hComponent, OMX_IndexParamAudioAmr, &sParam);
			}
			break;
		case OMX_AUDIO_CodingAAC:
			{
				OMX_AUDIO_PARAM_AACPROFILETYPE sParam;
				OMX_INIT_STRUCT(&sParam, OMX_AUDIO_PARAM_AACPROFILETYPE);
				sParam.nPortIndex = 1;
				OMX_GetParameter(AudioEncoder->hComponent, OMX_IndexParamAudioAac, &sParam);
				if (gm_audio_bitRate)
					sParam.nBitRate = gm_audio_bitRate;
				OMX_SetParameter(AudioEncoder->hComponent, OMX_IndexParamAudioAac, &sParam);
			}
			break;
		default:
			break;
	}

	return ret;
}

OMX_ERRORTYPE GMRecorder::SetVSParameter()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

	OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	sPortDef.nPortIndex = 1;
	OMX_GetParameter(VideoSource->hComponent, OMX_IndexParamPortDefinition, &sPortDef);
	sPortDef.format.video.nFrameWidth = gm_width;
	sPortDef.format.video.nFrameHeight = gm_height;
	sPortDef.format.video.xFramerate = gm_frames_per_second * Q16_SHIFT;
	OMX_SetParameter(VideoSource->hComponent, OMX_IndexParamPortDefinition, &sPortDef);

	if (gm_camera) {
		OMX_SetParameter(VideoSource->hComponent, OMX_IndexParamVideoCamera, gm_camera);
		OMX_SetParameter(VideoSource->hComponent, OMX_IndexParamVideoCameraProxy, gm_camera_proxy);
		OMX_SetParameter(VideoSource->hComponent, OMX_IndexParamVideoCameraId, &gm_cameraId);
	}

	if (gm_surface)
		OMX_SetParameter(VideoSource->hComponent, OMX_IndexParamVideoSurface, gm_surface);

	if (gm_capture_time_lapse == OMX_TRUE)
		OMX_SetParameter(VideoSource->hComponent, OMX_IndexParamTimeLapseUs, &gm_timeBetweenTimeLapseFrameCaptureUs);

	OMX_SetParameter(VideoSource->hComponent, OMX_IndexParamMaxFileDuration, &gm_timeUs);

	return ret;
}

OMX_ERRORTYPE GMRecorder::SetVEParameter()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	OMX_CONFIG_ROTATIONTYPE sRotation;

	OMX_INIT_STRUCT(&sRotation, OMX_CONFIG_ROTATIONTYPE);
	sRotation.nPortIndex = 0;
	sRotation.nRotation = gm_degrees;
	OMX_SetConfig(VideoEncoder->hComponent, OMX_IndexConfigCommonRotate, &sRotation);

	OMX_VIDEO_PARAM_BITRATETYPE sBitrate;

	OMX_INIT_STRUCT(&sBitrate, OMX_VIDEO_PARAM_BITRATETYPE);
	sBitrate.nPortIndex = 1;
	OMX_GetParameter(VideoEncoder->hComponent, OMX_IndexParamVideoBitrate, &sBitrate);
	sBitrate.eControlRate = OMX_Video_ControlRateConstant;
	sBitrate.nTargetBitrate = gm_video_bitRate;
	printf("Video bit rate: %d\n", gm_video_bitRate);
	OMX_SetParameter(VideoEncoder->hComponent, OMX_IndexParamVideoBitrate, &sBitrate);

	OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
	OMX_S32 PFrames;

	OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	sPortDef.nPortIndex = 1;
	OMX_GetParameter(VideoEncoder->hComponent, OMX_IndexParamPortDefinition, &sPortDef);
	sPortDef.format.video.nBitrate = gm_video_bitRate;
	OMX_SetParameter(VideoEncoder->hComponent, OMX_IndexParamPortDefinition, &sPortDef);

	// If seconds <  0, only the first frame is I frame, and rest are all P frames
	// If seconds == 0, all frames are encoded as I frames. No P frames
	// If seconds >  0, it is the time spacing (seconds) between 2 neighboring I frames
	if (gm_seconds > 0)
		PFrames = gm_frames_per_second * gm_seconds - 1;
	else if (gm_seconds < 0)
		PFrames = 0xffffffff;
	else
		PFrames = 0;

	switch(sPortDef.format.video.eCompressionFormat) {
		case OMX_VIDEO_CodingAVC:
			{
				OMX_VIDEO_PARAM_AVCTYPE sParam;
				OMX_INIT_STRUCT(&sParam, OMX_VIDEO_PARAM_AVCTYPE);
				OMX_GetParameter(VideoEncoder->hComponent, OMX_IndexParamVideoAvc, &sParam);
				sParam.eProfile = (OMX_VIDEO_AVCPROFILETYPE)gm_profile;
				sParam.eLevel = (OMX_VIDEO_AVCLEVELTYPE)gm_level;
				sParam.nPFrames = PFrames;
				OMX_SetParameter(VideoEncoder->hComponent, OMX_IndexParamVideoAvc, &sParam);
			}
			break;
		case OMX_VIDEO_CodingMPEG4:
			{
				OMX_VIDEO_PARAM_MPEG4TYPE sParam;
				OMX_INIT_STRUCT(&sParam, OMX_VIDEO_PARAM_MPEG4TYPE);
				OMX_GetParameter(VideoEncoder->hComponent, OMX_IndexParamVideoMpeg4, &sParam);
				sParam.eProfile = (OMX_VIDEO_MPEG4PROFILETYPE)gm_profile;
				sParam.eLevel = (OMX_VIDEO_MPEG4LEVELTYPE)gm_level;
				sParam.nPFrames = PFrames;
				OMX_GetParameter(VideoEncoder->hComponent, OMX_IndexParamVideoMpeg4, &sParam);
			}
			break;
		case OMX_VIDEO_CodingH263:
			{
				OMX_VIDEO_PARAM_H263TYPE sParam;
				OMX_INIT_STRUCT(&sParam, OMX_VIDEO_PARAM_H263TYPE);
				OMX_GetParameter(VideoEncoder->hComponent, OMX_IndexParamVideoH263, &sParam);
				sParam.eProfile = (OMX_VIDEO_H263PROFILETYPE)gm_profile;
				sParam.eLevel = (OMX_VIDEO_H263LEVELTYPE)gm_level;
				sParam.nPFrames = PFrames;
				OMX_SetParameter(VideoEncoder->hComponent, OMX_IndexParamVideoH263, &sParam);
			}
			break;
		default:
			break;
	}

	return ret;
}

OMX_ERRORTYPE GMRecorder::SetMuxerParameter()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 len;
    OMX_U8 *ptr = NULL;

    len = fsl_osal_strlen(gm_path) + 1;
    ptr = (OMX_U8*)FSL_MALLOC(sizeof(OMX_PARAM_CONTENTURITYPE) + len);
    if(ptr == NULL) {
        fsl_osal_mutex_unlock(Lock);
        return OMX_ErrorInsufficientResources;
    }

    Content = (OMX_PARAM_CONTENTURITYPE*)ptr;
    fsl_osal_strcpy((fsl_osal_char*)(&Content->contentURI), gm_path);

    printf("Loading content: %s\n", gm_path);

    ret = OMX_SetParameter(Muxer->hComponent, OMX_IndexParamContentURI, Content);
    if(ret != OMX_ErrorNone) {
		LOG_ERROR("Set URI to Muxer failed.\n");
		return ret;
	}

    ret = GetContentPipe(&hPipe);
    if(ret != OMX_ErrorNone)
		return ret;

    if(hPipe != NULL) {
        OMX_PARAM_CONTENTPIPETYPE sPipe;
        OMX_INIT_STRUCT(&sPipe, OMX_PARAM_CONTENTPIPETYPE);
        sPipe.hPipe = hPipe;
        ret = OMX_SetParameter(Muxer->hComponent, OMX_IndexParamCustomContentPipe, &sPipe);
        if(ret != OMX_ErrorNone)
            return ret;
    }

	OMX_SetParameter(Muxer->hComponent, OMX_IndexParamMaxFileSize, &gm_bytes);
	OMX_SetParameter(Muxer->hComponent, OMX_IndexParamInterleaveUs, &gm_durationUs);

	if (gm_Longitudex10000 > -3600000 && gm_Latitudex10000 > -3600000) {
		OMX_SetParameter(Muxer->hComponent, OMX_IndexParamLongitudex, &gm_Longitudex10000);
		OMX_SetParameter(Muxer->hComponent, OMX_IndexParamLatitudex, &gm_Latitudex10000);
	}

	return ret;
}

OMX_ERRORTYPE GMRecorder::ConnectComponent(
        GMComponent *pOutComp,
        OMX_U32 nOutPortIndex,
        GMComponent *pInComp,
        OMX_U32 nInPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = pOutComp->Link(nOutPortIndex,pInComp,nInPortIndex);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Connect component[%s:%d] with component[%s:%d] failed.\n",
                pOutComp->name, nOutPortIndex, pInComp->name, nInPortIndex);
        return ret;
    }

    ret = pInComp->Link(nInPortIndex, pOutComp, nOutPortIndex);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Connect component[%s:%d] with component[%s:%d] failed.\n",
                pInComp->name, nInPortIndex, pOutComp->name, nOutPortIndex);
        pOutComp->Link(nOutPortIndex, NULL, 0);
        return ret;
    }

    LOG_DEBUG("Connect component[%s:%d] with component[%s:%d] success.\n",
            pOutComp->name, nOutPortIndex, pInComp->name, nInPortIndex);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMRecorder::DisConnectComponent(
        GMComponent *pOutComp,
        OMX_U32 nOutPortIndex,
        GMComponent *pInComp,
        OMX_U32 nInPortIndex)
{
    pOutComp->Link(nOutPortIndex, NULL, 0);
    pInComp->Link(nInPortIndex, NULL, 0);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMRecorder::SetupPipeline()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (bHasAudio == OMX_TRUE) {
		SetASParameter();
		SetAEParameter();

		OMX_AUDIO_PARAM_PORTFORMATTYPE sAudioFmt;
		OMX_INIT_STRUCT(&sAudioFmt, OMX_AUDIO_PARAM_PORTFORMATTYPE);
		sAudioFmt.nPortIndex = 1;
		OMX_GetParameter(AudioEncoder->hComponent, OMX_IndexParamAudioPortFormat, &sAudioFmt);
		sAudioFmt.nPortIndex = Muxer->PortPara[AUDIO_PORT_PARA].nStartPortNumber;
		OMX_SetParameter(Muxer->hComponent, OMX_IndexParamAudioPortFormat, &sAudioFmt);
		Muxer->PortEnable(Muxer->PortPara[AUDIO_PORT_PARA].nStartPortNumber);

		ConnectComponent(AudioSource, 0, AudioEncoder, 0);
		Pipeline->Add(AudioSource);
		Pipeline->Add(AudioEncoder);
		ConnectComponent(AudioEncoder, 1, Muxer, 0);
	} else {
		Muxer->PortDisable(0);
	}

	if (bHasVideo == OMX_TRUE) {
		SetVSParameter();
		SetVEParameter();

		OMX_VIDEO_PARAM_PORTFORMATTYPE sVideoFmt;
		OMX_INIT_STRUCT(&sVideoFmt, OMX_VIDEO_PARAM_PORTFORMATTYPE);
		sVideoFmt.nPortIndex = 1;
		OMX_GetParameter(VideoEncoder->hComponent, OMX_IndexParamVideoPortFormat, &sVideoFmt);
		sVideoFmt.nPortIndex = Muxer->PortPara[VIDEO_PORT_PARA].nStartPortNumber;
		OMX_SetParameter(Muxer->hComponent, OMX_IndexParamVideoPortFormat, &sVideoFmt);
		Muxer->PortEnable(Muxer->PortPara[VIDEO_PORT_PARA].nStartPortNumber);

		ConnectComponent(VideoSource, 1, VideoEncoder, 0);
		Pipeline->Add(VideoSource);
		Pipeline->Add(VideoEncoder);
		ConnectComponent(VideoEncoder, 1, Muxer, 1);
	} else {
		Muxer->PortDisable(1);
	}

	Pipeline->Add(Muxer);
	SetMuxerParameter();

	return ret;
}

OMX_ERRORTYPE GMRecorder::GetContentPipe(CP_PIPETYPE **Pipe)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    *Pipe = NULL;
    if(fsl_osal_strstr((fsl_osal_char*)(gm_path), "sharedfd://"))
        ret = OMX_GetContentPipe((void**)Pipe, (OMX_STRING)"SHAREDFD_PIPE");
    else if(fsl_osal_strstr((fsl_osal_char*)(gm_path), "http://"))
        ret = OMX_GetContentPipe((void**)Pipe, (OMX_STRING)"HTTPS_PIPE");
    else
        ret = OMX_GetContentPipe((void**)Pipe, (OMX_STRING)"LOCAL_FILE_PIPE_NEW");

    if(*Pipe == NULL)
        return OMX_ErrorContentPipeCreationFailed;

    return ret;
}

OMX_ERRORTYPE GMRecorder::LoadClock()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_TIME_CONFIG_CLOCKSTATETYPE ClockState;
    OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE RefClock;

    ret = LoadComponent((OMX_STRING)"clocksrc", &Clock);
    if(ret != OMX_ErrorNone)
        return ret;

    OMX_S32 i;
    for(i=0; i<(OMX_S32)Clock->nPorts; i++)
        Clock->PortDisable(i);

    Clock->StateTransUpWard(OMX_StateIdle);
    Clock->StateTransUpWard(OMX_StateExecuting);

    OMX_INIT_STRUCT(&ClockState, OMX_TIME_CONFIG_CLOCKSTATETYPE);
    ClockState.eState = OMX_TIME_ClockStateWaitingForStartTime;
    if(bHasAudio == OMX_TRUE)
        ClockState.nWaitMask |= 1 << AUDIO_CLOCK_PORT;
    if(bHasVideo == OMX_TRUE)
        ClockState.nWaitMask |= 1 << VIDEO_CLOCK_PORT;
    OMX_SetConfig(Clock->hComponent, OMX_IndexConfigTimeClockState, &ClockState);

    OMX_INIT_STRUCT(&RefClock, OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE);
    if(bHasAudio == OMX_TRUE)
        RefClock.eClock = OMX_TIME_RefClockAudio;
    else
        RefClock.eClock = OMX_TIME_RefClockVideo;
    OMX_SetConfig(Clock->hComponent, OMX_IndexConfigTimeActiveRefClock, &RefClock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMRecorder::RemoveClock()
{
    OMX_TIME_CONFIG_CLOCKSTATETYPE ClockState;

    if(Clock == NULL)
        return OMX_ErrorNone;

    OMX_INIT_STRUCT(&ClockState, OMX_TIME_CONFIG_CLOCKSTATETYPE);
    ClockState.eState = OMX_TIME_ClockStateStopped;
    OMX_SetConfig(Clock->hComponent, OMX_IndexConfigTimeClockState, &ClockState);

    if(AudioSource != NULL)
        DeAttachClock(AUDIO_CLOCK_PORT, AudioSource, 1);
    if(VideoSource != NULL)
        DeAttachClock(VIDEO_CLOCK_PORT, VideoSource, 2);

    Clock->StateTransDownWard(OMX_StateIdle);
    Clock->StateTransDownWard(OMX_StateLoaded);

    Clock->UnLoad();
    FSL_DELETE(Clock);
    Clock = NULL;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMRecorder::AttachClock(
        OMX_U32 nClockPortIndex,
        GMComponent *pComponent,
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = OMX_SetupTunnel(Clock->hComponent, nClockPortIndex, pComponent->hComponent, nPortIndex);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Can't attach clock to component %s in port %d\n", pComponent->name, nPortIndex);
        return ret;
    }

    pComponent->TunneledPortEnable(nPortIndex);
    Clock->TunneledPortEnable(nClockPortIndex);
    Clock->WaitTunneledPortEnable(nClockPortIndex);
    pComponent->WaitTunneledPortEnable(nPortIndex);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMRecorder::DeAttachClock(
        OMX_U32 nClockPortIndex,
        GMComponent *pComponent,
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret1 = OMX_ErrorNone;
    OMX_ERRORTYPE ret2 = OMX_ErrorNone;

    Clock->TunneledPortDisable(nClockPortIndex);
    pComponent->TunneledPortDisable(nPortIndex);
    pComponent->WaitTunneledPortDisable(nPortIndex);
    Clock->WaitTunneledPortDisable(nClockPortIndex);

    OMX_SetupTunnel(Clock->hComponent, nClockPortIndex, NULL, 0);
    OMX_SetupTunnel(pComponent->hComponent, nPortIndex, NULL, 0);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE GMRecorder::StartPipeline()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (bHasAudio == OMX_TRUE) {
        ret = AudioSource->StateTransUpWard(OMX_StateIdle);
        if(ret != OMX_ErrorNone)
            return ret;
		AttachClock(AUDIO_CLOCK_PORT, AudioSource, 1);
        ret = AudioSource->StateTransUpWard(OMX_StatePause);
        if(ret != OMX_ErrorNone)
            return ret;
		ret = AudioSource->StartProcessNoWait();
		if(ret != OMX_ErrorNone) {
			fsl_osal_mutex_unlock(Lock);
			return ret;
		}

		ret = AudioEncoder->StateTransUpWard(OMX_StateIdle);
        if(ret != OMX_ErrorNone)
            return ret;
        ret = AudioEncoder->StateTransUpWard(OMX_StateExecuting);
        if(ret != OMX_ErrorNone)
            return ret;
        ret = AudioSource->PassOutBufferToPeer(0);
        if(ret != OMX_ErrorNone)
            return ret;
        ret = AudioEncoder->StartProcessNoWait();
        if(ret != OMX_ErrorNone)
            return ret;
 	}

	if (bHasVideo == OMX_TRUE) {
#ifdef ANDROID_BUILD
		// workaround for camera can't get memory.
		//VideoEncoder->SetPortBufferAllocateType(0, GM_SELF_ALLOC);
		//VideoSource->SetPortBufferAllocateType(1, GM_PEER_ALLOC);
#endif
		ret = VideoSource->StateTransUpWard(OMX_StateIdle);
        if(ret != OMX_ErrorNone)
            return ret;
		AttachClock(VIDEO_CLOCK_PORT, VideoSource, 2);
        ret = VideoSource->StateTransUpWard(OMX_StateExecuting);
        if(ret != OMX_ErrorNone)
            return ret;
        ret = VideoSource->StartProcessNoWait();
        if(ret != OMX_ErrorNone)
            return ret;

        ret = VideoEncoder->StateTransUpWard(OMX_StateIdle);
        if(ret != OMX_ErrorNone)
            return ret;
        ret = VideoEncoder->StateTransUpWard(OMX_StateExecuting);
        if(ret != OMX_ErrorNone)
            return ret;
        ret = VideoSource->PassOutBufferToPeer(1);
        if(ret != OMX_ErrorNone)
            return ret;
        ret = VideoEncoder->StartProcessNoWait();
        if(ret != OMX_ErrorNone)
            return ret;
	}

	ret = Muxer->StateTransUpWard(OMX_StateIdle);
	if(ret != OMX_ErrorNone)
		return ret;
	ret = Muxer->StateTransUpWard(OMX_StateExecuting);
	if(ret != OMX_ErrorNone)
		return ret;
	if (bHasAudio == OMX_TRUE) {
		ret = AudioEncoder->PassOutBufferToPeer(1);
		if(ret != OMX_ErrorNone)
			return ret;
	}
	if (bHasVideo == OMX_TRUE) {
		ret = VideoEncoder->PassOutBufferToPeer(1);
		if(ret != OMX_ErrorNone)
			return ret;
	}
	ret = Muxer->StartProcessNoWait();
	if(ret != OMX_ErrorNone)
		return ret;

	return ret;
}

OMX_ERRORTYPE GMRecorder::start()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    fsl_osal_mutex_lock(Lock);

	if (bError == OMX_TRUE) {
		fsl_osal_mutex_unlock(Lock);
        return OMX_ErrorBadParameter;
	}

    if(State != RECORDER_STATE_LOADED && State != RECORDER_STATE_PAUSE) {
        fsl_osal_mutex_unlock(Lock);
        return OMX_ErrorIncorrectStateOperation;
    }

    printf("start recorder.\n");

	if (bHasVideo == OMX_TRUE) {
		OMX_CONFIG_BOOLEANTYPE sCapturing;
		OMX_INIT_STRUCT(&sCapturing, OMX_CONFIG_BOOLEANTYPE);
		sCapturing.bEnabled = OMX_TRUE;
		ret = OMX_SetConfig(VideoSource->hComponent,OMX_IndexConfigCapturing, &sCapturing);
		if (ret != OMX_ErrorNone) {
			fsl_osal_mutex_unlock(Lock);
			return ret;
		}
	}

	if (bHasAudio == OMX_TRUE) {
		ret = AudioSource->StateTransUpWard(OMX_StateExecuting);
		if(ret != OMX_ErrorNone) {
			fsl_osal_mutex_unlock(Lock);
			return ret;
		}
	}

    State = RECORDER_STATE_PLAYING;
    fsl_osal_mutex_unlock(Lock);

	return ret;
}

OMX_ERRORTYPE GMRecorder::pause()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    fsl_osal_mutex_lock(Lock);

    if(State != RECORDER_STATE_PLAYING) {
        fsl_osal_mutex_unlock(Lock);
        return OMX_ErrorIncorrectStateOperation;
    }

    printf("Pause recorder.\n");


	if (bHasVideo == OMX_TRUE) {
		OMX_CONFIG_BOOLEANTYPE sCapturing;

		OMX_INIT_STRUCT(&sCapturing, OMX_CONFIG_BOOLEANTYPE);
		sCapturing.bEnabled = OMX_FALSE;
		ret = OMX_SetConfig(VideoSource->hComponent,OMX_IndexConfigCapturing, &sCapturing);
		if(ret != OMX_ErrorNone) {
			fsl_osal_mutex_unlock(Lock);
			return ret;
		}
	}

	if (bHasAudio == OMX_TRUE) {
		ret = AudioSource->StateTransDownWard(OMX_StatePause);
		if(ret != OMX_ErrorNone) {
			fsl_osal_mutex_unlock(Lock);
			return ret;
		}
	}

    if(Clock != NULL)
        Clock->StateTransDownWard(OMX_StatePause);

    State = RECORDER_STATE_PAUSE;

    fsl_osal_mutex_unlock(Lock);

	return ret;
}

OMX_ERRORTYPE GMRecorder::WaitStop()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	OMX_CONFIG_BOOLEANTYPE sEOS;

	OMX_INIT_STRUCT(&sEOS, OMX_CONFIG_BOOLEANTYPE);
	sEOS.bEnabled = OMX_TRUE;

	if (bHasAudio == OMX_TRUE)
		OMX_SetConfig(AudioSource->hComponent,OMX_IndexConfigEOS, &sEOS);
	if (bHasVideo == OMX_TRUE)
		OMX_SetConfig(VideoSource->hComponent,OMX_IndexConfigEOS, &sEOS);

    while(1) {
        LOG_DEBUG("audio: %d, video: %d, audio eos: %d, video eos: %d\n",
                bHasAudio, bHasVideo, bAudioEos, bVideoEos);
        if(((bHasAudio && bAudioEos) || !bHasAudio)
                && ((bHasVideo && bVideoEos) || !bHasVideo))
            break;
        fsl_osal_sleep(50000);
    }

	return ret;
}

OMX_ERRORTYPE GMRecorder::stop()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
    GMComponent *pComponent = NULL;
    OMX_U32 ComponentNum = 0;
    OMX_S32 i;

    fsl_osal_mutex_lock(Lock);

    if(State == RECORDER_STATE_STOP) {
        fsl_osal_mutex_unlock(Lock);
        return OMX_ErrorIncorrectStateOperation;
    }

    printf("Stop recorder.\n");

    State = RECORDER_STATE_STOP;

	if (bError != OMX_TRUE) {
		WaitStop();
	}

    //stop clock
    if(Clock != NULL) {
        OMX_TIME_CONFIG_CLOCKSTATETYPE ClockState;
        OMX_INIT_STRUCT(&ClockState, OMX_TIME_CONFIG_CLOCKSTATETYPE);
        OMX_GetConfig(Clock->hComponent, OMX_IndexConfigTimeClockState, &ClockState);
        ClockState.eState = OMX_TIME_ClockStateStopped;
        OMX_SetConfig(Clock->hComponent, OMX_IndexConfigTimeClockState, &ClockState);
    }

    ComponentNum = Pipeline->GetNodeCnt();
    for(i=0; i<(OMX_S32)ComponentNum; i++) {
        pComponent = Pipeline->GetNode(i);
        LOG_DEBUG("Stop %s step1.\n", pComponent->name);
        pComponent->StateTransDownWard(OMX_StateIdle);
    }

    RemoveClock();

    for(i=0; i<(OMX_S32)ComponentNum; i++) {
        pComponent = Pipeline->GetNode(0);
        LOG_DEBUG("Stop %s step2.\n", pComponent->name);
        Pipeline->Remove(pComponent);
        pComponent->StateTransDownWard(OMX_StateLoaded);
        pComponent->UnLoad();
        FSL_DELETE(pComponent);
    }

    reset();

	if(pAppCallback != NULL) 
		(*pAppCallback)(pAppData, RECORDER_EVENT_COMPLETION_STATUS, 0);

    fsl_osal_mutex_unlock(Lock);
	fsl_osal_sleep(1000);

	return ret;
}

OMX_ERRORTYPE GMRecorder::close()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	stop();
	return ret;
}

OMX_ERRORTYPE GMRecorder::getMaxAmplitude(OMX_S32 *max)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    fsl_osal_mutex_lock(Lock);

	if (AudioSource)
		OMX_GetConfig(AudioSource->hComponent, OMX_IndexConfigMaxAmplitude, max);

    fsl_osal_mutex_unlock(Lock);
	return ret;
}

OMX_ERRORTYPE GMRecorder::getMediaTime(OMX_TICKS *pMediaTime)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    OMX_TIME_CONFIG_TIMESTAMPTYPE sCur;

    fsl_osal_mutex_lock(Lock);
    OMX_INIT_STRUCT(&sCur, OMX_TIME_CONFIG_TIMESTAMPTYPE);
    sCur.nPortIndex = OMX_ALL;
	if (Clock)
		OMX_GetConfig(Clock->hComponent, OMX_IndexConfigTimeCurrentMediaTime, &sCur);
    *pMediaTime = sCur.nTimestamp;

    fsl_osal_mutex_unlock(Lock);

	return ret;
}






