/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file StreamingParser.h
 *  @brief Class definition of StreamingParser Component
 *  @ingroup FslParser
 */

#ifndef RTSP_PARSER_H
#define RTSP_PARSER_H

#include "Parser.h"
#include "avformat.h"

#define MAX_STREMA_NUM 16

class StreamingParser : public Parser {
    public:
        StreamingParser();
        OMX_ERRORTYPE ReadPacketFromStream();

    private:

        typedef enum {
            NONE,
            RTSP,
            HTTPLIVE,
        }STREAMFMT;

        class PktCache {
            public:
                PktCache() {Lock = NULL; FirstPktTs = LastPktTs = 0; bInit = OMX_FALSE;};
                OMX_ERRORTYPE InitCache();
                OMX_ERRORTYPE DeInitCache();
                OMX_TICKS GetCacheDepth();
                OMX_ERRORTYPE AddOnePkt(AVPacket *pkt);
                AVPacket *GetOnePkt();
                OMX_ERRORTYPE ResetCache();
            private:
                List<AVPacket> PktList;
                OMX_TICKS FirstPktTs;
                OMX_TICKS LastPktTs;
                OMX_PTR Lock;
                OMX_BOOL bInit;
        };

        AVFormatContext *ic;
        AVFormatParameters params;
        OMX_BOOL bSpecificDataSent[PORT_NUM];
        OMX_BOOL bNewSegment[PORT_NUM];
        OMX_PTR pThread;
        OMX_BOOL bStopThread;
        PktCache pktCache[PORT_NUM];
        OMX_PTR Lock;
        OMX_BOOL bStreamEos;
        OMX_BOOL bPaused;
        STREAMFMT eStreamFmt;
        List<AVPacket> AssemblePktList;

        virtual OMX_ERRORTYPE InstanceInit();
        virtual OMX_ERRORTYPE InstanceDeInit();
        virtual OMX_ERRORTYPE GetNextSample(Port *pPort,OMX_BUFFERHEADERTYPE *pBufferHdr);
        virtual OMX_ERRORTYPE DoSeek(OMX_U32 nPortIndex, OMX_U32 nSeekFlag);
        virtual void SetAudioCodecType(OMX_PARAM_PORTDEFINITIONTYPE *pPortDef, uint32 audio_num);
        virtual void SetVideoCodecType(OMX_PARAM_PORTDEFINITIONTYPE *pPortDef, uint32 video_num);
        virtual OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual void AbortReadSource(OMX_BOOL bAbort);
        virtual OMX_ERRORTYPE DoExec2Pause();
        virtual OMX_ERRORTYPE DoPause2Exec();

        OMX_U32 GetPortIndexByStreamId(OMX_S32 stream_id);
        OMX_S32 GetStreamIdByPortIndex(OMX_U32 nPortIndex);
        OMX_ERRORTYPE InitStreaming();
        OMX_ERRORTYPE DeInitStreaming();
        OMX_ERRORTYPE SetupPortMediaFormat();
        OMX_ERRORTYPE GetSpecificData(OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE *pBufferHdr);
        OMX_BOOL IsStreamEnabled(OMX_S32 stream_id);
        OMX_BOOL PacketCacheFree();
        OMX_BOOL PacketCacheEmpty();
        OMX_ERRORTYPE DumpDataToOMXBuffer(AVPacket *pkt, OMX_BUFFERHEADERTYPE *pBufferHdr);
        OMX_TICKS StreamTs2OMX(int64_t ts, OMX_S32 stream_id);
        int64_t OMXTs2Stream(OMX_TICKS ts, OMX_S32 stream_id);
        OMX_ERRORTYPE HandleStreamingSkip();
        AVPacket *AssemblePacket(AVPacket *pkt);
        AVPacket *AssembledPacket();
        OMX_ERRORTYPE FreeAssemblingPackets();
};

#endif
/* File EOF */
