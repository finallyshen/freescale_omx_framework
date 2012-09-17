/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/mxcfb.h>

#include "IpulibRender_mx6x.h"

#define ipu_fourcc(a,b,c,d) \
    (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))

#define PAGE_SHIFT      12
#ifndef PAGE_SIZE
#define PAGE_SIZE       (1 << PAGE_SHIFT)
#endif

#define DISP_WIDTH 1024
#define DISP_HEIGHT 768

static struct sigaction default_act;
static void segfault_signal_handler(int signum);

static void RegistSignalHandler()
{
    struct sigaction act;

    act.sa_handler = segfault_signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    fsl_osal_memset(&default_act, 0, sizeof(struct sigaction));
    sigaction(SIGSEGV, &act, &default_act);

    return;
}

static void UnRegistSignalHandler()
{
    struct sigaction act;

    sigaction(SIGSEGV, &default_act, NULL);

    return;
}

static void segfault_signal_handler(int signum)
{
    OMX_S32 fd_fb = 0;
    OMX_STRING device = (OMX_STRING)"/dev/graphics/fb1";

    fd_fb = open(device, O_RDWR, 0);
    if(fd_fb > 0) {
        int blank = 1;
        ioctl(fd_fb, FBIOBLANK, blank);
        close(fd_fb);
    }

    default_act.sa_handler(SIGSEGV);

    return;
}

static OMX_ERRORTYPE GetFBResolution(
        OMX_U32  number,
        OMX_U32 *width,
        OMX_U32 *height)
{
    OMX_S32 fd_fb;
    struct fb_var_screeninfo fb_var;
    char deviceName[20] ;

#ifdef ANDROID_BUILD
    sprintf(deviceName, "/dev/graphics/fb%d", (int)number);
#else
    sprintf(deviceName, "/dev/fb%d", number);
#endif

    LOG_DEBUG("to open %s\n", deviceName);

    if ((fd_fb = open(deviceName, O_RDWR, 0)) < 0) {
        LOG_ERROR("Unable to open %s\n", deviceName);
         return OMX_ErrorHardware;
     }

    if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
        LOG_ERROR("Get FB var info failed!\n");
        close(fd_fb);
        return OMX_ErrorHardware;
    }

    close(fd_fb);

    *width = fb_var.xres;
    *height = fb_var.yres;

    LOG_DEBUG("fb%d: x=%d, y=%d\n", number, *width, *height);

    return OMX_ErrorHardware;
}

static OMX_S32 roundUpToPageSize(size_t x) {
            return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}

IpulibRender::IpulibRender()
{
    OMX_U32 i; 

    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_render.ipulib.sw-based");
    ComponentVersion.s.nVersionMajor = 0x0;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x0;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"video_render.ipulib";

    nFrameBufferMin = 2;
    nFrameBufferActual = 2;
    TunneledSupplierType = OMX_BufferSupplyInput;
    fsl_osal_memset(&sVideoFmt, 0, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
    sVideoFmt.nFrameWidth = 320;
    sVideoFmt.nFrameHeight = 240;
    sVideoFmt.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    sVideoFmt.eCompressionFormat = OMX_VIDEO_CodingUnused;

    eRotation = ROTATE_NONE;
    OMX_INIT_STRUCT(&sRectIn, OMX_CONFIG_RECTTYPE);
    sRectIn.nWidth = sVideoFmt.nFrameWidth;
    sRectIn.nHeight = sVideoFmt.nFrameHeight;
    sRectIn.nTop = 0;
    sRectIn.nLeft = 0;
    OMX_INIT_STRUCT(&sRectOut, OMX_CONFIG_RECTTYPE);
    sRectOut.nWidth = DISP_WIDTH;
    sRectOut.nHeight = DISP_HEIGHT;
    sRectOut.nTop = 0;
    sRectOut.nLeft = 0;

    bInitDev = OMX_FALSE;
    lock = NULL;
    pShowFrame = NULL;
    nFrameLen = 0;
    bSuspend = OMX_FALSE;

    OMX_INIT_STRUCT(&sCapture, OMX_CONFIG_CAPTUREFRAME);
    sCapture.eType = CAP_NONE;
}

OMX_ERRORTYPE IpulibRender::InitRenderComponent()
{
    if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal)) {
        LOG_ERROR("Create mutex for v4l device failed.\n");
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::DeInitRenderComponent()
{
    CloseDevice();

    if(lock)
        fsl_osal_mutex_destroy(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::RenderGetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexOutputMode:
            {
                OMX_CONFIG_OUTPUTMODE *pOutputMode;
                pOutputMode = (OMX_CONFIG_OUTPUTMODE*)pStructure;
                CHECK_STRUCT(pOutputMode, OMX_CONFIG_OUTPUTMODE, ret);
                sOutputMode.bSetupDone = OMX_TRUE;
                fsl_osal_memcpy(pOutputMode, &sOutputMode, sizeof(OMX_CONFIG_OUTPUTMODE));
            }
            break;
        case OMX_IndexConfigCaptureFrame:
            {
                OMX_CONFIG_CAPTUREFRAME *pCapture;
                pCapture = (OMX_CONFIG_CAPTUREFRAME*)pStructure;
                CHECK_STRUCT(pCapture, OMX_CONFIG_CAPTUREFRAME, ret);
                pCapture->bDone = bCaptureFrameDone;
                if(sCapture.nFilledLen == 0)
                    ret = OMX_ErrorUndefined;
            }
            break;
        case OMX_IndexSysSleep:
            {
                OMX_CONFIG_SYSSLEEP *pSysSleep = (OMX_CONFIG_SYSSLEEP *)pStructure;
                CHECK_STRUCT(pSysSleep, OMX_CONFIG_SYSSLEEP, ret);
                pSysSleep->bSleep = bSuspend;
            }
            break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE IpulibRender::RenderSetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexOutputMode:
            {
                OMX_CONFIG_OUTPUTMODE *pOutputMode;
                pOutputMode = (OMX_CONFIG_OUTPUTMODE*)pStructure;
                CHECK_STRUCT(pOutputMode, OMX_CONFIG_OUTPUTMODE, ret);
                fsl_osal_memcpy(&sOutputMode, pOutputMode, sizeof(OMX_CONFIG_OUTPUTMODE));
                fsl_osal_memcpy(&sRectIn, &pOutputMode->sRectIn, sizeof(OMX_CONFIG_RECTTYPE));
                fsl_osal_memcpy(&sRectOut, &pOutputMode->sRectOut, sizeof(OMX_CONFIG_RECTTYPE));
                eRotation = pOutputMode->eRotation;
                ResetDevice();
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
                if (sCapture.eType == CAP_SNAPSHOT) {
                    if(pShowFrame == NULL) {
                        ret = OMX_ErrorUndefined;
                        break;
                    }
                    fsl_osal_memcpy(sCapture.pBuffer, pShowFrame, nFrameLen);
                    sCapture.nFilledLen = nFrameLen;
                    bCaptureFrameDone = OMX_TRUE;
                }
            }
            break;
        case OMX_IndexSysSleep:
            OMX_CONFIG_SYSSLEEP *pSysSleep;
            pSysSleep = (OMX_CONFIG_SYSSLEEP *)pStructure;
            bSuspend = pSysSleep->bSleep;
            fsl_osal_mutex_lock(lock);
            if(OMX_TRUE == bSuspend) {
                CloseDevice();
            }
            else {
                OMX_STATETYPE eState = OMX_StateInvalid;
                GetState(&eState);
                if(eState == OMX_StatePause)
                    StartDeviceInPause();
            }
            fsl_osal_mutex_unlock(lock);
            break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE IpulibRender::PortFormatChanged(OMX_U32 nPortIndex)
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

    if(bFmtChanged == OMX_TRUE)
        ResetDevice();

    return ret;
}

OMX_ERRORTYPE IpulibRender::OpenDevice()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::ResetDevice()
{
    fsl_osal_mutex_lock(lock);


    CloseDevice();

    OMX_STATETYPE eState = OMX_StateInvalid;
    GetState(&eState);
    if(eState == OMX_StatePause && bSuspend != OMX_TRUE)
        StartDeviceInPause();

    fsl_osal_mutex_unlock(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::CloseDevice()
{

    if(bInitDev != OMX_TRUE)
        return OMX_ErrorNone;

    DeInit();
    bInitDev = OMX_FALSE;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::StartDeviceInPause()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(bInitDev != OMX_FALSE)
        return OMX_ErrorNone;

    if(pShowFrame == NULL)
        return OMX_ErrorBadParameter;

    OMX_PTR pFrame = NULL;
    GetHwBuffer(pShowFrame, &pFrame);

    ret = Init();
    if(ret != OMX_ErrorNone)
        return ret;

    bInitDev = OMX_TRUE;

    ret = RenderFrame(pFrame);
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::WriteDevice(
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    fsl_osal_mutex_lock(lock);

    if(sCapture.eType == CAP_THUMBNAL) {
        fsl_osal_memcpy(sCapture.pBuffer, pBufferHdr->pBuffer, pBufferHdr->nFilledLen);
        sCapture.nFilledLen = pBufferHdr->nFilledLen;
        bCaptureFrameDone = OMX_TRUE;
        ports[0]->SendBuffer(pBufferHdr);
        fsl_osal_mutex_unlock(lock);
        return OMX_ErrorNone;
    }

    if(pBufferHdr->nFilledLen == 0) {
        ports[0]->SendBuffer(pBufferHdr);
        fsl_osal_mutex_unlock(lock);
        return OMX_ErrorNone;
    }

    pShowFrame = pBufferHdr->pBuffer;
    nFrameLen = pBufferHdr->nFilledLen;

    if(bSuspend == OMX_TRUE) {
        ports[0]->SendBuffer(pBufferHdr);
        fsl_osal_mutex_unlock(lock);
        return OMX_ErrorNone;
    }

    OMX_PTR pFrame = NULL;
    GetHwBuffer(pBufferHdr->pBuffer, &pFrame);

    if(bInitDev != OMX_TRUE) {
        ret = Init();
        if(ret != OMX_ErrorNone) {
            ports[0]->SendBuffer(pBufferHdr);
            fsl_osal_mutex_unlock(lock);
            return ret;
        }
        bInitDev = OMX_TRUE;
    }

    ret = RenderFrame(pFrame);
    if(ret != OMX_ErrorNone) {
        ports[0]->SendBuffer(pBufferHdr);
        fsl_osal_mutex_unlock(lock);
        return ret;
    }

    ports[0]->SendBuffer(pBufferHdr);

    fsl_osal_mutex_unlock(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::Init()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = FBInit();
    if(ret != OMX_ErrorNone)
        return ret;

    ret = IpuInit();
    if(ret != OMX_ErrorNone)
        return ret;

    RegistSignalHandler();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::DeInit()
{
    FBDeInit();
    IpuDeInit();
    UnRegistSignalHandler();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::RenderFrame(OMX_PTR pFrame)
{
    IpuProcessBuffer(pFrame);
    FBShowFrame();

    return OMX_ErrorNone;
}


OMX_ERRORTYPE IpulibRender::IpuInit()
{
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    mIpuFd = open("/dev/mxc_ipu", O_RDWR, 0);
    if(mIpuFd < 0) {
        LOG_ERROR("Open /dev/mxc_ipu failed.\n");
        return OMX_ErrorHardware;
    }

    fsl_osal_memset(&mTask, 0, sizeof(struct ipu_task));

    /* set ipu task in parameter */
    mTask.input.width = sVideoFmt.nFrameWidth;
    mTask.input.height = sVideoFmt.nFrameHeight;
    mTask.input.crop.pos.x = (sRectIn.nLeft + 7)/8*8;
    mTask.input.crop.pos.y = sRectIn.nTop;
    mTask.input.crop.w = sRectIn.nWidth/8*8;
    mTask.input.crop.h = sRectIn.nHeight;


    if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV420Planar) {
        mTask.input.format = ipu_fourcc('I', '4', '2', '0');
        LOG_DEBUG("Set IPU in format to YUVI420.\n");
    }
    else if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
        mTask.input.format = ipu_fourcc('N', 'V', '1', '2');
        LOG_DEBUG("Set IPU in format to NV12.\n");
    }
    else if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV422Planar) {
        mTask.input.format = ipu_fourcc('4', '2', '2','P');
        LOG_DEBUG("Set IPU in format to YUV422P.\n");
    }
    /* FIXME: 422 interleave is not supported by IPU	
    else if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV422SemiPlanar) {
        sInParam.fmt = ipu_fourcc('N', 'V', '1', '6');
        LOG_DEBUG("Set IPU in format to NV16.\n");
    }*/
    else{
        LOG_ERROR("Unsupported IPU color format : %d \n",sVideoFmt.eColorFormat);
        return OMX_ErrorHardware;
    }

    printf("IpulibRender sInParam width %d, height %d,crop x %d, y %d, w %d, h %d, color %d\n",
            mTask.input.width, mTask.input.height,
            mTask.input.crop.pos.x, mTask.input.crop.pos.y,
            mTask.input.crop.w, mTask.input.crop.h,
            mTask.input.format
          );  

    /* set ipu task out parameter */
    mTask.output.width = nFBWidth;
    mTask.output.height = nFBHeight;
    mTask.output.crop.pos.x = sRectOut.nLeft;
    mTask.output.crop.pos.y = sRectOut.nTop;
    mTask.output.crop.w = sRectOut.nWidth;
    mTask.output.crop.h = sRectOut.nHeight;

    mTask.output.format = ipu_fourcc('U', 'Y', 'V', 'Y');
    mTask.output.rotate = eRotation;

    printf("IpulibRender sOutParam width %d, height %d,crop x %d, y %d, rot: %d, color %d\n",
            mTask.output.crop.w, mTask.output.crop.h ,
            mTask.output.crop.pos.x, mTask.output.crop.pos.y,
            mTask.output.rotate, mTask.output.format
          );  

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::IpuDeInit()
{
    if(mIpuFd)
        close(mIpuFd);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::IpuProcessBuffer(OMX_PTR pFrame)
{
    mTask.input.paddr = (int)pFrame;
    mTask.output.paddr = (int)pFbPAddr[nNextFb];

    LOG_DEBUG("Ipu process buffer, in: %p, out: %p, fb idx: %d\n", 
            mTask.input.paddr, mTask.output.paddr, nNextFb);

    int ret = IPU_CHECK_ERR_INPUT_CROP;
    while(ret != IPU_CHECK_OK && ret > IPU_CHECK_ERR_MIN) {
        ret = ioctl(mIpuFd, IPU_CHECK_TASK, &mTask);
        switch(ret) {
            case IPU_CHECK_OK:
                break;
            case IPU_CHECK_ERR_SPLIT_INPUTW_OVER:
                mTask.input.crop.w -= 8;
                break;
            case IPU_CHECK_ERR_SPLIT_INPUTH_OVER:
                mTask.input.crop.h -= 8;
                break;
            case IPU_CHECK_ERR_SPLIT_OUTPUTW_OVER:
                mTask.output.crop.w -= 8;
                break;
            case IPU_CHECK_ERR_SPLIT_OUTPUTH_OVER:
                mTask.output.crop.h -= 8;;
                break;
            default:
                return OMX_ErrorUndefined;
        }
    }

    ret = ioctl(mIpuFd, IPU_QUEUE_TASK, &mTask);
    if(ret < 0) {
        LOG_ERROR("IpuProcessBuffer %p failed.\n", pFrame);
        return OMX_ErrorHardware;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::FBInit()
{
    int  retval = 0;
#ifdef ANDROID_BUILD
    OMX_STRING device_name = (OMX_STRING)"/dev/graphics/fb1";
#else
    OMX_STRING device_name = (OMX_STRING)"/dev/fb1";
#endif
    struct fb_var_screeninfo screen_info;
    struct fb_fix_screeninfo fscreeninfo;

    GetFBResolution(0, &nFBWidth, &nFBHeight);
    
    mFb = open(device_name, O_RDWR | O_NONBLOCK, 0);
    if(mFb < 0) {
        LOG_ERROR("Open device: %d failed.\n", device_name);
        return OMX_ErrorHardware;
    }

    retval = ioctl(mFb, FBIOGET_VSCREENINFO, &screen_info);
    if (retval < 0) {
        close(mFb);
        return OMX_ErrorHardware;
    }

    screen_info.xoffset = 0;
    screen_info.yoffset = 0;
    screen_info.bits_per_pixel = 16;
    screen_info.nonstd = V4L2_PIX_FMT_UYVY;
    screen_info.activate = FB_ACTIVATE_NOW;
    screen_info.xres = nFBWidth;
    screen_info.yres = nFBHeight;
    screen_info.yres_virtual = screen_info.yres * MAX_FB_BUFFERS;
    screen_info.xres_virtual = screen_info.xres;
    retval = ioctl(mFb, FBIOPUT_VSCREENINFO, &screen_info);
    if (retval < 0) {
        close(mFb);
        return OMX_ErrorHardware;
    }

    retval = ioctl(mFb, FBIOGET_FSCREENINFO, &fscreeninfo);
    if (retval < 0) {
        close(mFb);
        return OMX_ErrorHardware;
    }

    pFbPAddrBase = (OMX_PTR)fscreeninfo.smem_start;
    nFbSize = roundUpToPageSize(fscreeninfo.line_length * screen_info.yres_virtual);
    pFbVAddrBase = (unsigned short *)mmap(0, nFbSize, PROT_READ | PROT_WRITE, MAP_SHARED, mFb, 0);
    if ((int)pFbVAddrBase <= 0) {
        LOG_ERROR("Failed to map framebuffer2.\n");
        munmap(pFbVAddrBase, nFbSize);
        close(mFb);
        return OMX_ErrorHardware;
    }

    OMX_S32 i=0;
    OMX_S16 *pBuffer = (OMX_S16*)pFbVAddrBase;
    for(i=0; i<nFbSize/2; i++, pBuffer++)
        *pBuffer = 0x80;

    retval = ioctl(mFb, FBIOBLANK, FB_BLANK_UNBLANK);
    if (retval < 0) {
        munmap(pFbVAddrBase, nFbSize);
        close(mFb);
        return OMX_ErrorHardware;
    }

    OMX_S32 nBufferSize = nFbSize / MAX_FB_BUFFERS;
    for(i=0; i<MAX_FB_BUFFERS; i++)
        pFbPAddr[i] = (OMX_U8 *)pFbPAddrBase + i * nBufferSize;
    nNextFb = 0;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::FBDeInit()
{
    if(mFb) {
        munmap(pFbVAddrBase, nFbSize);
        ioctl(mFb, FBIOBLANK, 1);
        close(mFb);
        mFb = 0;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::FBShowFrame()
{
    int  retval = 0;
    struct fb_var_screeninfo screen_info;
    struct fb_fix_screeninfo fscreeninfo;

    retval = ioctl(mFb, FBIOGET_VSCREENINFO, &screen_info);
    if (retval < 0)
        return OMX_ErrorUndefined;

    retval = ioctl(mFb, FBIOGET_FSCREENINFO, &fscreeninfo);
    if (retval < 0)
        return OMX_ErrorUndefined;

    screen_info.yoffset = nFBHeight * nNextFb;
    screen_info.activate = FB_ACTIVATE_VBL;
    ioctl(mFb, FBIOPAN_DISPLAY, &screen_info);
    nNextFb ++;
    if(nNextFb >= MAX_FB_BUFFERS)
        nNextFb = 0;

    return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
    extern "C" {
        OMX_ERRORTYPE IpulibRenderInit(OMX_IN OMX_HANDLETYPE pHandle)
        {
            OMX_ERRORTYPE ret = OMX_ErrorNone;
            IpulibRender *obj = NULL;
            ComponentBase *base = NULL;

            obj = FSL_NEW(IpulibRender, ());
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
