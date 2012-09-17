/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file V4lRender.h
 *  @brief Class definition of V4lRender Component
 *  @ingroup V4lRender
 */

#ifndef V4lRender_h
#define V4lRender_h

#include "VideoRender.h"

#define MAX_V4L_BUFFER 22

/* display framebuffer type */
typedef struct {
    OMX_U32 nIndex;
    OMX_U32 nLength;
    OMX_PTR pVirtualAddr;
    OMX_PTR pPhyiscAddr;
}FB_DATA;

class V4lRender : public VideoRender {
    public:
        V4lRender();
    private:
        OMX_S32 device;
        OMX_CONFIG_RECTTYPE sRectOut;
        OMX_BOOL bStreamOn;
        OMX_BOOL bFrameBufferInit;
        OMX_U32 nFrameBuffer;
        OMX_U32 nAllocating;
        FB_DATA fb_data[MAX_PORT_BUFFER];
        struct v4l2_buffer v4lbufs[MAX_V4L_BUFFER];
        OMX_BUFFERHEADERTYPE *BufferHdrs[MAX_V4L_BUFFER];
        OMX_U32 nQueuedBuffer;
        OMX_U32 nRenderFrames;
        fsl_osal_mutex lock;
        OMX_CONFIG_CAPTUREFRAME sCapture;
        OMX_S32 EnqueueBufferIdx;
        OMX_CONFIG_OUTPUTMODE sOutputMode;
        OMX_BOOL bInTransitState;
        OMX_BOOL bCaptureFrameDone;
        OMX_BOOL bV4lNewApi;   /*based on kernel 2.6.38 later*/
        OMX_BOOL bDummyValid; /*true: one dummy buf has been filled into queue*/
        OMX_BOOL bFlushState;	/*in flush state, we make sure: one valid buffer will be SendBuffer() after V4lDeQueueBuffer()*/

        OMX_ERRORTYPE InitRenderComponent();
        OMX_ERRORTYPE DeInitRenderComponent();
        OMX_ERRORTYPE RenderGetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE RenderSetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE PortFormatChanged(OMX_U32 nPortIndex);
        OMX_ERRORTYPE DoAllocateBuffer(OMX_PTR *buffer, OMX_U32 nSize,OMX_U32 nPortIndex);
        OMX_ERRORTYPE DoFreeBuffer(OMX_PTR buffer,OMX_U32 nPortIndex);
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
        OMX_ERRORTYPE SetDeviceRotation();
        OMX_ERRORTYPE SetDeviceInputCrop();
        OMX_ERRORTYPE SetDeviceDisplayRegion();
        OMX_ERRORTYPE WriteDevice(OMX_BUFFERHEADERTYPE *pBufferHdr);
        OMX_ERRORTYPE CaptureFrame(OMX_U32 nIndex);
        OMX_ERRORTYPE SetOutputMode();
        OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex) ;

        OMX_ERRORTYPE V4lBufferInit();
        OMX_ERRORTYPE V4lBufferDeInit();
        OMX_ERRORTYPE V4lSearchBuffer(OMX_PTR pVirtualAddr, OMX_U32 *pIndex);
        OMX_ERRORTYPE V4lQueueBuffer(OMX_U32 nIndex);
        OMX_ERRORTYPE V4lDeQueueBuffer(OMX_U32 *nIndex);
        OMX_ERRORTYPE V4lStreamOff(OMX_BOOL bKeepLast);
        OMX_ERRORTYPE V4lStreamOnInPause();
        OMX_ERRORTYPE V4lSetInputCrop();
        OMX_ERRORTYPE V4lSetOutputCrop();
        OMX_ERRORTYPE V4lSetRotation();
        OMX_ERRORTYPE V4lSetOutputLayer(OMX_S32 output);
};

#endif
/* File EOF */
