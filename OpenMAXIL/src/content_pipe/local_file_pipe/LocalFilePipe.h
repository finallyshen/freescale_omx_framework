/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*!
 * @file OpenMAXIL/src/content_pipe/LocalFilePipe.h
 *
 * @brief Declares methods specific to freescale implementation of local file system content pipe.
 *
 *	The OMX_ContentPipe source contains the methods specific
 *	to the content pipe implementation for a local file sysytem.
 *
 * @ingroup OMX_Content_pipe
 */

#ifndef LocalFilePipe_h
#define LocalFilePipe_h

#include "OMX_Core.h"
#include "OMX_ContentPipe.h"

/** The OMX_LFILE_PIPE_API is platform specific definitions used
 *  to declare OMX function prototypes.  They are modified to meet the
 *  requirements for a particular platform */
#ifdef __WINCE_DLL
#   ifdef __OMX_LFILE_PIPE_EXPORTS
#       define OMX_LFILE_PIPE_API __declspec(dllexport)
#   else
#       define OMX_LFILE_PIPE_API __declspec(dllimport)
#   endif
#else
#   ifdef __OMX_LFILE_PIPE_EXPORTS
#       define OMX_LFILE_PIPE_API
#   else
#       define OMX_LFILE_PIPE_API extern
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
	OMX_LFILE_PIPE_API CPresult LocalFilePipe_Init(CP_PIPETYPE *pipe);

#ifdef __cplusplus
}
#endif

#endif
/* File EOF */






