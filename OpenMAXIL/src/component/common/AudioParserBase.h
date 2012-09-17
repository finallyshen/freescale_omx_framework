/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AudioParserBase.h
 *  @brief Class definition of AudioParserBase Component
 *  @ingroup AudioParserBase
 */

#ifndef AudioParserBase_h
#define AudioParserBase_h

#include "ComponentBase.h"
#include "Parser.h"
#include "AudioCoreParser.h"
#ifndef ANDROID_BUILD
#include "id3_parser/id3_parse.h"
#endif
#define MAXAUDIODURATIONHOUR (1024)
#define AUDIO_PARSER_READ_SIZE (16*1024)
#define AUDIO_PARSER_SEGMENT_SIZE (32*1024)

#define PARSERAUDIO_HEAD 0
#define PARSERAUDIO_MIDDLE 1
#define PARSERAUDIO_TAIL 2
#define PARSERAUDIO_BEGINPOINT 3
#define PARSERAUDIO_VBRDURATION 4

class AudioParserBase : public Parser {
	public:
		OMX_ERRORTYPE ParserCalculateVBRDuration(); 
        OMX_ERRORTYPE InstanceInit();
        OMX_ERRORTYPE InstanceDeInit();
	protected:
		OMX_BOOL bNeedSendCodecConfig;
		AUDIO_FILE_INFO FileInfo;
		OMX_U64 nEndPoint;
		OMX_U64 nFileSize;
		AUDIO_PARSERRETURNTYPE (*ParserFileHeader)(AUDIO_FILE_INFO *pFileInfo, \
				uint8 *pBuffer, uint32 nBufferLen);
		AUDIO_PARSERRETURNTYPE (*ParserFrame)(AUDIO_FRAME_INFO *pFrameInfo, \
				uint8 *pBuffer, uint32 nBufferLen);
        FslFileHandle sourceFileHandle;
	private:
        virtual OMX_ERRORTYPE GetCoreParser() = 0;
		OMX_ERRORTYPE AudioParserInstanceInit();
		OMX_ERRORTYPE ParserID3Tag();
		OMX_ERRORTYPE AudioParserFileHeader();
		OMX_ERRORTYPE ParserOneSegmentAudio();
		OMX_ERRORTYPE ParserThreeSegmentAudio();
		OMX_ERRORTYPE ParserFindBeginPoint();
		OMX_ERRORTYPE AudioParserInstanceDeInit();
		virtual OMX_AUDIO_CODINGTYPE GetAudioCodingType() = 0;
		OMX_ERRORTYPE SetupPortMediaFormat();
		virtual OMX_ERRORTYPE AudioParserSetCodecConfig(OMX_BUFFERHEADERTYPE *pOutBuffer);
        OMX_ERRORTYPE GetNextSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer);
		virtual OMX_U64 FrameBound(OMX_U64 nSkip);
		virtual OMX_S64 nGetTimeStamp(OMX_S64 *pSeekPoint);
		OMX_ERRORTYPE DoSeek(OMX_U32 nPortIndex, OMX_U32 nSeekFlag);
 		OMX_ERRORTYPE ParserAudioFrame(OMX_U8 *pBuffer, OMX_U32 nActuralRead, \
				OMX_U32 nSegmentCnt);
 		OMX_ERRORTYPE ParserAudioFrameOverlap(OMX_U8 *pBuffer, OMX_U32 nActuralRead, \
				OMX_U32 nSegmentCnt);
		OMX_U32 GetAvrageBitRate();
		OMX_U8 *AudioParserGetBuffer(OMX_U32 nBufferSize);
		OMX_ERRORTYPE AudioParserFreeBuffer(OMX_U8 *pBuffer);
		OMX_ERRORTYPE AudioParserBuildSeekTable(OMX_S32 nOffset, OMX_U32 nSamples, \
				OMX_U32 nSamplingRate);

		OMX_U64 nBeginPoint;
		OMX_U64 nBeginPointOffset;
		OMX_U64 nReadPoint;
		OMX_BOOL bCBR;
		OMX_BOOL bVBRDurationReady;
		OMX_BOOL bBeginPointFounded;
		OMX_U32 nAvrageBitRate;
		OMX_U8 *pTmpBufferPtr;
		OMX_U32 nOverlap;
		OMX_U64 nTotalBitRate;
		OMX_U32 nFrameCount;
		OMX_U64 **SeekTable[MAXAUDIODURATIONHOUR];
		OMX_S64 nSource2CurPos;
		OMX_U32 secctr;
		OMX_U32 minctr;
		OMX_U32 hourctr;
		OMX_U32 nOneSecondSample;
        OMX_U32 nSampleRate;
		AUDIO_FRAME_INFO FrameInfo;
		fsl_osal_ptr pThreadId;
		OMX_BOOL bStopVBRDurationThread;
		OMX_BOOL bSegmentStart;
		OMX_BOOL bTOCSeek;
#ifndef ANDROID_BUILD
		meta_data_v1 info;
		meta_data_v2 info_v2; 
#endif
};

#endif
/* File EOF */
