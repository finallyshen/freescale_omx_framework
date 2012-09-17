/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <ctype.h>
#include <OMXMediaRecorder.h>
#include <Surface.h>
#include "Mem.h"
#include "Log.h"
#include "OMX_Recorder.h"

using namespace android;

int eventhandler(void* context, RECORDER_EVENT eventID, void* Eventpayload)
{
	OMXRecorder* mRecorder = (OMXRecorder*) context;
	switch(eventID) {
		case RECORDER_EVENT_ERROR_UNKNOWN:
			{
				mRecorder->ProcessEvent(MEDIA_RECORDER_EVENT_ERROR, \
						MEDIA_RECORDER_ERROR_UNKNOWN, 0);
				break;
			}
		case RECORDER_EVENT_UNKNOWN:
			{
				mRecorder->ProcessEvent(MEDIA_RECORDER_EVENT_INFO, \
						MEDIA_RECORDER_INFO_UNKNOWN, 0);
				break;
			}
		case RECORDER_EVENT_MAX_DURATION_REACHED:
			{
                mRecorder->ProcessEvent(MEDIA_RECORDER_EVENT_INFO, \
						MEDIA_RECORDER_INFO_MAX_DURATION_REACHED, 0);
				break;
			}
		case RECORDER_EVENT_MAX_FILESIZE_REACHED:
			{
                mRecorder->ProcessEvent(MEDIA_RECORDER_EVENT_INFO, \
						MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED, 0);
				break;
			}
#ifdef ICS
		case RECORDER_EVENT_COMPLETION_STATUS:
			{
				mRecorder->ProcessEvent(MEDIA_RECORDER_TRACK_EVENT_INFO, \
						MEDIA_RECORDER_TRACK_INFO_COMPLETION_STATUS, 0);
				break;
			}
		case RECORDER_EVENT_PROGRESS_TIME_STATUS:
			{
				mRecorder->ProcessEvent(MEDIA_RECORDER_TRACK_EVENT_INFO, \
						MEDIA_RECORDER_TRACK_INFO_PROGRESS_IN_TIME, 0);
				break;
			}
#else
		case RECORDER_EVENT_COMPLETION_STATUS:
			{
				mRecorder->ProcessEvent(MEDIA_RECORDER_EVENT_INFO, \
						MEDIA_RECORDER_INFO_COMPLETION_STATUS, 0);
				break;
			}
		case RECORDER_EVENT_PROGRESS_TIME_STATUS:
			{
				mRecorder->ProcessEvent(MEDIA_RECORDER_EVENT_INFO, \
						MEDIA_RECORDER_INFO_PROGRESS_TIME_STATUS, 0);
				break;
			}
#endif
		default:
			break;
	}

	return 1;
}

status_t OMXRecorder::ProcessEvent(
		int msg, int ext1, int ext2)
{
	if (mListener == NULL)
        return BAD_VALUE;

	mListener->notify(msg, ext1, ext2);

    return NO_ERROR;
}

OMXRecorder::OMXRecorder()
{
	OMX_Recorder* mRecorder = NULL;

    LOG_DEBUG("OMXRecorder constructor.\n");

    recorder = NULL;

    LogInit(-1, NULL);

	mRecorder = OMX_RecorderCreate();
    if(mRecorder == NULL) {
        LOG_ERROR("Failed to create GMRecorder.\n");
    }

	if(mRecorder != NULL) 
		mRecorder->registerEventHandler(mRecorder, this, eventhandler);

    LOG_DEBUG("OMXRecorder Construct mRecorder recorder: %p\n", mRecorder);
    recorder = (void*)mRecorder;
	cameraPtr = NULL;
	cameraProxyPtr = NULL;
	mCamera = 0;
}

OMXRecorder::~OMXRecorder()
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder de-constructor %p.\n", mRecorder);

    if(mRecorder != NULL) {
        mRecorder->deleteIt(mRecorder);
        mRecorder = NULL;
		recorder = NULL;
    }

    LogDeInit();
}

status_t OMXRecorder::init()
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder init\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    //if(OMX_TRUE != mRecorder->init(mRecorder))
      //  return BAD_VALUE;

	// walkaround for setCamera before init.
	if (cameraPtr)
		if(OMX_TRUE != mRecorder->setCamera(mRecorder, cameraPtr, cameraProxyPtr))
			return BAD_VALUE;

    return NO_ERROR;
}

#ifdef ICS
status_t OMXRecorder::setAudioSource(
		audio_source_t as)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setAudioSource\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->setAudioSource(mRecorder, (OMX_S32)as))
        return BAD_VALUE;

    return NO_ERROR;
}
#else
status_t OMXRecorder::setAudioSource(
		audio_source as)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setAudioSource\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->setAudioSource(mRecorder, (OMX_S32)as))
        return BAD_VALUE;

    return NO_ERROR;
}
#endif

status_t OMXRecorder::setVideoSource(
		video_source vs)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setVideoSource\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->setVideoSource(mRecorder, (video_source_gm)vs))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::setOutputFormat(
		output_format of)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setOutputFormat\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->setOutputFormat(mRecorder, (output_format_gm)of))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::setAudioEncoder(
		audio_encoder ae)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setAudioEncoder\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->setAudioEncoder(mRecorder, (audio_encoder_gm)ae))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::setVideoEncoder(
		video_encoder ve)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setVideoEncoder\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->setVideoEncoder(mRecorder, (video_encoder_gm)ve))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::setVideoSize(
		int width, int height)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setVideoSize\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->setVideoSize(mRecorder, width, height))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::setVideoFrameRate(
		int frames_per_second)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setVideoFrameRate\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->setVideoFrameRate(mRecorder, frames_per_second))
        return BAD_VALUE;

    return NO_ERROR;
}

#ifdef ICS
status_t OMXRecorder::setCamera(
		const sp<ICamera>& camera, const sp<ICameraRecordingProxy>& proxy)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

	if (mRecorder == NULL)
        return BAD_VALUE;

    LOG_DEBUG("OMXRecorder setCamera\n");
	mCamera = camera;
	mCameraProxy = proxy;
	if(OMX_TRUE != mRecorder->setCamera(mRecorder, (OMX_PTR)(&mCamera), (OMX_PTR)(&mCameraProxy)))
		return BAD_VALUE;

	cameraPtr = (OMX_PTR)(&mCamera);
	cameraProxyPtr = (OMX_PTR)(&mCameraProxy);

    return NO_ERROR;
}
#endif

status_t OMXRecorder::setCamera(
		const sp<ICamera>& camera)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setCamera\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

	mCamera = camera;
	if(OMX_TRUE != mRecorder->setCamera(mRecorder, (OMX_PTR)(&mCamera), NULL))
		return BAD_VALUE;

	cameraPtr = (OMX_PTR)(&mCamera);

    return NO_ERROR;
}

status_t OMXRecorder::setPreviewSurface(
		const sp<ISurface>& surface)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setPreviewSurface\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

	mPreviewSurface = surface;

    if(OMX_TRUE != mRecorder->setPreviewSurface(mRecorder, (OMX_PTR)(&mPreviewSurface)))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::setPreviewSurface(
		const sp<Surface>& surface)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setPreviewSurface\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

	mPreviewSurface2 = surface;

    if(OMX_TRUE != mRecorder->setPreviewSurface(mRecorder, (OMX_PTR)(&mPreviewSurface2)))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::setOutputFile(
		const char *path)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setOutputFile\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->setOutputFile(mRecorder, (OMX_STRING)path))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::setOutputFile(
		int fd, int64_t offset, int64_t length)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder setOutputFile\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->setOutputFileFD(mRecorder, fd, offset, length))
        return BAD_VALUE;

    return NO_ERROR;
}

// Attempt to parse an int64 literal optionally surrounded by whitespace,
// returns true on success, false otherwise.
static bool safe_strtoi64(const char *s, int64_t *val) {
    char *end;
    *val = strtoll(s, &end, 10);

    if (end == s || errno == ERANGE) {
        return false;
    }

    // Skip trailing whitespace
    while (isspace(*end)) {
        ++end;
    }

    // For a successful return, the string must contain nothing but a valid
    // int64 literal optionally surrounded by whitespace.

    return *end == '\0';
}

// Return true if the value is in [0, 0x007FFFFFFF]
static bool safe_strtoi32(const char *s, int32_t *val) {
    int64_t temp;
    if (safe_strtoi64(s, &temp)) {
        if (temp >= 0 && temp <= 0x007FFFFFFF) {
            *val = static_cast<int32_t>(temp);
            return true;
        }
    }
    return false;
}

// Trim both leading and trailing whitespace from the given string.
static void TrimString(String8 *s) {
    size_t num_bytes = s->bytes();
    const char *data = s->string();

    size_t leading_space = 0;
    while (leading_space < num_bytes && isspace(data[leading_space])) {
        ++leading_space;
    }

    size_t i = num_bytes;
    while (i > leading_space && isspace(data[i - 1])) {
        --i;
    }

    s->setTo(String8(&data[leading_space], i - leading_space));
}

status_t OMXRecorder::setParameter(
        const String8 &key, const String8 &value) {
    LOG_LOG("setParameter: key (%s) => value (%s)", key.string(), value.string());

    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

	if (mRecorder == NULL)
        return BAD_VALUE;

    if (key == "max-duration") {
        int64_t max_duration_ms;
        if (safe_strtoi64(value.string(), &max_duration_ms)) {
            if(OMX_TRUE != mRecorder->setParamMaxFileDurationUs(mRecorder, 1000LL * max_duration_ms))
				return BAD_VALUE;
			return NO_ERROR;
		}
    } else if (key == "max-filesize") {
        int64_t max_filesize_bytes;
        if (safe_strtoi64(value.string(), &max_filesize_bytes)) {
            if(OMX_TRUE != mRecorder->setParamMaxFileSizeBytes(mRecorder, max_filesize_bytes))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "interleave-duration-us") {
        int32_t durationUs;
        if (safe_strtoi32(value.string(), &durationUs)) {
            if(OMX_TRUE != mRecorder->setParamInterleaveDuration(mRecorder, durationUs))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "param-movie-time-scale") {
        int32_t timeScale;
        if (safe_strtoi32(value.string(), &timeScale)) {
            if(OMX_TRUE != mRecorder->setParamMovieTimeScale(mRecorder, timeScale))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "param-use-64bit-offset") {
        int32_t use64BitOffset;
        if (safe_strtoi32(value.string(), &use64BitOffset)) {
            if(OMX_TRUE != mRecorder->setParam64BitFileOffset(mRecorder, (OMX_BOOL)(use64BitOffset != 0)))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "param-geotag-longitude") {
        int64_t longitudex10000;
        if (safe_strtoi64(value.string(), &longitudex10000)) {
            if(OMX_TRUE != mRecorder->setParamGeotagLongitude(mRecorder, longitudex10000))
				return BAD_VALUE;
			return NO_ERROR;
       }
    } else if (key == "param-geotag-latitude") {
        int64_t latitudex10000;
        if (safe_strtoi64(value.string(), &latitudex10000)) {
            if(OMX_TRUE != mRecorder->setParamGeotagLatitude(mRecorder, latitudex10000))
				return BAD_VALUE;
			return NO_ERROR;
       }
    } else if (key == "param-track-time-status") {
        int64_t timeDurationUs;
        if (safe_strtoi64(value.string(), &timeDurationUs)) {
            if(OMX_TRUE != mRecorder->setParamTrackTimeStatus(mRecorder, timeDurationUs))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "audio-param-sampling-rate") {
        int32_t sampling_rate;
        if (safe_strtoi32(value.string(), &sampling_rate)) {
            if(OMX_TRUE != mRecorder->setParamAudioSamplingRate(mRecorder, sampling_rate))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "audio-param-number-of-channels") {
        int32_t number_of_channels;
        if (safe_strtoi32(value.string(), &number_of_channels)) {
            if(OMX_TRUE != mRecorder->setParamAudioNumberOfChannels(mRecorder, number_of_channels))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "audio-param-encoding-bitrate") {
        int32_t audio_bitrate;
        if (safe_strtoi32(value.string(), &audio_bitrate)) {
            if(OMX_TRUE != mRecorder->setParamAudioEncodingBitRate(mRecorder, audio_bitrate))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "audio-param-time-scale") {
        int32_t timeScale;
        if (safe_strtoi32(value.string(), &timeScale)) {
            if(OMX_TRUE != mRecorder->setParamAudioTimeScale(mRecorder, timeScale))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "video-param-encoding-bitrate") {
        int32_t video_bitrate;
        if (safe_strtoi32(value.string(), &video_bitrate)) {
            if(OMX_TRUE != mRecorder->setParamVideoEncodingBitRate(mRecorder, video_bitrate))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "video-param-rotation-angle-degrees") {
        int32_t degrees;
        if (safe_strtoi32(value.string(), &degrees)) {
            if(OMX_TRUE != mRecorder->setParamVideoRotation(mRecorder, degrees))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "video-param-i-frames-interval") {
        int32_t seconds;
        if (safe_strtoi32(value.string(), &seconds)) {
            if(OMX_TRUE != mRecorder->setParamVideoIFramesInterval(mRecorder, seconds))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "video-param-encoder-profile") {
        int32_t profile;
        if (safe_strtoi32(value.string(), &profile)) {
            if(OMX_TRUE != mRecorder->setParamVideoEncoderProfile(mRecorder, profile))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "video-param-encoder-level") {
        int32_t level;
        if (safe_strtoi32(value.string(), &level)) {
            if(OMX_TRUE != mRecorder->setParamVideoEncoderLevel(mRecorder, level))
				return BAD_VALUE;
			return NO_ERROR;
        }
    } else if (key == "video-param-camera-id") {
        int32_t cameraId;
        if (safe_strtoi32(value.string(), &cameraId)) {
            if(OMX_TRUE != mRecorder->setParamVideoCameraId(mRecorder, cameraId))
				return BAD_VALUE;
			return NO_ERROR;
		}
	} else if (key == "video-param-time-scale") {
		int32_t timeScale;
		if (safe_strtoi32(value.string(), &timeScale)) {
			if(OMX_TRUE != mRecorder->setParamVideoTimeScale(mRecorder, timeScale))
				return BAD_VALUE;
			return NO_ERROR;
		}
	} else if (key == "time-lapse-enable") {
		int32_t timeLapseEnable;
		if (safe_strtoi32(value.string(), &timeLapseEnable)) {
			if(OMX_TRUE != mRecorder->setParamTimeLapseEnable(mRecorder, timeLapseEnable))
				return BAD_VALUE;
			return NO_ERROR;
		}
	} else if (key == "time-between-time-lapse-frame-capture") {
		int64_t timeBetweenTimeLapseFrameCaptureMs;
		if (safe_strtoi64(value.string(), &timeBetweenTimeLapseFrameCaptureMs)) {
			if(OMX_TRUE != mRecorder->setParamTimeBetweenTimeLapseFrameCapture(mRecorder, 
					1000LL * timeBetweenTimeLapseFrameCaptureMs))
				return BAD_VALUE;
			return NO_ERROR;
		}

	} else {
        LOG_ERROR("setParameter: failed to find key %s", key.string());
    }
    return BAD_VALUE;
}

status_t OMXRecorder::setParameters(const String8 &params) {
    LOG_LOG("setParameters: %s", params.string());
    const char *cparams = params.string();
    const char *key_start = cparams;
    for (;;) {
        const char *equal_pos = strchr(key_start, '=');
        if (equal_pos == NULL) {
            LOG_ERROR("Parameters %s miss a value", cparams);
            return BAD_VALUE;
        }
        String8 key(key_start, equal_pos - key_start);
        TrimString(&key);
        if (key.length() == 0) {
            LOG_ERROR("Parameters %s contains an empty key", cparams);
            return BAD_VALUE;
        }
        const char *value_start = equal_pos + 1;
        const char *semicolon_pos = strchr(value_start, ';');
        String8 value;
        if (semicolon_pos == NULL) {
            value.setTo(value_start);
        } else {
            value.setTo(value_start, semicolon_pos - value_start);
        }
        if (setParameter(key, value) != NO_ERROR) {
            return BAD_VALUE;
        }
        if (semicolon_pos == NULL) {
            break;  // Reaches the end
        }
        key_start = semicolon_pos + 1;
    }
    return NO_ERROR;
}

status_t OMXRecorder::setListener(const sp<IMediaRecorderClient> &listener) {
    LOG_DEBUG("OMXRecorder setListener\n");
    mListener = listener;

    return NO_ERROR;
}

status_t OMXRecorder::prepare()
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder prepare\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->prepare(mRecorder))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::start()
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder start\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->start(mRecorder))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::pause()
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder pause\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->pause(mRecorder))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::stop()
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder stop\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->stop(mRecorder))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::close()
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder close\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->close(mRecorder))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::reset()
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

    LOG_DEBUG("OMXRecorder reset\n");
	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->stop(mRecorder))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::getMaxAmplitude(
		int *max)
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

	if (mRecorder == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != mRecorder->getMaxAmplitude(mRecorder, (OMX_S32 *)max))
        return BAD_VALUE;

    return NO_ERROR;
}

status_t OMXRecorder::dump(int fd, const Vector<String16>& args) const
{
    OMX_Recorder* mRecorder = (OMX_Recorder*)recorder;

	if (mRecorder == NULL)
        return BAD_VALUE;

    return NO_ERROR;
}

#ifdef ICS
sp<ISurfaceTexture> OMXRecorder::querySurfaceMediaSource() const
{
	return NULL;
}
#endif
