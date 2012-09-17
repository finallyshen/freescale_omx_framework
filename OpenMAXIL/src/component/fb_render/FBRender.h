/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file FBRender.h
 *  @brief Class definition of FBRender Component
 *  @ingroup FBRender
 */

#ifndef FBRender_h
#define FBRender_h

#include "VideoRender.h"

#define FB_ERROR(fm, ...) 			LOG_ERROR(fm,##__VA_ARGS__)
#define FB_WARNING(fm, ...) 		LOG_WARNING(fm,##__VA_ARGS__)
#define FB_INFO(fm, ...)  			LOG_INFO(fm,##__VA_ARGS__)
#define FB_DEBUG(fm, ...) 			LOG_DEBUG(fm,##__VA_ARGS__)
#define FB_DEBUG_BUFFER(fm, ...) LOG_BUFFER(fm,##__VA_ARGS__)
#define FB_LOG(fm, ...) 			LOG_LOG(fm,##__VA_ARGS__)

class FBRender : public VideoRender {
    public:
        FBRender();
    private:
        OMX_S32 device;
        OMX_CONFIG_RECTTYPE sRectOut;
        OMX_BOOL bStreamOn;
        OMX_BOOL bFrameBufferInit;
        OMX_U32 nFrameBuffer;
        OMX_U32 nAllocating;
        OMX_BUFFERHEADERTYPE *BufferHdrs[32];
        OMX_U32 nQueuedBuffer;
        OMX_U32 nRenderFrames;
        fsl_osal_mutex lock;
        OMX_CONFIG_CAPTUREFRAME sCapture;
        OMX_U32 EnqueueBufferIdx;
        OMX_CONFIG_OUTPUTMODE sOutputMode;
        OMX_BOOL bInTransitState;
        OMX_BOOL bCaptureFrameDone;
        int lcd_w;
	 int lcd_h;
	 int fb_size;
	 unsigned short  * fb_ptr;
	 long unsigned int  phy_addr; 		

        OMX_ERRORTYPE InitRenderComponent();
        OMX_ERRORTYPE DeInitRenderComponent();
        OMX_ERRORTYPE RenderGetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE RenderSetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE PortFormatChanged(OMX_U32 nPortIndex);
#if 0		
        OMX_ERRORTYPE DoAllocateBuffer(OMX_PTR *buffer, OMX_U32 nSize);
        OMX_ERRORTYPE DoFreeBuffer(OMX_PTR buffer);
#endif		
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
        OMX_ERRORTYPE SetDeviceRotation();
        OMX_ERRORTYPE SetDeviceInputCrop();
        OMX_ERRORTYPE SetDeviceDisplayRegion();
        OMX_ERRORTYPE WriteDevice(OMX_BUFFERHEADERTYPE *pBufferHdr);
        OMX_ERRORTYPE CaptureFrame(OMX_U32 nIndex);
        OMX_ERRORTYPE SetOutputMode();

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
