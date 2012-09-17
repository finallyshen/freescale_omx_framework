/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "WmvDecoder.h"
#include "Mem.h"

#if defined(ICS) && defined(MX5X)
#define ALIGN_STRIDE(x)  (((x)+63)&(~63))
#define ALIGN_CHROMA(x) (((x) + 4095)&(~4095))
#else
#define ALIGN_STRIDE(x)  (((x)+31)&(~31))
#define ALIGN_CHROMA(x) (x)
#endif

static WMV9D_S32 WmvDecGetBitstream (
        WMV9D_S32 s32BufLen, 
        WMV9D_U8* pu8Buf, 
        WMV9D_S32* pEndOfFrame,
        WMV9D_Void* pvAppContext)
{
    WmvDecoder *obj = (WmvDecoder*)pvAppContext;
    return obj->DoGetBitstream(s32BufLen, pu8Buf, (OMX_S32*)pEndOfFrame);
}

WmvDecoder::WmvDecoder()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_decoder.wmv.sw-based");
    ComponentVersion.s.nVersionMajor = 0x0;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x0;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"video_decoder.wmv";

    fsl_osal_memset(&sInFmt, 0, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
    sInFmt.nFrameWidth = 320;
    sInFmt.nFrameHeight = 240;
    sInFmt.xFramerate = 30 * Q16_SHIFT;
    sInFmt.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    sInFmt.eCompressionFormat = OMX_VIDEO_CodingWMV;

    sOutFmt.nFrameWidth = 320;
    sOutFmt.nFrameHeight = 240;
    sOutFmt.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    sOutFmt.eCompressionFormat = OMX_VIDEO_CodingUnused;

    bFilterSupportPartilInput = OMX_FALSE;
    nInBufferCnt = 1;
    nInBufferSize = 128*1024;
    nOutBufferCnt = 1;
    nOutBufferSize = sOutFmt.nFrameWidth * sOutFmt.nFrameHeight * pxlfmt2bpp(sOutFmt.eColorFormat) / 8;

    pInBuffer = pOutBuffer = NULL;
    nInputSize = nInputOffset = 0;
    bInEos = OMX_FALSE;
    nFrameSize = 0;;
    pFrameBuffer = NULL;

    OMX_INIT_STRUCT(&sOutCrop, OMX_CONFIG_RECTTYPE);
    sOutCrop.nPortIndex = OUT_PORT;
    sOutCrop.nLeft = sOutCrop.nTop = 0;
    sOutCrop.nWidth = sInFmt.nFrameWidth;
    sOutCrop.nHeight = sInFmt.nFrameHeight;

    fsl_osal_memset(&sDecObj, 0, sizeof(sWmv9DecObjectType));
    eFormat = E_WMV9D_COMP_FMT_UNSUPPORTED;
    eWmvDecState = WMVDEC_LOADED;
    hLib = NULL;
    libMgr  = NULL;
}

OMX_ERRORTYPE WmvDecoder::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure) 
{ 
    if(nParamIndex ==  OMX_IndexParamVideoWmv) { 
        OMX_VIDEO_PARAM_WMVTYPE  *pPara; 
        pPara = (OMX_VIDEO_PARAM_WMVTYPE *)pComponentParameterStructure; 
        switch(eFormat) { 
            case E_WMV9D_COMP_FMT_WMV7: 
                pPara->eFormat = OMX_VIDEO_WMVFormat7; 
                break; 
            case E_WMV9D_COMP_FMT_WMV8: 
                pPara->eFormat = OMX_VIDEO_WMVFormat8; 
                break;
            case E_WMV9D_COMP_FMT_WMV9:
                pPara->eFormat = OMX_VIDEO_WMVFormat9;
                break;
            default:
                pPara->eFormat = OMX_VIDEO_WMVFormatUnused;
                break;
        }
        return OMX_ErrorNone;
    }
    else
        return OMX_ErrorUnsupportedIndex;
} 

OMX_ERRORTYPE WmvDecoder::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(nParamIndex ==  OMX_IndexParamVideoWmv) {
        OMX_VIDEO_PARAM_WMVTYPE  *pPara;
        pPara = (OMX_VIDEO_PARAM_WMVTYPE *)pComponentParameterStructure;

        switch(pPara->eFormat) {
            case OMX_VIDEO_WMVFormat7:
                eFormat = E_WMV9D_COMP_FMT_WMV7;
                break;
            case OMX_VIDEO_WMVFormat8:
                eFormat = E_WMV9D_COMP_FMT_WMV8;
                break;
            case OMX_VIDEO_WMVFormat9:
                eFormat = E_WMV9D_COMP_FMT_WMV9;
                break;
            default:
                eFormat = E_WMV9D_COMP_FMT_UNSUPPORTED;
                ret = OMX_ErrorBadParameter;
                break;
        }
    }
    else
        ret = OMX_ErrorUnsupportedIndex;

    return ret;
}

OMX_ERRORTYPE WmvDecoder::GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure)
{
    if(nParamIndex == OMX_IndexConfigCommonOutputCrop) {
        OMX_CONFIG_RECTTYPE *pRecConf = (OMX_CONFIG_RECTTYPE*)pComponentParameterStructure;
        if(pRecConf->nPortIndex == OUT_PORT) {
            pRecConf->nTop = sOutCrop.nTop;
            pRecConf->nLeft = sOutCrop.nLeft;
            pRecConf->nWidth = sOutCrop.nWidth;
            pRecConf->nHeight = sOutCrop.nHeight;
        }
        return OMX_ErrorNone;
    }
    else
        return OMX_ErrorUnsupportedIndex;
}

OMX_ERRORTYPE WmvDecoder::SetInputBuffer(
        OMX_PTR pBuffer, 
        OMX_S32 nSize, 
        OMX_BOOL bLast)
{
    if(pBuffer == NULL)
        return OMX_ErrorBadParameter;

    pInBuffer = pBuffer;
    nInputSize = nSize;
    nInputOffset = 0;
    bInEos = bLast;

    if(eWmvDecState == WMVDEC_STOP &&  nSize > 0)
        eWmvDecState = WMVDEC_RUN;

    LOG_DEBUG("wmvdec set input buffer: %p:%d:%d\n", pBuffer, nSize, bInEos);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::SetOutputBuffer(OMX_PTR pBuffer)
{
    if(pBuffer == NULL)
        return OMX_ErrorBadParameter;

    pOutBuffer = pBuffer;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::InitFilter()
{
    int status = E_WMV9D_SUCCESS;


    LOG_DEBUG("%s\n", pWMV9DecCodecVersionInfo());

    status = pWMV9DQuerymem (&sDecObj, sInFmt.nFrameHeight, sInFmt.nFrameWidth);
    if(E_WMV9D_SUCCESS != status)
        return OMX_ErrorUndefined;

    if(OMX_ErrorNone != AllocateDecoderMemory())
        return OMX_ErrorInsufficientResources;

    sDecObj.pfCbkBuffRead = WmvDecGetBitstream;
    sDecObj.pvAppContext = this;
    sDecObj.sDecParam.eCompressionFormat = eFormat;
    sDecObj.sDecParam.s32FrameRate   = sInFmt.xFramerate / Q16_SHIFT;
    sDecObj.sDecParam.s32BitRate     = sInFmt.nBitrate;
    sDecObj.sDecParam.u16FrameWidth  = (WMV9D_U16)sInFmt.nFrameWidth;
    sDecObj.sDecParam.u16FrameHeight = (WMV9D_U16)sInFmt.nFrameHeight;
    sDecObj.sDecParam.sOutputBuffer.tOutputFormat  = IYUV_WMV_PADDED;  

    if(OMX_ErrorNone != AllocateFrameBuffer()) {
        FreeDecoderMemory();
        return OMX_ErrorInsufficientResources;
    }

    status = pWMV9DInit (&sDecObj);
    if(status != E_WMV9D_SUCCESS) {
        FreeFrameBuffer();
        FreeDecoderMemory();
        return OMX_ErrorUndefined;
    }

    eWmvDecState = WMVDEC_RUN;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::DeInitFilter()
{
    pWMV9DFree (&sDecObj);
    FreeDecoderMemory();
    FreeFrameBuffer();
    eWmvDecState = WMVDEC_LOADED;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::InitFilterComponent()
{

    libMgr = FSL_NEW(ShareLibarayMgr, ());
    if(libMgr == NULL)
        return OMX_ErrorInsufficientResources;

    hLib = libMgr->load((OMX_STRING)"lib_WMV789_dec_v2_arm11_elinux.so");
    if(hLib == NULL) {
        LOG_WARNING("Can't load library lib_WMV789_dec_v2_arm11_elinux.\n");
        FSL_DELETE(libMgr);
        return OMX_ErrorComponentNotFound;
    }

    pWMV9DQuerymem = (eWmv9DecRetType (*)(sWmv9DecObjectType* , WMV9D_S32, WMV9D_S32))libMgr->getSymbol(hLib, (OMX_STRING)"eWMV9DQuerymem");
    pWMV9DInit = (eWmv9DecRetType (*)(sWmv9DecObjectType*))libMgr->getSymbol(hLib, (OMX_STRING)"eWMV9DInit");
    pWMV9DDecode = (eWmv9DecRetType (*)(sWmv9DecObjectType*, WMV9D_U32))libMgr->getSymbol(hLib,(OMX_STRING)"eWMV9DDecode");
    pWMV9DFree = (eWmv9DecRetType (*) (sWmv9DecObjectType *))libMgr->getSymbol(hLib,(OMX_STRING)"eWMV9DFree");
    pWMV9DecGetOutputFrame = (eWmv9DecRetType (*) (sWmv9DecObjectType *))libMgr->getSymbol(hLib,(OMX_STRING)"eWMV9DecGetOutputFrame");
    pWMV9DecCodecVersionInfo = (const char * (*) ())libMgr->getSymbol(hLib,(OMX_STRING)"WMV9DecCodecVersionInfo");

    if(pWMV9DQuerymem == NULL
            || pWMV9DInit == NULL
            || pWMV9DDecode == NULL
            || pWMV9DFree == NULL
            || pWMV9DecGetOutputFrame == NULL
            || pWMV9DecCodecVersionInfo == NULL
                ) {
        libMgr->unload(hLib);
        FSL_DELETE(libMgr);
        return OMX_ErrorComponentNotFound;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::DeInitFilterComponent()
{
    if (libMgr) {
        if (hLib)
            libMgr->unload(hLib);
        FSL_DELETE(libMgr);
    }
    return OMX_ErrorNone;
}

FilterBufRetCode WmvDecoder::FilterOneBuffer()
{
    FilterBufRetCode ret = FILTER_OK;

    LOG_DEBUG("eWmvDecState: %d\n", eWmvDecState);

    switch(eWmvDecState) {
        case WMVDEC_LOADED:
            DetectOutputFmt();
            break;
        case WMVDEC_INIT:
            ret = FILTER_DO_INIT;
            break;
        case WMVDEC_RUN:
            ret = DecodeOneFrame();
            break;
        case WMVDEC_STOP:
            break;
        default:
            break;
    }

    return ret;
}

OMX_ERRORTYPE WmvDecoder::GetOutputBuffer(OMX_PTR *ppBuffer,OMX_S32* pOutSize)
{
    //in case: sOutFmt.nFrameWidth != sDecObj.sDecParam.sOutputBuffer.s32YRowSize
    OMX_S32 Ysize = sOutFmt.nFrameWidth * sOutFmt.nFrameHeight;
    OMX_S32 Usize = Ysize/4;
    OMX_S32 i;
    OMX_U8* y = (OMX_U8*)pOutBuffer;
    OMX_U8* u = (OMX_U8*)ALIGN_CHROMA((OMX_U32)pOutBuffer+Ysize);
    OMX_U8* v = (OMX_U8*)ALIGN_CHROMA((OMX_U32)u+Usize);
    OMX_U8* ysrc=sDecObj.sDecParam.sOutputBuffer.pu8YBuf;
    OMX_U8* usrc=sDecObj.sDecParam.sOutputBuffer.pu8CbBuf;
    OMX_U8* vsrc=sDecObj.sDecParam.sOutputBuffer.pu8CrBuf;
    for(i=0;i<nPadHeight;i++)
    {
        fsl_osal_memcpy((OMX_PTR)y, (OMX_PTR)ysrc, sDecObj.sDecParam.sOutputBuffer.s32YRowSize);
        y+=sOutFmt.nFrameWidth;
        ysrc+=sDecObj.sDecParam.sOutputBuffer.s32YRowSize;
    }
    for(i=0;i<nPadHeight/2;i++)
    {
        fsl_osal_memcpy((OMX_PTR)u, (OMX_PTR)usrc, sDecObj.sDecParam.sOutputBuffer.s32CbRowSize);
        fsl_osal_memcpy((OMX_PTR)v, (OMX_PTR)vsrc, sDecObj.sDecParam.sOutputBuffer.s32CrRowSize);
        u+=sOutFmt.nFrameWidth/2;
        v+=sOutFmt.nFrameWidth/2;
        usrc+=sDecObj.sDecParam.sOutputBuffer.s32CbRowSize;
        vsrc+=sDecObj.sDecParam.sOutputBuffer.s32CrRowSize;
    }
    *ppBuffer = pOutBuffer;
    *pOutSize = sOutFmt.nFrameWidth * sOutFmt.nFrameHeight * pxlfmt2bpp(sOutFmt.eColorFormat) / 8;
    pOutBuffer = NULL;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::FlushInputBuffer()
{
    pInBuffer = NULL;
    nInputSize = 0;
    nInputOffset = 0;
    bInEos = OMX_FALSE;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::FlushOutputBuffer()
{
    pOutBuffer = NULL;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::DetectOutputFmt()
{
    OMX_S32 EXPANDY = 48;

    nPadWidth = (sInFmt.nFrameWidth +15)&(~15);
    nPadHeight = (sInFmt.nFrameHeight +15)&(~15);
    nPadWidth += EXPANDY + 16;
    nPadHeight += EXPANDY;   

    nOutBufferCnt = 3;
    sOutFmt.nFrameWidth = ALIGN_STRIDE(nPadWidth);
    sOutFmt.nFrameHeight = ALIGN_STRIDE(nPadHeight);
    nOutBufferSize = sOutFmt.nFrameWidth * sOutFmt.nFrameHeight * pxlfmt2bpp(sOutFmt.eColorFormat) / 8;

    sOutCrop.nLeft = (EXPANDY + 16) >> 1;
    sOutCrop.nTop = (EXPANDY + 16) >> 1;
    sOutCrop.nWidth = sInFmt.nFrameWidth & (~7);
    sOutCrop.nHeight = sInFmt.nFrameHeight;

    LOG_DEBUG("nPadWidth: %d, nPadHeight: %d\n", nPadWidth, nPadHeight);

    VideoFilter::OutputFmtChanged();

    eWmvDecState = WMVDEC_INIT;

    return OMX_ErrorNone;
}

FilterBufRetCode WmvDecoder::DecodeOneFrame()
{
    FilterBufRetCode ret = FILTER_OK;
    int status = E_WMV9D_SUCCESS;

    if(pOutBuffer == NULL)
        return FILTER_NO_OUTPUT_BUFFER;

    if(pInBuffer == NULL && bInEos != OMX_TRUE)
        return FILTER_NO_INPUT_BUFFER;

    if(pInBuffer != NULL && nInputSize > 0)
        status =  pWMV9DDecode (&sDecObj, nInputSize);

    if(status != E_WMV9D_SUCCESS) {
        ret = FILTER_INPUT_CONSUMED;
        if(bInEos == OMX_TRUE)
            ret = (FilterBufRetCode) (ret | FILTER_LAST_OUTPUT);
        else 
            ret = (FilterBufRetCode) (ret | FILTER_SKIP_OUTPUT);

        return ret;
    }

    if(nInputSize == 0)
        ret = FILTER_INPUT_CONSUMED;

    if(bInEos == OMX_TRUE) {
        ret = (FilterBufRetCode) (ret | FILTER_LAST_OUTPUT);
        eWmvDecState = WMVDEC_STOP;
    }

    status = pWMV9DecGetOutputFrame(&sDecObj);
    if(status != E_WMV9D_NO_OUTPUT)
        ret = (FilterBufRetCode) (ret | FILTER_HAS_OUTPUT);

    return ret;
}

OMX_ERRORTYPE WmvDecoder::AllocateDecoderMemory()
{
    sWmv9DecMemAllocInfoType* psMemInfo = &sDecObj.sMemInfo;
    WMV9D_S32     s32Count;
    WMV9D_Void*   pvUnalignedBuf;
    WMV9D_Void*   pvOldBuf;
    WMV9D_S32     s32ExtraSize;
    WMV9D_S32     s32Mask;

    for (s32Count = 0; s32Count < psMemInfo->s32NumReqs; s32Count++)
    {
        sWmv9DecMemBlockType* psMemBlk = psMemInfo->asMemBlks + s32Count;

        /* Get the extra amount to be allocated for the alignment */
        switch (psMemBlk->eMemAlign)
        {
            case E_WMV9D_ALIGN_NONE:
                s32ExtraSize = 0;
                s32Mask = 0xffffffff;
                break;

            case E_WMV9D_ALIGN_HALF_WORD:
                s32ExtraSize = 1;
                s32Mask = 0xfffffffe;
                break;

            case E_WMV9D_ALIGN_WORD:
                s32ExtraSize = 3;
                s32Mask = 0xfffffffc;
                break;

            case E_WMV9D_ALIGN_DWORD:
                s32ExtraSize = 7;
                s32Mask = 0xfffffff8;
                break;

            case E_WMV9D_ALIGN_QWORD:
                s32ExtraSize = 15;
                s32Mask = 0xfffffff0;
                break;

            case E_WMV9D_ALIGN_OCTAWORD:
                s32ExtraSize = 31;
                s32Mask = 0xffffffe0;
                break;

            default :
                s32ExtraSize = -1;  /* error condition */
                s32Mask = 0x00000000;
                printf ("Memory alignment type %d not supported\n",
                        psMemBlk->eMemAlign);
                break;
        }

        /* Save the old pointer, in case memory is being reallocated */
        pvOldBuf = psMemBlk->pvBuffer;

        /* Allocate new memory, if required */
        if (psMemBlk->s32Size > 0)
        {
            if (WMV9D_IS_SLOW_MEMORY(psMemBlk->s32MemType))
                pvUnalignedBuf = FSL_MALLOC (psMemBlk->s32Size + s32ExtraSize);
            else
                pvUnalignedBuf = FSL_MALLOC (psMemBlk->s32Size + s32ExtraSize);

            if(pvUnalignedBuf == NULL)
                return OMX_ErrorInsufficientResources;

            psMemBlk->pvBuffer = (WMV9D_Void*)(((WMV9D_S32)pvUnalignedBuf + s32ExtraSize) & s32Mask);
        }
        else
        {
            pvUnalignedBuf = NULL;
            psMemBlk->pvBuffer = NULL;
        }

        /* Check if the memory is being reallocated */
        if (WMV9D_IS_SIZE_CHANGED(psMemBlk->s32MemType))
        {
            if (psMemBlk->s32OldSize > 0)
            {
                WMV9D_S32  s32CopySize = psMemBlk->s32OldSize;

                if (psMemBlk->s32Size < s32CopySize)
                    s32CopySize = psMemBlk->s32Size;

                if (WMV9D_NEEDS_COPY_AT_RESIZE(psMemBlk->s32MemType))
                    fsl_osal_memcpy (psMemBlk->pvBuffer, pvOldBuf, s32CopySize);

                FSL_FREE (psMemBlk->pvUnalignedBuffer);
            }
        }

        /* Now save the new unaligned buffer pointer */
        psMemBlk->pvUnalignedBuffer = pvUnalignedBuf;

        LOG_DEBUG ("[%2d] size %d buffer %p\n", s32Count, psMemBlk->s32Size, psMemBlk->pvBuffer);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::FreeDecoderMemory()
{
    sWmv9DecMemAllocInfoType* psMemInfo = &sDecObj.sMemInfo;
    WMV9D_S32     s32Count;

    for (s32Count = 0; s32Count < psMemInfo->s32NumReqs; s32Count++)
    {
        if (psMemInfo->asMemBlks[s32Count].s32Size > 0)
            FSL_FREE (psMemInfo->asMemBlks[s32Count].pvUnalignedBuffer);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::AllocateFrameBuffer()
{
    OMX_S32 ySize;

    nFrameSize = nPadWidth * nPadHeight * 3 / 2;
    pFrameBuffer = FSL_MALLOC(nFrameSize * sizeof(WMV9D_U8));
    if(pFrameBuffer == NULL)
        return OMX_ErrorInsufficientResources;
    sDecObj.sDecParam.sOutputBuffer.pu8YBuf = (WMV9D_U8 *)pFrameBuffer;
    if(sDecObj.sDecParam.sOutputBuffer.pu8YBuf == NULL)
        return OMX_ErrorInsufficientResources;

    ySize = nPadWidth * nPadHeight;
    sDecObj.sDecParam.sOutputBuffer.pu8CbBuf = sDecObj.sDecParam.sOutputBuffer.pu8YBuf + ySize;
    sDecObj.sDecParam.sOutputBuffer.pu8CrBuf = sDecObj.sDecParam.sOutputBuffer.pu8CbBuf + ySize / 4;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WmvDecoder::FreeFrameBuffer()
{
    if(pFrameBuffer != NULL)
        FSL_FREE(pFrameBuffer);

    return OMX_ErrorNone;
}

OMX_S32 WmvDecoder::DoGetBitstream(
        OMX_U32 nLen, 
        OMX_U8 *pBuffer, 
        OMX_S32 *pEndOfFrame)
{
    OMX_S32 nRead = -1;

    if(nCodecDataLen > 0) {
        fsl_osal_memcpy(pBuffer, pCodecData, nCodecDataLen);
        nRead = nCodecDataLen;
        nCodecDataLen = 0;
        *pEndOfFrame = 1;
        return nRead;
    }

    if(nInputSize > nLen) {
        fsl_osal_memcpy(pBuffer, (OMX_PTR)((OMX_U32)pInBuffer + nInputOffset), nLen);
        nInputOffset += nLen;
        nInputSize -= nLen;
        nRead = nLen;
        *pEndOfFrame = 0;
    }
    else {
        fsl_osal_memcpy(pBuffer, (OMX_PTR)((OMX_U32)pInBuffer + nInputOffset), nInputSize);
        nRead = nInputSize;
        nInputOffset += nInputSize;
        nInputSize = 0;
        *pEndOfFrame = 1;
    }

    return nRead;
}

/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE WmvDecoderInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        WmvDecoder *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(WmvDecoder, ());
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
