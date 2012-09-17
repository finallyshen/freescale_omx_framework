/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#ifdef ANDROID_BUILD
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/videodev.h>
#include <linux/mxc_v4l2.h>
#include "V4lRender.h"
#include <linux/version.h>
#include <string.h>

#define DISP_WIDTH 1024
#define DISP_HEIGHT 768
#define BUFFER_NEW_RETRY_MAX 1000
#define USING_DUMMY_WORKAROUND	/*workaround for v4l render */
#ifdef USING_DUMMY_WORKAROUND
/*  
 - fix stream-on limitation(on iMX5): avoid queue the same buffer twice
 - fix de-queue limitation: in flush operation, may return all buffers(except dummy buf) to decoder
    so, needn't stream-off when seeking
 - overhead: need one/two additional memcpy for dummy buffer when begin/seeking
*/
#define V4L_DUMMY_BUF_NUM 1
#define ASSERT(exp) {if(!(exp)) {LOG_ERROR("error condition: %s, %d \r\n",__FUNCTION__,__LINE__);}}
#else
#define V4L_DUMMY_BUF_NUM 0
#endif


//#define LOG_DEBUG printf

static OMX_BOOL v4l_new_version()
{
#if 0   // capability.version is not supported ???
    struct v4l2_capability v4l_cap;
    if (ioctl(NULL, VIDIOC_QUERYCAP, &v4l_cap) < 0) {
        LOG_ERROR("set output failed\n");
        return OMX_FALSE;
    }
    if(v4l_cap.version>=KERNEL_VERSION(2,6,38)){
        return OMX_TRUE;
    }
    else{
        return OMX_FALSE;
    }
#else  //FIMXE: find better way to identify the v4l version 
    OMX_S32 v4l_dev;
    v4l_dev=open("/dev/video17", O_RDWR | O_NONBLOCK, 0);
    if(v4l_dev>0){
        close(v4l_dev);
        return OMX_TRUE;
    }
    else{
        return OMX_FALSE;
    }
#endif	
}

static OMX_BOOL v4l_system(
        char *pFileName, 
        char *value)
{
    FILE *fp = NULL;
    fp = fopen(pFileName, "w");
    if(fp == NULL) {
        printf("open %s failed.\n", pFileName);
        return OMX_FALSE;
    }
    fwrite(value, 1, fsl_osal_strlen(value), fp);
    fflush(fp);
    fclose(fp);
    return OMX_TRUE;
}

static OMX_BOOL switch_to_lcd()
{
#if 0
    int tvOutFd = 0;
    R1GpioInfo tmp = {GPIO1_17, CONFIG_ALT3, 0};
    tvOutFd = open("/dev/r1Gpio", O_RDONLY|O_NONBLOCK);
    ioctl(tvOutFd, DEV_CTRL_GPIO_SET_LOW, &tmp);
    close(tvOutFd);
#endif

    if(v4l_system("/sys/class/graphics/fb0/blank", "1\n") != OMX_TRUE)
        return OMX_FALSE;
    if(v4l_system("/sys/class/graphics/fb1/blank", "1\n") != OMX_TRUE)
        return OMX_FALSE;
    if(v4l_system("/sys/class/graphics/fb2/blank", "1\n") != OMX_TRUE)
        return OMX_FALSE;
    if(v4l_system("/sys/class/graphics/fb1/fsl_disp_property", "1-layer-fb\n") != OMX_TRUE)
        return OMX_FALSE;
    if(v4l_system("/sys/class/graphics/fb0/blank", "0\n") != OMX_TRUE)
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL switch_to_tv(TV_MODE eMode)
{
#if 0
    int tvOutFd = 0;
    R1GpioInfo tmp = {GPIO1_17, CONFIG_ALT3, 0};
    tvOutFd = open("/dev/r1Gpio", O_RDONLY|O_NONBLOCK);
    ioctl(tvOutFd, DEV_CTRL_GPIO_SET_HIGH, &tmp);
    close(tvOutFd);
#endif

    if(v4l_system("/sys/class/graphics/fb0/blank", "1\n") != OMX_TRUE)
        return OMX_FALSE;
    if(v4l_system("/sys/class/graphics/fb1/blank", "1\n") != OMX_TRUE)
        return OMX_FALSE;
    if(v4l_system("/sys/class/graphics/fb2/blank", "1\n") != OMX_TRUE)
        return OMX_FALSE;
    if(v4l_system("/sys/class/graphics/fb0/fsl_disp_property", "1-layer-fb\n") != OMX_TRUE)
        return OMX_FALSE;

    if(eMode == MODE_PAL) {
        if(v4l_system("/sys/class/graphics/fb1/mode", "U:720x576i-50\n") != OMX_TRUE)
            return OMX_FALSE;
    }
    if(eMode == MODE_NTSC) {
        if(v4l_system("/sys/class/graphics/fb1/mode", "U:720x480i-60\n") != OMX_TRUE)
            return OMX_FALSE;
    }

    if(v4l_system("/sys/class/graphics/fb1/blank", "0\n") != OMX_TRUE)
        return OMX_FALSE;

    return OMX_TRUE;
}


V4lRender::V4lRender()
{
    OMX_U32 i; 

    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_render.v4l.sw-based");
    ComponentVersion.s.nVersionMajor = 0x0;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x0;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = "video_render.v4l";

    nFrameBufferMin = 1;
    nFrameBufferActual = 1;
    nFrameBuffer = nFrameBufferActual;
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

    for(i=0; i<MAX_PORT_BUFFER; i++)
        fsl_osal_memset(&fb_data[i], 0, sizeof(FB_DATA));
    for(i=0; i<MAX_V4L_BUFFER; i++) {
        fsl_osal_memset(&v4lbufs[i], 0, sizeof(struct v4l2_buffer));
        BufferHdrs[i] = NULL;
    }

    device = 0;
    bStreamOn = OMX_FALSE;
    bFrameBufferInit = OMX_FALSE;
    nAllocating = 0;
    nQueuedBuffer = 0;
    nRenderFrames = 0;
    lock = NULL;

    OMX_INIT_STRUCT(&sCapture, OMX_CONFIG_CAPTUREFRAME);
    sCapture.eType = CAP_NONE;
    EnqueueBufferIdx = -1;

    OMX_INIT_STRUCT(&sOutputMode, OMX_CONFIG_OUTPUTMODE);
    sOutputMode.bTv = OMX_FALSE;
    sOutputMode.eFbLayer = LAYER_NONE;
    sOutputMode.eTvMode = MODE_NONE;
    bInTransitState = OMX_FALSE;
    bV4lNewApi	=v4l_new_version();
#ifdef USING_DUMMY_WORKAROUND	
    bDummyValid=OMX_FALSE;
    bFlushState	=OMX_FALSE;//OMX_TRUE;
#endif    
}

OMX_ERRORTYPE V4lRender::InitRenderComponent()
{
    if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal)) {
        LOG_ERROR("Create mutex for v4l device failed.\n");
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::DeInitRenderComponent()
{
    if(lock)
        fsl_osal_mutex_destroy(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::RenderGetConfig(
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
        case OMX_IndexSysSleep:
            {
                OMX_CONFIG_SYSSLEEP *pStreamOff;
                pStreamOff = (OMX_CONFIG_SYSSLEEP *)pStructure;
                CHECK_STRUCT(pStreamOff, OMX_CONFIG_SYSSLEEP, ret);
                pStreamOff->bSleep = (OMX_BOOL)!bStreamOn;
            }
            break;

        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

static void AdjustCropIn(OMX_CONFIG_RECTTYPE *pInRect)
{
    /*FIXME:a bug of v4l driver, if originally zero crop, then couldn't set crop again for zoom case.*/
    if (pInRect->nTop == 0)
    {
        pInRect->nTop = 16;
        pInRect->nHeight -= 16*2;
    }
    if (pInRect->nLeft == 0)
    {
        pInRect->nLeft = 16;
        pInRect->nWidth -= 16*2;
    }
}


OMX_ERRORTYPE V4lRender::RenderSetConfig(
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
                    CaptureFrame(EnqueueBufferIdx);
                    bCaptureFrameDone = OMX_TRUE;
                }
            }
            break;
        case OMX_IndexOutputMode:
            {
                OMX_CONFIG_OUTPUTMODE *pOutputMode;
                pOutputMode = (OMX_CONFIG_OUTPUTMODE*)pStructure;
                CHECK_STRUCT(pOutputMode, OMX_CONFIG_OUTPUTMODE, ret);
                fsl_osal_memcpy(&sOutputMode, pOutputMode, sizeof(OMX_CONFIG_OUTPUTMODE));
//                AdjustCropIn(&sOutputMode.sRectIn);
                SetOutputMode();
            }
            break;
        case OMX_IndexSysSleep:
            {
                OMX_CONFIG_SYSSLEEP *pStreamOff;
                pStreamOff = (OMX_CONFIG_SYSSLEEP *)pStructure;
                CHECK_STRUCT(pStreamOff, OMX_CONFIG_SYSSLEEP, ret);
                if (pStreamOff->bSleep)
                    V4lStreamOff(OMX_TRUE);
                else 
                    V4lStreamOnInPause();
            }
            break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE V4lRender::PortFormatChanged(OMX_U32 nPortIndex)
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
        nFrameBuffer = sPortDef.nBufferCountActual+V4L_DUMMY_BUF_NUM;
    }

    if(bFmtChanged && device > 0) {
        CloseDevice();
        OpenDevice();
    }

    return ret;
}

OMX_ERRORTYPE V4lRender::DoAllocateBuffer(
        OMX_PTR *buffer, 
        OMX_U32 nSize,
        OMX_U32 nPortIndex)
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

    *buffer = fb_data[nAllocating].pVirtualAddr;
    nAllocating ++;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::DoFreeBuffer(
        OMX_PTR buffer,
        OMX_U32 nPortIndex)
{
    nAllocating --;
    return OMX_ErrorNone;
}


OMX_ERRORTYPE V4lRender::OpenDevice()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_STRING device_name;
    if(bV4lNewApi){
        device_name="/dev/video17";   // for iMX6: /dev/video16(17/18/19/20)
    }
    else{
        device_name="/dev/video16";
    }
    LOG_DEBUG("v4l OpenDevice\n");

    if(device > 0)
        return OMX_ErrorNone;

    device = open(device_name, O_RDWR | O_NONBLOCK, 0);
    if(device < 0) {
        LOG_ERROR("Open device: %d failed.\n", device_name);
        return OMX_ErrorHardware;
    }

    ret = V4lSetOutputLayer(3);
    if(ret != OMX_ErrorNone)
        return ret;

    ret = V4lSetInputCrop();
    if(ret != OMX_ErrorNone)
        return ret;

    ret = V4lSetOutputCrop();
    if(ret != OMX_ErrorNone)
        return ret;

    ret = V4lSetRotation();
    if(ret != OMX_ErrorNone)
        return ret;

    ret = V4lBufferInit();
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::CloseDevice()
{
    LOG_DEBUG("v4l CloseDevice\n");
#ifdef USING_DUMMY_WORKAROUND	
   if(bStreamOn != OMX_FALSE) { 
        OMX_S32 type = V4L2_BUF_TYPE_VIDEO_OUTPUT;   	
        ioctl(device, VIDIOC_STREAMOFF, &type);        
        nQueuedBuffer = 0;    
        bStreamOn = OMX_FALSE;
    }
    bDummyValid=OMX_FALSE;
    bFlushState=OMX_FALSE;//OMX_TRUE;
#endif
    V4lBufferDeInit();

    if(device > 0)
        close(device);
    device = 0;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::SetDeviceRotation()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(device <= 0)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);
    V4lStreamOff(OMX_FALSE);
    ret = V4lSetRotation();
    if(ret != OMX_ErrorNone) {
        fsl_osal_mutex_unlock(lock);
        return ret;
    }
    fsl_osal_mutex_unlock(lock);

    return ret;
}

OMX_ERRORTYPE V4lRender::SetDeviceInputCrop()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(device <= 0)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);
    V4lStreamOff(OMX_FALSE);
    ret = V4lSetInputCrop();
    if(ret != OMX_ErrorNone) {
        fsl_osal_mutex_unlock(lock);
        return ret;
    }
    fsl_osal_mutex_unlock(lock);

    return ret;
}

OMX_ERRORTYPE V4lRender::SetDeviceDisplayRegion()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(device <= 0)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);
    V4lStreamOff(OMX_FALSE);
    ret = V4lSetOutputCrop();
    if(ret != OMX_ErrorNone) {
        fsl_osal_mutex_unlock(lock);
        return ret;
    }
    fsl_osal_mutex_unlock(lock);

    return ret;
}


OMX_ERRORTYPE V4lRender::WriteDevice(
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    FB_DATA *fb;
    OMX_U32 nIndex;

    fsl_osal_mutex_lock(lock);

    LOG_DEBUG("WriteDevice: %p, nAllocating %d, FilledLen %d, nRenderFrames %d, nFrameBuffer %d, hMarkTargetComponent %p\n", 
        pBufferHdr, nAllocating, pBufferHdr->nFilledLen, nRenderFrames, nFrameBuffer, pBufferHdr->hMarkTargetComponent);


    if(nAllocating > 0) {
        V4lSearchBuffer(pBufferHdr->pBuffer, &nIndex);
        BufferHdrs[nIndex] = pBufferHdr;
        //LOG_DEBUG("nIndex %d, nQueuedBuffer %d\n", nIndex, nQueuedBuffer);
    }
    else {
        nIndex = (nRenderFrames % (nFrameBuffer-V4L_DUMMY_BUF_NUM));
        fb = &fb_data[nIndex];
        fsl_osal_memcpy(fb->pVirtualAddr, pBufferHdr->pBuffer, pBufferHdr->nFilledLen);
        BufferHdrs[nIndex] = pBufferHdr;
    }

    if(sCapture.eType == CAP_THUMBNAL) {
        CaptureFrame(nIndex);
        bCaptureFrameDone = OMX_TRUE;
        ports[IN_PORT]->SendBuffer(BufferHdrs[nIndex]);
        BufferHdrs[nIndex] = NULL;
        fsl_osal_mutex_unlock(lock);
        return OMX_ErrorNone;
    }

    V4lQueueBuffer(nIndex);
#ifdef USING_DUMMY_WORKAROUND	
    if((2==nQueuedBuffer)&&(bFlushState==OMX_TRUE)){
        OMX_U32 nTmp;
        //make sure: always one valid buffer(except dummy buffer) will be dequeue later 
        //otherwise: pause+seek will be very slow or timeout
        ASSERT(OMX_TRUE==bDummyValid);
        V4lDeQueueBuffer(&nTmp);
        ASSERT(nTmp==(nFrameBuffer-V4L_DUMMY_BUF_NUM));
        memcpy(fb_data[nFrameBuffer-V4L_DUMMY_BUF_NUM].pVirtualAddr,fb_data[nIndex].pVirtualAddr,fb_data[nIndex].nLength);
        V4lQueueBuffer(nFrameBuffer-V4L_DUMMY_BUF_NUM);
        bFlushState=OMX_FALSE;
    }
#else
    if(pBufferHdr->hMarkTargetComponent != NULL){
        LOG_DEBUG("requeue current buffer to steam on v4l\n");
        if(bV4lNewApi){	
            //will bring unrecoverable error if the same buffer is queue twice
            //no display : pause -> seek -> no display -> resume -> display
            V4lQueueBuffer((nIndex+1)%nFrameBuffer); //enable another buffer if you want to have display when pause+seek
        }
        else{
            V4lQueueBuffer(nIndex);
        }
        //nQueuedBuffer--;
        //nRenderFrames--;
    }
#endif
    
    nIndex = 0;
    ret = V4lDeQueueBuffer(&nIndex);
    if(ret == OMX_ErrorNone) {
        OMX_BUFFERHEADERTYPE *pHdr = NULL;
        pHdr = BufferHdrs[nIndex];
        BufferHdrs[nIndex] = NULL;
    	       
        ports[IN_PORT]->SendBuffer(pHdr);
        //LOG_DEBUG("dequeued nIndex %d\n", nIndex);
    }
#ifndef USING_DUMMY_WORKAROUND	
    // when seeking in pause state, previous frame has been flushed in FlushComponent(), here no chance to call SendBuffer() and
    // OMX_EventMark will not be sent , and then seek action fails. So we add this SendEventMark() to trigger OMX_EventMark.
    if(pBufferHdr->hMarkTargetComponent != NULL){
        LOG_DEBUG("Send eventMark in pause state\n");
        ports[0]->SendEventMark(pBufferHdr);
    }
#endif
    fsl_osal_mutex_unlock(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::CaptureFrame(OMX_U32 nIndex)
{
    if(sCapture.pBuffer == NULL)
        return OMX_ErrorBadParameter;

    if(nIndex < 0)
        return OMX_ErrorBadParameter;

    fsl_osal_memcpy(sCapture.pBuffer, 
            fb_data[nIndex].pVirtualAddr, 
            sVideoFmt.nFrameWidth * sVideoFmt.nFrameHeight * pxlfmt2bpp(sVideoFmt.eColorFormat) / 8);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::SetOutputMode()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_S32 output;
    OMX_STATETYPE eState = OMX_StateInvalid;

    if(device <= 0)
        return OMX_ErrorNotReady;
    if (bInTransitState == OMX_TRUE)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);
    bInTransitState == OMX_TRUE;

    GetState(&eState);
    if(eState == OMX_StatePause)
        V4lStreamOff(OMX_TRUE);
    else
        V4lStreamOff(OMX_FALSE);

        
    output = 3;
    if(sOutputMode.bTv == OMX_TRUE) {
        if(sOutputMode.eFbLayer == LAYER1)
            output = 5;
        if(sOutputMode.eFbLayer == LAYER2)
            switch_to_tv(sOutputMode.eTvMode);
    }
    else {
        if(sOutputMode.eFbLayer == LAYER2)
            switch_to_lcd();
    }

    fsl_osal_memcpy(&sRectOut, &sOutputMode.sRectOut, sizeof(OMX_CONFIG_RECTTYPE));
    fsl_osal_memcpy(&sRectIn, &sOutputMode.sRectIn, sizeof(OMX_CONFIG_RECTTYPE));

    ret = V4lSetOutputLayer(output);
    if(ret != OMX_ErrorNone)
        goto err;

    ret = V4lSetInputCrop();
    if(ret != OMX_ErrorNone)
        goto err;

    ret = V4lSetOutputCrop();
    if(ret != OMX_ErrorNone)
        goto err;

    eRotation = sOutputMode.eRotation;
    ret = V4lSetRotation();
    if(ret != OMX_ErrorNone)
        goto err;


    if(eState == OMX_StatePause)
        V4lStreamOnInPause();

    fsl_osal_mutex_unlock(lock);
    return OMX_ErrorNone;

err:
    fsl_osal_mutex_unlock(lock);
    return ret;
}

OMX_ERRORTYPE V4lRender::V4lBufferInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    struct v4l2_requestbuffers buf_req;
    struct v4l2_buffer *pBuffer = NULL;
    OMX_S32 cr_left, cr_top, cr_right, cr_bottom;
    OMX_BOOL bSetCrop = OMX_FALSE;
    OMX_U32 i;

    LOG_DEBUG("V4lBufferInit nFrameBuffer %d\n", nFrameBuffer);

    fsl_osal_memset(&buf_req, 0, sizeof(buf_req));
    buf_req.count = nFrameBuffer;
    buf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf_req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(device, VIDIOC_REQBUFS, &buf_req) < 0) {
        LOG_ERROR("request buffers #%d failed.\n", nFrameBuffer);
        return OMX_ErrorInsufficientResources;
    }

    cr_left   = sRectIn.nLeft;
    cr_top    = sRectIn.nTop;
    cr_right  = sVideoFmt.nFrameWidth - (sRectIn.nLeft + sRectIn.nWidth);
    cr_bottom = sVideoFmt.nFrameHeight - (sRectIn.nTop + sRectIn.nHeight);
    if(cr_left != 0 || cr_right != 0 || cr_top != 0)
        bSetCrop = OMX_TRUE;

    for(i=0; i<nFrameBuffer; i++)
        fb_data[i].pVirtualAddr = NULL;

    for(i=0; i<nFrameBuffer; i++) {
        pBuffer = &v4lbufs[i];
        fsl_osal_memset(pBuffer, 0, sizeof(struct v4l2_buffer));
        pBuffer->index = i;
        pBuffer->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        pBuffer->memory = V4L2_MEMORY_MMAP;
        if (ioctl(device, VIDIOC_QUERYBUF, pBuffer) < 0) {
            LOG_ERROR("VIDIOC_QUERYBUF failed %d\n", pBuffer->index);
            ret = OMX_ErrorHardware;
            break;
        }

        fb_data[i].nIndex = i;
        fb_data[i].nLength = pBuffer->length;
        fb_data[i].pPhyiscAddr = (OMX_PTR)pBuffer->m.offset;
        fb_data[i].pVirtualAddr = mmap(
                NULL, 
                pBuffer->length,
                PROT_READ | PROT_WRITE, MAP_SHARED,
                device,
                pBuffer->m.offset);
        if(bV4lNewApi){
            /*
            * Workaround for new V4L interface change, this change
            * will be removed after V4L driver is updated for this.
            * Need to call QUERYBUF ioctl again after mmap.
            */
            if (ioctl(device, VIDIOC_QUERYBUF, pBuffer) < 0) {
                LOG_ERROR("VIDIOC_QUERYBUF failed %d\n", pBuffer->index);
                ret = OMX_ErrorHardware;
                break;
            }
            fb_data[i].nLength = pBuffer->length;
            fb_data[i].pPhyiscAddr = (OMX_PTR)pBuffer->m.offset;		
        }
        if(fb_data[i].pVirtualAddr == NULL) {
            LOG_ERROR("mmap buffer: %d failed.\n", i);
            ret = OMX_ErrorHardware;
            break;
        }

        AddHwBuffer(fb_data[i].pPhyiscAddr, fb_data[i].pVirtualAddr);

        if(bSetCrop == OMX_TRUE)
            pBuffer->m.offset = pBuffer->m.offset + (cr_top * sVideoFmt.nFrameWidth) + cr_left;
        LOG_DEBUG("%s,%d,v4l buf offset %x, base %x, cr_top %d, frame width %d, cr_left %d.\n",__FUNCTION__,__LINE__,
                pBuffer->m.offset,
                fb_data[i].pPhyiscAddr,
                cr_top,
                sVideoFmt.nFrameWidth,
                cr_left);

    }

    if(ret != OMX_ErrorNone) {
        V4lBufferDeInit();
        return ret;
    }

    bFrameBufferInit = OMX_TRUE;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::V4lBufferDeInit()
{
    OMX_U32 i;

    LOG_DEBUG("V4lBufferDeInit\n");

    for(i=0; i<nFrameBuffer; i++) {
        if(fb_data[i].pVirtualAddr != NULL) {
            RemoveHwBuffer(fb_data[i].pVirtualAddr);
            munmap(fb_data[i].pVirtualAddr, fb_data[i].nLength);
        }
    }

    bFrameBufferInit = OMX_FALSE;
    nAllocating = 0;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::V4lSearchBuffer(
        OMX_PTR pVirtualAddr, 
        OMX_U32 *pIndex)
{
    OMX_U32 nIndex = 0;
    OMX_U32 i;
    OMX_BOOL bFound = OMX_FALSE;

    for(i=0; i<nFrameBuffer; i++) {
        if(fb_data[i].pVirtualAddr == pVirtualAddr) {
            nIndex = i;
            bFound = OMX_TRUE;
            break;
        }
    }

    if(bFound != OMX_TRUE)
        return OMX_ErrorUndefined;

    *pIndex = nIndex;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::V4lQueueBuffer(
        OMX_U32 nIndex)
{
    struct v4l2_buffer *pBuffer;
    struct timeval queuetime;

    pBuffer = &v4lbufs[nIndex];
    gettimeofday(&queuetime, NULL);
    pBuffer->timestamp = queuetime;
    if(ioctl(device, VIDIOC_QBUF, pBuffer) < 0) {
        LOG_ERROR("Queue buffer #%d failed.\n", nIndex);
        SendEvent(OMX_EventError, OMX_ErrorHardware, 0, NULL);
        return OMX_ErrorHardware;
    }
    EnqueueBufferIdx = nIndex;

    nQueuedBuffer ++;
    nRenderFrames ++;
#ifdef USING_DUMMY_WORKAROUND	
    if(nIndex==nFrameBuffer-V4L_DUMMY_BUF_NUM){
        bDummyValid=OMX_TRUE;		
    }		
    if(1==nQueuedBuffer){
        //queue dummy buffer index
        ASSERT(bDummyValid==OMX_FALSE);
        memcpy(fb_data[nFrameBuffer-V4L_DUMMY_BUF_NUM].pVirtualAddr,fb_data[nIndex].pVirtualAddr,fb_data[nIndex].nLength);
        pBuffer = &v4lbufs[nFrameBuffer-V4L_DUMMY_BUF_NUM];
        gettimeofday(&queuetime, NULL);
        pBuffer->timestamp = queuetime;
        if(ioctl(device, VIDIOC_QBUF, pBuffer) < 0) {
            LOG_ERROR("Queue buffer #%d failed.\n", nIndex);
            SendEvent(OMX_EventError, OMX_ErrorHardware, 0, NULL);
            return OMX_ErrorHardware;
        }
        nQueuedBuffer ++;
        //nRenderFrames ++;
        bDummyValid=OMX_TRUE;
    }
#endif
    if(nQueuedBuffer >= 2 && bStreamOn != OMX_TRUE) {
        OMX_S32 type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        if (ioctl(device, VIDIOC_STREAMON, &type) < 0) {
            LOG_ERROR("Stream On failed.\n");
            SendEvent(OMX_EventError, OMX_ErrorHardware, 0, NULL);
            return OMX_ErrorHardware;
        }
        bStreamOn = OMX_TRUE;
        bInTransitState = OMX_FALSE;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::V4lDeQueueBuffer(
        OMX_U32 *pIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    struct v4l2_buffer v4lbuf;
    OMX_U32 i;

    if(nQueuedBuffer < 2)
        return OMX_ErrorNotReady;

    for(i=0; i<BUFFER_NEW_RETRY_MAX; i++) {
        fsl_osal_memset(&v4lbuf, 0, sizeof(struct v4l2_buffer));
        v4lbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        v4lbuf.memory = V4L2_MEMORY_MMAP;
        if(ioctl(device, VIDIOC_DQBUF, &v4lbuf) < 0) {
            usleep(1000);
            ret = OMX_ErrorNotReady;
            continue;
        }
        nQueuedBuffer --;
        *pIndex = v4lbuf.index;
#ifdef USING_DUMMY_WORKAROUND			
        if(v4lbuf.index==(nFrameBuffer-V4L_DUMMY_BUF_NUM)){
            bDummyValid=OMX_FALSE;
            ret = OMX_ErrorNotReady;
        }
        else
#endif
            ret = OMX_ErrorNone;
        break;
    }

    return ret;
}

OMX_ERRORTYPE V4lRender::V4lStreamOff(OMX_BOOL bKeepLast)
{
    OMX_S32 type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    OMX_U32 i;

    LOG_DEBUG("V4lRender::V4lStreamOff, bStreamOn %d, nQueuedBuffer %d, keepLast %d\n", bStreamOn, nQueuedBuffer, bKeepLast);
    
    if(bStreamOn != OMX_FALSE) {
 
        ioctl(device, VIDIOC_STREAMOFF, &type);
        
        for(i=0; i<nFrameBuffer; i++) {
            if(BufferHdrs[i] != NULL) {
                if(bKeepLast == OMX_TRUE && i == EnqueueBufferIdx)
                    continue;
                LOG_DEBUG("SendBuffer %d\n", i);
                ports[IN_PORT]->SendBuffer(BufferHdrs[i]);
                BufferHdrs[i] = NULL;
            }
        }

        nQueuedBuffer = 0;
    
        bStreamOn = OMX_FALSE;
#ifdef USING_DUMMY_WORKAROUND
        bDummyValid=OMX_FALSE;
        bFlushState=OMX_FALSE;//OMX_TRUE;
#endif
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::V4lStreamOnInPause()
{
    OMX_U32 nIndex;

    LOG_DEBUG("V4lRender::V4lStreamOnInPause, EnqueueBufferIdx %d\n", EnqueueBufferIdx);

    if(EnqueueBufferIdx < 0)
        return OMX_ErrorNone;

    V4lQueueBuffer(EnqueueBufferIdx);
#ifndef USING_DUMMY_WORKAROUND	
    V4lQueueBuffer(EnqueueBufferIdx);
#endif
    V4lDeQueueBuffer(&nIndex);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::V4lSetInputCrop()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    struct v4l2_format fmt;
    struct v4l2_mxc_offset off;
    struct v4l2_rect icrop = {0};	//for new v4l

    OMX_S32 in_width, in_height;
    OMX_S32 cr_left, cr_top, cr_right, cr_bottom;
    OMX_S32 in_width_chroma, in_height_chroma;
    OMX_S32 crop_left_chroma, crop_right_chroma, crop_top_chroma, crop_bottom_chroma;
    OMX_U32 i;
    OMX_CONFIG_RECTTYPE sRectSetup = sRectIn;
    sRectSetup.nWidth = sRectSetup.nWidth/8*8;
    sRectSetup.nLeft = (sRectSetup.nLeft + 7)/8*8;
    in_width = sRectSetup.nWidth;
    in_height = sRectSetup.nHeight;

    fsl_osal_memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    fmt.fmt.pix.width = in_width;
    fmt.fmt.pix.height = in_height;
    if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV420Planar) {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
        LOG_DEBUG("Set V4L in format to YUVI420.\n");
    }
    if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
        LOG_DEBUG("Set V4L in format to NV12.\n");
    }

    cr_left   = sRectSetup.nLeft;
    cr_top    = sRectSetup.nTop;
    cr_right  = sVideoFmt.nFrameWidth - (sRectSetup.nLeft + sRectSetup.nWidth);
    cr_bottom = sVideoFmt.nFrameHeight - (sRectSetup.nTop + sRectSetup.nHeight);

    in_width_chroma = in_width / 2;
    in_height_chroma = in_height / 2;

    crop_left_chroma = cr_left / 2;
    crop_right_chroma = cr_right / 2;
    crop_top_chroma = cr_top / 2;
    crop_bottom_chroma = cr_bottom / 2;

    if ((cr_left!=0) || (cr_top!=0) || (cr_right!=0) || (cr_bottom!=0)) {
        if(bV4lNewApi){
            /* This is aligned with new V4L interface on 2.6.38 kernel */
            fmt.fmt.pix.width = sVideoFmt.nFrameWidth;
            fmt.fmt.pix.height = sVideoFmt.nFrameHeight;
            icrop.left = sRectSetup.nLeft;
            icrop.top = sRectSetup.nTop;
            icrop.width = sRectSetup.nWidth;
            icrop.height = sRectSetup.nHeight;
            fmt.fmt.pix.priv =  (unsigned long)&icrop;
        }
        else{
            off.u_offset = ((cr_left + cr_right + in_width) * (in_height + cr_bottom)) - cr_left
                + (crop_top_chroma * (in_width_chroma + crop_left_chroma + crop_right_chroma))
                + crop_left_chroma;
            off.v_offset = off.u_offset +
                (crop_left_chroma + crop_right_chroma + in_width_chroma)
                * ( in_height_chroma  + crop_bottom_chroma + crop_top_chroma);
    
            fmt.fmt.pix.bytesperline = in_width + cr_left + cr_right;
            fmt.fmt.pix.priv = (OMX_S32) & off;
            fmt.fmt.pix.sizeimage = (in_width + cr_left + cr_right)
                * (in_height + cr_top + cr_bottom) * 3 / 2;
        }
    }
    else {
        fmt.fmt.pix.bytesperline = in_width;
        fmt.fmt.pix.priv = 0;
        fmt.fmt.pix.sizeimage = 0;
    }

    LOG_DEBUG("%s,%d,\n\ 
                      in_width %d,in_height %d\n\
                      cr_left %d, cr_top %d, cr_right %d cr_bottom %d\n\
                      u_offset %d, v_offset %d\n",__FUNCTION__,__LINE__,
                      in_width,in_height,
                      cr_left,cr_top,cr_right,cr_bottom,
                      off.u_offset,off.v_offset);

    if (ioctl(device, VIDIOC_S_FMT, &fmt) < 0) {
        LOG_ERROR("set format failed\n");
        return OMX_ErrorHardware;
    }

    if (ioctl(device, VIDIOC_G_FMT, &fmt) < 0) {
        LOG_ERROR("get format failed\n");
        return OMX_ErrorHardware;
    }

    /* set v4l buffers corp */
    for(i=0; i<nFrameBuffer; i++) {
        struct v4l2_buffer *pBuffer = NULL;
        pBuffer = &v4lbufs[i];
        if(pBuffer != NULL)
            pBuffer->m.offset = (OMX_U32)fb_data[i].pPhyiscAddr + (cr_top * sVideoFmt.nFrameWidth) + cr_left;
        LOG_DEBUG("%s,%d,v4l buf offset %x, base %x, cr_top %d, frame width %d, cr_left %d.\n",__FUNCTION__,__LINE__,
                pBuffer->m.offset,
                fb_data[i].pPhyiscAddr,
                cr_top,
                sVideoFmt.nFrameWidth,
                cr_left);
    }

    return ret;
}

OMX_ERRORTYPE V4lRender::V4lSetOutputCrop()
{
    struct v4l2_crop crop;

    crop.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    crop.c.width = sRectOut.nWidth;
    crop.c.height = sRectOut.nHeight;
    crop.c.top = sRectOut.nTop;
    crop.c.left = sRectOut.nLeft;

    LOG_DEBUG("%s,%d, width %d,height %d,top %d, left %d\n",__FUNCTION__,__LINE__,
            crop.c.width,crop.c.height,crop.c.top,crop.c.left);

    if (ioctl(device, VIDIOC_S_CROP, &crop) < 0) {
        LOG_ERROR("set crop failed\n");
        return OMX_ErrorHardware;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::V4lSetRotation()
{
    struct v4l2_control ctrl;
    if(bV4lNewApi){
        ctrl.id = V4L2_CID_ROTATE;
        switch(eRotation)
        {
            case ROTATE_NONE:
            case ROTATE_VERT_FLIP:
            case ROTATE_HORIZ_FLIP:
                ctrl.value =0;
                break;
            case ROTATE_180:
                ctrl.value =180;
                break;
            case ROTATE_90_RIGHT:
            case ROTATE_90_RIGHT_VFLIP:
            case ROTATE_90_RIGHT_HFLIP:
                ctrl.value =90;
                break;
            case ROTATE_90_LEFT	:
                ctrl.value = 270;
                break;
            default:
                ctrl.value =0;
                break;
        }
    }
    else{
        ctrl.id = V4L2_CID_PRIVATE_BASE;
        ctrl.value = (OMX_S32)eRotation;
    }
    if (ioctl(device, VIDIOC_S_CTRL, &ctrl) < 0) {
        LOG_ERROR("set rotate failed\n");
        fsl_osal_mutex_unlock(lock);
        return OMX_ErrorHardware;
    }

    LOG_DEBUG("%s,%d, set screen rotate to %d\n",__FUNCTION__,__LINE__,eRotation);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::V4lSetOutputLayer(OMX_S32 output)
{
    if(!bV4lNewApi){
        if (ioctl(device, VIDIOC_S_OUTPUT, &output) < 0) {
            LOG_ERROR("set output failed\n");
            return OMX_ErrorHardware;
        }
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lRender::FlushComponent(
        OMX_U32 nPortIndex) 
{
    LOG_DEBUG("V4lRender::FlushComponent\n");
#ifdef USING_DUMMY_WORKAROUND	
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nIndex;
    while(nQueuedBuffer>1){
        ret = V4lDeQueueBuffer(&nIndex);
        if(ret == OMX_ErrorNone) {
            OMX_BUFFERHEADERTYPE *pHdr = NULL;
            pHdr = BufferHdrs[nIndex];
            BufferHdrs[nIndex] = NULL;            
            ports[IN_PORT]->SendBuffer(pHdr);
        }
    }
    if((1==nQueuedBuffer)&& bDummyValid==OMX_FALSE){
        memcpy(fb_data[nFrameBuffer-V4L_DUMMY_BUF_NUM].pVirtualAddr,fb_data[EnqueueBufferIdx].pVirtualAddr,fb_data[EnqueueBufferIdx].nLength);
        ret=V4lQueueBuffer(nFrameBuffer-V4L_DUMMY_BUF_NUM);
        ret = V4lDeQueueBuffer(&nIndex);
        if(ret == OMX_ErrorNone) {
            OMX_BUFFERHEADERTYPE *pHdr = NULL;
            pHdr = BufferHdrs[nIndex];
            BufferHdrs[nIndex] = NULL;            
            ports[IN_PORT]->SendBuffer(pHdr);
        }
    }
    bFlushState	=OMX_TRUE;
#else
    if(bStreamOn == OMX_TRUE)
        V4lStreamOff(OMX_FALSE);
    else{
        int i;
        for(i=0; i<nFrameBuffer; i++) {
            if(BufferHdrs[i] != NULL) {
                LOG_DEBUG("FlushComponent SendBuffer %d\n", i);
                ports[IN_PORT]->SendBuffer(BufferHdrs[i]);
                BufferHdrs[i] = NULL;
            }
        }
        nQueuedBuffer = 0;
    }
#endif
    return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE V4lRenderInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        V4lRender *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(V4lRender, ());
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
