/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include "OMX_Implement.h"
#include "fsl_osal.h"
#include "Mem.h"
#include "Log.h"

#include "OMX_Core.h"
#include "OMX_Component.h"    
#include "Queue.h"

#define MAX_PORT_NUM 8
#define MAX_PORT_BUFFER 32

typedef fsl_osal_ptr fsl_osal_sem_object;

typedef enum {
    EVENT,
    EMPTY_DONE,
    FILL_DONE,
    EXIT
}MSG_TYPE;

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
    OMX_BOOL bAudioTrackStopped;
    OMX_BOOL bVideoTrackStopped;
    OMX_BOOL bError;
    OMX_BOOL bHoldBuffers;
    char *media_name;
    char dump_file_path0[128];
    char dump_file_path1[128];
    FILE *pOutFile0;
    FILE *pOutFile1;
    OMX_U32 nAudioTrackNum;
    OMX_U32 nVideoTrackNum;

    fsl_osal_sem_object sParserFormatSem;
    OMX_BOOL bAudioFormatDetected;
    OMX_BOOL bVideoFormatDetected;
}HTEST;

OMX_ERRORTYPE start_data_process(HTEST *hTest);
void * process_thread(void *ptr);

static void MakeDumpFileName(char *media_file_name,
        char *dump_file_path,int track_num)
{
    char* p = strrchr(media_file_name, '.');
    if (p ==NULL) 
    {
        return;
    }
    int suffix_len = strlen(p);
    int len_org = strlen(media_file_name);
    strncpy(dump_file_path,media_file_name, (len_org-suffix_len));
    dump_file_path[len_org-suffix_len] = '\0';
    char tmp[8];
    sprintf(tmp, "_track%d", track_num);
    strcat(dump_file_path, tmp);
    strcat(dump_file_path, ".data");
    printf("%s, track dump file name %s.\n",__FUNCTION__,dump_file_path);
    return;
}


OMX_ERRORTYPE write_data(HTEST *hTest, OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    if (pBufferHdr->nOutputPortIndex == 0)
        fwrite(pBufferHdr->pBuffer, 1, pBufferHdr->nFilledLen, hTest->pOutFile0);
    else if (pBufferHdr->nOutputPortIndex == 1)
        fwrite(pBufferHdr->pBuffer, 1, pBufferHdr->nFilledLen, hTest->pOutFile1);
    fflush(hTest->pOutFile0);
    fflush(hTest->pOutFile1);
    pBufferHdr->nFilledLen = 0;
    pBufferHdr->nOffset = 0;
    if(pBufferHdr->nFlags & OMX_BUFFERFLAG_EOS)
    {
        if (pBufferHdr->nOutputPortIndex == 0)
            hTest->bAudioTrackStopped = OMX_TRUE;
        else if (pBufferHdr->nOutputPortIndex == 1)
            hTest->bVideoTrackStopped = OMX_TRUE;
    }
    if (hTest->bAudioTrackStopped && hTest->bVideoTrackStopped)
        hTest->bStop = OMX_TRUE;

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

    printf("%s,%d,Client receive event [eEvent:%d, nData1: %d, nData2: %d]\n",__FUNCTION__,__LINE__,
            (int)eEvent, (int)nData1, (int)nData2);

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
    hTest->nPorts = get_component_ports(hComponent);
    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    for(i=0; i<hTest->nPorts; i++) {
        sPortDef.nPortIndex = i;
        OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
        if (sPortDef.eDomain == OMX_PortDomainAudio)
            hTest->nAudioTrackNum = i;
        if (sPortDef.eDomain == OMX_PortDomainVideo)
            hTest->nVideoTrackNum = i;

        hTest->PortDir[i] = sPortDef.eDir;
        if(hTest->PortDir[i] == OMX_DirInput)
            hTest->bAllocater[i] = OMX_FALSE;
        if(hTest->PortDir[i] == OMX_DirOutput)
            hTest->bAllocater[i] = OMX_TRUE;
    }
    OMX_PARAM_CONTENTURITYPE *content = NULL;

    content =(OMX_PARAM_CONTENTURITYPE *) fsl_osal_malloc_new(sizeof(OMX_PARAM_CONTENTURITYPE) + 1024);
    if (!content)
        return OMX_ErrorInsufficientResources;

    fsl_osal_memset(content, 0 , sizeof(OMX_PARAM_CONTENTURITYPE)+1024);
    OMX_INIT_STRUCT(content,OMX_PARAM_CONTENTURITYPE);
    char* uri = (char*)&(content->contentURI);
    fsl_osal_memcpy(uri, hTest->media_name, strlen(hTest->media_name)+1);

    ret = OMX_SetParameter(hTest->hComponent,OMX_IndexParamContentURI,content);
    if (ret != OMX_ErrorNone)
    {
        OMX_FreeHandle(hTest->hComponent);
        hTest->hComponent = NULL;
        return ret;
    }




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
    OMX_ERRORTYPE ret =  OMX_ErrorNone;
    OMX_U32 i;
    for(i=0; i<hTest->nPorts; i++)
        prepare_port_buffers(hTest, i);
    
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = hTest->nAudioTrackNum;
    sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingAutoDetect;
    ret = OMX_SetParameter(hTest->hComponent,OMX_IndexParamPortDefinition,&sPortDef);
    if (ret != OMX_ErrorNone)
        return ret;
    sPortDef.nPortIndex = hTest->nVideoTrackNum;
    sPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingAutoDetect;
    ret = OMX_SetParameter(hTest->hComponent,OMX_IndexParamPortDefinition,&sPortDef);
    if (ret != OMX_ErrorNone)
        return ret;

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
    else if(eCurState == OMX_StateIdle && eState == OMX_StateLoaded)
        Idle2Load(hTest);
    else
        printf("Ivalid state trans.\n");

    ret = WaitCommand(hTest, OMX_CommandStateSet, eState, NULL);
    if(ret != OMX_ErrorNone)
        return ret;

    printf("State trans: [%x] -> [%d] done.\n", eCurState, eState);

    /* Idle->Exec/Idle->Pause done */
    if(eCurState == OMX_StateIdle && (eState == OMX_StateExecuting || eState == OMX_StatePause))
        start_data_process(hTest);

    return ret;
}

OMX_ERRORTYPE port_flush(HTEST *hTest, OMX_U32 nPortIndex)
{
    OMX_COMPONENTTYPE *hComponent;

    hComponent = hTest->hComponent;
    SendCommand(hTest, OMX_CommandFlush, nPortIndex, NULL, OMX_TRUE);

    printf("Flush done.\n");

    return OMX_ErrorNone;
}

OMX_ERRORTYPE port_disable(HTEST *hTest, OMX_U32 nPortIndex)
{
    OMX_COMPONENTTYPE *hComponent;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    hComponent = hTest->hComponent;
    ret = SendCommand(hTest, OMX_CommandPortDisable, nPortIndex, NULL, OMX_FALSE);
    if(ret != OMX_ErrorNone)
        return ret;

    hTest->bHoldBuffers = OMX_TRUE;

    free_port_buffers(hTest, nPortIndex);

    WaitCommand(hTest, OMX_CommandPortDisable, nPortIndex, NULL);

    printf("port disbale done.\n");

    return OMX_ErrorNone;
}

OMX_ERRORTYPE port_enable(HTEST *hTest, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *hComponent;
    OMX_U32 i;

    hComponent = hTest->hComponent;
    ret = SendCommand(hTest, OMX_CommandPortEnable, nPortIndex, NULL, OMX_FALSE);
    if(ret != OMX_ErrorNone)
        return ret;

    prepare_port_buffers(hTest, nPortIndex);

    WaitCommand(hTest, OMX_CommandPortEnable, nPortIndex, NULL);

    hTest->bHoldBuffers = OMX_FALSE;
    for(i=0; i<hTest->nBufferHdr[nPortIndex]; i++) {
        if(hTest->PortDir[nPortIndex] == OMX_DirOutput) {
            OMX_FillThisBuffer(hTest->hComponent, hTest->pBufferHdr[nPortIndex][i]);
        }
    }

    printf("port enable done.\n");

    return OMX_ErrorNone;
}

OMX_ERRORTYPE do_seek(HTEST *hTest,OMX_U32 nSeekPos, OMX_TIME_SEEKMODETYPE mode )
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
    OMX_INIT_STRUCT(&timeStamp, OMX_TIME_CONFIG_TIMESTAMPTYPE);

    OMX_TIME_CONFIG_SEEKMODETYPE seek_mode;
    OMX_INIT_STRUCT(&seek_mode, OMX_TIME_CONFIG_SEEKMODETYPE);
    seek_mode.eType = mode;
    ret = OMX_SetConfig(hTest->hComponent,OMX_IndexConfigTimeSeekMode, &seek_mode);
    if (ret != OMX_ErrorNone)
        return ret;

    timeStamp.nPortIndex = hTest->nAudioTrackNum;
    ret = OMX_GetConfig(hTest->hComponent,OMX_IndexConfigTimePosition, &timeStamp);
    if (ret != OMX_ErrorNone)
        return ret;
    printf("%s,%d,before seek, audio pos %lld.\n",__FUNCTION__,__LINE__,timeStamp.nTimestamp);

    timeStamp.nPortIndex = hTest->nVideoTrackNum;
    ret = OMX_GetConfig(hTest->hComponent,OMX_IndexConfigTimePosition, &timeStamp);
    if (ret != OMX_ErrorNone)
        return ret;
    printf("%s,%d,before seek, video pos %lld.\n",__FUNCTION__,__LINE__,timeStamp.nTimestamp);


    timeStamp.nPortIndex = OMX_ALL;
    timeStamp.nTimestamp = nSeekPos*OMX_TICKS_PER_SECOND;
    ret = OMX_SetConfig(hTest->hComponent,OMX_IndexConfigTimePosition, &timeStamp);

    timeStamp.nPortIndex = hTest->nAudioTrackNum;
    ret = OMX_GetConfig(hTest->hComponent,OMX_IndexConfigTimePosition, &timeStamp);
    if (ret != OMX_ErrorNone)
        return ret;
    printf("%s,%d,seek result audio %lld.\n",__FUNCTION__,__LINE__,timeStamp.nTimestamp);

    timeStamp.nPortIndex = hTest->nVideoTrackNum;
    ret = OMX_GetConfig(hTest->hComponent,OMX_IndexConfigTimePosition, &timeStamp);
    if (ret != OMX_ErrorNone)
        return ret;
    printf("%s,%d,seek result video %lld.\n",__FUNCTION__,__LINE__,timeStamp.nTimestamp);

    return ret;
}

OMX_ERRORTYPE cmd_process(HTEST *hTest)
{
    char rep[10];
    OMX_BOOL bExit = OMX_FALSE;

    while(bExit == OMX_FALSE) {
        printf("Input test cmd:\n");
        printf("[f]flush\n");
        printf("[d]port disbale\n");
        printf("[e]port enable\n");
        printf("[k]seek\n");
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
        else if(rep[0] == 'f') {
            OMX_U32 nPortIndex = 0;
            printf("Want to flush port number:\n");
            scanf("%d", &nPortIndex);
            port_flush(hTest, nPortIndex);
        }
        else if(rep[0] == 'd') {
            OMX_U32 nPortIndex = 0;
            printf("Want to disbale port number:\n");
            scanf("%d", &nPortIndex);
            port_disable(hTest, nPortIndex);
        }
        else if(rep[0] == 'e') {
            OMX_U32 nPortIndex = 0;
            printf("Want to enable port number:\n");
            scanf("%d", &nPortIndex);
            port_enable(hTest, nPortIndex);
        }
        else if(rep[0] == 'k') {
            OMX_U32 nSeekPos = 0;
            printf("seek to second:\n");
            scanf("%d", &nSeekPos);
            OMX_U32 nSeekMode = 0;
            printf("seek mode [0:Fast,1:Accurate]:\n");
            scanf("%d", &nSeekMode);
            do_seek(hTest, nSeekPos, (OMX_TIME_SEEKMODETYPE)nSeekMode);
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
    printf("%s,%d,Client receive event [eEvent:%d, nData1: %d, nData2: %d]\n",__FUNCTION__,__LINE__,
            (int)pMsg->data.event.eEvent, 
            (int)pMsg->data.event.nData1, 
            (int)pMsg->data.event.nData2);

    if (pMsg->data.event.eEvent == OMX_EventBufferFlag &&
            pMsg->data.event.nData1 == OMX_BUFFERFLAG_EOS)
    {
        if (pMsg->data.event.nData2 == hTest->nAudioTrackNum)
            hTest->bAudioTrackStopped = OMX_TRUE;
        if (pMsg->data.event.nData2 == hTest->nVideoTrackNum)
            hTest->bVideoTrackStopped = OMX_TRUE;
    }

    if (pMsg->data.event.eEvent == OMX_EventPortSettingsChanged)
    {
        if (pMsg->data.event.nData1 == hTest->nAudioTrackNum)
            hTest->bAudioFormatDetected = OMX_TRUE;

        if (pMsg->data.event.nData1 == hTest->nVideoTrackNum)
            hTest->bVideoFormatDetected = OMX_TRUE;

        if (hTest->bAudioFormatDetected && hTest->bVideoFormatDetected)
            fsl_osal_sem_post(hTest->sParserFormatSem);
    }


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
                OMX_EmptyThisBuffer(sMsg.hComponent, pBufferHdr);
            }
        }
        else if(sMsg.type == FILL_DONE) {
            pBufferHdr = sMsg.data.buffer.pBuffer;
            if (hTest->bAudioTrackStopped && hTest->bVideoTrackStopped)
                break;

            if (pBufferHdr->nOutputPortIndex == hTest->nAudioTrackNum)
            {
                if (hTest->bAudioTrackStopped)
                    continue;
            }
            else if (pBufferHdr->nOutputPortIndex == hTest->nVideoTrackNum)
            {
                if (hTest->bVideoTrackStopped)
                    continue;
            }

            if(hTest->bHoldBuffers != OMX_TRUE)
                OMX_FillThisBuffer(sMsg.hComponent, pBufferHdr);
        }

    }

    return NULL;
}

OMX_ERRORTYPE parser_process_port_setting_changed(HTEST *hTest,OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    port_disable(hTest,nPortIndex);
    OMX_PARAM_U32TYPE sU32;
    OMX_INIT_STRUCT(&sU32, OMX_PARAM_U32TYPE);
    sU32.nPortIndex = nPortIndex;
    ret = OMX_GetParameter(hTest->hComponent,OMX_IndexParamNumAvailableStreams,&sU32);
    if (ret != OMX_ErrorNone)
        return ret;

    printf("%s,%d,track %d, number of available stream is %d.\n",__FUNCTION__,__LINE__,sU32.nPortIndex,sU32.nU32);

    /*active strem number should be from 0 to NumAvailable - 1*/
    sU32.nU32 = sU32.nU32 - 1;
    ret = OMX_SetParameter(hTest->hComponent,OMX_IndexParamActiveStream,&sU32);
    if (ret != OMX_ErrorNone)
        return ret;

    printf("%s,%d,track %d, set active stream number %d.\n",__FUNCTION__,__LINE__,sU32.nPortIndex,sU32.nU32);


    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    ret =   OMX_GetParameter(hTest->hComponent, OMX_IndexParamPortDefinition, &sPortDef);
    if (ret != OMX_ErrorNone)
        return ret;

    if (nPortIndex == hTest->nAudioTrackNum)
        printf("%s,%d,audio track %d, compress format %d.\n",__FUNCTION__,__LINE__,sPortDef.nPortIndex,sPortDef.format.audio.eEncoding);
    else
        printf("%s,%d,video track %d, compress format %d.\n",__FUNCTION__,__LINE__,sPortDef.nPortIndex,sPortDef.format.video.eCompressionFormat);

    port_enable(hTest,nPortIndex);

    return ret;
}

OMX_ERRORTYPE start_data_process(HTEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i, wait_cnt = 0;

    hTest->bHoldBuffers = OMX_FALSE;
    
    while (fsl_osal_sem_trywait(hTest->sParserFormatSem) !=E_FSL_OSAL_SUCCESS)
    {
        usleep(10*1000);
        if(hTest->bError)
            return OMX_ErrorUndefined;

        if((wait_cnt++) > 60*100)
        {
            //if load time is larger than 1 minutes, then timeout
            printf("timeout when wait output\n");
            return OMX_ErrorUndefined;
        }
    }

    ret = parser_process_port_setting_changed(hTest,hTest->nAudioTrackNum);
    if (ret != OMX_ErrorNone)
        return ret;
    ret = parser_process_port_setting_changed(hTest,hTest->nVideoTrackNum);
    if (ret != OMX_ErrorNone)
        return ret;


    OMX_CONFIG_SENDAUDIOFIRST sSendAudioFirst;
    OMX_INIT_STRUCT(&sSendAudioFirst, OMX_CONFIG_SENDAUDIOFIRST);
    sSendAudioFirst.bSendAudioFrameFirst = OMX_FALSE;
    ret = OMX_SetConfig(hTest->hComponent,(OMX_INDEXTYPE)OMX_IndexConfigParserSendAudioFirst, &sSendAudioFirst);
    if (ret != OMX_ErrorNone)
        return ret;
//    do_seek(hTest, 30, OMX_TIME_SeekModeFast);
    /* Send output buffers */
    for(i=0; i<hTest->nBufferHdr[0]; i++) {
        hTest->pBufferHdr[0][i]->nFilledLen = 0;
        hTest->pBufferHdr[0][i]->nOffset = 0;
        OMX_FillThisBuffer(hTest->hComponent, hTest->pBufferHdr[0][i]);
    }

    for(i=0; i<hTest->nBufferHdr[1]; i++) {
        hTest->pBufferHdr[1][i]->nFilledLen = 0;
        hTest->pBufferHdr[1][i]->nOffset = 0;
        OMX_FillThisBuffer(hTest->hComponent, hTest->pBufferHdr[1][i]);
    }

    return ret;
}

HTEST * create_test(
        OMX_STRING component,
        OMX_STRING in_file)
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
    
    MakeDumpFileName(in_file,hTest->dump_file_path0,0);
    MakeDumpFileName(in_file,hTest->dump_file_path1,1);

    hTest->pOutFile0 = fopen(hTest->dump_file_path0, "wb");
    if(hTest->pOutFile0 == NULL) {
        printf("Failed to open file: %s\n", hTest->dump_file_path0);
        return 0;
    }
    hTest->pOutFile1 = fopen(hTest->dump_file_path1, "wb");
    if(hTest->pOutFile1 == NULL) {
        printf("Failed to open file: %s\n", hTest->dump_file_path1);
        return 0;
    }

    fsl_osal_sem_init(&(hTest->sParserFormatSem), 0, 0);
    return hTest;
}

OMX_ERRORTYPE delete_test(HTEST *hTest)
{
        
    fsl_osal_sem_destroy(hTest->sParserFormatSem);
    fclose(hTest->pOutFile0);
    fclose(hTest->pOutFile1);
    hTest->pMsgQ->Free();
    FSL_FREE(hTest);

    return OMX_ErrorNone;
}

int main(int argc, char *argv[])
{
    HTEST *hTest = NULL;
    OMX_STRING component = NULL;
    OMX_STRING in_file = NULL;

    if(argc < 2) {
        printf("Usage: ./bin <in_file>\n");
        return 0;
    }
    
    OMX_Init();
    component = "OMX.Freescale.std.parser.mp4.sw-based";
    in_file = argv[1];
    hTest = create_test(component, in_file);
    hTest->media_name = argv[1];
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
