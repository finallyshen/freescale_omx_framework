/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>

#include "IpulibRender.h"

#define ipu_fourcc(a,b,c,d) \
    (((__u32)(a)<<0)|((__u32)(b)<<8)|((__u32)(c)<<16)|((__u32)(d)<<24))

#define DISP_WIDTH 1024
#define DISP_HEIGHT 768

static struct sigaction default_act = {{0}, 0, 0, 0};
static void segfault_signal_handler(int signum);

static void RegistSignalHandler()
{
    struct sigaction act;

    act.sa_handler = segfault_signal_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
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
#ifdef ANDROID_BUILD
    OMX_STRING device = (OMX_STRING)"/dev/graphics/fb2";
#else
    OMX_STRING device = (OMX_STRING)"/dev/fb2";
#endif

    fd_fb = open(device, O_RDWR, 0);
    if(fd_fb > 0) {
        int blank = 1;
        ioctl(fd_fb, FBIOBLANK, blank);
        close(fd_fb);
    }

    default_act.sa_handler(SIGSEGV);

    return;
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
    fsl_osal_memset(&ipu_handle, 0, sizeof(ipu_lib_handle_t));

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

    UnRegistSignalHandler();

    mxc_ipu_lib_task_uninit(&ipu_handle);
    bInitDev = OMX_FALSE;
    fsl_osal_memset(&ipu_handle, 0, sizeof(ipu_lib_handle_t));

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

    ret = IpulibInit(pFrame);
    if(ret != OMX_ErrorNone)
        return ret;

    bInitDev = OMX_TRUE;

    if(pFrame == NULL)
        fsl_osal_memcpy(ipu_handle.inbuf_start[0], pShowFrame, ipu_handle.ifr_size);

    ret = IpulibRenderFrame(pFrame);
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
        ret = IpulibInit(pFrame);
        if(ret != OMX_ErrorNone) {
            ports[0]->SendBuffer(pBufferHdr);
            fsl_osal_mutex_unlock(lock);
            return ret;
        }
        bInitDev = OMX_TRUE;
    }

    if(pFrame == NULL)
        fsl_osal_memcpy(ipu_handle.inbuf_start[0], pBufferHdr->pBuffer, ipu_handle.ifr_size);

    ret = IpulibRenderFrame(pFrame);
    if(ret != OMX_ErrorNone) {
        ports[0]->SendBuffer(pBufferHdr);
        fsl_osal_mutex_unlock(lock);
        return ret;
    }

    ports[0]->SendBuffer(pBufferHdr);

    fsl_osal_mutex_unlock(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::IpulibInit(OMX_PTR pFrame)
{
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    int mode, ret;

    /* set ipu task in parameter */
    fsl_osal_memset(&sInParam, 0, sizeof(ipu_lib_input_param_t));
    sInParam.width = sVideoFmt.nFrameWidth;
    sInParam.height = sVideoFmt.nFrameHeight;
    sInParam.input_crop_win.pos.x = (sRectIn.nLeft + 7)/8*8;
    sInParam.input_crop_win.pos.y = sRectIn.nTop;
    sInParam.input_crop_win.win_w = sRectIn.nWidth/8*8;
    sInParam.input_crop_win.win_h = sRectIn.nHeight;
    if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV420Planar) {
        sInParam.fmt = ipu_fourcc('I', '4', '2', '0');
        LOG_DEBUG("Set IPU in format to YUVI420.\n");
    }
    else if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
        sInParam.fmt = ipu_fourcc('N', 'V', '1', '2');
        LOG_DEBUG("Set IPU in format to NV12.\n");
    }
    else if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV422Planar) {
        sInParam.fmt = ipu_fourcc('4', '2', '2','P');
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
    sInParam.user_def_paddr[0] = (int)pFrame;

    printf("IpulibRender sInParam width %d, height %d,crop x %d, y %d, w %d, h %d, color %d\n",
            sInParam.width,sInParam.height ,
            sInParam.input_crop_win.pos.x,sInParam.input_crop_win.pos.y,
            sInParam.input_crop_win.win_w,sInParam.input_crop_win.win_h,
            sInParam.fmt
          );  

    /* set ipu task out parameter */
    fsl_osal_memset(&sOutParam, 0, sizeof(ipu_lib_output_param_t));
    sOutParam.width = sRectOut.nWidth;
    sOutParam.height = sRectOut.nHeight;
    sOutParam.fmt = ipu_fourcc('U', 'Y', 'V', 'Y');
    sOutParam.rot = eRotation;

    sOutParam.show_to_fb = true;
    sOutParam.fb_disp.pos.x = sRectOut.nLeft;
    sOutParam.fb_disp.pos.y = sRectOut.nTop;
    if(sOutputMode.bTv != OMX_TRUE)
        sOutParam.fb_disp.fb_num = 2;
    else
        sOutParam.fb_disp.fb_num = 1;

    printf("IpulibRender sOutParam width %d, height %d,crop x %d, y %d, rot: %d, color %d\n",
            sOutParam.width, sOutParam.height ,
            sOutParam.fb_disp.pos.x, sOutParam.fb_disp.pos.y,
            sOutParam.rot, sOutParam.fmt
          );  

    /* ipu lib task init */
    mode = OP_NORMAL_MODE | TASK_PP_MODE;
    fsl_osal_memset(&ipu_handle, 0, sizeof(ipu_lib_handle_t));

    ret = mxc_ipu_lib_task_init(&sInParam, NULL, &sOutParam, mode, &ipu_handle);
    if(ret < 0) {
        LOG_ERROR("mxc_ipu_lib_task_init failed!\n");
        return OMX_ErrorHardware;
    }

    RegistSignalHandler();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE IpulibRender::IpulibRenderFrame(OMX_PTR pFrame)
{
    int ret;

    /* call ipu task */
    ret = mxc_ipu_lib_task_buf_update(&ipu_handle, (int)pFrame, NULL, NULL, NULL, NULL);
    if(ret < 0) {
        LOG_ERROR("mxc_ipu_lib_task_buf_update failed, ret: %d\n", ret);
        return OMX_ErrorHardware;
    }

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
