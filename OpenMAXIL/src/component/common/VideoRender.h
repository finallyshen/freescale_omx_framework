/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file VideoRender.h
 *  @brief Class definition of VideoRender Component
 *  @ingroup VideoRender
 */

#ifndef VideoRender_h
#define VideoRender_h

#include "ComponentBase.h"
#include "ShareLibarayMgr.h"
#include "video_frame_visitor.h"

#define NUM_PORTS 2
#define IN_PORT 0
#define CLK_PORT 1

#define CHECK_STRUCT(ps,t,r) \
    do { \
        OMX_CHECK_STRUCT(ps, t, r); \
        if(r != OMX_ErrorNone) \
        break; \
        if(ps->nPortIndex != IN_PORT) { \
            r = OMX_ErrorBadPortIndex; \
            break; \
        } \
    } while(0);

class VideoRender : public ComponentBase {
    public:
        VideoRender();
    protected:
        OMX_U32 nFrameBufferMin;
        OMX_U32 nFrameBufferActual;
        OMX_BUFFERSUPPLIERTYPE TunneledSupplierType;
        OMX_VIDEO_PORTDEFINITIONTYPE sVideoFmt;
        ROTATION eRotation;
        OMX_CONFIG_RECTTYPE sRectIn;
    private:
        TUNNEL_INFO hClock;
        OMX_TIME_CLOCKSTATE ClockState;
        OMX_S32 ClockScale;
        OMX_BUFFERHEADERTYPE *pSyncFrame;
        OMX_U32 nFrameCnt;
        OMX_U32 nDropCnt;
        OMX_U32 nDeviceDropCnt;
        OMX_U32 nContiniousDrop;
        ShareLibarayMgr LibMgr;
        OMX_PTR hVisitorLib;
        VFV_INTERFACE hVistorItf;
        OMX_PTR hVistor;
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        virtual OMX_ERRORTYPE InitRenderComponent();
        virtual OMX_ERRORTYPE DeInitRenderComponent();
        OMX_ERRORTYPE DoLoaded2Idle();
        OMX_ERRORTYPE DoIdle2Loaded();
        OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        virtual OMX_ERRORTYPE RenderGetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        virtual OMX_ERRORTYPE RenderSetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE ProcessDataBuffer();
        OMX_ERRORTYPE ProcessClkBuffer();
        OMX_ERRORTYPE SyncFrame(OMX_BUFFERHEADERTYPE *pBufferHdr);
        OMX_ERRORTYPE SyncRequest(OMX_BUFFERHEADERTYPE *pBufferHdr);
        OMX_ERRORTYPE RenderFrame(OMX_BUFFERHEADERTYPE *pBufferHdr);
        OMX_BOOL IsDropFrame(OMX_TICKS ts, OMX_TICKS meida);
        OMX_ERRORTYPE NotifyClkVideoLate();
        virtual OMX_ERRORTYPE OpenDevice() = 0;
        virtual OMX_ERRORTYPE CloseDevice() = 0;
        virtual OMX_ERRORTYPE WriteDevice(OMX_BUFFERHEADERTYPE *pBufferHdr) = 0;
        virtual OMX_ERRORTYPE SetDeviceRotation();
        virtual OMX_ERRORTYPE SetDeviceInputCrop();
        OMX_ERRORTYPE InitVideoVisitors();
        OMX_ERRORTYPE DeInitVideoVisitors();
        OMX_ERRORTYPE Report2VideoVisitors(OMX_BUFFERHEADERTYPE *pBufferHdr);
        virtual OMX_U32 GetDeviceDropFames() { return 0;};
};

#endif
/* File EOF */
