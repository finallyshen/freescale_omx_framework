/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file WavParser.h
 *  @brief Class definition of WavParser Component
 *  @ingroup WavParser
 */

#ifndef WavParser_h
#define WavParser_h

#include "AudioParserBase.h"
#include "wav_parser/WavCoreParser.h"

class WavParser : public AudioParserBase {
    public:
        WavParser();
    private:
		OMX_ERRORTYPE GetCoreParser();
		OMX_AUDIO_CODINGTYPE GetAudioCodingType();
		OMX_ERRORTYPE AudioParserSetCodecConfig(OMX_BUFFERHEADERTYPE *pOutBuffer);
		OMX_U64 FrameBound(OMX_U64 nSkip);
};

#endif
/* File EOF */
