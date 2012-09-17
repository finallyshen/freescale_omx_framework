/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Parser.h
 *  @brief Class definition of Parser Component
 *  @ingroup Parser
 */

#ifndef PARSER_H
#define PARSER_H

#include "ComponentBase.h"
#include "OMX_ContentPipe.h"
#include "fsl_types.h"
#include "fsl_parser.h"
#include "file_stream.h"
#define PORT_NUM 3
#define AUDIO_OUTPUT_PORT 0
#define VIDEO_OUTPUT_PORT 1
#define CLOCK_PORT 2
#define DEFAULT_PARSER_AUDIO_OUTPUT_BUFSIZE (16*1024)
#define DEFAULT_PARSER_OUTPUT_BUFSIZE (512*1024)
#define MAX_AVAIL_TRACK 32
#define MAX_MATADATA_NUM 32
#define INDEX_FILE_NAME_MAX_LEN 256

class Parser : public ComponentBase {
    public:
        Parser();
        CP_PIPETYPE *hPipe;
        OMX_S8 *pMediaName;
        OMX_ERRORTYPE SetMetadata(OMX_STRING key, OMX_STRING value, OMX_U32 valueSize);
        OMX_STRING GetMimeFromComponentRole(OMX_U8 *componentRole); 
        OMX_ERRORTYPE CheckContentAvailableBytes(OMX_PTR h, OMX_U32 nb);
        OMX_BOOL IsStreamingSource() {return isStreamingSource;}
        OMX_U64 nContentLen;
    private:
        OMX_BUFFERHEADERTYPE *pAudioOutBufHdr;
        OMX_BUFFERHEADERTYPE *pVideoOutBufHdr;
        OMX_BUFFERHEADERTYPE *pClockBufHdr;
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE ProcessDataBuffer();
        OMX_ERRORTYPE GetAndSendOneVideoBuf();
        OMX_ERRORTYPE GetAndSendOneAudioBuf();
        OMX_ERRORTYPE GetAndSendNextSyncVideoBuf();
        OMX_ERRORTYPE GetAndSendPrevSyncVideoBuf();
        OMX_ERRORTYPE ProcessClkBuffer();
        OMX_ERRORTYPE Seek(OMX_U32 nPortIndex);
        OMX_U32 CalcPreCacheSize();

        OMX_S32 nClockScale;
        OMX_BOOL bAudioNewSegment;
        OMX_BOOL bVideoNewSegment;
        OMX_BOOL bAudioEOS;
        OMX_BOOL bVideoEOS;
        OMX_BOOL bSendAudioFrameFirst;
        OMX_TICKS sCurrAudioTime;
        OMX_TICKS sCurrVideoTime;

        OMX_BOOL bStreamCorrupted;
        OMX_BOOL bSkip2Iframe;

        OMX_U32 nPreCacheSize;
        CPint64 nCheckCacheOff;
        OMX_BOOL bAbortBuffering;

    protected:		
        CPhandle hMedia;
        OMX_BOOL isLiveSource;
        OMX_BOOL isStreamingSource;
		OMX_BOOL bGetMetadata;
        OMX_TIME_CONFIG_SEEKMODETYPE sSeekMode;
        OMX_PARAM_COMPONENTROLETYPE sCompRole;
        OMX_TICKS sAudioSeekPos;
        OMX_TICKS sVideoSeekPos;
        OMX_TICKS sActualVideoSeekPos;
        OMX_TICKS sActualAudioSeekPos;
        OMX_BOOL bSeekable;
        OMX_BOOL bHasVideoTrack;
        OMX_BOOL bHasAudioTrack;
        OMX_S32 nMediaNameLen;

        OMX_CONFIG_METADATAITEMCOUNTTYPE sMatadataItemCount;
        OMX_CONFIG_METADATAITEMTYPE *psMatadataItem[MAX_MATADATA_NUM];

        OMX_AUDIO_PARAM_PORTFORMATTYPE sAudioPortFormat;
        OMX_VIDEO_PARAM_PORTFORMATTYPE sVideoPortFormat;
        OMX_BOOL bVideoBOS;
        OMX_U32 nNumAvailAudioStream;
        OMX_U32 nNumAvailVideoStream;
        OMX_U32 nActiveAudioNum;
        OMX_U32 nActiveAudioNumToClient;
        OMX_U32 nActiveVideoNum;
        OMX_BOOL bAudioActived;
        OMX_BOOL bVideoActived;

        OMX_TICKS usDuration;
        OMX_TICKS nAudioDuration;
        OMX_TICKS nVideoDuration;

        FslFileStream fileOps;
        ParserMemoryOps memOps;

        OMX_U32 nAudioTrackNum[MAX_AVAIL_TRACK];
        OMX_U32 nVideoTrackNum[MAX_AVAIL_TRACK];
        uint32 numTracks;
        char index_file_name[INDEX_FILE_NAME_MAX_LEN];
        OMX_BOOL bAudioCodecDataSent;
        OMX_BOOL bVideoCodecDataSent;
        void MakeIndexDumpFileName(char *media_file_name);

        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual OMX_ERRORTYPE GetNextSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer) = 0;
        virtual OMX_ERRORTYPE GetNextSyncSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer);
        virtual OMX_ERRORTYPE GetPrevSyncSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer);
        virtual OMX_ERRORTYPE DoSeek(OMX_U32 nPortIndex, OMX_U32 nSeekFlag) = 0;
        virtual OMX_BOOL SendCodecInitData(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer);
        virtual void SetAudioCodecType(OMX_PARAM_PORTDEFINITIONTYPE *pPortDef, uint32 audio_num);
        virtual void SetVideoCodecType(OMX_PARAM_PORTDEFINITIONTYPE *pPortDef, uint32 video_num);
        virtual void ActiveTrack(uint32 track_number);
        virtual void DisableTrack(uint32 track_number);
        virtual void AbortReadSource(OMX_BOOL bAbort);
};

#endif
/* File EOF */
