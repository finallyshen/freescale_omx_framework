/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file V4lSource.h
 *  @brief Class definition of V4lSource Component
 *  @ingroup V4lSource
 */

#ifndef V4lSource_h
#define V4lSource_h

#include "VideoSource.h"

#define MAX_V4L_BUFFER 22
#define BUFFER_NEW_RETRY_MAX 1000

/* capture framebuffer type */
typedef struct {
    OMX_U32 nIndex;
    OMX_U32 nLength;
    OMX_PTR pVirtualAddr;
    OMX_PTR pPhyiscAddr;
}FB_DATA;

class V4lSource : public VideoSource {
    public:
        V4lSource();
    private:
        OMX_ERRORTYPE InitSourceComponent();
        OMX_ERRORTYPE DeInitSourceComponent();
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
		OMX_ERRORTYPE DoAllocateBuffer(OMX_PTR *buffer, OMX_U32 nSize,OMX_U32 nPortIndex);
        OMX_ERRORTYPE DoFreeBuffer(OMX_PTR buffer,OMX_U32 nPortIndex);
        OMX_ERRORTYPE SetDevice();
		OMX_ERRORTYPE SetSensorMode();
		OMX_ERRORTYPE PortFormatChanged(OMX_U32 nPortIndex);
		OMX_ERRORTYPE SetVideoFormat();
		OMX_ERRORTYPE SetWhiteBalance();
		OMX_ERRORTYPE SetDigitalZoom();
		OMX_ERRORTYPE SetExposureValue();
		OMX_ERRORTYPE SetRotation();
        OMX_ERRORTYPE StartDevice();
        OMX_ERRORTYPE StopDevice();
        OMX_ERRORTYPE SendBufferToDevice();
        OMX_ERRORTYPE GetOneFrameFromDevice();
        OMX_TICKS GetDelayofFrame();
        OMX_TICKS GetFrameTimeStamp();
        OMX_ERRORTYPE SetDeviceRotation();
        OMX_ERRORTYPE SetDeviceInputCrop();
        OMX_ERRORTYPE SetDeviceDisplayRegion();

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

        int fd_v4l;
		OMX_S32 g_extra_pixel;
		OMX_S32 g_input;
        OMX_CONFIG_RECTTYPE sRectIn;
        OMX_CONFIG_RECTTYPE sRectOut;
        OMX_BOOL bStreamOn;
        OMX_BOOL bFrameBufferInit;
        OMX_U32 nFrameBuffer;
        OMX_U32 nAllocating;
        FB_DATA fb_data[MAX_PORT_BUFFER];
        struct v4l2_buffer v4lbufs[MAX_V4L_BUFFER];
        OMX_BUFFERHEADERTYPE *BufferHdrs[MAX_V4L_BUFFER];
        OMX_U32 nQueuedBuffer;
        OMX_U32 nSourceFrames;
        fsl_osal_mutex lock;
		OMX_TICKS nCurTS;
        OMX_S32 EnqueueBufferIdx;

};

#endif
/* File EOF */
