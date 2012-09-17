/**
 *  Copyright (c) 2010-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file SorensonDec.h
 *  @brief Class definition of SorensonDec Component
 *  @ingroup SorensonDec
 */

#ifndef SorensonDec_h
#define SorensonDec_h

#include "VideoFilter.h"
#include "h263_api.h"

typedef enum {
    SORENSONDEC_LOADED,
    SORENSONDEC_INIT,
    SORENSONDEC_RUN,
    SORENSONDEC_SEEK,	/*workaround for seek: in this state, we need to paser frame type and only feed I frame to decoder to avoid mosaic output*/
    SORENSONDEC_STOP
} SORENSONDECSTATE;

class SorensonDec : public VideoFilter {
    public:
        SorensonDec();
        OMX_S32 DoGetBitstream(OMX_U32 nLen, OMX_U8 *pBuffer, OMX_S32 *pEndOfFrame);
        unsigned char * GetFrameBuffer();
        void RejectFrameBuffer( unsigned char * mem_ptr);
        void ReleaseFrameBuffer( unsigned char * mem_ptr);

    private:
        OMX_PTR pInBuffer;
        OMX_U32 nInputSize;
        OMX_U32 nInputOffset;
        OMX_BOOL bInEos;
        OMX_PTR pOutBuffer;
        void *DecStruct ;
        OMX_S32 nPadWidth;
        OMX_S32 nPadHeight;
        OMX_S32 nFrameSize;
        OMX_PTR pFrameBuffer;
        OMX_CONFIG_RECTTYPE sOutCrop;
        SORENSONDECSTATE eSorensonDecState;
        OMX_S32 FrameBufSize;
        OMX_S32 MinFrameBufferCount;
        INFO_STRUCT_T Info;
        NAL_BUF_T NalBuf;
        OMX_U8 *FrameBuffer[16]; // Internal frame buffer for codec
        OMX_U32 nFrameBufState[16]; 
        OMX_U32 nOutputBufGot;
        OMX_S32 DecStructSize;
        OMX_U32 DecFrameWidth;
        OMX_U32 DecFrameHeight;

        OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);

        OMX_ERRORTYPE SetInputBuffer(OMX_PTR pBuffer, OMX_S32 nSize, OMX_BOOL bLast);
        OMX_ERRORTYPE SetOutputBuffer(OMX_PTR pBuffer);
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
        OMX_BOOL CheckResolutionChanged();
        void CopyOutputFrame(OMX_U8 *pOutputFrame);
};

#endif
/* File EOF */
