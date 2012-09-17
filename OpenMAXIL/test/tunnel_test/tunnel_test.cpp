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
#include <unistd.h>
#include "OMX_Implement.h"
#include "fsl_osal.h"
#include "Mem.h"
#include "Log.h"

#include "OMX_Core.h"
#include "OMX_Component.h"    
#include "Queue.h"

#define MAX_TUNNELED_COMPONENT 3
#define MAX_PORT_NUM 8
#define MAX_PORT_BUFFER 32

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

typedef struct _HCOMPONENT {
    OMX_COMPONENTTYPE *hComponent;
    OMX_U32 nPorts;
    OMX_BOOL bPortTunneled[MAX_PORT_NUM];
    OMX_BOOL bAllocater[MAX_PORT_NUM];
    OMX_U32 nBufferHdr[MAX_PORT_NUM];
    OMX_DIRTYPE PortDir[MAX_PORT_NUM];
    OMX_BUFFERHEADERTYPE *pBufferHdr[MAX_PORT_NUM][MAX_PORT_BUFFER];
    EVENT_TYPE sCmdDone;
    OMX_BOOL bError;
}HCOMPONENT;

typedef struct _HTEST{
    OMX_U32 nComponents;
    HCOMPONENT component[MAX_TUNNELED_COMPONENT];
    OMX_STATETYPE eState;
    Queue *pMsgQ;
    fsl_osal_ptr pThreadId;
    OMX_BOOL bStop;
    OMX_BOOL bError;
    OMX_BOOL bHoldBuffers;
    OMX_PTR pLibHandle[MAX_TUNNELED_COMPONENT];
    char *lib_name[MAX_TUNNELED_COMPONENT];
    char *itf_name[MAX_TUNNELED_COMPONENT];
    FILE *pInFile;
    FILE *pOutFile;
}HTEST;

OMX_ERRORTYPE ComponentStateTrans(
        HCOMPONENT *component, 
        OMX_STATETYPE eState);
OMX_ERRORTYPE WaitComponentStateTransDone(
        HCOMPONENT *component, 
        OMX_STATETYPE eState);
OMX_ERRORTYPE SendCommand2Component(
        HCOMPONENT *component, 
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam1,
        OMX_PTR pCmdData);
OMX_ERRORTYPE Load2Idle(HCOMPONENT *component);
OMX_ERRORTYPE Idle2Load(HCOMPONENT *component);
OMX_ERRORTYPE WaitComponentCommand(
        HCOMPONENT *component, 
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam1,
        OMX_PTR pCmdData);
OMX_ERRORTYPE prepare_port_buffers(HCOMPONENT *component, OMX_U32 nPortIndex);
OMX_ERRORTYPE free_port_buffers(HCOMPONENT *component, OMX_U32 nPortIndex);
OMX_ERRORTYPE start_data_process(HTEST *hTest);
void * process_thread(void *ptr);
OMX_ERRORTYPE process_event(HTEST *hTest, MSG *pMsg);

OMX_ERRORTYPE read_data(HTEST *hTest, OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    OMX_U32 size = 0;

    size = fread(pBufferHdr->pBuffer, 1, pBufferHdr->nAllocLen, hTest->pInFile);
    pBufferHdr->nFilledLen = size;
    pBufferHdr->nOffset = 0;

    if(size < pBufferHdr->nAllocLen)
        pBufferHdr->nFlags = OMX_BUFFERFLAG_EOS;

    return OMX_ErrorNone;

}

OMX_ERRORTYPE write_data(HTEST *hTest, OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    fwrite(pBufferHdr->pBuffer, 1, pBufferHdr->nFilledLen, hTest->pOutFile);
    fflush(hTest->pOutFile);
    pBufferHdr->nFilledLen = 0;
    pBufferHdr->nOffset = 0;
    if(pBufferHdr->nFlags & OMX_BUFFERFLAG_EOS)
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
    OMX_U32 i;

    printf("Client receive event [eEvent:%d, nData1: %d, nData2: %d]\n",
            (int)eEvent, (int)nData1, (int)nData2);

    for(i=0; i<hTest->nComponents; i++) {
        if(hComponent == hTest->component[i].hComponent)
            break;
    }

    if(eEvent == OMX_EventCmdComplete) {
        hTest->component[i].sCmdDone.eEvent = OMX_EventCmdComplete;
        hTest->component[i].sCmdDone.nData1 = nData1;
        hTest->component[i].sCmdDone.nData2 = nData2;
        hTest->component[i].sCmdDone.pEventData = pEventData;
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

OMX_ERRORTYPE load_component(HTEST *hTest, OMX_U32 idx)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *hComponent = NULL;
    OMX_COMPONENTINITTYPE pInit = NULL;
    OMX_STRING cError;

    hTest->pLibHandle[idx] = dlopen(hTest->lib_name[idx], RTLD_NOW);
    if(hTest->pLibHandle[idx] == NULL) {
        cError = dlerror();
        printf("%s\n", cError);
        return OMX_ErrorInvalidComponentName;
    }

    pInit = (OMX_COMPONENTINITTYPE)dlsym(hTest->pLibHandle[idx], hTest->itf_name[idx]);
    if(pInit == NULL) {
        cError = dlerror();
        printf("%s\n", cError);
        return OMX_ErrorInvalidComponent;
    }

    hComponent = (OMX_COMPONENTTYPE*)fsl_osal_malloc_new(sizeof(OMX_COMPONENTTYPE));
    if(hComponent == NULL) {
        printf("Failed to allocate memory for hComponent.\n");
        return OMX_ErrorInsufficientResources;
    }
    OMX_INIT_STRUCT(hComponent, OMX_COMPONENTTYPE);

    ret = pInit((OMX_HANDLETYPE)(hComponent));
    if(ret != OMX_ErrorNone) {
        printf("Load component failed.\n");
        return OMX_ErrorInvalidComponentName;
    }

    hComponent->SetCallbacks(hComponent, &gCallBacks, hTest);
    hTest->component[idx].hComponent = hComponent;


    return OMX_ErrorNone;
}

OMX_ERRORTYPE load_components(HTEST *hTest)
{
    OMX_U32 i;

    for(i=0; i<hTest->nComponents; i++)
        load_component(hTest, i);

    hTest->eState = OMX_StateLoaded;

    fsl_osal_thread_create(&hTest->pThreadId, NULL, process_thread, hTest);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE unload_components(HTEST *hTest)
{
    OMX_U32 i;

    for(i=0; i<hTest->nComponents; i++)
        hTest->component[i].hComponent->ComponentDeInit(hTest->component[i].hComponent);

    fsl_osal_thread_destroy(hTest->pThreadId);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE tunnel_component(
        OMX_COMPONENTTYPE *hAComponent, 
        OMX_U32 nAPort, 
        OMX_COMPONENTTYPE *hBComponent,
        OMX_U32 nBPort)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_TUNNELSETUPTYPE sTunnelSetup;

    hAComponent->ComponentTunnelRequest(hAComponent, nAPort, hBComponent, nBPort, &sTunnelSetup);
    ret = hBComponent->ComponentTunnelRequest(hBComponent, nBPort, hAComponent, nAPort, &sTunnelSetup);
    if(ret != OMX_ErrorNone)
        hAComponent->ComponentTunnelRequest(hAComponent, nAPort, NULL, 0, NULL);

    return ret;
}

OMX_ERRORTYPE ForwardTrans(
        HTEST *hTest,
        OMX_STATETYPE eState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i;

    for(i=0; i<hTest->nComponents; i++) {
        ret = ComponentStateTrans(&hTest->component[i], eState);
        if(ret != OMX_ErrorNone)
            return ret;
    }

    for(i=0; i<hTest->nComponents; i++) {
        ret = WaitComponentStateTransDone(&hTest->component[i], eState);
        if(ret != OMX_ErrorNone)
            return ret;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE BackwardTrans(
        HTEST *hTest,
        OMX_STATETYPE eState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_S32 i;

    for(i=hTest->nComponents-1; i>=0; i--) {
        ret = ComponentStateTrans(&hTest->component[i], eState);
        if(ret != OMX_ErrorNone)
            return ret;
    }

    for(i=hTest->nComponents-1; i>=0; i--) {
        ret = WaitComponentStateTransDone(&hTest->component[i], eState);
        if(ret != OMX_ErrorNone)
            return ret;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE StateTrans(
        HTEST *hTest, 
        OMX_STATETYPE eState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    /* Loaded->Idle */
    if(hTest->eState == OMX_StateLoaded && eState == OMX_StateIdle)
        BackwardTrans(hTest, eState);
    /* Idle->Loaded */
    else if(hTest->eState == OMX_StateIdle && eState == OMX_StateLoaded)
        BackwardTrans(hTest, eState);
    /* Exec->Idle */
    else if((hTest->eState == OMX_StateExecuting || hTest->eState == OMX_StatePause)
            && eState == OMX_StateIdle) {
        hTest->bHoldBuffers = OMX_TRUE;
        ForwardTrans(hTest, eState);
    }
    else
        ForwardTrans(hTest, eState);

    /* Idle->Exec/Idle->Pause done */
    if(hTest->eState == OMX_StateIdle && (eState == OMX_StateExecuting || eState == OMX_StatePause))
        start_data_process(hTest);

    printf("State trans: [%x] -> [%d] done.\n", hTest->eState, eState);

    hTest->eState = eState;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE tunneled_port_pair_disable(HTEST *hTest)
{
    OMX_COMPONENTTYPE *hComponent;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    /* disable supplier port firstly */
    ret = SendCommand2Component(&hTest->component[0], OMX_CommandPortDisable, 1, NULL);
    if(ret != OMX_ErrorNone)
        return ret;

    /* then disable non-supplier port */
    ret = SendCommand2Component(&hTest->component[1], OMX_CommandPortDisable, 0, NULL);
    if(ret != OMX_ErrorNone)
        return ret;

    WaitComponentCommand(&hTest->component[0], OMX_CommandPortDisable, 1, NULL);
    WaitComponentCommand(&hTest->component[1], OMX_CommandPortDisable, 0, NULL);

    printf("port disbale done.\n");

    return OMX_ErrorNone;
}

OMX_ERRORTYPE tunneled_port_pair_enable(HTEST *hTest)
{
    OMX_COMPONENTTYPE *hComponent;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    /* enable non-supplier port firstly */
    ret = SendCommand2Component(&hTest->component[1], OMX_CommandPortEnable, 0, NULL);
    if(ret != OMX_ErrorNone)
        return ret;

    /* then enable supplier port */
    ret = SendCommand2Component(&hTest->component[0], OMX_CommandPortEnable, 1, NULL);
    if(ret != OMX_ErrorNone)
        return ret;

    WaitComponentCommand(&hTest->component[1], OMX_CommandPortEnable, 0, NULL);
    WaitComponentCommand(&hTest->component[0], OMX_CommandPortEnable, 1, NULL);

    printf("port enbale done.\n");

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentStateTrans(
        HCOMPONENT *component, 
        OMX_STATETYPE eState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *hComponent = NULL;
    OMX_STATETYPE eCurState = OMX_StateInvalid;

    hComponent = component->hComponent;
    OMX_GetState(hComponent, &eCurState);
    ret = SendCommand2Component(component, OMX_CommandStateSet,eState,NULL);
    if(ret != OMX_ErrorNone) {
        printf("State trans to %d failed.\n", eState);
        return ret;
    }

    /* Loaded->Idle */
    if(eCurState == OMX_StateLoaded && eState == OMX_StateIdle)
        Load2Idle(component);
    /* Idle->Loaded */
    else if(eCurState == OMX_StateIdle && eState == OMX_StateLoaded)
        Idle2Load(component);

    return ret;
}

OMX_ERRORTYPE WaitComponentStateTransDone(
        HCOMPONENT *component, 
        OMX_STATETYPE eState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = WaitComponentCommand(component, OMX_CommandStateSet, eState, NULL);
    if(ret != OMX_ErrorNone)
        return ret;


    return ret;
}

OMX_ERRORTYPE SendCommand2Component(
        HCOMPONENT *component, 
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam1,
        OMX_PTR pCmdData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_HANDLETYPE hComponent = component->hComponent;

    component->sCmdDone.eEvent = OMX_EventMax;
    component->bError = OMX_FALSE;
    ret = OMX_SendCommand(hComponent, Cmd, nParam1, pCmdData);
    if(ret != OMX_ErrorNone && ret != OMX_ErrorNotComplete)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE WaitComponentCommand(
        HCOMPONENT *component, 
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam1,
        OMX_PTR pCmdData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    while(1) {
        if(component->sCmdDone.eEvent == OMX_EventCmdComplete
                && component->sCmdDone.nData1 == Cmd
                && component->sCmdDone.nData2 == nParam1
                && component->sCmdDone.pEventData == pCmdData)
            break;
        else if(component->bError == OMX_TRUE) {
            ret = OMX_ErrorUndefined;
            break;
        }
        else
            usleep(10000);
    }

    return ret;
}


OMX_ERRORTYPE Load2Idle(HCOMPONENT *component)
{
    OMX_U32 i;
    for(i=0; i<component->nPorts; i++) {
        if(component->bPortTunneled[i] != OMX_TRUE)
            prepare_port_buffers(component, i);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE prepare_port_buffers(HCOMPONENT *component, OMX_U32 nPortIndex)
{
    OMX_COMPONENTTYPE *hComponent = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    OMX_U8 *pBuffer = NULL;
    OMX_U32 i;

    hComponent = component->hComponent;
    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
    for(i=0; i<sPortDef.nBufferCountActual; i++) {
        if(component->bAllocater[nPortIndex] == OMX_TRUE) {
            OMX_AllocateBuffer(hComponent, &pBufferHdr, nPortIndex, NULL, sPortDef.nBufferSize);
            printf("Allocate buffer done.\n");
        }
        else {
            pBuffer = (OMX_U8*)FSL_MALLOC(sPortDef.nBufferSize);
            OMX_UseBuffer(hComponent, &pBufferHdr, nPortIndex, NULL, sPortDef.nBufferSize, pBuffer);
            printf("Use buffer done.\n");
        }
        component->pBufferHdr[nPortIndex][i] = pBufferHdr;
    }
    component->nBufferHdr[nPortIndex] = sPortDef.nBufferCountActual;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Idle2Load(HCOMPONENT *component)
{
    OMX_U32 i;

    for(i=0; i<component->nPorts; i++) {
        if(component->bPortTunneled[i] != OMX_TRUE)
            free_port_buffers(component, i);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE free_port_buffers(HCOMPONENT *component, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *hComponent;
    OMX_U32 i;

    hComponent = component->hComponent;
    for(i=0; i<component->nBufferHdr[nPortIndex]; i++) {
        OMX_U8 *ptr = component->pBufferHdr[nPortIndex][i]->pBuffer;
        while(1) {
            ret = OMX_FreeBuffer(hComponent, nPortIndex, component->pBufferHdr[nPortIndex][i]);
            if(ret == OMX_ErrorNotReady)
                usleep(10000);
            else
                break;
        }
        component->pBufferHdr[nPortIndex][i] = 0;
        if(component->bAllocater[nPortIndex] == OMX_FALSE)
            FSL_FREE(ptr);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE start_data_process(HTEST *hTest)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i;

    hTest->bHoldBuffers = OMX_FALSE;

    /* Send output buffers */
    for(i=0; i<hTest->component[1].nBufferHdr[1]; i++) {
        hTest->component[1].pBufferHdr[1][i]->nFilledLen = 0;
        hTest->component[1].pBufferHdr[1][i]->nOffset = 0;
        OMX_FillThisBuffer(hTest->component[1].hComponent, hTest->component[1].pBufferHdr[1][i]);
    }

    /* Send input buffers */
    for(i=0; i<hTest->component[0].nBufferHdr[0]; i++) {
        read_data(hTest, hTest->component[0].pBufferHdr[0][i]);
        OMX_EmptyThisBuffer(hTest->component[0].hComponent, hTest->component[0].pBufferHdr[0][i]);
    }

    return ret;
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

OMX_ERRORTYPE process_event(HTEST *hTest, MSG *pMsg)
{
    printf("Client receive event [eEvent:%d, nData1: %d, nData2: %d]\n",
            (int)pMsg->data.event.eEvent, 
            (int)pMsg->data.event.nData1, 
            (int)pMsg->data.event.nData2);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE cmd_process(HTEST *hTest)
{
    char rep[10];
    OMX_BOOL bExit = OMX_FALSE;

    while(bExit == OMX_FALSE) {
        printf("Input test cmd:\n");
        printf("[l]load component\n");
        printf("[t]tunnel component\n");
        printf("[s]state trans\n");
        printf("[d]port disbale\n");
        printf("[e]port enable\n");
        printf("[x]exit\n");
        scanf("%s", rep);
        if(rep[0] == 'l')
            load_components(hTest);
        else if(rep[0] == 't') {
            OMX_U32 aIdx, aPort, bIdx, bPort;
            printf("Tunnel method:\n");
            printf("AIdx,APort,BIdx,BPort\n");
            scanf("%d,%d,%d,%d", &aIdx, &aPort, &bIdx, &bPort);
            hTest->component[aIdx].bPortTunneled[aPort] = OMX_TRUE;
            hTest->component[bIdx].bPortTunneled[bPort] = OMX_TRUE;
            tunnel_component(hTest->component[aIdx].hComponent, aPort, 
                    hTest->component[bIdx].hComponent, bPort);
        }
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
        else if(rep[0] == 'd') {
            tunneled_port_pair_disable(hTest);
        }
        else if(rep[0] == 'e') {
            tunneled_port_pair_enable(hTest);
        }
        else if(rep[0] == 'x') {
            MSG sMsg;
            sMsg.type = EXIT;
            hTest->pMsgQ->Add(&sMsg);
            hTest->bStop = OMX_TRUE;
            bExit = OMX_TRUE;
            unload_components(hTest);
        }
    }

    return OMX_ErrorNone;
}


int main(int argc, char *argv[])
{
    HTEST *hTest = NULL;

    if(argc < 3) {
        printf("Usage: ./bin <in_file> <out_file>\n");
        return 0;
    }

    hTest = (HTEST*)fsl_osal_malloc_new(sizeof(HTEST));
    if(hTest == NULL) {
        printf("Failed to allocate memory for test handle.\n");
        return 0;
    }
    fsl_osal_memset(hTest, 0, sizeof(HTEST));

    hTest->nComponents = 2;

    hTest->lib_name[0] = "../lib/lib_omx_template_arm11_elinux.so";
    hTest->itf_name[0] = "TemplateInit";
    hTest->component[0].nPorts = 2;
    hTest->component[0].PortDir[0] = OMX_DirInput;
    hTest->component[0].bAllocater[0] = OMX_TRUE;
    hTest->component[0].PortDir[1] = OMX_DirOutput;
    hTest->component[0].bAllocater[1] = OMX_TRUE;

    hTest->lib_name[1] = "../lib/lib_omx_template_arm11_elinux.so";
    hTest->itf_name[1] = "TemplateInit";
    hTest->component[1].nPorts = 2;
    hTest->component[1].PortDir[0] = OMX_DirInput;
    hTest->component[1].bAllocater[0] = OMX_TRUE;
    hTest->component[1].PortDir[1] = OMX_DirOutput;
    hTest->component[1].bAllocater[1] = OMX_TRUE;

    hTest->pMsgQ = FSL_NEW(Queue, ());
    if(hTest->pMsgQ == NULL) {
        printf("Create message queue failed.\n");
        return 0;
    }
    hTest->pMsgQ->Create(128, sizeof(MSG), E_FSL_OSAL_TRUE);

    hTest->pInFile = fopen(argv[1], "rb");
    if(hTest->pInFile == NULL) {
        printf("Failed to open file: %s\n", argv[1]);
        return 0;
    }

    hTest->pOutFile = fopen(argv[2], "wb");
    if(hTest->pOutFile == NULL) {
        printf("Failed to open file: %s\n", argv[2]);
        return 0;
    }

    cmd_process(hTest);

    return 1;
}

/* File EOF */
