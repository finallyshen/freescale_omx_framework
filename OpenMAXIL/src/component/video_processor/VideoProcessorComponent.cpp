/**
 *  Copyright (c) 2009-2010,2012 Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */


#include <string.h>
#include "VideoProcessorComponent.h"

typedef struct
{
    int id;
    int width;
    int height;
    int bitsPerPixel;
    void* phyAddr;
    void* virtAddr;
}EGLImageKHRInternal;

/* Utility functions */
static int omx2ipu_pxlfmt(OMX_COLOR_FORMATTYPE omx_pxlfmt)
{
    int ipu_pxlfmt; // bit per pixel

    switch(omx_pxlfmt) {
        case  OMX_COLOR_Format16bitRGB565:
            ipu_pxlfmt = v4l2_fourcc('R', 'G', 'B', 'P');
            break;
        case  OMX_COLOR_Format24bitRGB888:
            ipu_pxlfmt = v4l2_fourcc('R', 'G', 'B', '3');
            break;
        case  OMX_COLOR_Format24bitBGR888:
            ipu_pxlfmt = v4l2_fourcc('B', 'G', 'R', '3');
            break;
        case  OMX_COLOR_Format32bitBGRA8888:
            ipu_pxlfmt = v4l2_fourcc('B', 'G', 'R', 'A');
            break;
        case  OMX_COLOR_Format32bitARGB8888:
            ipu_pxlfmt = v4l2_fourcc('R', 'G', 'B', 'A');
            break;
        case  OMX_COLOR_FormatYUV420Planar:
            ipu_pxlfmt = v4l2_fourcc('I', '4', '2', '0');
            break;

        case OMX_COLOR_FormatUnused:
        case  OMX_COLOR_FormatMonochrome:
        case  OMX_COLOR_Format8bitRGB332:
        case  OMX_COLOR_Format12bitRGB444:
        case  OMX_COLOR_Format16bitARGB4444:
        case  OMX_COLOR_Format16bitARGB1555:
        case  OMX_COLOR_Format16bitBGR565:
        case  OMX_COLOR_Format18bitRGB666:
        case  OMX_COLOR_Format18bitARGB1665:
        case  OMX_COLOR_Format19bitARGB1666:
        case  OMX_COLOR_Format24bitARGB1887:
        case  OMX_COLOR_Format25bitARGB1888:
        case  OMX_COLOR_FormatYUV411Planar:
        case  OMX_COLOR_FormatYUV411PackedPlanar:
        case  OMX_COLOR_FormatYUV420PackedPlanar:
        case  OMX_COLOR_FormatYUV420SemiPlanar:
        case  OMX_COLOR_FormatYUV422Planar:
        case  OMX_COLOR_FormatYUV422PackedPlanar:
        case  OMX_COLOR_FormatYUV422SemiPlanar:
        case  OMX_COLOR_FormatYCbYCr:
        case  OMX_COLOR_FormatYCrYCb:
        case  OMX_COLOR_FormatCbYCrY:
        case  OMX_COLOR_FormatCrYCbY:
        case  OMX_COLOR_FormatYUV444Interleaved:
        case  OMX_COLOR_FormatRawBayer8bit:
        case  OMX_COLOR_FormatRawBayer10bit:
        case  OMX_COLOR_FormatRawBayer8bitcompressed:
        case  OMX_COLOR_FormatL2:
        case  OMX_COLOR_FormatL4:
        case  OMX_COLOR_FormatL8:
        case  OMX_COLOR_FormatL16:
        case  OMX_COLOR_FormatL24:
        case  OMX_COLOR_FormatL32:
        case  OMX_COLOR_FormatYUV420PackedSemiPlanar:
        case  OMX_COLOR_FormatYUV422PackedSemiPlanar:
        case  OMX_COLOR_Format18BitBGR666:
        case  OMX_COLOR_Format24BitARGB6666:
        case  OMX_COLOR_Format24BitABGR6666:
        default:
            ipu_pxlfmt = 0;
            break;
    }
    
    return ipu_pxlfmt;  
}

void ipu_output_cb(void *arg, int index)
{
    VideoProcessorComponent *pCompPriv = (VideoProcessorComponent *)arg;
    pCompPriv->nOutBufIdx = index;
}


#if 0
static OMX_ERRORTYPE VideoProcessor_GetComponentVersion(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STRING pComponentName,
        OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
        OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
        OMX_OUT OMX_UUIDTYPE* pComponentUUID)
{
    OMX_STATETYPE eState = OMX_StateInvalid;
    OMX_COMPONENTTYPE *pComponent = NULL;

    if ( hComponent == NULL )
        return OMX_ErrorInvalidComponent;

    eState = ((COMPONENT_PRIVATEDATA*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate))->sCompCurrentState.eState;
    if (eState == OMX_StateInvalid)
        return OMX_ErrorInvalidState;

    pComponent = (OMX_COMPONENTTYPE *)hComponent;

    strcpy(pComponentName, VP_NAME);
    pComponentVersion->nVersion = pComponent->nVersion.nVersion;
    if(pSpecVersion == NULL)
        return OMX_ErrorBadParameter;

    pSpecVersion->nVersion = VP_COMP_VERSION;
    if(pComponentVersion->nVersion != pSpecVersion->nVersion)
        return OMX_ErrorVersionMismatch;

    return OMX_ErrorNone;
}

#endif
OMX_ERRORTYPE VideoProcessorComponent::omxSetHeader(OMX_PTR pHeader, OMX_S32 nHeaderSize)
{
    fsl_osal_memset(pHeader,0,nHeaderSize);
    OMX_VERSIONTYPE* pVersion = (OMX_VERSIONTYPE*)((OMX_U8 *)pHeader + sizeof(OMX_U32));
    *((OMX_U32*)pHeader) = nHeaderSize;
    pVersion->nVersion = ComponentVersion.nVersion;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoProcessorComponent::omxCheckHeader(OMX_PTR pHeader, OMX_S32 nHeaderSize)
{
    OMX_U32 nSize;  
    OMX_VERSIONTYPE* pVersion = (OMX_VERSIONTYPE*)((OMX_U8*)pHeader + sizeof(OMX_U32));
    if(pHeader == NULL)
        return OMX_ErrorBadParameter;
        
    nSize = *((OMX_U32*)pHeader);
    if(nSize != (OMX_U32)nHeaderSize)
        LOG_ERROR("Header Size wrong\n");

    if(pVersion->nVersion !=  ComponentVersion.nVersion)
        LOG_ERROR("Version  wrong\n");
    
    return OMX_ErrorNone;
}

VideoProcessorComponent::VideoProcessorComponent()
{
    fsl_osal_strcpy((fsl_osal_char*)name, VP_NAME);
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    bInContext = OMX_FALSE;
    nPorts = 2;

    bStarted = OMX_FALSE;
    bReset = OMX_FALSE;
    bFlush = OMX_FALSE;
    bUseEGLImage = OMX_FALSE;
    fsl_osal_memset(&ipu_handle,0,sizeof(ipu_lib_handle_t));
    fsl_osal_memset(&sInParam,0,sizeof(ipu_lib_input_param_t));
    fsl_osal_memset(&sOutParam,0,sizeof(ipu_lib_output_param_t));	
}

OMX_ERRORTYPE VideoProcessorComponent::UseEGLImage(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex, 
        OMX_PTR pAppPrivate, 
        void* eglImage)
{
    bUseEGLImage = OMX_TRUE;
    return CurState->UseEGLImage(ppBufferHdr, nPortIndex, pAppPrivate, eglImage);
}

OMX_ERRORTYPE  VideoProcessorComponent::InitComponent()
{

    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    /**
     * Initialising inport information 
     * video raw data in YUV420 format
     */

    VP_DEBUG(" Begin InitComponent\n");
    fsl_osal_memset(&sPortDef, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    sPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nVersion.nVersion = VP_COMP_VERSION;
    sPortDef.nPortIndex = VP_IN_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.nBufferCountActual = 1;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferSize = 0;
    sPortDef.eDomain = OMX_PortDomainVideo;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    sPortDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    
    eRetVal = ports[VP_IN_PORT]->SetPortDefinition(&sPortDef);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /**
     * Initialising outport information
     * video raw data in RGB565 format
     */
    sPortDef.nPortIndex = VP_OUT_PORT;
    sPortDef.eDir = OMX_DirOutput;
    sPortDef.format.video.eColorFormat = OMX_COLOR_Format16bitRGB565;
    sPortDef.format.video.nFrameWidth = 1024;
    sPortDef.format.video.nFrameHeight = 768;
    eRetVal = ports[VP_OUT_PORT]->SetPortDefinition(&sPortDef);

    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    pInBufHdr[0]=pInBufHdr[1]= NULL;
    pOutBufHdr[0]=pOutBufHdr[1]= NULL;
    VP_DEBUG(" Exit InitComponent\n");
    return eRetVal;
}

 OMX_ERRORTYPE VideoProcessorComponent::GetParameter(
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR ComponentParameterStructure)
{   
    if(ComponentParameterStructure == NULL)
        return OMX_ErrorBadParameter;

    switch(nParamIndex)
    {
        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
                ports[pPortDef->nPortIndex]->GetPortDefinition(pPortDef);
                break;
            }
        case OMX_IndexParamCompBufferSupplier:
            {
                OMX_PARAM_BUFFERSUPPLIERTYPE *temp = (OMX_PARAM_BUFFERSUPPLIERTYPE*)ComponentParameterStructure;
                /* Need peer port be the buffer supplier */
                if(temp->nPortIndex == VP_IN_PORT)
                    temp->eBufferSupplier = OMX_BufferSupplyOutput;
                else
                    temp->eBufferSupplier = OMX_BufferSupplyUnspecified;
                break;
            }
        default:
            return OMX_ErrorUnsupportedIndex;
    }

    return OMX_ErrorNone;
}


OMX_ERRORTYPE VideoProcessorComponent::SetParameter(
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR ComponentParameterStructure)
{

    OMX_PARAM_BUFFERSUPPLIERTYPE *pBufferSupplierType;

    if(ComponentParameterStructure == NULL)
        return OMX_ErrorBadParameter;

    switch(nIndex)
    {
        case OMX_IndexParamCompBufferSupplier:
            {
                pBufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE *)ComponentParameterStructure;
                 ports[pBufferSupplierType->nPortIndex]->SetSupplierType(pBufferSupplierType->eBufferSupplier);
                break;
            }
        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
                pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
                ports[pPortDef->nPortIndex]->SetPortDefinition( pPortDef);
                break;
            }
        case OMX_IndexParamVideoPortFormat:
            break;
        default:
            return OMX_ErrorUnsupportedIndex;
    }

    return OMX_ErrorNone;
}


OMX_ERRORTYPE  VideoProcessorComponent::GetConfig(
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_INOUT OMX_PTR pComponentConfigStructure)

{

    if(pComponentConfigStructure == NULL)
        return OMX_ErrorBadParameter;

    switch(nIndex)
    {
        case OMX_IndexConfigCommonInputCrop:
            {
                fsl_osal_memcpy(pComponentConfigStructure, &sInRect, sizeof(OMX_CONFIG_RECTTYPE));
                break;
            }
	 case OMX_IndexConfigCommonOutputCrop:
            {
                fsl_osal_memcpy(pComponentConfigStructure, &sOutRect, sizeof(OMX_CONFIG_RECTTYPE));
                break;
            }

        default:
            return OMX_ErrorUnsupportedIndex;
    }
    return OMX_ErrorNone;
}


OMX_ERRORTYPE VideoProcessorComponent::SetConfig(
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentConfigStructure)
{
    if(pComponentConfigStructure == NULL)
        return OMX_ErrorBadParameter;

    switch(nIndex)
    {
        case OMX_IndexConfigCommonInputCrop:
            {
                fsl_osal_memcpy(&sInRect,(OMX_CONFIG_RECTTYPE *)pComponentConfigStructure, sizeof(OMX_CONFIG_RECTTYPE));
                if(bStarted == OMX_TRUE)
                    bReset = OMX_TRUE;
                break;
            }
	 case OMX_IndexConfigCommonOutputCrop:
            {
                fsl_osal_memcpy(&sOutRect,(OMX_CONFIG_RECTTYPE *)pComponentConfigStructure, sizeof(OMX_CONFIG_RECTTYPE));
                if(bStarted == OMX_TRUE)
                    bReset = OMX_TRUE;
                break;
            }	
        case OMX_IndexConfigCommonRotate:
            {
                fsl_osal_memcpy(&sRotation,(OMX_CONFIG_ROTATIONTYPE *)pComponentConfigStructure, sizeof(OMX_CONFIG_ROTATIONTYPE));
                break;
            }
        default:
            return OMX_ErrorUnsupportedIndex;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoProcessorComponent::DeInitComponent()
{
    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;
    return eRetVal;
}

OMX_ERRORTYPE VideoProcessorComponent::ComponentReturnBuffer(OMX_U32 nPortIndex)
{
    if(nPortIndex == VP_IN_PORT ) {
        if( pInBufHdr[0] != NULL){  
            ports[VP_IN_PORT]->SendBuffer(pInBufHdr[0]);
            pInBufHdr[0] = NULL;
        }

        if( pInBufHdr[1] != NULL){  
            ports[VP_IN_PORT]->SendBuffer(pInBufHdr[1]);
            pInBufHdr[1] = NULL;
        }
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoProcessorComponent::FlushComponent(OMX_U32 nPortIndex)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoProcessorComponent::VideoProcessor_init( )
{
    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BUFFERHEADERTYPE *pBufHdr;
    OMX_U32 nEGLImageWidth, nEGLImageHeight;
    int mode, ret;

    VP_DEBUG(" Begin Init\n");
    /* Get In/Out port format */
    omxSetHeader(&sPortDef,sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    sPortDef.nPortIndex = VP_IN_PORT;
    ports[VP_IN_PORT]->GetPortDefinition( &sPortDef);
    nInWidth = sPortDef.format.video.nFrameWidth;
    nInHeight = sPortDef.format.video.nFrameHeight;
    eInColorFmt = sPortDef.format.video.eColorFormat;

    sPortDef.nPortIndex = VP_OUT_PORT;
    ports[VP_OUT_PORT]->GetPortDefinition(&sPortDef);
    nOutWidth = sPortDef.format.video.nFrameWidth;
    nOutHeight = sPortDef.format.video.nFrameHeight;
    eOutColorFmt = sPortDef.format.video.eColorFormat;

    if(sInRect.nWidth==0 && sInRect.nHeight==0)
    {
        sInRect.nWidth = nInWidth;
        sInRect.nHeight = nInHeight;
    }
	
    if( nOutWidth == 0 ||nOutHeight ==0 ){
        nOutWidth = 1024;
        sPortDef.format.video.nFrameWidth = nOutWidth;
        nOutHeight =  768;
        sPortDef.format.video.nFrameHeight = nOutHeight;
	 eOutColorFmt = OMX_COLOR_Format16bitRGB565;
	 sPortDef.format.video.eColorFormat = eOutColorFmt;
        ports[VP_OUT_PORT]->SetPortDefinition(&sPortDef);
    }
	
    /* set ipu task in parameter */
    sInParam.width = nInWidth;
    sInParam.height = nInHeight;
    sInParam.fmt = omx2ipu_pxlfmt(eInColorFmt);
    sInParam.input_crop_win.pos.x = (sInRect.nLeft + 7)/8*8;
    sInParam.input_crop_win.pos.y = sInRect.nTop;
    sInParam.input_crop_win.win_w = (sInRect.nWidth 
                                                - (sInParam.input_crop_win.pos.x - sInRect.nLeft))/8*8;
    sInParam.input_crop_win.win_h = sInRect.nHeight;
    
    OMX_PTR pPhyAddr;
    ports[VP_IN_PORT]->GetBuffer( &pBufHdr);
        GetHwBuffer(pBufHdr->pBuffer, &pPhyAddr);
    sInParam.user_def_paddr[0] = (int)pPhyAddr;
    pInBufHdr[0] = pBufHdr;
    VP_DEBUG("VP Inbuffer 0 %p, offset %p\n",pBufHdr,pPhyAddr);
    ports[VP_IN_PORT]->GetBuffer( &pBufHdr);
        GetHwBuffer(pBufHdr->pBuffer, &pPhyAddr);
    sInParam.user_def_paddr[1] = (int)pPhyAddr;
    pInBufHdr[1] = pBufHdr;
    VP_DEBUG("VP Inbuffer 1 %p, offset %p\n",pBufHdr,pPhyAddr);
	
    VP_DEBUG("VP sInParam width %d, height %d,crop x %d, y %d, w %d, h %d, color %d\n",
        sInParam.width,sInParam.height ,
        sInParam.input_crop_win.pos.x,sInParam.input_crop_win.pos.y,
        sInParam.input_crop_win.win_w,sInParam.input_crop_win.win_h,
        sInParam.fmt
        );  

    if(bUseEGLImage == OMX_TRUE){
        ports[VP_OUT_PORT]->GetBuffer( &pBufHdr);
        EGLImageKHRInternal * eglImage = (EGLImageKHRInternal *)pBufHdr->pPlatformPrivate;
        sOutParam.user_def_paddr[0] = (int)eglImage->phyAddr;
        pOutBufHdr[0] = pBufHdr;

        nEGLImageWidth = eglImage->width;
        nEGLImageHeight = eglImage->height;
        OMX_S32 nImageSize = nEGLImageWidth * nEGLImageHeight * eglImage->bitsPerPixel / 8;
        fsl_osal_memset(eglImage->virtAddr, 0, nImageSize);

        ports[VP_OUT_PORT]->GetBuffer( &pBufHdr);
        eglImage = (EGLImageKHRInternal *)pBufHdr->pPlatformPrivate;
        sOutParam.user_def_paddr[1] = (int)eglImage->phyAddr;
        pOutBufHdr[1] = pBufHdr;
        fsl_osal_memset(eglImage->virtAddr,0,nImageSize);
    }
	
    /* set ipu task out parameter */
    sOutParam.width = nOutWidth;
    sOutParam.height = nOutHeight;
    sOutParam.fmt = omx2ipu_pxlfmt(eOutColorFmt);

    if(sOutRect.nWidth==0 && sOutRect.nHeight==0)
    {
        sOutRect.nWidth = nOutWidth;
        sOutRect.nHeight = nOutHeight;
    }
	
    /* calc out pos according to in fmt*/
    if(sInParam.input_crop_win.win_w * sOutRect.nHeight
            >= sInParam.input_crop_win.win_h * sOutRect.nWidth) {
        /* keep target width*/
        OMX_U32 height = sOutRect.nWidth
                         * sInParam.input_crop_win.win_h / sInParam.input_crop_win.win_w;
        sOutParam.output_win.win_w = sOutRect.nWidth;
        sOutParam.output_win.win_h = height;
        sOutParam.output_win.pos.x = 0;
        sOutParam.output_win.pos.y = (sOutRect.nHeight - height)/2;
    }
    else {
        /* keep target height*/
        OMX_U32 width = sOutRect.nHeight
                        * sInParam.input_crop_win.win_w / sInParam.input_crop_win.win_h;
        sOutParam.output_win.win_w = width;
        sOutParam.output_win.win_h = sOutRect.nHeight;
        sOutParam.output_win.pos.x = (( sOutRect.nWidth - width)/2 + 7)/8*8;
        sOutParam.output_win.pos.y = 0;
    }

    if(bUseEGLImage == OMX_TRUE){
        sOutParam.width = nEGLImageWidth;
        sOutParam.height = nEGLImageHeight;
    }

    VP_DEBUG("VP sOutParam width %d, height %d,crop x %d, y %d, w %d, h %d,color %d\n",
        sOutParam.width,sOutParam.height ,
        sOutParam.output_win.pos.x,sOutParam.output_win.pos.y,
        sOutParam.output_win.win_w,sOutParam.output_win.win_h,
        sOutParam.fmt
        );  
    /* ipu lib task init */
    mode = OP_STREAM_MODE;
    ret = mxc_ipu_lib_task_init(&sInParam, NULL, &sOutParam, mode, &ipu_handle);
    if(ret < 0) {
        VP_ERROR("mxc_ipu_lib_task_init failed!\n");
        return OMX_ErrorHardware;
    }
    nInFrames = 0;
    nOutFrames = 0;
    VP_DEBUG(" Exit Init\n");
    return eRetVal;
}

OMX_ERRORTYPE VideoProcessorComponent::VideoProcessor_DeInit( )
{
    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;

    /* return input buffers queued in processing queue */
    if(pInBufHdr[0] != NULL){
        ports[VP_IN_PORT]->SendBuffer(pInBufHdr[0]);
        pInBufHdr[0] = NULL;
    }
    if(pInBufHdr[1] != NULL){
        ports[VP_IN_PORT]->SendBuffer(pInBufHdr[1]);
        pInBufHdr[1] = NULL;
    }

    if(bUseEGLImage == OMX_TRUE){
        if(pOutBufHdr[0] != NULL){
            ports[VP_OUT_PORT]->SendBuffer(pOutBufHdr[0]);
            pOutBufHdr[0] = NULL;
        }

        if(pOutBufHdr[1] != NULL){
            ports[VP_OUT_PORT]->SendBuffer(pOutBufHdr[1]);
            pOutBufHdr[1] = NULL;
        }
    }

    bUseEGLImage = OMX_FALSE;
    mxc_ipu_lib_task_uninit(&ipu_handle);

    return eRetVal;
}

OMX_ERRORTYPE VideoProcessorComponent::ProcessDataBuffer()
{
    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufHdr;

    if(bReset == OMX_TRUE) {
        VideoProcessor_DeInit();
        bStarted = OMX_FALSE;
        bReset = OMX_FALSE;
    }

    if(bStarted == OMX_FALSE) {
        /* ipu lib task init after get 2 frames */
        OMX_U32 nInBufCntMin =2, nOutBufCntMin = 1;      

        if(bUseEGLImage == OMX_TRUE){
            nOutBufCntMin =2;
        }

        if(ports[VP_IN_PORT]->BufferNum() >= nInBufCntMin && ports[VP_OUT_PORT]->BufferNum() >= nOutBufCntMin) {
            eRetVal = VideoProcessor_init();
            if(eRetVal != OMX_ErrorNone) {
                SendEvent(OMX_EventError, eRetVal, 0, NULL);
                return eRetVal;
            }

            eRetVal = VideoProcessor_ProcessData(NULL);
            bStarted = OMX_TRUE;
        }
        else
            eRetVal = OMX_ErrorNoMore;

    }   else {
        if(ports[VP_IN_PORT]->BufferNum() > 0 && ports[VP_OUT_PORT]->BufferNum() > 0) {
            ports[VP_IN_PORT]->GetBuffer( &pBufHdr);
            pInBufHdr[1] = pBufHdr;
            nInFrames++;

            if(bUseEGLImage == OMX_TRUE){
                ports[VP_OUT_PORT]->GetBuffer( &pBufHdr);
                pOutBufHdr[1]  = pBufHdr;
            }

            /* check if need to flush previous frame */
            if(pInBufHdr[1]->nFlags & OMX_BUFFERFLAG_STARTTIME)
                bFlush = OMX_TRUE;

            OMX_PTR pPhyAddr;
            GetHwBuffer(pInBufHdr[1]->pBuffer, &pPhyAddr);
            eRetVal = VideoProcessor_ProcessData(pPhyAddr);
        }
        else
            eRetVal = OMX_ErrorNoMore;
    }

    return eRetVal;
}


OMX_ERRORTYPE VideoProcessorComponent::VideoProcessor_ProcessData( OMX_PTR buf)
{
    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pOutBuffer;
    OMX_U32 size = 0;
    int ret;

    VP_DEBUG(" Process data %p\n",buf);

    /* call ipu task */
    ret = mxc_ipu_lib_task_buf_update(&ipu_handle, (int)buf, NULL, NULL, ipu_output_cb, this);
    if(ret < 0) {
        VP_ERROR("mxc_ipu_lib_task_buf_update failed.\n");
        SendEvent(OMX_EventError, OMX_ErrorHardware, 0, NULL);
        return OMX_ErrorHardware;
    }

    /* check if processing buffer is empty */
    if(pInBufHdr[0] == NULL) {
        VP_ERROR("processing buffer queue is empty.\n");
        SendEvent(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
        return OMX_ErrorUndefined;
    }

    if(bUseEGLImage == OMX_TRUE) {
        pOutBuffer = pOutBufHdr[0];
        pOutBufHdr[0] = pOutBufHdr[1];
        pOutBufHdr[1] = NULL;
    }
    else
        ports[VP_OUT_PORT]->GetBuffer(&pOutBuffer);

    pOutBuffer->nTimeStamp = pInBufHdr[0]->nTimeStamp;
    pOutBuffer->nFlags = pInBufHdr[0]->nFlags;

    /* return processed input buffer */
    ports[VP_IN_PORT]->SendBuffer(pInBufHdr[0]);
    pInBufHdr[0] = pInBufHdr[1];
    pInBufHdr[1] = NULL;

    if(pInBufHdr[0]->nFlags & OMX_BUFFERFLAG_EOS) {
        printf("VPP send EOS buffer.\n");
        pOutBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
    }

    /* check if need to flush previous frame */
    if(bFlush == OMX_FALSE) {
        size = (nOutWidth&(~7)) * nOutHeight * pxlfmt2bpp(eOutColorFmt)/8;
        if(bUseEGLImage != OMX_TRUE) 
            fsl_osal_memcpy(pOutBuffer->pBuffer, ipu_handle.outbuf_start[nOutBufIdx], size);
        pOutBuffer->nFilledLen = size;
        pOutBuffer->nOffset = 0;
        nOutFrames++ ; 

        //printf("Send eglImage: %p\n", pOutBuffer->pPlatformPrivate);

        ports[VP_OUT_PORT]->SendBuffer(pOutBuffer);
    }
    else
        bFlush = OMX_FALSE;

    VP_DEBUG(" Exit process data\n");
    return eRetVal;
}

OMX_ERRORTYPE VideoProcessorComponent::InstanceDeInit()
{
    if(bStarted == OMX_TRUE) {
        VideoProcessor_DeInit();
        bStarted = OMX_FALSE;
    }

    return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE VideoProcessor_ComponentInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        VideoProcessorComponent *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(VideoProcessorComponent,());
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
