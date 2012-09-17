/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <dlfcn.h>
#include <unistd.h>
#include "OMX_Implement.h"
#include "PlatformResourceMgrItf.h"
#include "fsl_osal.h"
#include "Mem.h"
#include "Log.h"

#include "OMX_Core.h"
#include "OMX_Component.h"    
#include "Queue.h"

#define MAX_PORT_NUM 8
#define MAX_PORT_BUFFER 32
#define FRAME_DURATION 40000 

typedef enum {
    EVENT,
    EMPTY_DONE,
    FILL_DONE,
    EXIT
}MSG_TYPE;

typedef enum {
    SCREEN_NONE,
    SCREEN_STD,
    SCREEN_FUL,
    SCREEN_ZOOM
}SCREENTYPE;

typedef struct {
    OMX_EVENTTYPE eEvent;
    OMX_U32 nData1;
    OMX_U32 nData2;
    OMX_PTR pEventData;
}EVENT_TYPE;

typedef struct {
    OMX_BUFFERHEADERTYPE* pBuffer;
}BUFFER;

typedef struct _MSG {
    OMX_HANDLETYPE hComponent;
    MSG_TYPE type;
    union {
        EVENT_TYPE event;
        BUFFER buffer;
    }data;
}MSG;

typedef struct _HTEST {
    OMX_COMPONENTTYPE *hComponent;
    OMX_COMPONENTTYPE *hClk;
    OMX_STRING name;
    OMX_U32 nPorts;
    OMX_BOOL bAllocater[MAX_PORT_NUM];
    OMX_U32 nBufferHdr[MAX_PORT_NUM];
    OMX_DIRTYPE PortDir[MAX_PORT_NUM];
    OMX_BUFFERHEADERTYPE *pBufferHdr[MAX_PORT_NUM][MAX_PORT_BUFFER];
    Queue *pMsgQ;
    EVENT_TYPE sCmdDone;
    fsl_osal_ptr pThreadId;
    OMX_BOOL bStop;
    OMX_BOOL bError;
    OMX_BOOL bHoldBuffers;
    OMX_TICKS ts;
    SCREENTYPE eScreen;
    OMX_CONFIG_RECTTYPE sRectIn;
    OMX_CONFIG_RECTTYPE sRectOut;
    OMX_CONFIG_RECTTYPE sDispWindow;
    FILE *pInFile;
    OMX_CONFIG_OUTPUTMODE sOutputMode;
    OMX_CONFIG_ROTATIONTYPE sRotate;
    OMX_BOOL bRotate;
}HTEST;

OMX_ERRORTYPE add_clock(HTEST *hTest);
OMX_ERRORTYPE remove_clock(HTEST *hTest);
OMX_ERRORTYPE start_data_process(HTEST *hTest);
void * process_thread(void *ptr);

OMX_ERRORTYPE read_data(HTEST *hTest, OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_U32 size = 0;
    OMX_PTR pBuffer;
    OMX_U32 length;
    OMX_PTR pPhyiscAddr = NULL;

    pBuffer = pBufferHdr->pBuffer;
    length = pBufferHdr->nAllocLen;

#if 0
    GetHwBuffer(pBuffer, &pPhyiscAddr);
    printf("Phy addr: %p\n", pPhyiscAddr);
#endif

    size = fread(pBuffer, 1, length, hTest->pInFile);
    pBufferHdr->nFilledLen = size;
    pBufferHdr->nOffset = 0;
    pBufferHdr->nTimeStamp = hTest->ts;
    hTest->ts += 40000;

    if(size < length) {
        pBufferHdr->nFlags = OMX_BUFFERFLAG_EOS;
        hTest->bStop = OMX_TRUE;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE write_data(HTEST *hTest, OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    return OMX_ErrorNone;
}


OMX_ERRORTYPE gm_EventHandler(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    MSG sMsg;
    HTEST *hTest = (HTEST*)pAppData;

    printf("Client receive event [eEvent:%d, nData1: %x, nData2: %d]\n",
            (int)eEvent, nData1, (int)nData2);

    if(hComponent == hTest->hComponent) {
        if(eEvent == OMX_EventCmdComplete) {
            hTest->sCmdDone.eEvent = OMX_EventCmdComplete;
            hTest->sCmdDone.nData1 = nData1;
            hTest->sCmdDone.nData2 = nData2;
            hTest->sCmdDone.pEventData = pEventData;
        }
        else if(eEvent == OMX_EventError)
            hTest->bError = OMX_TRUE;
        else {
            sMsg.hComponent = hComponent;
            sMsg.type = EVENT;
            sMsg.data.event.eEvent = eEvent;
            sMsg.data.event.nData1 = nData1;
            sMsg.data.event.nData2 = nData2;
            sMsg.data.event.pEventData = pEventData;
            hTest->pMsgQ->Add(&sMsg);
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE gm_EmptyBufferDone(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    MSG sMsg;
    HTEST *hTest = (HTEST*)pAppData;

    //printf("Client receive EmptyBufferDone [%p]\n", pBuffer);

    pBuffer->nFilledLen = 0;
    pBuffer->nFlags = 0;

    sMsg.hComponent = hComponent;
    sMsg.type = EMPTY_DONE;
    sMsg.data.buffer.pBuffer = pBuffer;
    hTest->pMsgQ->Add(&sMsg);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE gm_FillBufferDone(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    MSG sMsg;
    HTEST *hTest = (HTEST*)pAppData;

    //printf("Client receive FillBufferDone [%p]\n", pBuffer);

    write_data(hTest, pBuffer);

    sMsg.hComponent = hComponent;
    sMsg.type = FILL_DONE;
    sMsg.data.buffer.pBuffer = pBuffer;
    hTest->pMsgQ->Add(&sMsg);

    return OMX_ErrorNone;
}

/**/
OMX_CALLBACKTYPE gCallBacks =
{
    gm_EventHandler,
    gm_EmptyBufferDone,
    gm_FillBufferDone
};

OMX_ERRORTYPE WaitCommand(
        HTEST *hTest, 
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam1,
        OMX_PTR pCmdData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    while(1) {
        if(hTest->sCmdDone.eEvent == OMX_EventCmdComplete
                && hTest->sCmdDone.nData1 == (OMX_U32)Cmd
                && hTest->sCmdDone.nData2 == nParam1
                && hTest->sCmdDone.pEventData == pCmdData)
            break;
        else if(hTest->bError == OMX_TRUE) {
            ret = OMX_ErrorUndefined;
            break;
        }
        else
            usleep(10000);
    }

    return ret;
}

OMX_ERRORTYPE SendCommand(
        HTEST *hTest, 
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam1,
        OMX_PTR pCmdData,
        OMX_BOOL bSync)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_HANDLETYPE hComponent = hTest->hComponent;

    hTest->sCmdDone.eEvent = OMX_EventMax;
    hTest->bError = OMX_FALSE;
    ret = OMX_SendCommand(hComponent, Cmd, nParam1, pCmdData);
    if(ret != OMX_ErrorNone && ret != OMX_ErrorNotComplete)
        return ret;

    if(bSync == OMX_TRUE)
        WaitCommand(hTest, Cmd, nParam1, pCmdData);

    return OMX_ErrorNone;
}

OMX_U32 get_component_ports(OMX_HANDLETYPE hComponent)
{
    OMX_PORT_PARAM_TYPE sPortPara;
    OMX_U32 aPorts, vPorts, iPorts, oPorts;

    OMX_INIT_STRUCT(&sPortPara, OMX_PORT_PARAM_TYPE);
    aPorts = vPorts = iPorts = oPorts = 0;

    if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamAudioInit, &sPortPara))
        aPorts = sPortPara.nPorts;
    if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamVideoInit, &sPortPara))
        vPorts = sPortPara.nPorts;
    if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamImageInit, &sPortPara))
        iPorts = sPortPara.nPorts;
    if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamOtherInit, &sPortPara))
        oPorts = sPortPara.nPorts;

    return aPorts + vPorts + iPorts + oPorts;
}

OMX_ERRORTYPE load_component(HTEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_HANDLETYPE hComponent = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_U32 i;

    ret = OMX_GetHandle(&hComponent, hTest->name, hTest, &gCallBacks);
    if(ret != OMX_ErrorNone) {
        printf("Load component %s failed.\n", hTest->name);
        return ret;
    }

    hTest->hComponent = (OMX_COMPONENTTYPE*)hComponent;
#if 0
    hTest->nPorts = get_component_ports(hComponent);
    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    for(i=0; i<hTest->nPorts; i++) {
        sPortDef.nPortIndex = i;
        OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
        hTest->PortDir[i] = sPortDef.eDir;
        if(hTest->PortDir[i] == OMX_DirInput)
            hTest->bAllocater[i] = OMX_FALSE;
        if(hTest->PortDir[i] == OMX_DirOutput)
            hTest->bAllocater[i] = OMX_TRUE;
    }
#endif

    hTest->nPorts = 1;
    hTest->PortDir[0] = OMX_DirInput;
    hTest->bAllocater[0] = OMX_TRUE;
    //hTest->bAllocater[0] = OMX_FALSE;

    fsl_osal_thread_create(&hTest->pThreadId, NULL, process_thread, hTest);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE unload_component(HTEST *hTest)
{
    OMX_FreeHandle(hTest->hComponent);
    fsl_osal_thread_destroy(hTest->pThreadId);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE prepare_port_buffers(HTEST *hTest, OMX_U32 nPortIndex)
{
    OMX_COMPONENTTYPE *hComponent = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    OMX_U8 *pBuffer = NULL;
    OMX_U32 i;

    hComponent = hTest->hComponent;
 
    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
            
    if (sPortDef.eDomain == OMX_PortDomainVideo)
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE sVideoFormat;
        OMX_INIT_STRUCT(&sVideoFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
        sVideoFormat.nPortIndex = nPortIndex;
        sVideoFormat.eColorFormat = OMX_COLOR_FormatYUV420Planar;
        sVideoFormat.eCompressionFormat = OMX_VIDEO_CodingUnused;
        if (OMX_SetParameter(hComponent, OMX_IndexParamVideoPortFormat, &sVideoFormat)!=OMX_ErrorNone)
        {
            printf( "Can not set video format to video render.\n");
        }

        OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
        sPortDef.nPortIndex = nPortIndex;
        sPortDef.nBufferSize = 720*480*3/2;
        sPortDef.nBufferCountActual = sPortDef.nBufferCountMin = 6;
        sPortDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
        sPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
        sPortDef.format.video.nFrameWidth = 720;
        sPortDef.format.video.nFrameHeight = 480;
        OMX_SetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
    }

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
    for(i=0; i<sPortDef.nBufferCountActual; i++) {
        if(hTest->bAllocater[nPortIndex] == OMX_TRUE) {
            OMX_AllocateBuffer(hComponent, &pBufferHdr, nPortIndex, NULL, sPortDef.nBufferSize);
            printf("Allocate buffer done.\n");
        }
        else {
            pBuffer = (OMX_U8*)FSL_MALLOC(sPortDef.nBufferSize);
            OMX_UseBuffer(hComponent, &pBufferHdr, nPortIndex, NULL, sPortDef.nBufferSize, pBuffer);
            printf("Use buffer done.\n");
        }
        hTest->pBufferHdr[nPortIndex][i] = pBufferHdr;
    }
    hTest->nBufferHdr[nPortIndex] = sPortDef.nBufferCountActual;


    return OMX_ErrorNone;
}

OMX_ERRORTYPE free_port_buffers(HTEST *hTest, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *hComponent;
    OMX_U32 i;

    hComponent = hTest->hComponent;
    for(i=0; i<hTest->nBufferHdr[nPortIndex]; i++) {
        OMX_U8 *ptr = NULL;
        if(hTest->pBufferHdr[nPortIndex][i] == NULL)
            continue;
        ptr = hTest->pBufferHdr[nPortIndex][i]->pBuffer;
        OMX_FreeBuffer(hComponent, nPortIndex, hTest->pBufferHdr[nPortIndex][i]);
        hTest->pBufferHdr[nPortIndex][i] = 0;
        if(hTest->bAllocater[nPortIndex] == OMX_FALSE)
            FSL_FREE(ptr);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Load2Idle(HTEST *hTest)
{
    OMX_U32 i;
    for(i=0; i<hTest->nPorts; i++)
        prepare_port_buffers(hTest, i);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Idle2Load(HTEST *hTest)
{
    OMX_U32 i;

    for(i=0; i<hTest->nPorts; i++)
        free_port_buffers(hTest, i);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StateTrans(HTEST *hTest, OMX_STATETYPE eState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *hComponent = NULL;
    OMX_STATETYPE eCurState = OMX_StateInvalid;

    hComponent = hTest->hComponent;
    OMX_GetState(hComponent, &eCurState);

    if(eCurState == OMX_StateIdle && eState == OMX_StateLoaded)
        remove_clock(hTest);

    if(eCurState == OMX_StateExecuting && eState == OMX_StatePause)
        OMX_SendCommand(hTest->hClk, OMX_CommandStateSet, OMX_StatePause, NULL);

    ret = SendCommand(hTest, OMX_CommandStateSet,eState,NULL, OMX_FALSE);
    if(ret != OMX_ErrorNone) {
        printf("State trans to %d failed.\n", eState);
        return ret;
    }

    /* Loaded->Idle */
    if(eCurState == OMX_StateLoaded && eState == OMX_StateIdle)
        Load2Idle(hTest);
    /* Exec->Idle */
    else if(eCurState == OMX_StateExecuting && eState == OMX_StateIdle)
        hTest->bHoldBuffers = OMX_TRUE;
    /* Pause->Idle */
    else if(eCurState == OMX_StatePause && eState == OMX_StateIdle)
        hTest->bHoldBuffers = OMX_TRUE;
    /* Idle->Loaded */
    else if(eCurState == OMX_StateIdle && eState == OMX_StateLoaded) {
        Idle2Load(hTest);
    }
    else
        printf("Ivalid state trans.\n");

    ret = WaitCommand(hTest, OMX_CommandStateSet, eState, NULL);
    if(ret != OMX_ErrorNone)
        return ret;

    printf("State trans: [%x] -> [%d] done.\n", eCurState, eState);


    if(eCurState == OMX_StatePause && eState == OMX_StateExecuting)
        OMX_SendCommand(hTest->hClk, OMX_CommandStateSet, OMX_StateExecuting, NULL);

    /* Idle->Exec/Idle->Pause done */ 
    if(eCurState == OMX_StateIdle && (eState == OMX_StateExecuting || eState == OMX_StatePause)) {
        add_clock(hTest);
        start_data_process(hTest);
    }

    return ret;
}

OMX_ERRORTYPE add_clock(HTEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_HANDLETYPE hClock = NULL;
    OMX_STRING name = "OMX.Freescale.std.clocksrc.sw-based";
    OMX_TIME_CONFIG_CLOCKSTATETYPE ClockState;
    OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE RefClock;

    ret = OMX_GetHandle(&hClock, name, hTest, &gCallBacks);
    if(ret != OMX_ErrorNone) {
        printf("Load component %s failed.\n", name);
        return ret;
    }

    hTest->hClk = (OMX_COMPONENTTYPE*)hClock;
    OMX_SendCommand(hClock, OMX_CommandStateSet, OMX_StateIdle, 0);
    OMX_SendCommand(hClock, OMX_CommandStateSet, OMX_StateExecuting, 0);

    ret = OMX_SetupTunnel(hClock, 0, hTest->hComponent, 1);
    if(ret != OMX_ErrorNone) {
        printf("Tunnel clock with component failed.\n");
        return ret;
    }

    SendCommand(hTest, OMX_CommandPortEnable, 1, NULL, OMX_FALSE);
    OMX_SendCommand(hClock, OMX_CommandPortEnable, 0, NULL);
    WaitCommand(hTest, OMX_CommandPortEnable, 1, NULL);

    OMX_INIT_STRUCT(&ClockState, OMX_TIME_CONFIG_CLOCKSTATETYPE);
    ClockState.eState = OMX_TIME_ClockStateWaitingForStartTime;
    ClockState.nWaitMask = 1;
    OMX_SetConfig(hClock, OMX_IndexConfigTimeClockState, &ClockState);

    OMX_INIT_STRUCT(&RefClock, OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE);
    RefClock.eClock = OMX_TIME_RefClockVideo;
    OMX_SetConfig(hClock, OMX_IndexConfigTimeActiveRefClock, &RefClock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE remove_clock(HTEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_HANDLETYPE hClock = NULL;

    hClock = hTest->hClk;
    OMX_SendCommand(hClock, OMX_CommandPortDisable, 0, NULL);
    SendCommand(hTest, OMX_CommandPortDisable, 1, NULL, OMX_TRUE);

    //OMX_SetupTunnel(hClock, 0, NULL, 0);
    //OMX_SetupTunnel(hTest->hComponent, 1, NULL, 0);

    OMX_SendCommand(hClock, OMX_CommandStateSet, OMX_StateIdle, 0);
    OMX_SendCommand(hClock, OMX_CommandStateSet, OMX_StateLoaded, 0);

    OMX_FreeHandle(hClock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE dump_rect(OMX_CONFIG_RECTTYPE *pRect)
{
    printf("\n\
            rect width %d, rect crop left %d\n\
            rect height %d,rect crop top %d\n",
            pRect->nWidth,pRect->nLeft,
            pRect->nHeight,pRect->nTop);
}

OMX_ERRORTYPE dump_output_mode(OMX_CONFIG_OUTPUTMODE *pOutputMode)
{
    printf("%s,%d.\n",__FUNCTION__,__LINE__);
    printf("\n\
            is tv out %d\n\
            fb layer %d\n\
            tv mode %d\n\
            rotate %d\n",
            pOutputMode->bTv,
            pOutputMode->eFbLayer,
            pOutputMode->eTvMode,
            pOutputMode->eRotation);
    printf("input rect \n");
    dump_rect(&pOutputMode->sRectIn);
    printf("output rect \n");
    dump_rect(&pOutputMode->sRectOut);
    return OMX_ErrorNone;
}


OMX_ERRORTYPE calc_crop(HTEST *hTest, 
        SCREENTYPE eScreen,
        OMX_CONFIG_RECTTYPE *pInRect,
        OMX_CONFIG_RECTTYPE *pOutRect,
        OMX_CONFIG_RECTTYPE *pCropRect)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nWidth, nHeight;

    if(eScreen == SCREEN_STD) {
        /* Need set display crop */
        if(pInRect->nWidth * pOutRect->nHeight >= pInRect->nHeight * pOutRect->nWidth) {
            /* Crop display height */
            nHeight = pInRect->nHeight * pOutRect->nWidth / pInRect->nWidth;
            pCropRect->nTop = (pOutRect->nHeight - nHeight) / 2;
            pCropRect->nLeft = 0;
            pCropRect->nWidth = pOutRect->nWidth;
            pCropRect->nHeight = nHeight;
        }
        else {
            /* Crop display width */
            nWidth = pInRect->nWidth * pOutRect->nHeight / pInRect->nHeight;
            pCropRect->nLeft = (pOutRect->nWidth - nWidth) / 2;
            pCropRect->nTop = 0;
            pCropRect->nWidth = nWidth;
            pCropRect->nHeight = pOutRect->nHeight;
        }
    }


    if(eScreen == SCREEN_ZOOM) {
        /* Need set input crop */
        if(pInRect->nWidth * pOutRect->nHeight >= pInRect->nHeight * pOutRect->nWidth) {
            /* Crop in width */
			OMX_S32 nScale = pOutRect->nHeight * 1000 / pInRect->nHeight;
			/*480/240 = 2*/
			OMX_S32 nAmplifiedWid = (pInRect->nWidth * nScale /1000 + 7)/8*8;
			//400* 2 * 1.333 = 1056
			/*1056*480 is desired when full screen without crop and keep ratio*/
			OMX_S32 nCropLeftOnAmplifiedRect = (nAmplifiedWid - pOutRect->nWidth)/2;//(1056-640)/2=208
			OMX_S32 nCropLeftOnInRect = (nCropLeftOnAmplifiedRect * 1000/ nScale + 7)/8*8;//208/2 = 104
#if 0	
            /*FIXME: a bug of v4l when display layer2 on tv*/
            if (pInRect->nTop == 0){
                pInRect->nTop = 16;
                pInRect->nHeight -= 16*2;
            }
#endif
            pCropRect->nLeft = nCropLeftOnInRect + pInRect->nLeft; 
            pCropRect->nTop = pInRect->nTop;
            pCropRect->nWidth = pInRect->nWidth - 2*nCropLeftOnInRect;
            pCropRect->nHeight = pInRect->nHeight;

			printf("%s,%d,original input rect width %d\n\
                    amplifying scale %d,\n\
                    width of amplified rect %d,\n\
                    crop_left_on amplified rect %d,\n\
                    crop_left on input rect width %d,\n\
                    width after crop the decoder output padded left %d\n",
                    __FUNCTION__,__LINE__,
                    pInRect->nWidth,
                    nScale,
                    nAmplifiedWid,
                    nCropLeftOnAmplifiedRect,
                    nCropLeftOnInRect,
                    pCropRect->nWidth);
        }
        else 
        {
            OMX_S32 nScale = pOutRect->nWidth * 1000 / pInRect->nWidth;
			OMX_S32 nAmplifiedHeight = (pInRect->nHeight * nScale/1000 + 7)/8*8;
			//480*1.333 = 640,means desired rect is 640*640 when full screen and keep ratio without crop.
			OMX_S32 nCropTopOnAmplifiedRect = (nAmplifiedHeight - pOutRect->nHeight)/2;//(640-480)/2=80
			OMX_S32 nCropTopOnInRect = (nCropTopOnAmplifiedRect * 1000/ nScale  + 7)/8*8;//80 /1.333 = 60
#if 0
            /*FIXME: a bug of v4l when display layer2 on tv*/
            if (pInRect->nLeft==0){
                pInRect->nLeft = 16;
                pInRect->nWidth -= 32;
            }
#endif

            pCropRect->nLeft = pInRect->nLeft; 
            pCropRect->nTop = pInRect->nTop + nCropTopOnInRect;
            pCropRect->nWidth = pInRect->nWidth;
            pCropRect->nHeight = pInRect->nHeight - 2*nCropTopOnInRect;

			printf("%s,%d,original input rect height %d\n\
                    amplifying scale %d,\n\
                    height of amplified rect %d,\n\
                    crop_top_on amplified rect %d,\n\
                    crop_top on input rect height %d,\n\
                    height after including the crop of decoder padded top %d\n",
                    __FUNCTION__,__LINE__,
                    pInRect->nHeight,
                    nScale,
                    nAmplifiedHeight,
                    nCropTopOnAmplifiedRect,
                    nCropTopOnInRect,
                    pCropRect->nHeight);

        }
    }

    return ret;
}
#ifdef MX37
OMX_ERRORTYPE set_crop(HTEST *hTest, SCREENTYPE eScreen,OMX_CONFIG_RECTTYPE *pCropRect, OMX_CONFIG_OUTPUTMODE *pOutputMode)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nWidth, nHeight;

    if(eScreen == SCREEN_STD) {
        fsl_osal_memcpy(&pOutputMode->sRectIn, &hTest->sRectIn, sizeof(OMX_CONFIG_RECTTYPE));
        if (!hTest->bRotate)
        {
            if (!pOutputMode->bTv)
                fsl_osal_memcpy(&pOutputMode->sRectOut, pCropRect, sizeof(OMX_CONFIG_RECTTYPE));
            else /*FIXME: a bug of v4l driver,crop on top need divide by 2*/
            {
                pOutputMode->sRectOut.nHeight   = pCropRect->nHeight;
                pOutputMode->sRectOut.nTop      = pCropRect->nTop/2;
                pOutputMode->sRectOut.nWidth    = pCropRect->nWidth;
                pOutputMode->sRectOut.nLeft     = pCropRect->nLeft;
            }
        }
        else 
        {
            pOutputMode->sRectOut.nHeight   = pCropRect->nWidth;
            pOutputMode->sRectOut.nTop      = pCropRect->nLeft;
            pOutputMode->sRectOut.nWidth    = pCropRect->nHeight;
            pOutputMode->sRectOut.nLeft     = pCropRect->nTop;
        }
    }

    if(eScreen == SCREEN_FUL) {
        fsl_osal_memcpy(&pOutputMode->sRectIn, &hTest->sRectIn, sizeof(OMX_CONFIG_RECTTYPE));
        fsl_osal_memcpy(&pOutputMode->sRectOut, &hTest->sRectOut, sizeof(OMX_CONFIG_RECTTYPE));

    }

    if(eScreen == SCREEN_ZOOM) {
        fsl_osal_memcpy(&pOutputMode->sRectIn, pCropRect, sizeof(OMX_CONFIG_RECTTYPE));
        fsl_osal_memcpy(&pOutputMode->sRectOut, &hTest->sRectOut, sizeof(OMX_CONFIG_RECTTYPE));
    }
    dump_output_mode(pOutputMode);
    return ret;
}
#else 
OMX_ERRORTYPE set_crop(HTEST *hTest, SCREENTYPE eScreen,OMX_CONFIG_RECTTYPE *pCropRect, OMX_CONFIG_OUTPUTMODE *pOutputMode)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nWidth, nHeight;

    if(eScreen == SCREEN_STD) {
        fsl_osal_memcpy(&pOutputMode->sRectIn, &hTest->sRectIn, sizeof(OMX_CONFIG_RECTTYPE));
        if (!hTest->bRotate)
        {
            fsl_osal_memcpy(&pOutputMode->sRectOut, pCropRect, sizeof(OMX_CONFIG_RECTTYPE));
        }
        else 
        {
            pOutputMode->sRectOut.nHeight   = pCropRect->nWidth;
            pOutputMode->sRectOut.nTop      = pCropRect->nLeft;
            pOutputMode->sRectOut.nWidth    = pCropRect->nHeight;
            pOutputMode->sRectOut.nLeft     = pCropRect->nTop;
        }
    }

    if(eScreen == SCREEN_FUL) {
        fsl_osal_memcpy(&pOutputMode->sRectIn, &hTest->sRectIn, sizeof(OMX_CONFIG_RECTTYPE));
        fsl_osal_memcpy(&pOutputMode->sRectOut, &hTest->sRectOut, sizeof(OMX_CONFIG_RECTTYPE));

    }

    if(eScreen == SCREEN_ZOOM) {
        fsl_osal_memcpy(&pOutputMode->sRectIn, pCropRect, sizeof(OMX_CONFIG_RECTTYPE));
        fsl_osal_memcpy(&pOutputMode->sRectOut, &hTest->sRectOut, sizeof(OMX_CONFIG_RECTTYPE));
    }
    dump_output_mode(pOutputMode);
    return ret;
}
#endif
#ifdef MX37
OMX_ERRORTYPE modify_output_rect_if_rotate(HTEST *hTest, OMX_CONFIG_OUTPUTMODE *pOutputMode)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (pOutputMode->eRotation)
    {
        case ROTATE_NONE:
        case ROTATE_VERT_FLIP:
        case ROTATE_HORIZ_FLIP:
        case ROTATE_180:
            hTest->bRotate = OMX_FALSE;
            break;
        case ROTATE_90_RIGHT:
        case ROTATE_90_RIGHT_VFLIP:
        case ROTATE_90_RIGHT_HFLIP:
        case ROTATE_90_LEFT:
            hTest->bRotate = OMX_TRUE;
            break;
        default :
            break;
    }

    if (!pOutputMode->bTv)
    {
        hTest->sRectOut.nTop = 0;
        hTest->sRectOut.nWidth = 480;// 240;
        hTest->sRectOut.nLeft = 0;
        hTest->sRectOut.nHeight = 640;// 400;
    }
    else if (pOutputMode->eTvMode == MODE_PAL)
    {
        hTest->sRectOut.nTop = 0;
        hTest->sRectOut.nWidth = 720;
        hTest->sRectOut.nLeft = 0;
        hTest->sRectOut.nHeight = 576;
    }
    else if (pOutputMode->eTvMode == MODE_NTSC)
    {
        hTest->sRectOut.nTop = 0;
        hTest->sRectOut.nWidth = 720;
        hTest->sRectOut.nLeft = 0;
        hTest->sRectOut.nHeight = 480;
    }


        
    if (!pOutputMode->bTv)
    {
        if (hTest->bRotate)
        {
            hTest->sDispWindow.nTop = 0;
            hTest->sDispWindow.nWidth = 640;// 400;
            hTest->sDispWindow.nLeft = 0;
            hTest->sDispWindow.nHeight = 480;// 240;
        }
        else 
        {
            hTest->sDispWindow.nTop = 0;
            hTest->sDispWindow.nWidth = 480;// 240;
            hTest->sDispWindow.nLeft = 0;
            hTest->sDispWindow.nHeight = 640;// 400;
        }
    }
    else if (pOutputMode->eTvMode == MODE_PAL)
    {
        hTest->sDispWindow.nTop = 0;
        hTest->sDispWindow.nWidth = 720;
        hTest->sDispWindow.nLeft = 0;
        hTest->sDispWindow.nHeight = 576;
    }
    else if (pOutputMode->eTvMode == MODE_NTSC)
    {
        hTest->sDispWindow.nTop = 0;
        hTest->sDispWindow.nWidth = 720;
        hTest->sDispWindow.nLeft = 0;
        hTest->sDispWindow.nHeight = 480;
    }

    return ret;
}
#else 
OMX_ERRORTYPE modify_output_rect_if_rotate(HTEST *hTest, OMX_CONFIG_OUTPUTMODE *pOutputMode)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (pOutputMode->eRotation)
    {
        case ROTATE_NONE:
        case ROTATE_VERT_FLIP:
        case ROTATE_HORIZ_FLIP:
        case ROTATE_180:
            hTest->bRotate = OMX_FALSE;
            break;
        case ROTATE_90_RIGHT:
        case ROTATE_90_RIGHT_VFLIP:
        case ROTATE_90_RIGHT_HFLIP:
        case ROTATE_90_LEFT:
            hTest->bRotate = OMX_TRUE;
            break;
        default :
            break;
    }

    if (!pOutputMode->bTv)
    {
        hTest->sRectOut.nTop = 0;
        hTest->sRectOut.nWidth = 1024;// 240;
        hTest->sRectOut.nLeft = 0;
        hTest->sRectOut.nHeight = 768;// 400;
    }
    else if (pOutputMode->eTvMode == MODE_PAL)
    {
        hTest->sRectOut.nTop = 0;
        hTest->sRectOut.nWidth = 720;
        hTest->sRectOut.nLeft = 0;
        hTest->sRectOut.nHeight = 576;
    }
    else if (pOutputMode->eTvMode == MODE_NTSC)
    {
        hTest->sRectOut.nTop = 0;
        hTest->sRectOut.nWidth = 720;
        hTest->sRectOut.nLeft = 0;
        hTest->sRectOut.nHeight = 480;
    }


        
    if (!pOutputMode->bTv)
    {
        if (hTest->bRotate)
        {
            hTest->sDispWindow.nTop = 0;
            hTest->sDispWindow.nWidth = 768;// 400;
            hTest->sDispWindow.nLeft = 0;
            hTest->sDispWindow.nHeight = 1024;// 240;
        }
        else 
        {
            hTest->sDispWindow.nTop = 0;
            hTest->sDispWindow.nWidth = 1024;// 240;
            hTest->sDispWindow.nLeft = 0;
            hTest->sDispWindow.nHeight = 768;// 400;
        }
    }
    else if (pOutputMode->eTvMode == MODE_PAL)
    {
        hTest->sDispWindow.nTop = 0;
        hTest->sDispWindow.nWidth = 720;
        hTest->sDispWindow.nLeft = 0;
        hTest->sDispWindow.nHeight = 576;
    }
    else if (pOutputMode->eTvMode == MODE_NTSC)
    {
        hTest->sDispWindow.nTop = 0;
        hTest->sDispWindow.nWidth = 720;
        hTest->sDispWindow.nLeft = 0;
        hTest->sDispWindow.nHeight = 480;
    }

    return ret;
}
#endif 

OMX_BOOL is_same_screen_mode(HTEST *hTest, SCREENTYPE eScreen, OMX_BOOL bTvOut, OMX_U32 nTvMode, OMX_U32 nRotate)
{
    if (hTest->eScreen == eScreen &&
        hTest->sOutputMode.bTv  == bTvOut &&
        hTest->sOutputMode.eTvMode == (TV_MODE)nTvMode &&
        hTest->sOutputMode.eRotation == (ROTATION)nRotate)
        return OMX_TRUE;

    return OMX_FALSE;
}

OMX_ERRORTYPE overwrite_rotate_in_tvout_mode(HTEST *hTest,OMX_CONFIG_OUTPUTMODE *pOutputMode)
{
    if (pOutputMode->bTv)
    {
        pOutputMode->eRotation = ROTATE_NONE;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE screen_mode(HTEST *hTest, SCREENTYPE eScreen, OMX_BOOL bTvOut, OMX_U32 nTvMode, OMX_U32 nRotate)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (is_same_screen_mode(hTest, eScreen, bTvOut, nTvMode, nRotate))
        return ret;

    OMX_CONFIG_OUTPUTMODE sOutputMode;
    OMX_INIT_STRUCT(&sOutputMode, OMX_CONFIG_OUTPUTMODE);
    sOutputMode.nPortIndex = 0;
    sOutputMode.bTv = bTvOut;
    sOutputMode.eFbLayer = LAYER2;
    sOutputMode.eTvMode = (TV_MODE)nTvMode;
    sOutputMode.eRotation = (ROTATION)nRotate;
    OMX_CONFIG_RECTTYPE sCropRect;
    OMX_INIT_STRUCT(&sCropRect, OMX_CONFIG_RECTTYPE);

    overwrite_rotate_in_tvout_mode(hTest, &sOutputMode);

    modify_output_rect_if_rotate(hTest, &sOutputMode);

    calc_crop(hTest,eScreen,&hTest->sRectIn,&hTest->sDispWindow,&sCropRect);
    
    set_crop(hTest,eScreen,&sCropRect,&sOutputMode);

    printf("%s,%d,set rotate to %d\n",__FUNCTION__,__LINE__,sOutputMode.eRotation);    
    printf("%s,%d,set screen mode to %d\n",__FUNCTION__,__LINE__,eScreen);    
    OMX_SetConfig(hTest->hComponent, OMX_IndexOutputMode, &sOutputMode);

    fsl_osal_memcpy(&hTest->sOutputMode, &sOutputMode, sizeof(OMX_CONFIG_OUTPUTMODE));
    hTest->eScreen = eScreen;
    return ret;
}

OMX_ERRORTYPE test_rotate(HTEST *hTest, OMX_U32 nRotate)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (nRotate == (OMX_U32)hTest->sOutputMode.eRotation)
        return ret;
    if (nRotate > 7)
        return OMX_ErrorBadParameter;

    screen_mode(hTest, hTest->eScreen, hTest->sOutputMode.bTv, (OMX_U32)hTest->sOutputMode.eTvMode, nRotate);

    return ret;
}

OMX_ERRORTYPE cmd_process(HTEST *hTest)
{
    char rep[10];
    OMX_BOOL bExit = OMX_FALSE;

    while(bExit == OMX_FALSE) {
        printf("Input test cmd:\n");
        printf("[m]change screen mode\n");
        printf("[r]rotate the screeen\n");
        printf("[x]exit\n");
        scanf("%s", rep);
        if(rep[0] == 'l')
            load_component(hTest);
        else if(rep[0] == 's') {
            OMX_U32 state = 0;
            printf("State trans to:\n");
            printf("0 -- Invalid\n");
            printf("1 -- Loaded\n");
            printf("2 -- Idle\n");
            printf("3 -- Executing\n");
            printf("4 -- Pause\n");
            printf("5 -- WaitForResources\n");
            scanf("%d", &state);
            StateTrans(hTest, (OMX_STATETYPE)state);
        }
        else if(rep[0] == 'm') {
            OMX_U32 mode;
            printf("Input screen mode:\n");
            printf("1 - STD, 2 - FULL, 3 - ZOOM\n");
            scanf("%d", &mode);
            OMX_U32 bTvOut;
            printf("Select output to tvset:\n");
            printf("0 - output to LCD, 1 - output to tvset\n");
            scanf("%d", &bTvOut);
            OMX_U32 nTvMode;
            printf("Select tv mode:\n");
            printf("0 - None, 1 - PAL, 2 - NTSC");
            scanf("%d", &nTvMode);
            screen_mode(hTest, (SCREENTYPE)mode, (OMX_BOOL)bTvOut,nTvMode, ROTATE_NONE);
        }
        else if(rep[0] == 'r') {
            OMX_U32 nRotate;
            printf("Input rotate direction:\n");
            printf("0:base direction.\n\
                    1:rotate anti-clockwise 180 and mirror\n\
                    2:mirror\n\
                    3:rotate clockwise 180\n\
                    4:rotate clockwise 90\n\
                    5:rotate clockwise 90 and mirror\n\
                    6:rotate anti-clockwise 90 and mirror\n\
                    7:rotate anti-clockwise 90\n");
            scanf("%d", &nRotate);
            test_rotate(hTest, nRotate);
        }
        else if(rep[0] == 'x') {
            MSG sMsg;
            sMsg.type = EXIT;
            hTest->pMsgQ->Add(&sMsg);
            hTest->bStop = OMX_TRUE;
            bExit = OMX_TRUE;
            unload_component(hTest);
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE process_event(HTEST *hTest, MSG *pMsg)
{
    printf("Client receive event [eEvent:%d, nData1: %d, nData2: %d]\n",
            (int)pMsg->data.event.eEvent, 
            (int)pMsg->data.event.nData1, 
            (int)pMsg->data.event.nData2);

    return OMX_ErrorNone;
}

void * process_thread(void *ptr)
{
    HTEST *hTest = (HTEST*)ptr;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    MSG sMsg;

    while(1) {
        hTest->pMsgQ->Get(&sMsg);
        if(sMsg.type == EVENT) {
            process_event(hTest, &sMsg);
        }
        else if(sMsg.type == EMPTY_DONE) {
            pBufferHdr = sMsg.data.buffer.pBuffer;
            if(hTest->bHoldBuffers != OMX_TRUE) {
                read_data(hTest, pBufferHdr);
                pBufferHdr->nTimeStamp = hTest->ts;
                hTest->ts += FRAME_DURATION;
                OMX_EmptyThisBuffer(sMsg.hComponent, pBufferHdr);
            }
        }
        else if(sMsg.type == FILL_DONE) {
            pBufferHdr = sMsg.data.buffer.pBuffer;
            if(hTest->bHoldBuffers != OMX_TRUE)
                OMX_FillThisBuffer(sMsg.hComponent, pBufferHdr);
        }

        if(hTest->bStop == OMX_TRUE)
            break;
    }

    return NULL;
}
#define DEC_OUTPUT_FRAME_WIDTH 720 
#define DEC_OUTPUT_FRAME_HEIGHT 480 
#define TEST_PUDGY_INPUT_CASE
#ifdef TEST_PUDGY_INPUT_CASE
#define DEC_OUTPUT_CROP_LEFT 0 
#define DEC_OUTPUT_CROP_TOP 0
#else
#define DEC_OUTPUT_CROP_LEFT 20 
#define DEC_OUTPUT_CROP_TOP 120
#endif 

#define SET_TO_STD_MODE_AS_DEFAULT

OMX_ERRORTYPE set_default_screen_mode(HTEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    hTest->sRectIn.nTop = DEC_OUTPUT_CROP_TOP;
    hTest->sRectIn.nHeight = DEC_OUTPUT_FRAME_HEIGHT - DEC_OUTPUT_CROP_TOP*2;
    hTest->sRectIn.nLeft = (DEC_OUTPUT_CROP_LEFT + 7)/8*8;
    hTest->sRectIn.nWidth = DEC_OUTPUT_FRAME_WIDTH - (DEC_OUTPUT_CROP_LEFT + 7)/8*8*2;

#ifdef SET_TO_STD_MODE_AS_DEFAULT
    screen_mode(hTest, SCREEN_STD, OMX_FALSE, MODE_NONE, ROTATE_NONE);
#else 
    OMX_INIT_STRUCT(&hTest->sRotate, OMX_CONFIG_ROTATIONTYPE);
    OMX_SetConfig(hTest->hComponent,OMX_IndexConfigCommonRotate, &hTest->sRotate);

    OMX_INIT_STRUCT(&hTest->sRectIn, OMX_CONFIG_RECTTYPE);
    OMX_SetConfig(hTest->hComponent,OMX_IndexConfigCommonInputCrop, &hTest->sRectIn);
        
    OMX_INIT_STRUCT(&hTest->sRectOut, OMX_CONFIG_RECTTYPE);
   
    modify_output_rect_if_rotate(hTest);
    
    OMX_SetConfig(hTest->hComponent,OMX_IndexConfigCommonOutputCrop, &hTest->sRectOut);
#endif 

    return ret;
}
OMX_ERRORTYPE start_data_process(HTEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i;

    hTest->bHoldBuffers = OMX_FALSE;

    set_default_screen_mode(hTest);

    /* Send output buffers */
    for(i=0; i<hTest->nBufferHdr[1]; i++) {
        hTest->pBufferHdr[1][i]->nFilledLen = 0;
        hTest->pBufferHdr[1][i]->nOffset = 0;
        OMX_FillThisBuffer(hTest->hComponent, hTest->pBufferHdr[1][i]);
    }

    hTest->ts = 0;
    hTest->pBufferHdr[0][0]->nFlags = OMX_BUFFERFLAG_STARTTIME;
    /* Send input buffers */
    for(i=0; i<hTest->nBufferHdr[0]; i++) {
        read_data(hTest, hTest->pBufferHdr[0][i]);
        hTest->pBufferHdr[0][i]->nTimeStamp = hTest->ts;
        hTest->ts += FRAME_DURATION;
        OMX_EmptyThisBuffer(hTest->hComponent, hTest->pBufferHdr[0][i]);
    }

    return ret;
}

HTEST * create_test(
        OMX_STRING component,
        OMX_STRING in_file,
        OMX_STRING out_file)
{
    HTEST *hTest = NULL;

    hTest = (HTEST*)fsl_osal_malloc_new(sizeof(HTEST));
    if(hTest == NULL) {
        printf("Failed to allocate memory for test handle.\n");
        return 0;
    }
    fsl_osal_memset(hTest, 0, sizeof(HTEST));

    hTest->name = component;

    hTest->pMsgQ = FSL_NEW(Queue, ());
    if(hTest->pMsgQ == NULL) {
        printf("Create message queue failed.\n");
        return 0;
    }
    hTest->pMsgQ->Create(128, sizeof(MSG), E_FSL_OSAL_TRUE);

    hTest->pInFile = fopen(in_file, "rb");
    if(hTest->pInFile == NULL) {
        printf("Failed to open file: %s\n", in_file);
        return 0;
    }

    return hTest;
}

OMX_ERRORTYPE delete_test(HTEST *hTest)
{
    fclose(hTest->pInFile);
    hTest->pMsgQ->Free();
    FSL_FREE(hTest);

    return OMX_ErrorNone;
}

int main(int argc, char *argv[])
{
    HTEST *hTest = NULL;
    OMX_STRING component = NULL;
    OMX_STRING in_file = NULL, out_file = NULL;

    if(argc < 2) {
        printf("Usage: ./bin <in_file>\n");
        return 0;
    }

    OMX_Init();
    component = "OMX.Freescale.std.video_render.v4l.sw-based";
    in_file = argv[1];
    out_file = NULL;
    hTest = create_test(component, in_file, out_file);
    if(hTest == NULL) {
        printf("Create test failed.\n");
        return 0;
    }
            
    load_component(hTest);
    StateTrans(hTest, (OMX_STATETYPE)2);
    StateTrans(hTest, (OMX_STATETYPE)3);
    cmd_process(hTest);
    delete_test(hTest);
    OMX_Deinit();

    return 1;
}

/* File EOF */
