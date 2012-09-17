/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Mp3Parser.h
 *  @brief Class definition of Mp3Parser Component
 *  @ingroup Mp3Parser
 */

#ifndef Mp3Parser_h
#define Mp3Parser_h

#include "AudioParserBase.h"
#include "mp3_parser_v2/Mp3CoreParser.h"

class Mp3Parser : public AudioParserBase {
	public:
		Mp3Parser();
		OMX_ERRORTYPE SetSource(OMX_PARAM_CONTENTURITYPE *Content, OMX_PARAM_CONTENTPIPETYPE *Pipe);
		OMX_ERRORTYPE UnloadParserMetadata();
		OMX_U32 GetMetadataNum();
		OMX_U32 GetMetadataSize(OMX_U32 index);
		OMX_ERRORTYPE GetMetadata(OMX_CONFIG_METADATAITEMTYPE *pMatadataItem);
	private:
		OMX_ERRORTYPE GetCoreParser();
		OMX_AUDIO_CODINGTYPE GetAudioCodingType();
};

#endif
/* File EOF */
