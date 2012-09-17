/**
 *  Copyright (c) 2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file GMEgl.h
 *  @brief Interface definition of EGL used in GMPlayer
 *  @ingroup GMPlayer
 */

#ifndef _GMEgl_h_
#define _GMEgl_h_

#include "OMX_Core.h"

typedef enum {
    GMEGLDisplayNONE = -1,
    GMEGLDisplayX11,	
    GMEGLDisplayFB
}GMEglDisplayType;

OMX_ERRORTYPE GMCreateEglContext(OMX_PTR *pContext, OMX_U32 nWidth, OMX_U32 nHeight, GMEglDisplayType dispType);
OMX_ERRORTYPE GMDestroyEglContext(OMX_PTR Context);
OMX_ERRORTYPE GMCreateEglImage(OMX_PTR Context, OMX_PTR *pImage);
OMX_ERRORTYPE GMEglDrawImage(OMX_PTR Context, OMX_PTR Image);

#endif

