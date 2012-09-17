/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file FslParser.h
 *  @brief Class definition of FslParser Component
 *  @ingroup FslParser
 */

#ifndef RM_PARSER_H
#define RM_PARSER_H

#include "Parser.h"
#include "fsl_media_types.h"
#include "fsl_parser.h"

#include "ShareLibarayMgr.h"

#define MAX_TRACK_COUNT 32

#ifndef __WINCE  /* LINUX */
    typedef void * HANDLE;
#else /* WINCE */
    void asciiToUnicodeString(const uint8 * src, WCHAR * des);
#endif


typedef struct MediaBuf 
{
    OMX_BUFFERHEADERTYPE buf;
    struct MediaBuf *prev;
    struct MediaBuf *next;
}MediaBuf;


typedef struct
{
    /* creation & deletion */
    FslParserVersionInfo                getVersionInfo;
    FslCreateParser                     createParser;
    FslDeleteParser                     deleteParser;

    /* index export/import */
    FslParserInitializeIndex            initializeIndex;
    FslParserImportIndex                importIndex;
    FslParserExportIndex                exportIndex;

    /* movie properties */
    FslParserIsSeekable                 isSeekable;
    FslParserGetMovieDuration           getMovieDuration;
    FslParserGetUserData                getUserData;
    FslParserGetMetaData                getMetaData;
    
    FslParserGetNumTracks               getNumTracks;
    
    FslParserGetNumPrograms             getNumPrograms;
    FslParserGetProgramTracks           getProgramTracks;

    /* generic track properties */
    FslParserGetTrackType               getTrackType;
    FslParserGetTrackDuration           getTrackDuration;    
    FslParserGetLanguage                getLanguage;   
    FslParserGetBitRate                 getBitRate;
    FslParserGetDecSpecificInfo         getDecoderSpecificInfo;    
   
    /* video properties */
    FslParserGetVideoFrameWidth         getVideoFrameWidth;
    FslParserGetVideoFrameHeight        getVideoFrameHeight;
    FslParserGetVideoFrameRate          getVideoFrameRate;

    /* audio properties */
    FslParserGetAudioNumChannels        getAudioNumChannels;
    FslParserGetAudioSampleRate         getAudioSampleRate;
    FslParserGetAudioBitsPerSample      getAudioBitsPerSample;
    FslParserGetAudioBlockAlign         getAudioBlockAlign;
    FslParserGetAudioChannelMask        getAudioChannelMask;
    FslParserGetAudioBitsPerFrame       getAudioBitsPerFrame;

    /* text/subtitle properties */
    FslParserGetTextTrackWidth          getTextTrackWidth;
    FslParserGetTextTrackHeight         getTextTrackHeight;

    /* sample reading, seek & trick mode */
    FslParserGetReadMode                getReadMode;
    FslParserSetReadMode                setReadMode;

    FslParserEnableTrack                enableTrack;
    
    FslParserGetNextSample              getNextSample;
    FslParserGetNextSyncSample          getNextSyncSample;
    
    FslParserGetFileNextSample              getFileNextSample;
    FslParserGetFileNextSyncSample          getFileNextSyncSample;
    
    FslParserSeek                       seek;    
   
}FslParserInterface;

typedef struct 
{
    MediaType mediaType;
    uint32 decoderType;
    uint32 decoderSubtype;
    bool isCbr;
    uint64 usSampleDuration; /* sample duration for VBR stream. 0 for CBR stream*/
    uint32 bitRate;  /* bitrate for CBR stream, 0 for VBR stream */
    uint64 usDuration;
    uint32 trackNum; /* 0-based track number */
    uint32 width;
    uint32 height;
    uint32 numChannels;
    uint32 sampleRate;
    uint32 bitsPerSample;
    uint32 textTrackWidth;
    uint32 textTrackHeight;
    uint32 maxSampleSize;
    uint8 *decoderSpecificInfo;
    uint32 decoderSpecificInfoSize;
    uint32 decoderSpecificInfoOffset;
    bool  bCodecInitDataSent;
	
    bool isAAC; /* AAC audio except BSAC, need to prepend ADTS header */
    bool isH264Video; /* H.264 video, need to add NAL start code */
    bool isBSAC;
    uint32 firstBSACTrackNum;
    uint32 audioBlockAlign;
    uint32 audioChannelMask; 

    MediaBuf *UsedListHead;
    MediaBuf *UsedListTail;
    MediaBuf *FreeListHead;
    MediaBuf *FreeListTail;
    int32 total_allocated;
    int32 total_used;
    int32 total_freed;
    int32 buffer_size;
    bool is_not_first_partial_buffer;
    OMX_TICKS nPartialFrameTimeStamp;

    uint32 bits_per_frame; 
    union {
        OMX_AUDIO_PARAM_PCMMODETYPE pcm_type;
        OMX_AUDIO_PARAM_MP3TYPE mp3_type;
        OMX_AUDIO_PARAM_AMRTYPE amr_type;
        OMX_AUDIO_PARAM_AC3TYPE ac3_type;
        OMX_AUDIO_PARAM_AACPROFILETYPE aac_type;
        OMX_AUDIO_PARAM_VORBISTYPE vorbis_type;
        OMX_AUDIO_PARAM_WMATYPE wma_type;
		OMX_AUDIO_PARAM_RATYPE ra_param;
    } audio_type;
	OMX_AUDIO_PARAM_WMATYPE_EXT wma_type_ext;
	union {
        OMX_VIDEO_PARAM_WMVTYPE wmv_type; 
		OMX_VIDEO_PARAM_RVTYPE rv_param;
    } video_type;
}Track;


class FslParser : public Parser {
    public:
        FslParser();
        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        uint8 * GetEmptyBufFromList(uint32 track_num,uint32 *size, void **buf_context);
        void ReturnBufToFreeList(uint32 track_num, void * buf_context);

    private:
        OMX_U32  trackNum;
        fsl_osal_mutex sParserMutex;
        OMX_U32 nVideoDataSize;
        OMX_U32 nAudioDataSize;
        OMX_U64 startTime;
        OMX_U64 endTime;
        OMX_U32 sampleFlag;
        OMX_U32 nextSampleSize;

        uint8 appConext[16];
        ParserOutputBufferOps outputBufferOps;
        FslParserInterface * IParser;
        HANDLE hParserDll;
        uint32 read_mode;

        FslParserHandle  parserHandle;

        ShareLibarayMgr *LibMgr;

        Track tracks[MAX_TRACK_COUNT];

        OMX_ERRORTYPE InstanceInit();
        OMX_ERRORTYPE InstanceDeInit();

        int32 ExportIndexTable(FslParserHandle  parser_handle);
        int32 ImportIndexTable(FslParserHandle  parser_handle);
        virtual OMX_ERRORTYPE GetNextSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer);
        virtual OMX_ERRORTYPE GetNextSyncSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer);
        virtual OMX_ERRORTYPE GetPrevSyncSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer);
        OMX_ERRORTYPE GetSyncSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer,uint32 direction);
        OMX_ERRORTYPE GetNextSampleFileMode(MediaBuf **pBuf, uint32 *data_size, uint32 track_num);
        OMX_ERRORTYPE GetSyncSampleFileMode(MediaBuf **pBuf, uint32 *data_size, uint32 track_num,uint32 direction);
        OMX_ERRORTYPE GetOneSample(MediaBuf **pBuf, uint32 *data_size, uint32 track_num,bool is_sync, uint32 direction);

        virtual OMX_ERRORTYPE DoSeek(OMX_U32 nPortIndex,OMX_U32 nSeekFlag);
        OMX_ERRORTYPE SetupPortMediaFormat();
        OMX_ERRORTYPE CheckAllPortDectedFinished();
        OMX_ERRORTYPE InitCoreParser();
        virtual void SetAudioCodecType(OMX_PARAM_PORTDEFINITIONTYPE *pPortDef, uint32 audio_num);
        virtual void SetVideoCodecType(OMX_PARAM_PORTDEFINITIONTYPE *pPortDef, uint32 video_num);
        virtual void ActiveTrack(uint32 track_number);		
        virtual void DisableTrack(uint32 track_number);		
        int32 CreateParserInterface();
        OMX_ERRORTYPE AddBufferToReadyList(uint32 track_num, void * buf_context);
        OMX_ERRORTYPE RemoveBufferFromReadyList(uint32 track_num, void * buf_context);
        OMX_ERRORTYPE ReturnBufToTrack(uint32 track_num, void * buf_context);
};

#endif
/* File EOF */
