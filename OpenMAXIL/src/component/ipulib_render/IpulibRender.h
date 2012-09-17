/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file IpulibRender.h
 *  @brief Class definition of IpulibRender Component
 *  @ingroup IpulibRender
 */

#ifndef IpulibRender_h
#define IpulibRender_h

#include <stdint.h>                                                               
#include "mxc_ipu_hl_lib.h"
#include "VideoRender.h"

class IpulibRender : public VideoRender {
    public:
        IpulibRender();
    private:
        OMX_U32 nFrame;
        OMX_BOOL bInitDev;
        fsl_osal_mutex lock;
        ipu_lib_handle_t ipu_handle;
        ipu_lib_input_param_t sInParam;
        ipu_lib_output_param_t sOutParam;
        OMX_CONFIG_RECTTYPE sRectOut;
        OMX_CONFIG_OUTPUTMODE sOutputMode;
        OMX_BOOL bCaptureFrameDone;
        OMX_CONFIG_CAPTUREFRAME sCapture;
        OMX_PTR pShowFrame;
        OMX_U32 nFrameLen;
        OMX_BOOL bSuspend;

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

        OMX_ERRORTYPE IpulibInit(OMX_PTR pFrame);
        OMX_ERRORTYPE IpulibRenderFrame(OMX_PTR pFrame);
};

#endif
/* File EOF */
