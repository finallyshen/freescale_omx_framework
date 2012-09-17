/**
 *  Copyright (c) 2009-2010,2012 Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */


#ifndef VIDEOPROCESSORCOMPONENT_H
#define VIDEOPROCESSORCOMPONENT_H

#include "ComponentBase.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>                                                               

#include "OMX_Core.h"
#include "OMX_Video.h"
#include "OMX_Component.h"
//#include "ComponentUtils.h"

#include "mxc_ipu_hl_lib.h"


/* Specification version */
#define VERSIONMAJOR    1
#define VERSIONMINOR    1
#define VERSIONREVISION 0
#define VERSIONSTEP     0

#define VP_COMP_VERSION 0x00020101
#define VP_NUMPORTS 2
#define VP_IN_PORT 0
#define VP_OUT_PORT 1

static const OMX_STRING VP_NAME = "OMX.Freescale.std.video.processor.hw-based";


#define VP_ERROR(fm, ...) 			LOG_ERROR(fm,##__VA_ARGS__)
#define VP_WARNING(fm, ...) 		LOG_WORNING(fm,##__VA_ARGS__)
#define VP_INFO(fm, ...)  			LOG_INFO(fm,##__VA_ARGS__)
#define VP_DEBUG(fm, ...) 			LOG_DEBUG(fm,##__VA_ARGS__)
#define VP_DEBUG_BUFFER(fm, ...) LOG_BUFFER(fm,##__VA_ARGS__)
#define VP_LOG(fm, ...) 			LOG_LOG(fm,##__VA_ARGS__)
#ifdef __cplusplus
}
#endif /* __cplusplus */

class VideoProcessorComponent: public  ComponentBase{
	public:
       	 VideoProcessorComponent();
         OMX_ERRORTYPE UseEGLImage(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex, 
                 OMX_PTR pAppPrivate, void* eglImage);
		 OMX_ERRORTYPE GetPort(Port ** ppPort, OMX_S32 nIndex );
	private:		 
		OMX_ERRORTYPE InitComponent();
		OMX_ERRORTYPE DeInitComponent();
		OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
		OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure) ;
		OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure) ;
		OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure) ;
//		OMX_ERRORTYPE GetExtensionIndex(OMX_STRING cParameterName, OMX_INDEXTYPE* pIndexType);	
		OMX_ERRORTYPE omxSetHeader(OMX_PTR  pHeader, OMX_S32 nHeaderSize);
		OMX_ERRORTYPE omxCheckHeader(OMX_PTR pHeader, OMX_S32 nHeaderSize);	
		OMX_ERRORTYPE ProcessDataBuffer();
		OMX_ERRORTYPE ComponentReturnBuffer(OMX_U32 nPortIndex);
		OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex);
//		OMX_ERRORTYPE DoLoaded2Idle();
//		OMX_ERRORTYPE DoIdle2Loaded();
		OMX_ERRORTYPE InstanceDeInit();
//		OMX_ERRORTYPE InstanceInit() ;
		OMX_ERRORTYPE VideoProcessor_init( );
		OMX_ERRORTYPE VideoProcessor_DeInit( );
		OMX_ERRORTYPE VideoProcessor_ProcessData(OMX_PTR buf);

	public:
		OMX_U32 nOutBufIdx;
		
	private:
		ipu_lib_handle_t ipu_handle;
		ipu_lib_input_param_t sInParam;
		ipu_lib_output_param_t sOutParam;

		OMX_HANDLETYPE hSrcCompHandle;
		OMX_HANDLETYPE hSinkCompHandle;

		OMX_U32 nInWidth;
		OMX_U32 nInHeight;
		OMX_CONFIG_RECTTYPE sInRect;
		OMX_CONFIG_RECTTYPE sOutRect;
		OMX_COLOR_FORMATTYPE eInColorFmt;
		OMX_U32 nOutWidth;
		OMX_U32 nOutHeight;
		OMX_COLOR_FORMATTYPE eOutColorFmt;
		OMX_CONFIG_ROTATIONTYPE sRotation;

		OMX_S32 nInFrames;
		OMX_S32 nOutFrames;
		OMX_BOOL bStarted;
		OMX_BOOL bReset;
		OMX_BOOL bFlush;
              OMX_BOOL bUseEGLImage;
		OMX_HANDLETYPE in_queue;
		OMX_HANDLETYPE out_queue;
		OMX_BUFFERHEADERTYPE *pInBufHdr[2];
		OMX_BUFFERHEADERTYPE *pOutBufHdr[2];
};


#endif
/* File EOF */




