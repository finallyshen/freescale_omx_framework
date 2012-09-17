/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Ac3Parser.h
 *  @brief Class definition of Ac3Parser Component
 *  @ingroup Ac3Parser
 */

#ifndef Ac3Parser_h
#define Ac3Parser_h

#include "AudioParserBase.h"
#include "ac3_parser/Ac3CoreParser.h"

class Ac3Parser : public AudioParserBase {
    public:
        Ac3Parser();
    private:
		OMX_ERRORTYPE GetCoreParser();
		OMX_AUDIO_CODINGTYPE GetAudioCodingType();
};

#endif
/* File EOF */
