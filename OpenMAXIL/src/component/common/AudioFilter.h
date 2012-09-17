/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AudioFilter.h
 *  @brief Class definition of AudioFilter Component
 *  @ingroup AudioFilter
 */

#ifndef AudioFilter_h
#define AudioFilter_h

#include "ComponentBase.h"
#include "RingBuffer.h"

#define AUDIO_FILTER_INPUT_PORT 0
#define AUDIO_FILTER_OUTPUT_PORT 1
#define AUDIO_FILTER_PORT_NUMBER 2
#define CHECKFRAMEHEADERREADLEN 1024
#define AUDIO_MAXIMUM_ERROR_COUNT 100

typedef enum {
    AUDIO_FILTER_SUCCESS,
    AUDIO_FILTER_EOS,
    AUDIO_FILTER_FAILURE,
}AUDIO_FILTERRETURNTYPE;

class AudioFilter : public ComponentBase {
    public:
		RingBuffer AudioRingBuffer;
		OMX_BOOL bReceivedEOS;
	protected:
        OMX_ERRORTYPE ProcessDataBuffer();
		OMX_S32 nInputPortBufferSize;
		OMX_S32 nOutputPortBufferSize;
		OMX_BOOL bDecoderEOS;
		OMX_U32 nPushModeInputLen;
		OMX_U32 nRingBufferScale;
		OMX_TICKS TS_PerFrame;
		OMX_BOOL bInstanceReset;
        OMX_BUFFERHEADERTYPE *pOutBufferHdr;
        OMX_BUFFERHEADERTYPE *pInBufferHdr;
        OMX_AUDIO_PARAM_PCMMODETYPE PcmMode; /**< For output port */
    private:
        OMX_ERRORTYPE InstanceInit();
        OMX_ERRORTYPE InstanceDeInit();
        virtual OMX_ERRORTYPE AudioFilterInstanceInit() = 0;
        virtual OMX_ERRORTYPE AudioFilterCodecInit() = 0;
        virtual OMX_ERRORTYPE AudioFilterInstanceDeInit() = 0;
        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual OMX_ERRORTYPE AudioFilterGetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure) = 0;
        virtual OMX_ERRORTYPE AudioFilterSetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure) = 0;
        virtual OMX_ERRORTYPE AudioFilterSetParameterPCM();
		virtual OMX_ERRORTYPE AudioFilterInstanceReset();
		virtual OMX_ERRORTYPE AudioFilterCheckCodecConfig();
		virtual OMX_ERRORTYPE AudioFilterCheckFrameHeader();
        virtual AUDIO_FILTERRETURNTYPE AudioFilterFrame() = 0;
		virtual OMX_ERRORTYPE AudioFilterReset() = 0;
		OMX_ERRORTYPE ProcessInputBufferFlag();
		OMX_ERRORTYPE ProcessInputDataBuffer();
		OMX_ERRORTYPE ProcessOutputDataBuffer();
        virtual OMX_ERRORTYPE ComponentReturnBuffer(OMX_U32 nPortIndex);
        OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex);
		OMX_AUDIO_PARAM_PORTFORMATTYPE PortFormat[AUDIO_FILTER_PORT_NUMBER];
		OMX_BOOL bFirstFrame;
		OMX_BOOL bCodecInit;
		OMX_BOOL bDecoderInitFail;
};

#endif
/* File EOF */
