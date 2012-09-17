/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file FslMuxer.h
 *  @brief Class definition of FslMuxer Component
 *  @ingroup FslMuxer
 */

#ifndef FslMuxer_h
#define FslMuxer_h

#include "Muxer.h"
#include "fsl_muxer.h"
#include "ShareLibarayMgr.h"

class FslMuxer : public Muxer {
    public:
        FslMuxer();
    private:
        typedef struct {
            FslMuxerApiVersionInfo GetApiVersion;
            FslMuxerVersionInfo GetMuxerVersion;
            FslCreateMuxer Create;
            FslDeleteMuxer Delete;
            FslMuxerSetMovieProperty SetMovieProperty;
            FslMuxerSetMovieMetadata SetMovieMetadata;
            FslMuxerWriteHeader WriteHeader;
            FslMuxerWriteTrailer WriteTrailer;
			FslMuxerGetTrailerSize GetTrailerSize;
			FslMuxerAddTrack AddTrack;
            FslMuxerSetTrackProperty SetTrackProperty;
            FslMuxerSetTrackMetadata SetTrackMetadata;
            FslMuxerAddSample AddSample;
        }FSL_MUXER_ITF;

        ShareLibarayMgr LibMgr;
        OMX_PTR hMuxerLib;
        FSL_MUXER_ITF hItf;
        FslMuxerHandle hMuxer;

        OMX_ERRORTYPE InitMuxerComponent();
        OMX_ERRORTYPE DeInitMuxerComponent();
        OMX_ERRORTYPE InitMuxer();
        OMX_ERRORTYPE DeInitMuxer();
		OMX_ERRORTYPE AddLocationInfo(OMX_S64 nLongitudex, OMX_S64 nLatitudex);
        OMX_ERRORTYPE AddMp3Track(OMX_U32 trackId, OMX_AUDIO_PARAM_MP3TYPE *pParameter);
        OMX_ERRORTYPE AddAacTrack(OMX_U32 track, OMX_AUDIO_PARAM_AACPROFILETYPE *pParameter);
        OMX_ERRORTYPE AddAmrTrack(OMX_U32 trackId, OMX_AUDIO_PARAM_AMRTYPE *pParameter);
        OMX_ERRORTYPE AddAvcTrack(OMX_U32 trackId, OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, OMX_VIDEO_PARAM_AVCTYPE *pParameter);
        OMX_ERRORTYPE AddH263Track(OMX_U32 trackId, OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, OMX_VIDEO_PARAM_H263TYPE *pParameter);
        OMX_ERRORTYPE AddMpeg4Track(OMX_U32 trackId, OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, OMX_VIDEO_PARAM_MPEG4TYPE *pParameter);
        OMX_ERRORTYPE AddTrackCodecData(OMX_U32 trackId, OMX_PTR pData, OMX_U32 nLen);
        OMX_ERRORTYPE AddTracksDone();
        OMX_ERRORTYPE AddOneSample(OMX_U32 trackId, TRACK_DATA *pData);
		OMX_U32 GetTrailerSize();
        OMX_ERRORTYPE AddSampleDone();

        OMX_ERRORTYPE LoadCoreMuxer();
        OMX_ERRORTYPE UnLoadCoreMuxer();
};

#endif
/* File EOF */
