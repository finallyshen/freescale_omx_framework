/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file OverlayRender.h
 *  @brief Class definition of OverlayRender Component
 *  @ingroup OverlayRender
 */

#ifndef OverlayRender_h
#define OverlayRender_h

#include <stdint.h>                                                               
#include <surfaceflinger/ISurface.h>
#include <ui/Overlay.h>
#include <Queue.h>
#include "VideoRender.h"

using android::sp;
using android::ISurface;
using android::Overlay;
using android::OverlayRef;

class OverlayRender : public VideoRender {
    public:
        OverlayRender();
    private:

        OMX_U32 nFrame;
        OMX_CONFIG_OUTPUTMODE sOutputMode;
        OMX_BOOL bCaptureFrameDone;
        OMX_CONFIG_CAPTUREFRAME sCapture;
        OMX_PTR pShowFrame;
        OMX_U32 nFrameLen;
        OMX_BOOL bSuspend;

        sp<ISurface>            mSurface;
        sp<Overlay>             mOverlay;
        fsl_osal_mutex          lock;
        OMX_BOOL                bInitDev;
        sp<OverlayRef>        ref ;
        Queue                      * qBufferInOverlay;
        
        OMX_CONFIG_RECTTYPE sRectOut;

        OMX_ERRORTYPE InitRenderComponent();
        OMX_ERRORTYPE DeInitRenderComponent();
        OMX_ERRORTYPE RenderGetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE RenderSetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE PortFormatChanged(OMX_U32 nPortIndex);
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
        OMX_ERRORTYPE ResetDevice();
        OMX_ERRORTYPE StartDeviceInPause();
        OMX_ERRORTYPE WriteDevice(OMX_BUFFERHEADERTYPE *pBufferHdr);
        OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex) ;
        OMX_ERRORTYPE SetDeviceInputCrop();

        OMX_ERRORTYPE OverlayInit();
        OMX_ERRORTYPE OverlayRenderFrame(OMX_PTR pFrame);
};

#endif
/* File EOF */
