/**
 *  Copyright (c) 2010,2012 Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#ifndef HttpsPipe_h
#define HttpsPipe_h

#include "OMX_Core.h"
#include "OMX_ContentPipe.h"

/** The OMX_HTTPS_PIPE_API is platform specific definitions used
 *  to declare OMX function prototypes.  They are modified to meet the
 *  requirements for a particular platform */
#ifdef __WINCE_DLL
#   ifdef __OMX_HTTPS_PIPE_EXPORTS
#       define OMX_HTTPS_PIPE_API __declspec(dllexport)
#   else
#       define OMX_HTTPS_PIPE_API __declspec(dllimport)
#   endif
#else
#   ifdef __OMX_HTTPS_PIPE_EXPORTS
#       define OMX_HTTPS_PIPE_API
#   else
#       define OMX_HTTPS_PIPE_API extern
#   endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! This method is the entry point function to this content pipe implementation. This function
 	is registered with core during content pipe registration (Will happen at OMX_Init).
 	Later when OMX_GetContentPipe is called for this implementation this entry point function
    is called, and content pipe function pointers are assigned with the implementation function
 	pointers.

    @param [InOut] pipe
              pipe is the handle to the content pipe that is passed.

    @return OMX_ERRORTYPE
             This will denote the success or failure of the method call.
 */
OMX_HTTPS_PIPE_API CPresult HttpsPipe_Init(CP_PIPETYPE *pipe);

#ifdef __cplusplus
}
#endif

#endif
/* File EOF */



