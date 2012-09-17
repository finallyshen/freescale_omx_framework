/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */


//#define LOG_NDEBUG 0
#define LOG_TAG "FSLRenderer"
#include <utils/Log.h>

#include <media/stagefright/HardwareAPI.h>
#include <media/stagefright/VideoRenderer.h>

#include <binder/MemoryHeapBase.h>
#include <binder/MemoryHeapPmem.h>
#include <media/stagefright/MediaDebug.h>
#include <surfaceflinger/ISurface.h>
#include "mxc_ipu_hl_lib.h"
#include "FSLRenderer.h"
#include "PlatformResourceMgrItf.h"

#define ipu_fourcc(a,b,c,d) \
    (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))


using android::sp;
using android::ISurface;
using android::VideoRenderer;

VideoRenderer *createRenderer(
        const android::sp<android::ISurface> &surface,
        const char *componentName,
        OMX_COLOR_FORMATTYPE colorFormat,
        size_t displayWidth, size_t displayHeight,
        size_t decodedWidth, size_t decodedHeight)
{
	//LOGD("componentName %s, colorFormat %d\n",componentName,colorFormat);
	if (colorFormat == OMX_COLOR_FormatYUV420Planar
        && !strncmp(componentName, "OMX.Freescale.std.video_decoder.",32)) {
        return new android::FSLRenderer(colorFormat,surface, 
        				displayWidth, displayHeight,
                decodedWidth, decodedHeight);
    }
	return NULL;
}

namespace android {

FSLRenderer::FSLRenderer(
        OMX_COLOR_FORMATTYPE colorFormat,
        const sp<ISurface> &surface,
        size_t displayWidth, size_t displayHeight,
        size_t decodedWidth, size_t decodedHeight)
    : mColorFormat(colorFormat),
      mISurface(surface),
      mDisplayWidth(displayWidth),
      mDisplayHeight(displayHeight),
      mDecodedWidth(decodedWidth),
      mDecodedHeight(decodedHeight)
 {  
      
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    int mode, ret;  
	ipu_lib_input_param_t sInParam;
	ipu_lib_output_param_t sOutParam;

    /* set ipu task input parameter */
    memset(&sInParam, 0, sizeof(ipu_lib_input_param_t));
    sInParam.width = mDecodedWidth;
    sInParam.height = mDecodedHeight;
    sInParam.input_crop_win.pos.x = 0;
    sInParam.input_crop_win.pos.y = 0;
    sInParam.input_crop_win.win_w = (mDecodedWidth + 7)/8*8;
    sInParam.input_crop_win.win_h = mDecodedHeight;
    sInParam.fmt = ipu_fourcc('I', '4', '2', '0');
    //sInParam.user_def_paddr[0] = 0;

    LOGV("sInParam width %d, height %d,crop x %d, y %d, w %d, h %d, color %d\n",
            sInParam.width,sInParam.height ,
            sInParam.input_crop_win.pos.x,sInParam.input_crop_win.pos.y,
            sInParam.input_crop_win.win_w,sInParam.input_crop_win.win_h,
            sInParam.fmt
          );  

    /* set ipu task output parameter */
    memset(&sOutParam, 0, sizeof(ipu_lib_output_param_t));
	if( mISurface != 0 ){
		int left,right,top,bottom,rot;
		status_t err = mISurface->getDestRect(&left, &right, &top, &bottom, &rot);
		if(0 == err) {
			LOGV("mISurface: left %d, right %d, top %d, bottom %d, rot %d\n",
				left, right, top, bottom, rot);
			sOutParam.width = right-left;
			sOutParam.height = bottom-top;
	 		sOutParam.fb_disp.pos.x = left;
			sOutParam.fb_disp.pos.y = top;
			sOutParam.rot = rot;
		}
	}else{
		sOutParam.width = mDisplayWidth;
		sOutParam.height = mDisplayHeight;
		sOutParam.fb_disp.pos.x = 0;
		sOutParam.fb_disp.pos.y = 0;
    }
    sOutParam.fmt = ipu_fourcc('U', 'Y', 'V', 'Y');
    sOutParam.show_to_fb = true;
	
    if(1)// lcd mode
        sOutParam.fb_disp.fb_num = 2;
    else // tv mode
        sOutParam.fb_disp.fb_num = 1;

    LOGV("sOutParam width %d, height %d,crop x %d, y %d, rot: %d, color %d\n",
            sOutParam.width, sOutParam.height ,
            sOutParam.fb_disp.pos.x, sOutParam.fb_disp.pos.y,
            sOutParam.rot, sOutParam.fmt
          );  

    /* ipu lib task init */
    mode = OP_NORMAL_MODE | TASK_PP_MODE;
    memset(&ipu_handle, 0, sizeof(ipu_lib_handle_t));
    ret = mxc_ipu_lib_task_init(&sInParam, NULL, &sOutParam, mode, &ipu_handle);
    if(ret < 0) {
        LOGE("mxc_ipu_lib_task_init failed!\n");
    }
}
   

FSLRenderer::~FSLRenderer() {
    mxc_ipu_lib_task_uninit(&ipu_handle);
}

void FSLRenderer::render(
        const void *data, size_t size, void *platformPrivate) {
        
	int err;
	OMX_PTR pVirtualAddr,pPhyiscAddr;
	//pVirtualAddr = data;
	GetHwBuffer( (void *)data, &pPhyiscAddr);
	if(pPhyiscAddr == NULL){
		LOGE("Can not find buffer %p\n",data);
		return;
	}
	
    /* call ipu task */
    err = mxc_ipu_lib_task_buf_update(&ipu_handle, (int)pPhyiscAddr, NULL, NULL, NULL, NULL);
  	CHECK_EQ(err, 0);
}

}  // namespace android




