/**
 *  Copyright (c) 2009,2012 Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "FakeVideoRender.h"

FakeVideoRender::FakeVideoRender()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_render.fake.sw-based");
    ComponentVersion.s.nVersionMajor = 0x0;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x0;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = "video_render.fake";

    nFrameBufferMin = 1;
    nFrameBufferActual = 1;
    fsl_osal_memset(&sVideoFmt, 0, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
    sVideoFmt.nFrameWidth = 800;
    sVideoFmt.nFrameHeight = 600;
    sVideoFmt.eCompressionFormat = OMX_VIDEO_CodingUnused;
    sVideoFmt.eColorFormat = OMX_COLOR_Format16bitRGB565;

    eRotation = ROTATE_NONE;
    OMX_INIT_STRUCT(&sRectIn, OMX_CONFIG_RECTTYPE);
    sRectIn.nWidth = sVideoFmt.nFrameWidth;
    sRectIn.nHeight = sVideoFmt.nFrameHeight;
    sRectIn.nTop = 0;
    sRectIn.nLeft = 0;
}

OMX_ERRORTYPE FakeVideoRender::OpenDevice()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE FakeVideoRender::CloseDevice()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE FakeVideoRender::WriteDevice(
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    ports[0]->SendBuffer(pBufferHdr);
    return OMX_ErrorNone;
}

/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE FakeVideoRenderInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        FakeVideoRender *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(FakeVideoRender, ());
        if(obj == NULL)
            return OMX_ErrorInsufficientResources;

        base = (ComponentBase*)obj;
        ret = base->ConstructComponent(pHandle);
        if(ret != OMX_ErrorNone)
            return ret;

        return ret;
    }
}

/* File EOF */
