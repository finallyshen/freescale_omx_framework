/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#ifndef _omx_common_h_
#define _omx_common_h_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    OMX_VIDEO_RENDER_IPULIB,
    OMX_VIDEO_RENDER_OVERLAY,
    OMX_VIDEO_RENDER_SURFACE,
    OMX_VIDEO_RENDER_V4L,
    OMX_VIDEO_RENDER_FB,
    OMX_VIDEO_RENDER_EGL,
    OMX_VIDEO_RENDER_NUM,
}OMX_VIDEO_RENDER_TYPE;

typedef struct {
    const OMX_VIDEO_RENDER_TYPE type;
    const char* name;
}VIDEO_RENDER_MAP_ENTRY;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
