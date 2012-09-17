/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/mxcfb.h>
#include <unistd.h>
#include "FBRender.h"

#define DISP_WIDTH 1024
#define DISP_HEIGHT 768
#define BUFFER_NEW_RETRY_MAX 1000

FBRender::FBRender()
{
    OMX_U32 i; 

    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_render.fb.sw-based");
    ComponentVersion.s.nVersionMajor = 0x0;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x0;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = "video_render.fb";

    nFrameBufferMin = 1;
    nFrameBufferActual = 1;
    nFrameBuffer = nFrameBufferActual;
    TunneledSupplierType = OMX_BufferSupplyOutput;
    fsl_osal_memset(&sVideoFmt, 0, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
    sVideoFmt.nFrameWidth = 1024;
    sVideoFmt.nFrameHeight = 768;
    sVideoFmt.eColorFormat = OMX_COLOR_Format16bitRGB565;
    sVideoFmt.eCompressionFormat = OMX_VIDEO_CodingUnused;

    eRotation = ROTATE_NONE;
    OMX_INIT_STRUCT(&sRectIn, OMX_CONFIG_RECTTYPE);
    sRectIn.nWidth = sVideoFmt.nFrameWidth;
    sRectIn.nHeight = sVideoFmt.nFrameHeight;
    sRectIn.nTop = 0;
    sRectIn.nLeft = 0;
    OMX_INIT_STRUCT(&sRectOut, OMX_CONFIG_RECTTYPE);
    sRectOut.nWidth = sVideoFmt.nFrameWidth;
    sRectOut.nHeight = sVideoFmt.nFrameHeight;
    sRectOut.nTop = 0;
    sRectOut.nLeft = 0;


    device = 0;
    bStreamOn = OMX_FALSE;
    bFrameBufferInit = OMX_FALSE;
    nAllocating = 0;
    nQueuedBuffer = 0;
    nRenderFrames = 0;
    lock = NULL;

    OMX_INIT_STRUCT(&sCapture, OMX_CONFIG_CAPTUREFRAME);
    sCapture.eType = CAP_NONE;
    EnqueueBufferIdx = 0;

    OMX_INIT_STRUCT(&sOutputMode, OMX_CONFIG_OUTPUTMODE);
    sOutputMode.bTv = OMX_FALSE;
    sOutputMode.eFbLayer = LAYER_NONE;
    sOutputMode.eTvMode = MODE_NONE;
    bInTransitState = OMX_FALSE;
}

OMX_ERRORTYPE FBRender::InitRenderComponent()
{
    if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal)) {
        LOG_ERROR("Create mutex for v4l device failed.\n");
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FBRender::DeInitRenderComponent()
{
    if(lock)
        fsl_osal_mutex_destroy(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FBRender::RenderGetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexConfigCommonOutputCrop:
            {
                OMX_CONFIG_RECTTYPE *pRect;
                pRect = (OMX_CONFIG_RECTTYPE*)pStructure;
                CHECK_STRUCT(pRect, OMX_CONFIG_RECTTYPE, ret);
                fsl_osal_memcpy(pRect, &sRectOut, sizeof(OMX_CONFIG_RECTTYPE));
            }
            break;
        case OMX_IndexOutputMode:
            {
                OMX_CONFIG_OUTPUTMODE *pOutputMode;
                pOutputMode = (OMX_CONFIG_OUTPUTMODE*)pStructure;
                CHECK_STRUCT(pOutputMode, OMX_CONFIG_OUTPUTMODE, ret);
                sOutputMode.bSetupDone = bStreamOn;
                fsl_osal_memcpy(pOutputMode, &sOutputMode, sizeof(OMX_CONFIG_OUTPUTMODE));
            }
            break;
        case OMX_IndexConfigCaptureFrame:
            {
                OMX_CONFIG_CAPTUREFRAME *pCapture;
                pCapture = (OMX_CONFIG_CAPTUREFRAME*)pStructure;
                CHECK_STRUCT(pCapture, OMX_CONFIG_CAPTUREFRAME, ret);
                pCapture->bDone = bCaptureFrameDone;
            }
            break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE FBRender::RenderSetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexConfigCommonOutputCrop:
            {
                OMX_CONFIG_RECTTYPE *pRect;
                pRect = (OMX_CONFIG_RECTTYPE*)pStructure;
                CHECK_STRUCT(pRect, OMX_CONFIG_RECTTYPE, ret);
                fsl_osal_memcpy(&sRectOut, pRect, sizeof(OMX_CONFIG_RECTTYPE));
                SetDeviceDisplayRegion();
            }
            break;
        case OMX_IndexConfigCaptureFrame:
            {
                OMX_CONFIG_CAPTUREFRAME *pCapture;
                pCapture = (OMX_CONFIG_CAPTUREFRAME*)pStructure;
                CHECK_STRUCT(pCapture, OMX_CONFIG_CAPTUREFRAME, ret);
                sCapture.eType = pCapture->eType;
                sCapture.pBuffer = pCapture->pBuffer;
                bCaptureFrameDone = OMX_FALSE;
                if (sCapture.eType == CAP_SNAPSHOT)
                {
                    //CaptureFrame(EnqueueBufferIdx);
                    bCaptureFrameDone = OMX_TRUE;
                }
            }
            break;

        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE FBRender::PortFormatChanged(OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BOOL bFmtChanged = OMX_FALSE;

    if(nPortIndex != IN_PORT)
        return OMX_ErrorBadPortIndex;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    ports[IN_PORT]->GetPortDefinition(&sPortDef);

    if(sVideoFmt.nFrameWidth != sPortDef.format.video.nFrameWidth) {
        sVideoFmt.nFrameWidth = sPortDef.format.video.nFrameWidth;
        bFmtChanged = OMX_TRUE;
    }
    if(sVideoFmt.nFrameHeight != sPortDef.format.video.nFrameHeight) {
        sVideoFmt.nFrameHeight = sPortDef.format.video.nFrameHeight;
        bFmtChanged = OMX_TRUE;
    }
    if(sVideoFmt.eColorFormat != sPortDef.format.video.eColorFormat) {
        sVideoFmt.eColorFormat = sPortDef.format.video.eColorFormat;
        bFmtChanged = OMX_TRUE;
    }
    if(nFrameBuffer != sPortDef.nBufferCountActual) {
        nFrameBuffer = sPortDef.nBufferCountActual;
    }

    if(bFmtChanged && device > 0) {
        CloseDevice();
        OpenDevice();
    }

    return ret;
}
#if 0
OMX_ERRORTYPE FBRender::DoAllocateBuffer(
        OMX_PTR *buffer, 
        OMX_U32 nSize)
{
    OMX_U32 i = 0;

    /* wait until v4l device is opened */
    while(bFrameBufferInit != OMX_TRUE) {
        usleep(10000);
        i++;
        if(i >= 10)
            return OMX_ErrorHardware;
    }

    if(nAllocating >= nFrameBuffer) {
        LOG_ERROR("Requested buffer number beyond system can provide: [%:%]\n", nAllocating, nFrameBuffer);
        return OMX_ErrorInsufficientResources;
    }

    *buffer = fb_ptr;
    nAllocating ++;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FBRender::DoFreeBuffer(
        OMX_PTR buffer)
{
    nAllocating --;
    return OMX_ErrorNone;
}
#endif

OMX_ERRORTYPE FBRender::OpenDevice()
{
    int  retval = 0;
    OMX_STRING device_name = "/dev/fb0";
    struct fb_var_screeninfo screen_info;
    struct fb_fix_screeninfo fscreeninfo;
    struct mxcfb_gbl_alpha gbl_alpha;
    if(device > 0)
        return OMX_ErrorNone;

    device = open(device_name, O_RDWR, 0);
    if(device < 0) {
        LOG_ERROR("Open device: %d failed.\n", device_name);
        return OMX_ErrorHardware;
    }

    retval = ioctl(device, FBIOBLANK, FB_BLANK_UNBLANK);
    if (retval < 0)
        goto err1;
    
    gbl_alpha.alpha = 0xFF;
    retval = ioctl(device, MXCFB_SET_GBL_ALPHA, &gbl_alpha);
    retval = ioctl(device, FBIOGET_VSCREENINFO, &screen_info);
    if (retval < 0)
        goto err2;
    FB_INFO("Set %s to 16-bpp\n",device_name);
    screen_info.bits_per_pixel = 16;
    screen_info.yoffset = 0;
    retval = ioctl(device, FBIOPUT_VSCREENINFO, &screen_info);
    if (retval < 0)
        goto err2;
    fb_size = screen_info.xres * screen_info.yres_virtual * screen_info.bits_per_pixel / 8;
    lcd_w = screen_info.xres;
    lcd_h = screen_info.yres;
    FB_INFO("LCD: x_res: %d, y_res: %d\n", screen_info.xres, screen_info.yres);

    retval = ioctl(device, FBIOGET_FSCREENINFO, &fscreeninfo);
    if (retval < 0)
        goto err2;

    phy_addr = fscreeninfo.smem_start;
	
    fb_ptr = (unsigned short *)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, device, 0);
    if ((int)fb_ptr <= 0)
    {
        FB_ERROR("\nError: failed to map framebuffer device 0 to memory.\n");
        goto err2;
    }
	
    return OMX_ErrorNone;
err2:
    munmap(fb_ptr, fb_size);
err1:
    close(device);
    return OMX_ErrorHardware;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FBRender::CloseDevice()
{
    if(device > 0){
	   fsl_osal_memset(fb_ptr,0,fb_size);
    	   munmap(fb_ptr,fb_size);
    	   close(device);
    	   device = 0;   	
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE FBRender::SetDeviceRotation()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(device <= 0)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);
		//Add code here
    fsl_osal_mutex_unlock(lock);

    return ret;
}

OMX_ERRORTYPE FBRender::SetDeviceInputCrop()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(device <= 0)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);

    fsl_osal_mutex_unlock(lock);

    return ret;
}

OMX_ERRORTYPE FBRender::SetDeviceDisplayRegion()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(device <= 0)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);

    fsl_osal_mutex_unlock(lock);

    return ret;
}


OMX_ERRORTYPE FBRender::WriteDevice(
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nIndex;

    fsl_osal_mutex_lock(lock);

    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;
    OMX_S32 i;
    OMX_U8 *dst, *src;
    src = pBufferHdr->pBuffer;
    dst = (OMX_U8 *)fb_ptr;
    nRenderFrames ++;
    FB_DEBUG("RenderFrame %p\n",pBufferHdr->pBuffer);

    for(i=0; i<sRectOut.nHeight; i++) {
       fsl_osal_memcpy(dst, src, sRectOut.nWidth*2);
        dst += lcd_w*2;
        src += sRectOut.nWidth*2;
    }
	

    fsl_osal_sleep(10000);
    ports[0]->SendBuffer(pBufferHdr);
    fsl_osal_mutex_unlock(lock);
    return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE FBRenderInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        FBRender *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(FBRender, ());
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
