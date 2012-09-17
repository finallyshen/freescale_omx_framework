/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Muxer.h
 *  @brief Class definition of Muxer base
 *  @ingroup Muxer
 */

#ifndef Muxer_h
#define Muxer_h

#include "ComponentBase.h"
#include "OMX_ContentPipe.h"
#include "fsl_muxer.h"

#define MAX_PORTS 32

typedef struct {
    OMX_PORTDOMAINTYPE eDomain;
    OMX_U32 nBufferCount;
    OMX_U32 nBufferSize;
} PORT_INFO;

typedef struct {
    OMX_TICKS nTimeStamp;
    OMX_TICKS nDuration;
    OMX_PTR pBuffer;
    OMX_U32 nAllocLen;
    OMX_U32 nFilledLen;
    OMX_U32 nFlags;
} TRACK_DATA;

class Muxer : public ComponentBase {
    public:
        Muxer();
        OMX_PTR OpenContent(CP_ACCESSTYPE eAccess);
        OMX_ERRORTYPE CloseContent(OMX_PTR hContent);
        OMX_U32 WriteContent(OMX_PTR hContent, OMX_PTR pBuffer, OMX_S32 nSize);
        OMX_ERRORTYPE SeekContent(OMX_PTR hContent, OMX_S64 nOffset, CP_ORIGINTYPE eOrigin);
        OMX_S64 TellContent(OMX_PTR hContent);
		OMX_S64 nTotalWrite;
    protected:
        PORT_INFO port_info[MAX_PORTS];
        FslFileStream streamOps;
        MuxerMemoryOps memOps;
    private:
        typedef struct {
            OMX_BOOL bTrackSetted;
            OMX_BOOL bTrackAdded;
            OMX_TICKS nHeadTs;
            List<TRACK_DATA> FreeBuffers;
            List<TRACK_DATA> ReadyBuffers;
            TRACK_DATA TrackBuffer;
            TRACK_DATA CodecData;
        } TRACK_INFO;

        OMX_BUFFERHEADERTYPE *pBufferHdr[MAX_PORTS];
        TRACK_INFO Tracks[MAX_PORTS];
        union {
            OMX_AUDIO_PARAM_PCMMODETYPE PcmPara;
            OMX_AUDIO_PARAM_MP3TYPE Mp3Para;
            OMX_AUDIO_PARAM_AMRTYPE AmrPara;
            OMX_AUDIO_PARAM_AACPROFILETYPE AacProfile;
            OMX_AUDIO_PARAM_WMATYPE WmaPara;
            OMX_VIDEO_PARAM_H263TYPE H263Para;
            OMX_VIDEO_PARAM_MPEG4TYPE Mpeg4Para;
            OMX_VIDEO_PARAM_AVCTYPE AvcPara;
        } TrackParameter[MAX_PORTS];
        OMX_TICKS nMinTrackTs;
        OMX_U32 nEosMask;
		OMX_U32 nAddSampleMask;
        OMX_U32 nGetCodecDataMask;
        OMX_TICKS nMaxInterleave;
        OMX_BOOL bCodecDataAdded;
		OMX_U32 nHeadSize;
		OMX_S64 nMaxFileSize;
		OMX_S64 nLongitudex;
		OMX_S64 nLatitudex;
		OMX_BOOL bAddSampleDone;

        CP_PIPETYPE *hPipe;
        OMX_PTR pURL;
		OMX_PTR hContent;
		OMX_U32 nURLLen;

        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE InstanceInit();
        OMX_ERRORTYPE InstanceDeInit();
        OMX_ERRORTYPE ProcessDataBuffer();
        OMX_ERRORTYPE ComponentReturnBuffer(OMX_U32 nPortIndex);
        OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex);
        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);

        OMX_ERRORTYPE InitTracks();
        OMX_ERRORTYPE InitMetaData();
        OMX_ERRORTYPE InitDone();
        OMX_ERRORTYPE InitAudioTrack(OMX_U32 nPortIndex);
        OMX_ERRORTYPE InitVideoTrack(OMX_U32 nPortIndex);
        OMX_ERRORTYPE InitTrackMetaData(OMX_U32 nPortIndex);
        OMX_ERRORTYPE ProcessTrackCodecData(OMX_U32 track);
        OMX_ERRORTYPE AddCodecData();
        OMX_ERRORTYPE TakeOneBufferFromPort();
        OMX_ERRORTYPE AddToReadyBuffers(OMX_U32 track);
        TRACK_DATA *GetEmptyTrackData(OMX_U32 track, OMX_U32 nSize);
        TRACK_DATA *GetOneTrackData(OMX_U32 track);
		OMX_ERRORTYPE ReturnTrackData(OMX_U32 track, TRACK_DATA *pData);
		OMX_ERRORTYPE ReturnAllBuffer();
		OMX_ERRORTYPE HandleSDFull();
		OMX_ERRORTYPE CheckEOS(OMX_S32 i);

        virtual OMX_ERRORTYPE InitMuxerComponent();
        virtual OMX_ERRORTYPE DeInitMuxerComponent();
        virtual OMX_ERRORTYPE InitMuxer() = 0;
        virtual OMX_ERRORTYPE DeInitMuxer() = 0;
		virtual OMX_ERRORTYPE AddLocationInfo(OMX_S64 nLongitudex, OMX_S64 nLatitudex);
        virtual OMX_ERRORTYPE AddPcmTrack(OMX_U32 track, OMX_AUDIO_PARAM_PCMMODETYPE *pParameter);
        virtual OMX_ERRORTYPE AddMp3Track(OMX_U32 track, OMX_AUDIO_PARAM_MP3TYPE *pParameter);
        virtual OMX_ERRORTYPE AddAmrTrack(OMX_U32 track, OMX_AUDIO_PARAM_AMRTYPE *pParameter);
        virtual OMX_ERRORTYPE AddAacTrack(OMX_U32 track, OMX_AUDIO_PARAM_AACPROFILETYPE *pParameter);
        virtual OMX_ERRORTYPE AddWmaTrack(OMX_U32 track, OMX_AUDIO_PARAM_WMATYPE *pParameter);
        virtual OMX_ERRORTYPE AddH263Track(OMX_U32 track, OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, OMX_VIDEO_PARAM_H263TYPE *pParameter);
        virtual OMX_ERRORTYPE AddMpeg4Track(OMX_U32 track, OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, OMX_VIDEO_PARAM_MPEG4TYPE *pParameter);
        virtual OMX_ERRORTYPE AddAvcTrack(OMX_U32 track, OMX_VIDEO_PORTDEFINITIONTYPE *pFormat, OMX_VIDEO_PARAM_AVCTYPE *pParameter);
        virtual OMX_ERRORTYPE AddTrackCodecData(OMX_U32 track, OMX_PTR pData, OMX_U32 nLen) = 0;
        virtual OMX_ERRORTYPE AddTracksDone();
        virtual OMX_ERRORTYPE AddOneSample(OMX_U32 track, TRACK_DATA *pData) = 0;
		virtual OMX_U32 GetTrailerSize();
        virtual OMX_ERRORTYPE AddSampleDone();
};

#endif
/* File EOF */
