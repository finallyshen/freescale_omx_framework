/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "OverlayRender.h"

#define NO_ERROR 0
#define OVERLAY_HOLD_BUFFER_NUM 2

//#define LOG_DEBUG printf

OverlayRender::OverlayRender()
{
    OMX_U32 i; 

    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_render.overlay.sw-based");
    ComponentVersion.s.nVersionMajor = 0x0;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x0;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"video_render.overlay";

    nFrameBufferMin = 2;
    nFrameBufferActual = 2;
    TunneledSupplierType = OMX_BufferSupplyInput;
    fsl_osal_memset(&sVideoFmt, 0, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
    sVideoFmt.nFrameWidth = 800;
    sVideoFmt.nFrameHeight = 480;
    sVideoFmt.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    sVideoFmt.eCompressionFormat = OMX_VIDEO_CodingUnused;

    OMX_INIT_STRUCT(&sRectIn, OMX_CONFIG_RECTTYPE);
    sRectIn.nWidth = sVideoFmt.nFrameWidth;
    sRectIn.nHeight = sVideoFmt.nFrameHeight;
    sRectIn.nTop = 0;
    sRectIn.nLeft = 0;

    lock = NULL;
    pShowFrame = NULL;
    nFrameLen = 0;
    bSuspend = OMX_FALSE;
    mOverlay = NULL;
    mSurface = NULL;
    ref = NULL;
    
    OMX_INIT_STRUCT(&sCapture, OMX_CONFIG_CAPTUREFRAME);
    sCapture.eType = CAP_NONE;
   
}

OMX_ERRORTYPE OverlayRender::InitRenderComponent()
{
    if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal)) {
        LOG_ERROR("Create mutex for v4l device failed.\n");
        return OMX_ErrorInsufficientResources;
    }

    qBufferInOverlay = FSL_NEW(Queue, ());
    if(qBufferInOverlay == NULL) {
        LOG_ERROR("New queue for buffers in overlay failed.\n");
        return OMX_ErrorInsufficientResources;
    }

    if(qBufferInOverlay->Create(8, sizeof(OMX_BUFFERHEADERTYPE*), E_FSL_OSAL_TRUE)
            != QUEUE_SUCCESS) {
        LOG_ERROR("Init buffer queue failed.\n");
        FSL_DELETE(qBufferInOverlay);
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OverlayRender::DeInitRenderComponent()
{
    CloseDevice();

    if(lock)
        fsl_osal_mutex_destroy(lock);

    qBufferInOverlay->Free();
    FSL_DELETE(qBufferInOverlay);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OverlayRender::RenderGetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
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

OMX_ERRORTYPE OverlayRender::RenderSetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
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
            {
                OMX_CONFIG_SYSSLEEP *pSysSleep = (OMX_CONFIG_SYSSLEEP *)pStructure;
                CHECK_STRUCT(pSysSleep, OMX_CONFIG_SYSSLEEP, ret);
                bSuspend = pSysSleep->bSleep;
                
                fsl_osal_mutex_lock(lock);
                if(bSuspend==OMX_TRUE)
                    CloseDevice();
                else {
                    OMX_STATETYPE eState = OMX_StateInvalid;
                    GetState(&eState);
                    if(eState == OMX_StatePause)
                        StartDeviceInPause();
                }
                fsl_osal_mutex_unlock(lock);
                
        }
            break;
        case OMX_IndexParamSurface:
            {
                LOG_DEBUG("OverlayRender::SetConfig OMX_IndexParamSurface\n");
                fsl_osal_mutex_lock(lock);

                sp<ISurface> *pSurface;
                pSurface = (sp<ISurface> *)pStructure;
                if(mSurface != *pSurface){
                    mSurface = *pSurface;
                    if(mOverlay != NULL){
                        mOverlay->destroy();
                        mOverlay=NULL;
                        OverlayInit();
                    }
                }

                fsl_osal_mutex_unlock(lock);
            }
            break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}


OMX_ERRORTYPE OverlayRender::SetDeviceInputCrop()
{
    if(mOverlay != NULL)
        mOverlay->setCrop(sRectIn.nLeft, sRectIn.nTop, sRectIn.nWidth, sRectIn.nHeight);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OverlayRender::PortFormatChanged(OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BOOL bSizeChanged = OMX_FALSE;
    OMX_BOOL bColorFormatChanged = OMX_FALSE;
    
    if(nPortIndex != IN_PORT)
        return OMX_ErrorBadPortIndex;


    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    ports[IN_PORT]->GetPortDefinition(&sPortDef);

    LOG_DEBUG("OverlayRender::PortFormatChanged, old w/h %d/%d, new w/d %d/%d, color %d\n", 
        sVideoFmt.nFrameWidth, sVideoFmt.nFrameHeight, sPortDef.format.video.nFrameWidth, sPortDef.format.video.nFrameHeight, sPortDef.format.video.eColorFormat);

    if(sVideoFmt.nFrameWidth != sPortDef.format.video.nFrameWidth) {
        sVideoFmt.nFrameWidth = sPortDef.format.video.nFrameWidth;
        bSizeChanged = OMX_TRUE;
    }
    if(sVideoFmt.nFrameHeight != sPortDef.format.video.nFrameHeight) {
        sVideoFmt.nFrameHeight = sPortDef.format.video.nFrameHeight;
        bSizeChanged = OMX_TRUE;
    }

    if(sVideoFmt.eColorFormat != sPortDef.format.video.eColorFormat) {
        sVideoFmt.eColorFormat = sPortDef.format.video.eColorFormat;
        bSizeChanged = OMX_TRUE;
    }

    if(bColorFormatChanged == OMX_TRUE || bSizeChanged == OMX_TRUE){
        if(mOverlay != NULL)
            LOG_ERROR("format changed after overlay created! should not happen...\n");
    }
        
    if(bColorFormatChanged == OMX_TRUE){
        // re-create overlay
    }
    else if(bSizeChanged == OMX_TRUE){
        if(mOverlay != NULL){
            FlushComponent(0);
            // not implemented yet
            //mOverlay->resizeInput(sVideoFmt.nFrameWidth, sVideoFmt.nFrameHeight);
        }
    }
    return ret;
}

OMX_ERRORTYPE OverlayRender::OpenDevice()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE OverlayRender::ResetDevice()
{
    fsl_osal_mutex_lock(lock);

    LOG_DEBUG("OverlayRender::ResetDevice\n");

    CloseDevice();

    OMX_STATETYPE eState = OMX_StateInvalid;
    GetState(&eState);
    if(eState == OMX_StatePause && bSuspend != OMX_TRUE)
        StartDeviceInPause();

    fsl_osal_mutex_unlock(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OverlayRender::CloseDevice()
{
    LOG_DEBUG("OverlayRender::CloseDevice\n");

    while(qBufferInOverlay->Size()){
        OMX_BUFFERHEADERTYPE *pBufferHdr;
        qBufferInOverlay->Get(&pBufferHdr);
        ports[0]->SendBuffer(pBufferHdr);     
    }

    if(mOverlay != NULL){
        mOverlay->destroy();
        mOverlay = NULL;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OverlayRender::StartDeviceInPause()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("OverlayRender::StartDeviceInPause() pShowFrame %p\n", pShowFrame);

    if(pShowFrame == NULL)
        return OMX_ErrorBadParameter;

    OMX_PTR pFrame = NULL;
    GetHwBuffer(pShowFrame, &pFrame);

    if(mOverlay == NULL){
        ret = OverlayInit();
        if(ret != OMX_ErrorNone)
            return ret;
    }

#if 0
    {
        static FILE *pfTest = NULL;
        pfTest = fopen("/sdcard/extsd/DumpData.yuv", "wb");
        if(pfTest == NULL)
            printf("Unable to open test file! \n");
        if(pfTest != NULL) {        
                printf("dump data w/h %d/%d\n", sVideoFmt.nFrameWidth, sVideoFmt.nFrameHeight);
                fwrite(pShowFrame, sizeof(char), sVideoFmt.nFrameWidth * sVideoFmt.nFrameHeight, pfTest);
                fflush(pfTest);
                fclose(pfTest);
                pfTest = NULL;
         }
    }
#endif

    ret = OverlayRenderFrame(pFrame);
    if(ret != OMX_ErrorNone)
        return ret;

    // v4l need two frames to start working
    ret = OverlayRenderFrame(pFrame);
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OverlayRender::WriteDevice(
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

    if(mOverlay == NULL) {
        ret = OverlayInit();
        if(ret != OMX_ErrorNone) {
            ports[0]->SendBuffer(pBufferHdr);
            fsl_osal_mutex_unlock(lock);
            return ret;
        }
    }

    ret = OverlayRenderFrame(pFrame);
    if(ret != OMX_ErrorNone) {
        ports[0]->SendBuffer(pBufferHdr);
        fsl_osal_mutex_unlock(lock);
        return ret;
    }

    qBufferInOverlay->Add(&pBufferHdr);
    if(qBufferInOverlay->Size() > OVERLAY_HOLD_BUFFER_NUM){
        OMX_BUFFERHEADERTYPE * pHdr;
        qBufferInOverlay->Get(&pHdr);
        ports[0]->SendBuffer(pHdr);
    }

    // when seeking in pause state, previous frame has been flushed in FlushComponent(), here no chance to call SendBuffer() and
    // OMX_EventMark will not be sent , and then seek action fails. So we add this SendEventMark() to trigger OMX_EventMark.
    if(pBufferHdr->hMarkTargetComponent != NULL){
        ports[0]->SendEventMark(pBufferHdr);
    }

    fsl_osal_mutex_unlock(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OverlayRender::OverlayInit()
{
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    int mode;
    int ret = NO_ERROR;

    int videoFormat;

    switch(sVideoFmt.eColorFormat ){
        case OMX_COLOR_FormatYUV420SemiPlanar:
            videoFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
            break;

        case OMX_COLOR_FormatYUV420Planar:
            videoFormat = HAL_PIXEL_FORMAT_YCbCr_420_I;
            break;

        case OMX_COLOR_Format16bitRGB565:
            videoFormat = HAL_PIXEL_FORMAT_RGB_565;
            break;

        case OMX_COLOR_FormatYUV422Planar:
            videoFormat = HAL_PIXEL_FORMAT_YCbCr_422_I;
            break;

        default:
        LOG_ERROR("Not supported color format %d by overlay !\n");
        return OMX_ErrorUndefined;

    }

    if(mOverlay != NULL){
        LOG_DEBUG("Overlay already inited\n");
        return OMX_ErrorNone;
    }
    
    LOG_DEBUG("Render through overlay HAL\n");
    LOG_DEBUG("sVideoFmt w %d, h %d, format %d\n", sVideoFmt.nFrameWidth, sVideoFmt.nFrameHeight, videoFormat);

    sp<OverlayRef> ref = NULL;
    if(mSurface == NULL){
        LOG_ERROR("mSurface is not initialized\n");
        return OMX_ErrorUndefined;
    }

    ref = mSurface->createOverlay(sVideoFmt.nFrameWidth, sVideoFmt.nFrameHeight, videoFormat,0);
    if ( ref.get() == NULL )
    {
       LOG_ERROR("Can not create overlay!");
       return OMX_ErrorUndefined;
    }

    mOverlay = new Overlay(ref);
    LOG_DEBUG("Overlay creation ok\n");

    LOG_DEBUG("crop: %d,%d, %d/%d\n", sRectIn.nLeft, sRectIn.nTop, sRectIn.nWidth, sRectIn.nHeight);

    SetDeviceInputCrop();

    ret = mOverlay->setParameter(OVERLAY_MODE,OVERLAY_PUSH_MODE);
    if (ret != NO_ERROR) {
        LOG_ERROR("Can not set OVERLAY_PUSH_MODE");
        mOverlay->destroy();
        mOverlay = NULL;
        return OMX_ErrorUndefined;
    }

    ret = mOverlay->setParameter(OVERLAY_FOR_PLAYBACK, 1);
    if (ret != NO_ERROR) {
        LOG_ERROR("Can not set OVERLAY_FOR_PLAYBACK");
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OverlayRender::OverlayRenderFrame(OMX_PTR pFrame)
{
    overlay_buffer_t overlay_buffer;
    void* bufferAddr = NULL;
    int queueTimes = 0;
    int ret = NO_ERROR;
    OMX_ERRORTYPE OMXResult = OMX_ErrorNone;

    if(mOverlay == NULL){
        LOG_ERROR("mOverlay not initialized!\n");
        return OMX_ErrorUndefined;
    }

    //LOG_DEBUG("OverlayRenderFrame %p\n", pFrame);

    overlay_buffer = (overlay_buffer_t)pFrame;
    ret = mOverlay->queueBuffer(overlay_buffer);
    if (ret != NO_ERROR) {
       OMXResult = OMX_ErrorUndefined;
       LOG_ERROR("Error!Cannot queue current overlay buffer 0x%x, ret %d",overlay_buffer, ret);
    }        

    return OMXResult;
}

OMX_ERRORTYPE OverlayRender::FlushComponent(
        OMX_U32 nPortIndex) 
{
    LOG_DEBUG("OverlayRender FlushCompoment\n");

    while(qBufferInOverlay->Size()){
        OverlayRenderFrame(NULL);
        OMX_BUFFERHEADERTYPE *pBufferHdr;
        qBufferInOverlay->Get(&pBufferHdr);
        ports[0]->SendBuffer(pBufferHdr);     
    }
    
    return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
    extern "C" {
        OMX_ERRORTYPE OverlayRenderInit(OMX_IN OMX_HANDLETYPE pHandle)
        {
            OMX_ERRORTYPE ret = OMX_ErrorNone;
            OverlayRender *obj = NULL;
            ComponentBase *base = NULL;

            obj = FSL_NEW(OverlayRender, ());
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
