/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AacParser.h
 *  @brief Class definition of AacParser Component
 *  @ingroup AacParser
 */

#ifndef AacParser_h
#define AacParser_h

#include "aac_parser/AacCoreParser.h"
#include "AudioParserBase.h"

class AacParser : public AudioParserBase {
    public:
        AacParser();
    private:
		OMX_ERRORTYPE GetCoreParser();
		OMX_AUDIO_CODINGTYPE GetAudioCodingType();
};

#endif
/* File EOF */
