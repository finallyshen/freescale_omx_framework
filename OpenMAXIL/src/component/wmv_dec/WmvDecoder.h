/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file WmvDecoder.h
 *  @brief Class definition of WmvDecoder Component
 *  @ingroup WmvDecoder
 */

#ifndef WmvDecoder_h
#define WmvDecoder_h

#include "VideoFilter.h"
#include "wmv789_dec_api.h"
#include "ShareLibarayMgr.h"

typedef enum {
    WMVDEC_LOADED,
    WMVDEC_INIT,
    WMVDEC_RUN,
    WMVDEC_STOP
} WMVDECSTATE;

class WmvDecoder : public VideoFilter {
    public:
        WmvDecoder();
        OMX_S32 DoGetBitstream(OMX_U32 nLen, OMX_U8 *pBuffer, OMX_S32 *pEndOfFrame);
    private:
        OMX_PTR pInBuffer;
        OMX_U32 nInputSize;
        OMX_U32 nInputOffset;
        OMX_BOOL bInEos;
        OMX_PTR pOutBuffer;
        sWmv9DecObjectType sDecObj;
        eWmv9DecCompFmtType eFormat;
        OMX_S32 nPadWidth;
        OMX_S32 nPadHeight;
        OMX_S32 nFrameSize;
        OMX_PTR pFrameBuffer;
        OMX_CONFIG_RECTTYPE sOutCrop;
        WMVDECSTATE eWmvDecState;

        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);

        OMX_ERRORTYPE SetInputBuffer(OMX_PTR pBuffer, OMX_S32 nSize, OMX_BOOL bLast);
        OMX_ERRORTYPE SetOutputBuffer(OMX_PTR pBuffer);
        OMX_ERRORTYPE InitFilterComponent();
        OMX_ERRORTYPE DeInitFilterComponent();
        OMX_ERRORTYPE InitFilter();
        OMX_ERRORTYPE DeInitFilter();
        FilterBufRetCode FilterOneBuffer();
        OMX_ERRORTYPE GetOutputBuffer(OMX_PTR *ppBuffer,OMX_S32* pOutSize);
        OMX_ERRORTYPE FlushInputBuffer();
        OMX_ERRORTYPE FlushOutputBuffer();

        OMX_ERRORTYPE DetectOutputFmt();
        FilterBufRetCode DecodeOneFrame();
        OMX_ERRORTYPE AllocateDecoderMemory();
        OMX_ERRORTYPE FreeDecoderMemory();
        OMX_ERRORTYPE AllocateFrameBuffer();
        OMX_ERRORTYPE FreeFrameBuffer();

        ShareLibarayMgr *libMgr;
        OMX_PTR hLib;
        eWmv9DecRetType (*pWMV9DQuerymem) (sWmv9DecObjectType* , WMV9D_S32, WMV9D_S32);
        eWmv9DecRetType (*pWMV9DInit) (sWmv9DecObjectType*);
        eWmv9DecRetType (*pWMV9DDecode) (sWmv9DecObjectType*, WMV9D_U32);
        eWmv9DecRetType (*pWMV9DFree) (sWmv9DecObjectType *);
        eWmv9DecRetType  (*pWMV9DecGetOutputFrame) (sWmv9DecObjectType *);
        const char * (*pWMV9DecCodecVersionInfo)();

};

#endif
/* File EOF */
