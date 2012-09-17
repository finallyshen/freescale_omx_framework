/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file GMRecorder.h
 *  @brief Class definition of recorder graph manager
 *  @ingroup GMRecorder
 */

#ifndef GMRecorder_h
#define GMRecorder_h

#include "GMComponent.h"
#include "OMX_ContentPipe.h"
#include "Queue.h"
#include "List.h"

#include "GMRecorder.h"
#include "OMX_Recorder.h"

#define MAX_COMPONENT_NUM 8

typedef enum {
    RECORDER_STATE_NULL,
    RECORDER_STATE_LOADING,
    RECORDER_STATE_LOADED,
    RECORDER_STATE_PAUSE,
    RECORDER_STATE_PLAYING,
    RECORDER_STATE_EOS,
    RECORDER_STATE_STOP
}RECORDER_STATE;

typedef struct {
    GMComponent *pComponent;
    OMX_EVENTTYPE eEvent;
    OMX_U32 nData1;
    OMX_U32 nData2;
    OMX_PTR pEventData;
}GM_SYSEVENT;

class GMRecorder {
	public:
		OMX_ERRORTYPE init();
		OMX_ERRORTYPE setAudioSource(OMX_S32 as);
		OMX_ERRORTYPE setVideoSource(video_source_gm vs);
		OMX_ERRORTYPE setOutputFormat(output_format_gm of);
		OMX_ERRORTYPE setAudioEncoder(audio_encoder_gm ae);
		OMX_ERRORTYPE setVideoEncoder(video_encoder_gm ve);
		OMX_ERRORTYPE setVideoSize(OMX_S32 width, OMX_S32 height);
		OMX_ERRORTYPE setVideoFrameRate(OMX_S32 frames_per_second);
		OMX_ERRORTYPE setCamera(OMX_PTR camera, OMX_PTR cameraProxy);
		OMX_ERRORTYPE setPreviewSurface(OMX_PTR surface);
		OMX_ERRORTYPE setOutputFile(const OMX_STRING path);
		OMX_ERRORTYPE setOutputFileFD(OMX_S32 fd, OMX_S64 offset, OMX_S64 length);
		OMX_ERRORTYPE setParamMaxFileDurationUs(OMX_TICKS timeUs);
		OMX_ERRORTYPE setParamMaxFileSizeBytes(OMX_S64 bytes);
		OMX_ERRORTYPE setParamInterleaveDuration(OMX_S32 durationUs);
		OMX_ERRORTYPE setParamMovieTimeScale(OMX_S32 timeScale);
		OMX_ERRORTYPE setParam64BitFileOffset(OMX_BOOL use64Bit);
		OMX_ERRORTYPE setParamGeotagLongitude(OMX_S64 longitudex10000);
		OMX_ERRORTYPE setParamGeotagLatitude(OMX_S64 latitudex10000);
		OMX_ERRORTYPE setParamTrackTimeStatus(OMX_S64 timeDurationUs);
		OMX_ERRORTYPE setParamAudioSamplingRate(OMX_S32 sampleRate);
		OMX_ERRORTYPE setParamAudioNumberOfChannels(OMX_S32 channels);
		OMX_ERRORTYPE setParamAudioEncodingBitRate(OMX_S32 bitRate);
		OMX_ERRORTYPE setParamAudioTimeScale(OMX_S32 timeScale);
		OMX_ERRORTYPE setParamVideoEncodingBitRate(OMX_S32 bitRate);
		OMX_ERRORTYPE setParamVideoRotation(OMX_S32 degrees);
		OMX_ERRORTYPE setParamVideoIFramesInterval(OMX_S32 seconds);
		OMX_ERRORTYPE setParamVideoEncoderProfile(OMX_S32 profile);
		OMX_ERRORTYPE setParamVideoEncoderLevel(OMX_S32 level);
		OMX_ERRORTYPE setParamVideoCameraId(OMX_S32 cameraId);
		OMX_ERRORTYPE setParamVideoTimeScale(OMX_S32 timeScale);
		OMX_ERRORTYPE setParamTimeLapseEnable(OMX_S32 timeLapseEnable);
		OMX_ERRORTYPE setParamTimeBetweenTimeLapseFrameCapture(OMX_S64 timeUs);

		OMX_ERRORTYPE registerEventHandler(OMX_PTR context, RECORDER_EVENTHANDLER handler);
		OMX_ERRORTYPE prepare();
		OMX_ERRORTYPE start();
		OMX_ERRORTYPE pause();
		OMX_ERRORTYPE stop();
		OMX_ERRORTYPE close();
		OMX_ERRORTYPE reset();
		OMX_ERRORTYPE getMaxAmplitude(OMX_S32 *max);
		OMX_ERRORTYPE getMediaTime(OMX_TICKS *pMediaTime);
		OMX_ERRORTYPE deleteIt();

        OMX_ERRORTYPE SysEventHandler(GM_SYSEVENT *pSysEvent);
        OMX_ERRORTYPE AddSysEvent(GM_SYSEVENT *pSysEvent);
        OMX_ERRORTYPE GetSysEvent(GM_SYSEVENT *pSysEvent);

        OMX_PTR PmContext;
    private:
        OMX_PARAM_CONTENTURITYPE *Content;
        List<GMComponent> *Pipeline;
        Queue *SysEventQueue;
        fsl_osal_ptr SysEventThread;
        RECORDER_EVENTHANDLER pAppCallback;
        OMX_PTR pAppData;
        fsl_osal_mutex Lock;
        RECORDER_STATE State;
        OMX_BOOL bNeedAudio;
        OMX_BOOL bNeedVideo;
        OMX_BOOL bSetAudioSource;
        OMX_BOOL bSetVideoSource;
        GMComponent *Muxer;
        GMComponent *AudioEncoder;
        GMComponent *VideoEncoder;
        GMComponent *AudioSource;
        GMComponent *VideoSource;
        GMComponent *Clock;
        OMX_BOOL bHasAudio;
        OMX_BOOL bHasVideo;
        OMX_AUDIO_CODINGTYPE AudioFmt;
        OMX_VIDEO_CODINGTYPE VideoFmt;
        OMX_BOOL bAudioEos;
        OMX_BOOL bVideoEos;
        OMX_BOOL bVideoFFEos;
        OMX_BOOL bMediaEos;
		OMX_BOOL bMaxDuration;
        OMX_BOOL bAudioMarkDone;
        OMX_BOOL bVideoMarkDone;
        OMX_TICKS MediaDuration;
        OMX_BOOL bError;

		CP_PIPETYPE *hPipe;

		int mSharedFd;
		OMX_S32 gm_as;
		video_source_gm gm_vs;
		audio_encoder_gm gm_ae;
		video_encoder_gm gm_ve;
		output_format_gm gm_of;
		OMX_STRING gm_path;
		OMX_S32 gm_width;
		OMX_S32 gm_height;
		OMX_S32 gm_frames_per_second;
		OMX_PTR gm_camera;
		OMX_PTR gm_camera_proxy;
		OMX_PTR gm_surface;
		OMX_TICKS gm_timeUs;
		OMX_S64 gm_bytes;
		OMX_TICKS gm_durationUs;
		OMX_S32 gm_movie_timeScale;
		OMX_BOOL gm_use64Bit;
		OMX_S64 gm_Latitudex10000;
		OMX_S64 gm_Longitudex10000;
		OMX_TICKS gm_timeDurationUs;
		OMX_S32 gm_sampleRate;
		OMX_S32 gm_channels;
		OMX_S32 gm_audio_bitRate;
		OMX_S32 gm_audio_timeScale;
		OMX_S32 gm_video_bitRate;
		OMX_S32 gm_degrees;
		OMX_S32 gm_seconds;
		OMX_S32 gm_profile;
		OMX_S32 gm_level;
		OMX_S32 gm_cameraId;
		OMX_S32 gm_video_timeScale;
		OMX_BOOL gm_capture_time_lapse;
		OMX_S64 gm_timeBetweenTimeLapseFrameCaptureUs;


		OMX_ERRORTYPE MediaTypeInspectBySubfix(OMX_STRING path);
        OMX_ERRORTYPE LoadComponent(OMX_STRING role, GMComponent **ppComponent);
        OMX_ERRORTYPE LoadMuxerComponent();
        OMX_ERRORTYPE LoadRecorderComponent();
		OMX_ERRORTYPE DecidePipeLine();
		OMX_ERRORTYPE LoadASComponent();
		OMX_ERRORTYPE LoadAEComponent();
		OMX_ERRORTYPE LoadVSComponent();
		OMX_ERRORTYPE LoadVEComponent();
		OMX_ERRORTYPE SetASParameter();
		OMX_ERRORTYPE SetAEParameter();
		OMX_ERRORTYPE SetVSParameter();
		OMX_ERRORTYPE SetVEParameter();
		OMX_ERRORTYPE SetMuxerParameter();
		OMX_ERRORTYPE GetContentPipe(CP_PIPETYPE **Pipe);
        OMX_ERRORTYPE LoadClock();
        OMX_ERRORTYPE RemoveClock();
        OMX_ERRORTYPE WaitStop();
        OMX_ERRORTYPE AttachClock(OMX_U32 nClockPortIndex, GMComponent *pComponent, OMX_U32 nPortIndex);
        OMX_ERRORTYPE DeAttachClock(OMX_U32 nClockPortIndex, GMComponent *pComponent, OMX_U32 nPortIndex);
        OMX_ERRORTYPE SetupPipeline();
		OMX_ERRORTYPE StartPipeline();
		OMX_ERRORTYPE ConnectComponent(GMComponent *pOutComp, OMX_U32 nOutPortIndex, GMComponent *pInComp, OMX_U32 nInPortIndex);
		OMX_ERRORTYPE DisConnectComponent(GMComponent *pOutComp, OMX_U32 nOutPortIndex, GMComponent *pInComp, OMX_U32 nInPortIndex);

};

#endif
