/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file FlacParser.h
 *  @brief Class definition of FlacParser Component
 *  @ingroup FlacParser
 */

#ifndef FlacParser_h
#define FlacParser_h

#include "AudioParserBase.h"
#include "flac_parser/FlacCoreParser.h"

class FlacParser : public AudioParserBase {
    public:
        FlacParser();
    private:
		OMX_ERRORTYPE GetCoreParser();
		OMX_AUDIO_CODINGTYPE GetAudioCodingType();
		OMX_ERRORTYPE AudioParserSetCodecConfig(OMX_BUFFERHEADERTYPE *pOutBuffer);
		OMX_S64 nGetTimeStamp(OMX_S64 *pSeekPoint);
};

#endif
/* File EOF */
