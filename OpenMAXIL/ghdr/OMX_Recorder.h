/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#ifndef _omx_recorder_h_
#define _omx_recorder_h_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "fsl_osal.h"

enum video_source_gm {
    VIDEO_SOURCE_DEFAULT = 0,
    VIDEO_SOURCE_CAMERA = 1,

    VIDEO_SOURCE_LIST_END  // must be last - used to validate audio source type
};

//Please update media/java/android/media/MediaRecorder.java if the following is updated.
enum output_format_gm {
    OUTPUT_FORMAT_DEFAULT = 0,
    OUTPUT_FORMAT_THREE_GPP = 1,
    OUTPUT_FORMAT_MPEG_4 = 2,


    OUTPUT_FORMAT_AUDIO_ONLY_START = 3, // Used in validating the output format.  Should be the
                                        //  at the start of the audio only output formats.

    /* These are audio only file formats */
    OUTPUT_FORMAT_RAW_AMR = 3, //to be backward compatible
    OUTPUT_FORMAT_AMR_NB = 3,
    OUTPUT_FORMAT_AMR_WB = 4,
    OUTPUT_FORMAT_AAC_ADIF = 5,
    OUTPUT_FORMAT_AAC_ADTS = 6,

    /* Stream over a socket, limited to a single stream */
    OUTPUT_FORMAT_RTP_AVP = 7,

    /* H.264/AAC data encapsulated in MPEG2/TS */
    OUTPUT_FORMAT_MPEG2TS = 8,

    OUTPUT_FORMAT_MP3 = 9,
    OUTPUT_FORMAT_PCM16 = 10,
    OUTPUT_FORMAT_LIST_END // must be last - used to validate format type
};

enum audio_encoder_gm {
    AUDIO_ENCODER_DEFAULT = 0,
    AUDIO_ENCODER_AMR_NB = 1,
    AUDIO_ENCODER_AMR_WB = 2,
    AUDIO_ENCODER_AAC = 3,
    AUDIO_ENCODER_AAC_PLUS = 4,
    AUDIO_ENCODER_EAAC_PLUS = 5,
    AUDIO_ENCODER_MP3 = 6,
    AUDIO_ENCODER_PCM16 = 7,
    AUDIO_ENCODER_LIST_END // must be the last - used to validate the audio encoder type
};

enum video_encoder_gm {
    VIDEO_ENCODER_DEFAULT = 0,
    VIDEO_ENCODER_H263 = 1,
    VIDEO_ENCODER_H264 = 2,
    VIDEO_ENCODER_MPEG_4_SP = 3,

    VIDEO_ENCODER_LIST_END // must be the last - used to validate the video encoder type
};

typedef enum
{
	RECORDER_EVENT_NONE,
	RECORDER_EVENT_ERROR_UNKNOWN,
    RECORDER_EVENT_UNKNOWN,
    RECORDER_EVENT_MAX_DURATION_REACHED,
    RECORDER_EVENT_MAX_FILESIZE_REACHED,
    RECORDER_EVENT_COMPLETION_STATUS,
    RECORDER_EVENT_PROGRESS_FRAME_STATUS,
    RECORDER_EVENT_PROGRESS_TIME_STATUS,
    RECORDER_EVENT_EOS,
    RECORDER_EVENT_END
}RECORDER_EVENT;

typedef int (*RECORDER_EVENTHANDLER)(void* context, RECORDER_EVENT eventID, void* Eventpayload);

struct OMX_Recorder
{
	OMX_BOOL (*init)(OMX_Recorder *h);
	OMX_BOOL (*setAudioSource)(OMX_Recorder *h, OMX_S32 as);
	OMX_BOOL (*setVideoSource)(OMX_Recorder *h, video_source_gm vs);
	OMX_BOOL (*setOutputFormat)(OMX_Recorder *h, output_format_gm of);
	OMX_BOOL (*setAudioEncoder)(OMX_Recorder *h, audio_encoder_gm ae);
	OMX_BOOL (*setVideoEncoder)(OMX_Recorder *h, video_encoder_gm ve);
	OMX_BOOL (*setVideoSize)(OMX_Recorder *h, OMX_S32 width, OMX_S32 height);
	OMX_BOOL (*setVideoFrameRate)(OMX_Recorder *h, OMX_S32 frames_per_second);
	OMX_BOOL (*setCamera)(OMX_Recorder *h, OMX_PTR camera, OMX_PTR cameraProxy);
	OMX_BOOL (*setPreviewSurface)(OMX_Recorder *h, OMX_PTR surface);
	OMX_BOOL (*setOutputFile)(OMX_Recorder *h, const OMX_STRING path);
	OMX_BOOL (*setOutputFileFD)(OMX_Recorder *h, OMX_S32 fd, OMX_S64 offset, OMX_S64 length);
	OMX_BOOL (*setParamMaxFileDurationUs)(OMX_Recorder *h, OMX_TICKS timeUs);
	OMX_BOOL (*setParamMaxFileSizeBytes)(OMX_Recorder *h, OMX_S64 bytes);
	OMX_BOOL (*setParamInterleaveDuration)(OMX_Recorder *h, OMX_S32 durationUs);
	OMX_BOOL (*setParamMovieTimeScale)(OMX_Recorder *h, OMX_S32 timeScale);
	OMX_BOOL (*setParam64BitFileOffset)(OMX_Recorder *h, OMX_BOOL use64Bit);
	OMX_BOOL (*setParamGeotagLongitude)(OMX_Recorder *h, OMX_S64 longitudex10000);
	OMX_BOOL (*setParamGeotagLatitude)(OMX_Recorder *h, OMX_S64 latitudex10000);
	OMX_BOOL (*setParamTrackTimeStatus)(OMX_Recorder *h, OMX_S64 timeDurationUs);
	OMX_BOOL (*setParamAudioSamplingRate)(OMX_Recorder *h, OMX_S32 sampleRate);
	OMX_BOOL (*setParamAudioNumberOfChannels)(OMX_Recorder *h, OMX_S32 channels);
	OMX_BOOL (*setParamAudioEncodingBitRate)(OMX_Recorder *h, OMX_S32 bitRate);
	OMX_BOOL (*setParamAudioTimeScale)(OMX_Recorder *h, OMX_S32 timeScale);
	OMX_BOOL (*setParamVideoEncodingBitRate)(OMX_Recorder *h, OMX_S32 bitRate);
	OMX_BOOL (*setParamVideoRotation)(OMX_Recorder *h, OMX_S32 degrees);
	OMX_BOOL (*setParamVideoIFramesInterval)(OMX_Recorder *h, OMX_S32 seconds);
	OMX_BOOL (*setParamVideoEncoderProfile)(OMX_Recorder *h, OMX_S32 profile);
	OMX_BOOL (*setParamVideoEncoderLevel)(OMX_Recorder *h, OMX_S32 level);
	OMX_BOOL (*setParamVideoCameraId)(OMX_Recorder *h, OMX_S32 cameraId);
	OMX_BOOL (*setParamVideoTimeScale)(OMX_Recorder *h, OMX_S32 timeScale);
	OMX_BOOL (*setParamTimeLapseEnable)(OMX_Recorder *h, OMX_S32 timeLapseEnable);
	OMX_BOOL (*setParamTimeBetweenTimeLapseFrameCapture)(OMX_Recorder *h, OMX_S64 timeUs);
	OMX_BOOL (*registerEventHandler)(OMX_Recorder *h, OMX_PTR context, RECORDER_EVENTHANDLER handler);
	OMX_BOOL (*prepare)(OMX_Recorder *h);
	OMX_BOOL (*start)(OMX_Recorder *h);
	OMX_BOOL (*pause)(OMX_Recorder *h);
	OMX_BOOL (*stop)(OMX_Recorder *h);
	OMX_BOOL (*close)(OMX_Recorder *h);
	OMX_BOOL (*reset)(OMX_Recorder *h);
	OMX_BOOL (*getMaxAmplitude)(OMX_Recorder *h, OMX_S32 *max);
	OMX_BOOL (*getMediaTime)(OMX_Recorder *h, OMX_TICKS *pMediaTime);
	OMX_BOOL (*deleteIt)(OMX_Recorder *h);
	
	void* pData;
};

OMX_Recorder* OMX_RecorderCreate();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
