/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "VideoRender.h"

#define MAX_DELAY 20000 //20ms

VideoRender::VideoRender()
{
    bInContext = OMX_FALSE;
    nPorts = NUM_PORTS;
}

OMX_ERRORTYPE VideoRender::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BUFFERSUPPLIERTYPE SupplierType;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = IN_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainVideo;
    fsl_osal_memcpy(&sPortDef.format.video, &sVideoFmt, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = nFrameBufferMin;
    sPortDef.nBufferCountActual = nFrameBufferActual;
    sPortDef.nBufferSize = sPortDef.format.video.nFrameWidth
        * sPortDef.format.video.nFrameHeight
        * pxlfmt2bpp(sVideoFmt.eColorFormat) / 8;
    ret = ports[IN_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for in port failed.\n");
        return ret;
    }

    ports[IN_PORT]->SetSupplierType(TunneledSupplierType);

    sPortDef.nPortIndex = CLK_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainOther;
    sPortDef.format.other.eFormat = OMX_OTHER_FormatTime;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_FALSE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 1;
    sPortDef.nBufferSize = sizeof(OMX_TIME_MEDIATIMETYPE);
    ret = ports[CLK_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for clk port failed.\n");
        return ret;
    }

    SupplierType = OMX_BufferSupplyOutput;
    ports[CLK_PORT]->SetSupplierType(SupplierType);

    fsl_osal_memset(&hClock, 0, sizeof(TUNNEL_INFO));
    ClockState = OMX_TIME_ClockStateStopped;
    ClockScale = Q16_SHIFT;
    pSyncFrame = NULL;
    nFrameCnt = nDropCnt = nDeviceDropCnt = nContiniousDrop = 0;

    InitVideoVisitors();

    ret = InitRenderComponent();
    if(ret != OMX_ErrorNone)
        return ret;

    return ret;
}

OMX_ERRORTYPE VideoRender::DeInitComponent()
{
    DeInitRenderComponent();
    DeInitVideoVisitors();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::InitRenderComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::DeInitRenderComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::DoLoaded2Idle()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = OpenDevice();
    if(ret != OMX_ErrorNone) {
        CloseDevice();
        SendEvent(OMX_EventError, ret, 0, NULL);
        return ret;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::DoIdle2Loaded()
{
    CloseDevice();
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::GetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexConfigCommonRotate:
            {
                OMX_CONFIG_ROTATIONTYPE *pRotate;
                pRotate = (OMX_CONFIG_ROTATIONTYPE*)pStructure;
                CHECK_STRUCT(pRotate, OMX_CONFIG_ROTATIONTYPE, ret);
                pRotate->nRotation = eRotation;
            }
            break;
        case OMX_IndexConfigCommonMirror:
            ret = OMX_ErrorNotImplemented;
            break;
        case OMX_IndexConfigCommonScale:
            ret = OMX_ErrorNotImplemented;
            break;
        case OMX_IndexConfigCommonInputCrop:
            {
                OMX_CONFIG_RECTTYPE *pRect;
                pRect = (OMX_CONFIG_RECTTYPE*)pStructure;
                CHECK_STRUCT(pRect, OMX_CONFIG_RECTTYPE, ret);
                fsl_osal_memcpy(pRect, &sRectIn, sizeof(OMX_CONFIG_RECTTYPE));
            }
            break;
        default:
            ret = RenderGetConfig(nParamIndex, pStructure);
            break;
    }

    return ret;
}

OMX_ERRORTYPE VideoRender::SetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        case OMX_IndexConfigCommonRotate:
            {
                OMX_CONFIG_ROTATIONTYPE *pRotate;
                pRotate = (OMX_CONFIG_ROTATIONTYPE*)pStructure;
                CHECK_STRUCT(pRotate, OMX_CONFIG_ROTATIONTYPE, ret);
                eRotation = (ROTATION)pRotate->nRotation;
                SetDeviceRotation();
            }
            break;
        case OMX_IndexConfigCommonMirror:
            ret = OMX_ErrorNotImplemented;
            break;
        case OMX_IndexConfigCommonScale:
            ret = OMX_ErrorNotImplemented;
            break;
        case OMX_IndexConfigCommonInputCrop:
            {
                OMX_CONFIG_RECTTYPE *pRect;
                pRect = (OMX_CONFIG_RECTTYPE*)pStructure;
                CHECK_STRUCT(pRect, OMX_CONFIG_RECTTYPE, ret);
                fsl_osal_memcpy(&sRectIn, pRect, sizeof(OMX_CONFIG_RECTTYPE));
                SetDeviceInputCrop();
            }
            break;
        default:
            ret = RenderSetConfig(nParamIndex, pStructure);
            break;
    }

    return ret;
}

OMX_ERRORTYPE VideoRender::RenderGetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure)
{
    return OMX_ErrorUnsupportedIndex;
}

OMX_ERRORTYPE VideoRender::RenderSetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure)
{
    return OMX_ErrorUnsupportedIndex;
}

OMX_ERRORTYPE VideoRender::ProcessDataBuffer()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;

    if(ports[IN_PORT]->BufferNum() == 0)
        return OMX_ErrorNoMore;

    if(pSyncFrame != NULL)
        return OMX_ErrorNoMore;

    ports[IN_PORT]->GetBuffer(&pBufferHdr);

    if(nFrameCnt == 0 && ports[CLK_PORT]->IsEnabled() == OMX_TRUE) {
        OMX_TIME_CONFIG_CLOCKSTATETYPE sState;
        OMX_INIT_STRUCT(&sState, OMX_TIME_CONFIG_CLOCKSTATETYPE);
        ports[CLK_PORT]->GetTunneledInfo(&hClock);
        OMX_GetConfig(hClock.hTunneledComp, OMX_IndexConfigTimeClockState, &sState);
        ClockState = sState.eState;
    }

    nFrameCnt ++;

    LOG_DEBUG("VideoRender get bufer: %p:%lld:%x\n", 
            pBufferHdr->pBuffer, pBufferHdr->nTimeStamp, pBufferHdr->nFlags);

    if(ports[CLK_PORT]->IsEnabled() == OMX_TRUE)
        ret = SyncFrame(pBufferHdr);
    else
        ret = RenderFrame(pBufferHdr);

    return ret;
}

OMX_ERRORTYPE VideoRender::ProcessClkBuffer()
{
    OMX_BUFFERHEADERTYPE *pClkBuffer = NULL;
    OMX_TIME_MEDIATIMETYPE *pUpdate = NULL;
    OMX_TIME_UPDATETYPE eType;

    if(ports[CLK_PORT]->IsEnabled() != OMX_TRUE)
        return OMX_ErrorNoMore;

    if(ports[CLK_PORT]->BufferNum() == 0)
        return OMX_ErrorNoMore;

    ports[CLK_PORT]->GetBuffer(&pClkBuffer);
    pUpdate = (OMX_TIME_MEDIATIMETYPE*)pClkBuffer->pBuffer;
    eType = pUpdate->eUpdateType;

    switch(eType) {
        case OMX_TIME_UpdateClockStateChanged:
            ClockState = pUpdate->eState;
            if(ClockState == OMX_TIME_ClockStateRunning 
                    || ClockState == OMX_TIME_ClockStateStopped) {
                if(pSyncFrame != NULL) {
                    SyncRequest(pSyncFrame);
                    pSyncFrame = NULL;
                }
            }
            break;
        case OMX_TIME_UpdateScaleChanged:
            ClockScale = pUpdate->xScale;
            break;
        case OMX_TIME_UpdateRequestFulfillment:
            {
                OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
                pBufferHdr = (OMX_BUFFERHEADERTYPE*)pUpdate->nClientPrivate;
                if(pBufferHdr->nFlags & OMX_BUFFERFLAG_EOS) {
                    nDeviceDropCnt = GetDeviceDropFames();
                    LOG_INFO("Total frames: %d, Total Droped frames: %d, Render device dropped frames: %d\n", 
                            nFrameCnt, nDropCnt + nDeviceDropCnt, nDeviceDropCnt);
                    RenderFrame(pBufferHdr);
                    SendEvent(OMX_EventBufferFlag, IN_PORT, OMX_BUFFERFLAG_EOS, NULL);
                    break;
                }

                if(OMX_TRUE == IsDropFrame(pBufferHdr->nTimeStamp, pUpdate->nMediaTimestamp)) {
                    LOG_DEBUG("Buffer is late, drop. Buffer ts: %lld, Media ts: %lld\n",
                            pBufferHdr->nTimeStamp, pUpdate->nMediaTimestamp);
                    nDropCnt ++;
                    pBufferHdr->nTimeStamp = -1;
                    ports[IN_PORT]->SendBuffer(pBufferHdr);
                    break;
                }
                RenderFrame(pBufferHdr);
            }
            break;
        default :
            break;
    }

    ports[CLK_PORT]->SendBuffer(pClkBuffer);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::SyncFrame(
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ports[CLK_PORT]->GetTunneledInfo(&hClock);
    if(pBufferHdr->nFlags & OMX_BUFFERFLAG_STARTTIME) {
        OMX_TIME_CONFIG_TIMESTAMPTYPE sStartTime;
        OMX_INIT_STRUCT(&sStartTime, OMX_TIME_CONFIG_TIMESTAMPTYPE);
        sStartTime.nPortIndex = hClock.nTunneledPort;
        sStartTime.nTimestamp = pBufferHdr->nTimeStamp;
        ret = OMX_SetConfig(hClock.hTunneledComp, OMX_IndexConfigTimeClientStartTime, &sStartTime);
        if(ret == OMX_ErrorNone) {
            pSyncFrame = pBufferHdr;
            return OMX_ErrorNone;
        }
    }

    SyncRequest(pBufferHdr);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::SyncRequest(
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(ClockState == OMX_TIME_ClockStateRunning) {
        if(pBufferHdr->nFlags & OMX_BUFFERFLAG_STARTTIME ||
                ClockScale < (OMX_S32)(MIN_RATE*Q16_SHIFT) || ClockScale > (OMX_S32)(MAX_RATE*Q16_SHIFT)) {
            OMX_TIME_CONFIG_TIMESTAMPTYPE sRefTime;
            OMX_INIT_STRUCT(&sRefTime, OMX_TIME_CONFIG_TIMESTAMPTYPE);
            sRefTime.nPortIndex = hClock.nTunneledPort;
            sRefTime.nTimestamp = pBufferHdr->nTimeStamp;
            OMX_SetConfig(hClock.hTunneledComp, OMX_IndexConfigTimeCurrentVideoReference, &sRefTime);
        }

        OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE sRequest;
        OMX_INIT_STRUCT(&sRequest, OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE);
        sRequest.nPortIndex = hClock.nTunneledPort;
        sRequest.nMediaTimestamp = pBufferHdr->nTimeStamp;
        sRequest.pClientPrivate = (OMX_PTR)pBufferHdr;
        do {
            ret = OMX_SetConfig(hClock.hTunneledComp, OMX_IndexConfigTimeMediaTimeRequest, &sRequest);
            if (OMX_TRUE == bHasCmdToProcess())
                break;
        } while (ret == OMX_ErrorNotReady);
        if(ret != OMX_ErrorNone)
            ports[IN_PORT]->SendBuffer(pBufferHdr);
    }
    else
        ports[IN_PORT]->SendBuffer(pBufferHdr);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE VideoRender::RenderFrame(
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    Report2VideoVisitors(pBufferHdr);
    WriteDevice(pBufferHdr);
    return ret;
}

#define DROP_TH_CNT 6
static OMX_TICKS drop_th[DROP_TH_CNT] = {
    300000,
    400000,
    500000,
    600000,
    700000,
    800000,
};

static OMX_U32 frame_th[DROP_TH_CNT] = {
    1, 2, 3, 4, 5, 6
};

OMX_BOOL VideoRender::IsDropFrame(
        OMX_TICKS ts, 
        OMX_TICKS meida)
{
    OMX_BOOL ret = OMX_FALSE;
    OMX_TICKS diff = meida - ts;

    if(diff > MAX_DELAY) {
        OMX_S32 i;
        for(i=0; i<DROP_TH_CNT; i++) {
            if(diff < drop_th[i]) {
                if(nContiniousDrop >= frame_th[i]) {
                    nContiniousDrop = 0;
                    return OMX_FALSE;
                }
                else {
                    nContiniousDrop ++;
                    return OMX_TRUE;
                }
            }
        }

        if(i == DROP_TH_CNT) {
            //NotifyClkVideoLate();
            //nContiniousDrop = 0;
            return OMX_TRUE;
        }
    }
    else
        nContiniousDrop = 0;

    return ret;
}

OMX_ERRORTYPE VideoRender::NotifyClkVideoLate()
{
    TUNNEL_INFO sTunnelInfo;
    OMX_TIME_CONFIG_TIMEVIDEOLATE sVideoLate;

    fsl_osal_memset(&sTunnelInfo, 0, sizeof(TUNNEL_INFO));
    ports[CLK_PORT]->GetTunneledInfo(&sTunnelInfo);
    if(sTunnelInfo.hTunneledComp == NULL)
        return OMX_ErrorInvalidComponent;

    OMX_INIT_STRUCT(&sVideoLate, OMX_TIME_CONFIG_TIMEVIDEOLATE);
    sVideoLate.nPortIndex = sTunnelInfo.nTunneledPort;
    sVideoLate.bLate = OMX_TRUE;
    OMX_SetConfig(sTunnelInfo.hTunneledComp, OMX_IndexConfigTimeVideoLate, &sVideoLate);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::SetDeviceRotation()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::SetDeviceInputCrop()
{
    return OMX_ErrorNone;
}

typedef int (*CREATE_ITF)(VFV_INTERFACE *interface);

OMX_ERRORTYPE VideoRender::InitVideoVisitors()
{
    OMX_STRING lib_name = (OMX_STRING)"libfslxec.so";
    OMX_STRING symbol_name = (OMX_STRING)"CreateVFVInterface";
    CREATE_ITF CreateItf = NULL;

    hVisitorLib = NULL;
    hVistor = NULL;
    fsl_osal_memset(&hVistorItf, 0, sizeof(VFV_INTERFACE));

    hVisitorLib = LibMgr.load(lib_name);
    if(hVisitorLib == NULL) {
        LOG_WARNING("Unable to load %s\n", lib_name);
        return OMX_ErrorBadParameter;
    }

    CreateItf = (CREATE_ITF)LibMgr.getSymbol(hVisitorLib, symbol_name);
    if(CreateItf == NULL) {
        LibMgr.unload(hVisitorLib);
        hVisitorLib = NULL;
        LOG_ERROR("Get symbol %s from %s failed.\n", symbol_name, lib_name);
        return OMX_ErrorBadParameter;
    }

    if(CreateItf(&hVistorItf) < 0) {
        LibMgr.unload(hVisitorLib);
        hVisitorLib = NULL;
        LOG_ERROR("Create interface from %s failed.\n", lib_name);
        return OMX_ErrorUndefined;
    }

    hVistor = (*hVistorItf.init)();
    if(hVistor == NULL) {
        LibMgr.unload(hVisitorLib);
        hVisitorLib = NULL;
        LOG_ERROR("init visitor failed, visitor is %s\n", lib_name);
        return OMX_ErrorUndefined;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::DeInitVideoVisitors()
{
    if(hVistor != NULL) {
        (*hVistorItf.deinit)(hVistor);
        hVistor = NULL;
    }

    if(hVisitorLib != NULL) {
        LibMgr.unload(hVisitorLib);
        hVisitorLib = NULL;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoRender::Report2VideoVisitors(OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    VIDEO_FMT fmt = FMT_NONE;

    switch(sVideoFmt.eColorFormat) {
        case OMX_COLOR_FormatYUV420Planar:
            fmt = YUV420Planar;
            break;
        case OMX_COLOR_FormatYUV420PackedPlanar:
            fmt = YUV420PackedPlanar;
            break;
        case OMX_COLOR_FormatYUV420SemiPlanar:
            fmt = YUV420SemiPlanar;
            break;
        case OMX_COLOR_FormatYUV422Planar:
            fmt = YUV422Planar;
            break;
        case OMX_COLOR_FormatYUV422PackedPlanar:
            fmt = YUV422PackedPlanar;
            break;
        case OMX_COLOR_FormatYUV422SemiPlanar:
            fmt = YUV422SemiPlanar;
            break;
        default:
            break;
    }

    if(hVistorItf.report != NULL && hVistor != NULL)
        (*hVistorItf.report)(hVistor, nFrameCnt, sVideoFmt.nFrameWidth, sVideoFmt.nFrameHeight, fmt, (char*)pBufferHdr->pBuffer);

    return OMX_ErrorNone;
}

/* File EOF */
