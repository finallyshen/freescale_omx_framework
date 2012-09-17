/**
 *  Copyright (c) 2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

 
/* Header files */

#include <ctype.h>
#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>

#include "OMX_Component.h"
#include "Mem.h"
#include "video_decoder_test.h"


/* Macro definitions */

#define DEBUG_PRINT(...) /*printf( __VA_ARGS__)*/
#define INFO_PRINT(...) printf( __VA_ARGS__)

/* Type definitions */

/* Function declarations */

static OMX_ERRORTYPE VDT_EventHandlerCallback(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData);

static OMX_ERRORTYPE VDT_EmptyBufferDoneCallback(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE VDT_FillBufferDoneCallback(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

/* Global variable definitions */

static OMX_CALLBACKTYPE gVDTCallbacks = {
    VDT_EventHandlerCallback,
    VDT_EmptyBufferDoneCallback,
    VDT_FillBufferDoneCallback
};

static const OMX_STRING PARSER_NAME = "OMX.Freescale.std.parser.fsl.sw-based";
static const OMX_STRING PARSER_ROLE = "parser.avi";

static const OMX_STRING AVC_DECODER_NAME = "OMX.Freescale.std.video_decoder.avc.hw-based";
static const OMX_STRING AVC_DECODER_ROLE = "video_decoder.avc";

static const OMX_STRING MPEG4_DECODER_NAME = "OMX.Freescale.std.video_decoder.avc.hw-based";
static const OMX_STRING MPEG4_DECODER_ROLE = "video_decoder.mpeg4";

static const OMX_STRING V4L_RENDER_NAME = "OMX.Freescale.std.video_render.v4l.sw-based";


/* Function definitions */


/*******************************************************************************
* Function: VDT_FrameMessage
*
* Description: This function is used to frame a message to be posted
*               in any of the 3 queues -Event queue,Empty pBuffer queue or
*               Filled Buffer Queue.
*
* Returns: OMX_ERRORTYPE
*
* Arguments: type - Type of message to be posted
*               ppMsg - Pointer to pointer to message to be posted
*
********************************************************************************/
OMX_ERRORTYPE VDT_FrameMessage(Message **ppMsg, Message_type type)
{
    Message* pMsg = NULL;
    fsl_osal_ptr pBuffer = NULL;

    /*Allocating memory for message*/
    pBuffer = FSL_MALLOC(sizeof(Message));
    if(pBuffer == NULL)
        return OMX_ErrorInsufficientResources;

    pMsg = (Message *)pBuffer;
    fsl_osal_memset((fsl_osal_ptr)pMsg, 0, sizeof(Message));

    pMsg->MsgType = type;
    *ppMsg = pMsg;
    return OMX_ErrorNone;
}



/*******************************************************************************
* Function: VDT_PostMessage
*
* Description: This function is used to post a message in any of the 3 queues,
*               Event queue, Empty Buffer queue or Filled Buffer Queue.
*
* Returns: OMX_ERRORTYPE
*
* Arguments: pomx_client_params - Pointer to ILClient structure
*               pMsg - Pointer to message to be posted
*
********************************************************************************/
OMX_ERRORTYPE VDT_PostMessage(AppHandler *pAppHandler, Message *pMsg)
{
    QUEUE_ERRORTYPE eRetVal = QUEUE_FAILURE;

    switch(pMsg->MsgType)
    {
        case DataBuffer:
        {
            if(pMsg->data.type == EmptyBuffer)
            {
                eRetVal = EnQueue(pAppHandler->EmptyBufQueue, (OMX_PTR)pMsg, E_FSL_OSAL_FALSE);
                if(eRetVal != QUEUE_SUCCESS)
                    return OMX_ErrorUndefined;
            }
            else if(pMsg->data.type == FilledBuffer)
            {
                eRetVal = EnQueue(pAppHandler->FilledBufQueue, (OMX_PTR)pMsg, E_FSL_OSAL_FALSE);
                if(eRetVal != QUEUE_SUCCESS)
                    return OMX_ErrorUndefined;
            }
            break;
        }
        case Eventype:
        {
            eRetVal = EnQueue(pAppHandler->EventQueue, (OMX_PTR)pMsg, E_FSL_OSAL_FALSE);
            if(eRetVal != QUEUE_SUCCESS)
                return OMX_ErrorUndefined;
            break;
        }
        default:
        break;
    }

    return OMX_ErrorNone;
}



/*******************************************************************************
* Function: VDT_ReadEventQueue
*
* Description: This function returns an entry from the event queue.
*
* Returns:   void
*
* Arguments: AppHandler - Pointer to an application structure
*            Message *  - Pointer to a message
*
********************************************************************************/
void VDT_ReadEventQueue(AppHandler *pAppHandler, Message *pMsg)
{
    assert(pMsg != 0);

    fsl_osal_memset((fsl_osal_ptr)pMsg, 0, sizeof(Message));
    ReadQueue(pAppHandler->EventQueue, pMsg, E_FSL_OSAL_TRUE);

    return ;
}


/*******************************************************************************
  Function: VDT_Initialize

  Description: This function is used to initialize the parameters of
                Video IL Client structure.

  Returns: OMX_ERRORTYPE

  Arguments: pVDTClientParams - Pointer to Video ILClient structure

********************************************************************************/
static OMX_ERRORTYPE VDT_Initialize(VDT_CLIENT_PARAMS *pVDTClientParams)
{
    QUEUE_ERRORTYPE eRetQueue = QUEUE_SUCCESS;

    pVDTClientParams->hDecoder = NULL;
    pVDTClientParams->hParser = NULL;
    pVDTClientParams->hRender = NULL;
    pVDTClientParams->hContentPipe = NULL;

    pVDTClientParams->bsetting_changed = OMX_FALSE;

    pVDTClientParams->bEOS = OMX_FALSE;

    pVDTClientParams->nParserAudioPortIdx = 0;
    pVDTClientParams->nParserVideoPortIdx = 1;

    eRetQueue = CreateQueue(&pVDTClientParams->sAppHandler.EventQueue,
                            MAX_OMXQUEUE_SIZE,
                            E_FSL_OSAL_TRUE,
                            sizeof(Message));

    if(eRetQueue != QUEUE_SUCCESS)
        return OMX_ErrorInsufficientResources;

    eRetQueue = CreateQueue(&pVDTClientParams->sAppHandler.EmptyBufQueue,
                            MAX_OMXQUEUE_SIZE,
                            E_FSL_OSAL_FALSE,
                            sizeof(Message));
    if(eRetQueue != QUEUE_SUCCESS)
        return OMX_ErrorInsufficientResources;

    eRetQueue = CreateQueue(&pVDTClientParams->sAppHandler.FilledBufQueue,
                            MAX_OMXQUEUE_SIZE,
                            E_FSL_OSAL_FALSE,
                            sizeof(Message));
    if(eRetQueue != QUEUE_SUCCESS)
        return OMX_ErrorInsufficientResources;

    return OMX_ErrorNone;
}



/*******************************************************************************
  Function: VDT_DeInitialize

  Description: This function is used to de-initialize the parameters of
            Video IL Client structure.

  Returns: OMX_ERRORTYPE

  Arguments: pVDTClientParams - Pointer to Video ILClient structure

********************************************************************************/

static OMX_ERRORTYPE VDT_DeInitialize(VDT_CLIENT_PARAMS *pVDTClientParams)
{
    QUEUE_ERRORTYPE eRetVal = QUEUE_FAILURE;
    Message         sMsg;

    while(GetQueueSize(pVDTClientParams->sAppHandler.EventQueue) > 0)
        ReadQueue(pVDTClientParams->sAppHandler.EventQueue, &sMsg, E_FSL_OSAL_TRUE);

    while(GetQueueSize(pVDTClientParams->sAppHandler.EmptyBufQueue) > 0)
        ReadQueue(pVDTClientParams->sAppHandler.EmptyBufQueue, &sMsg, E_FSL_OSAL_TRUE);

    while(GetQueueSize(pVDTClientParams->sAppHandler.FilledBufQueue) > 0)
        ReadQueue(pVDTClientParams->sAppHandler.FilledBufQueue, &sMsg, E_FSL_OSAL_TRUE);

    eRetVal = DeleteQueue(pVDTClientParams->sAppHandler.EventQueue);
    if(eRetVal != QUEUE_SUCCESS)
        return OMX_ErrorUndefined;

    eRetVal = DeleteQueue(pVDTClientParams->sAppHandler.EmptyBufQueue);
    if(eRetVal != QUEUE_SUCCESS)
        return OMX_ErrorUndefined;

    eRetVal = DeleteQueue(pVDTClientParams->sAppHandler.FilledBufQueue);
    if(eRetVal != QUEUE_SUCCESS)
        return OMX_ErrorUndefined;


    return OMX_ErrorNone;
}


/*******************************************************************************
 Function       : VDT_LoadParser

 Description    : This function is used to initialize parser component

 Returns        : OMX_ERRORTYPE

 Arguments      : pVDTClientParams - Pointer to ILClient structure

********************************************************************************/

static OMX_ERRORTYPE VDT_LoadParser(VDT_CLIENT_PARAMS *pVDTClientParams)
{
    OMX_ERRORTYPE           eRetVal = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE    *pBufferHdr;
    OMX_U8                  *ptr = NULL;
    OMX_S8                  cSourceComponentName[128];
    CP_PIPETYPE             *hPipe;
    OMX_PARAM_CONTENTURITYPE *Content;
    OMX_U8                  len;

    /* Load component */
    
    fsl_osal_strcpy((fsl_osal_char *)cSourceComponentName,(fsl_osal_char *)PARSER_NAME); 
    eRetVal = OMX_GetHandle(&pVDTClientParams->hParser,
                            (OMX_STRING)cSourceComponentName,
                            &pVDTClientParams->sAppHandler,
                            &gVDTCallbacks);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* set Parser role */

    OMX_STRING role = PARSER_ROLE;
    OMX_PARAM_COMPONENTROLETYPE CurRole;
    OMX_INIT_STRUCT(&CurRole, OMX_PARAM_COMPONENTROLETYPE);
    fsl_osal_memcpy(&CurRole.cRole, role, OMX_MAX_STRINGNAME_SIZE);
    OMX_SetParameter(pVDTClientParams->hParser, 
                     OMX_IndexParamStandardComponentRole, 
                     &CurRole);

    DEBUG_PRINT("set parser role ok\n");

    /* set URI */
    
    len = fsl_osal_strlen((fsl_osal_char*)pVDTClientParams->pInfilename) + 1;
    ptr = (OMX_U8*)FSL_MALLOC(sizeof(OMX_PARAM_CONTENTURITYPE) + len);
    if(ptr == NULL) {
        return OMX_ErrorInsufficientResources;
    }

    Content = (OMX_PARAM_CONTENTURITYPE*)ptr;
    OMX_INIT_STRUCT(Content, OMX_PARAM_CONTENTURITYPE);
    fsl_osal_strcpy((fsl_osal_char*)(&Content->contentURI), (fsl_osal_char*)pVDTClientParams->pInfilename);

    DEBUG_PRINT("Loading content: %s\n", pVDTClientParams->pInfilename);

    eRetVal = OMX_SetParameter(pVDTClientParams->hParser, 
                               OMX_IndexParamContentURI, 
                               Content);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* content pipe */
    
    eRetVal = OMX_GetContentPipe((void**)&hPipe, (OMX_STRING)"LOCAL_FILE_PIPE_NEW");
    if(hPipe == NULL)
        return OMX_ErrorContentPipeCreationFailed;

    OMX_PARAM_CONTENTPIPETYPE sPipe;
    OMX_INIT_STRUCT(&sPipe, OMX_PARAM_CONTENTPIPETYPE);
    sPipe.hPipe = hPipe;
    eRetVal = OMX_SetParameter(pVDTClientParams->hParser, 
                               OMX_IndexParamCustomContentPipe, 
                               &sPipe);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    DEBUG_PRINT("Parser GetContentPipe ok\n");

    /* disable audio port */

    eRetVal = OMX_SendCommand(pVDTClientParams->hParser, 
                              OMX_CommandPortDisable, 
                              pVDTClientParams->nParserAudioPortIdx, 
                              NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    do {
        fsl_osal_sleep(100000);
    } while (pVDTClientParams->sAppHandler.eState != (OMX_STATETYPE)OMX_CommandPortDisable);

    /* audo detect video fmt */

    OMX_VIDEO_PARAM_PORTFORMATTYPE sVideoPortFmt;
    OMX_INIT_STRUCT(&sVideoPortFmt, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    sVideoPortFmt.nPortIndex = pVDTClientParams->nParserVideoPortIdx;
    
    eRetVal = OMX_GetParameter(pVDTClientParams->hParser, 
                               OMX_IndexParamVideoPortFormat, 
                               &sVideoPortFmt);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    sVideoPortFmt.eCompressionFormat = OMX_VIDEO_CodingAutoDetect;
    
    eRetVal = OMX_SetParameter(pVDTClientParams->hParser, 
                               OMX_IndexParamVideoPortFormat, 
                               &sVideoPortFmt);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;
    pVDTClientParams->VideoFmt = OMX_VIDEO_CodingAutoDetect;

    DEBUG_PRINT("Parser set audo detect ok\n");

    /* set parser to idle */

    eRetVal = OMX_SendCommand(pVDTClientParams->hParser, 
                          OMX_CommandStateSet, 
                          OMX_StateIdle, 
                          NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    DEBUG_PRINT("Parser set idle ok\n");

    /* parser allocate buffer */
    
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef ;
    OMX_U32 i;
    
    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = pVDTClientParams->nParserVideoPortIdx;
    eRetVal = OMX_GetParameter(pVDTClientParams->hParser, 
                               OMX_IndexParamPortDefinition, 
                               &sPortDef);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    pVDTClientParams->nVESBufSize = sPortDef.nBufferSize;
    pVDTClientParams->nVESBufCnt  = sPortDef.nBufferCountActual;
    /*
    DEBUG_PRINT("Parser Port 1: size %d, Cnt %d, Dir %d, Enabled %d Populated %d\n",
                    sPortDef.nBufferSize, sPortDef.nBufferCountActual, sPortDef.eDir, sPortDef.bEnabled, sPortDef.bPopulated);
    */
    
    for(i=0; i<pVDTClientParams->nVESBufCnt; i++) {
        pBufferHdr = NULL;
        eRetVal = OMX_AllocateBuffer(pVDTClientParams->hParser, 
                                     &pBufferHdr, 
                                     pVDTClientParams->nParserVideoPortIdx, 
                                     NULL, 
                                     pVDTClientParams->nVESBufSize);
        if(eRetVal != OMX_ErrorNone) {
            INFO_PRINT("%s allocate buffer for port %d failed.\n", "Parser", 1);
            return eRetVal;
        }
        pVDTClientParams->pParserOutBufHdr[i] = pBufferHdr;
        /*
        DEBUG_PRINT("%s allocate buffer %p:%p for port %d\n", "Parser", pBufferHdr, pBufferHdr->pBuffer, 1); 
        */
    }


    /* parser wait to idle */
    
    do {
        OMX_GetState(pVDTClientParams->hParser, &pVDTClientParams->sAppHandler.eState);
    } while (pVDTClientParams->sAppHandler.eState != OMX_StateIdle);


    /* set parser to exec */

    eRetVal = OMX_SendCommand(pVDTClientParams->hParser, 
                                OMX_CommandStateSet, 
                                OMX_StateExecuting, 
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;


    /* parser wait for exec */
    
    do {
        OMX_GetState(pVDTClientParams->hParser, &pVDTClientParams->sAppHandler.eState);
    } while (pVDTClientParams->sAppHandler.eState != OMX_StateExecuting);

    DEBUG_PRINT("parser wait for exec OK!\n");

    /* parser set stream number */

    OMX_PARAM_U32TYPE u32Type;
    OMX_INIT_STRUCT(&u32Type, OMX_PARAM_U32TYPE);
    u32Type.nPortIndex = pVDTClientParams->nParserVideoPortIdx;
    OMX_GetParameter(pVDTClientParams->hParser, 
                     OMX_IndexParamNumAvailableStreams, 
                     &u32Type);
    DEBUG_PRINT("Total video track number: %ld\n", u32Type.nU32);

    if(u32Type.nU32 > 0) {
        u32Type.nU32 = 0;
        OMX_SetParameter(pVDTClientParams->hParser, 
                         OMX_IndexParamActiveStream, 
                         &u32Type);
    }

    
    /* free resources */


    FSL_FREE(Content);

    return OMX_ErrorNone;
}


/*******************************************************************************
 Function       : VDT_UnLoadParser

 Description    : This function is used to uninitialize parser component

 Returns        : OMX_ERRORTYPE

 Arguments      : pVDTClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE VDT_UnLoadParser(VDT_CLIENT_PARAMS *pVDTClientParams)
{
    OMX_ERRORTYPE       eRetVal = OMX_ErrorNone;

    if(pVDTClientParams->hParser == NULL)
        return OMX_ErrorNone;
    
    eRetVal = OMX_SendCommand(pVDTClientParams->hParser,
                                OMX_CommandStateSet,
                                OMX_StateIdle,
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    do
    {
        OMX_GetState(pVDTClientParams->hParser, &pVDTClientParams->sAppHandler.eState);
    }while (pVDTClientParams->sAppHandler.eState != OMX_StateIdle);

    eRetVal = OMX_SendCommand(pVDTClientParams->hParser,
                             OMX_CommandStateSet,
                             OMX_StateLoaded,
                             NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* Free Buffers for parser */

    OMX_U32 i;
    for(i=0; i<pVDTClientParams->nVESBufCnt; i++) {
        eRetVal = OMX_FreeBuffer(pVDTClientParams->hParser,
                                1, 
                                pVDTClientParams->pParserOutBufHdr[i]);
        if(eRetVal != OMX_ErrorNone) {
            INFO_PRINT("%s free buffer for port 1 failed idx %ld retval %x.\n", "Parser", i, eRetVal);
            return eRetVal;
        }
    }

    do
    {
        OMX_GetState(pVDTClientParams->hParser, &pVDTClientParams->sAppHandler.eState);
    }while (pVDTClientParams->sAppHandler.eState != OMX_StateLoaded);

    return OMX_ErrorNone;
    
}


/*******************************************************************************
 Function       : VDT_LoadVideoDecoder

 Description    : This function is used to initialize video decoder component

 Returns        : OMX_ERRORTYPE

 Arguments      : pVDTClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE VDT_LoadVideoDecoder(VDT_CLIENT_PARAMS *pVDTClientParams)
{
    OMX_PARAM_PORTDEFINITIONTYPE    sParamPort;
    OMX_ERRORTYPE                   eRetVal = OMX_ErrorNone;
    OMX_U16                         nCounter = 0;
    OMX_BUFFERHEADERTYPE            *pBufferHdr;
    OMX_PTR                         pBuffer = NULL;
    OMX_S8                          cComponentName[VIDEOD_COMP_NAME_LENGTH];
    OMX_S8                          role[VIDEOD_COMP_NAME_LENGTH] ;
    
    switch(pVDTClientParams->VideoFmt)
    {
        case OMX_VIDEO_CodingAVC:
            fsl_osal_strcpy((fsl_osal_char *)cComponentName,(fsl_osal_char *)AVC_DECODER_NAME);
            fsl_osal_strcpy((fsl_osal_char *)role,(fsl_osal_char *)AVC_DECODER_ROLE);
            break;

        case OMX_VIDEO_CodingMPEG4:
            fsl_osal_strcpy((fsl_osal_char *)cComponentName,(fsl_osal_char *)MPEG4_DECODER_NAME);
            fsl_osal_strcpy((fsl_osal_char *)role,(fsl_osal_char *)MPEG4_DECODER_ROLE);
            break;
            
        default:
            INFO_PRINT("Unsupported video format: %d\n", pVDTClientParams->VideoFmt);
            return OMX_ErrorBadParameter;
    }

    eRetVal = OMX_GetHandle(&pVDTClientParams->hDecoder,
                            (OMX_STRING)cComponentName,
                            &pVDTClientParams->sAppHandler,
                            &gVDTCallbacks);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;
    
    OMX_PARAM_COMPONENTROLETYPE CurRole;
    OMX_INIT_STRUCT(&CurRole, OMX_PARAM_COMPONENTROLETYPE);
    fsl_osal_memcpy(&CurRole.cRole, role, OMX_MAX_STRINGNAME_SIZE);
    OMX_SetParameter(pVDTClientParams->hDecoder, 
                     OMX_IndexParamStandardComponentRole, 
                     &CurRole);

    /* set video format */
    OMX_INIT_STRUCT(&sParamPort, OMX_PARAM_PORTDEFINITIONTYPE);
    sParamPort.nPortIndex = 0;
    eRetVal = OMX_GetParameter(pVDTClientParams->hDecoder, OMX_IndexParamPortDefinition, &sParamPort);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    sParamPort.format.video.eCompressionFormat = pVDTClientParams->VideoFmt;
    sParamPort.nBufferCountActual = pVDTClientParams->nVESBufCnt;
    sParamPort.nBufferSize = pVDTClientParams->nVESBufSize;

    eRetVal = OMX_SetParameter(pVDTClientParams->hDecoder, OMX_IndexParamPortDefinition, &sParamPort);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    DEBUG_PRINT("set video format success\n");

    /* set decoder to idle */

    eRetVal = OMX_SendCommand(pVDTClientParams->hDecoder, 
                                OMX_CommandStateSet, 
                                OMX_StateIdle, 
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* decoder allocate buffer */

    OMX_PORT_PARAM_TYPE sVideoPortInfo ;

    OMX_INIT_STRUCT(&sVideoPortInfo, OMX_PORT_PARAM_TYPE);
    eRetVal = OMX_GetParameter(pVDTClientParams->hDecoder, OMX_IndexParamVideoInit, &sVideoPortInfo);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;
    
    for(nCounter = sVideoPortInfo.nStartPortNumber;
        nCounter < (sVideoPortInfo.nStartPortNumber + sVideoPortInfo.nPorts);
        nCounter++)
    {
        sParamPort.nPortIndex = nCounter;
        eRetVal = OMX_GetParameter(pVDTClientParams->hDecoder, OMX_IndexParamPortDefinition, &sParamPort);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;

        if(sParamPort.eDir == OMX_DirInput) {
            OMX_U32 i = 0;
            for(i=0; i<pVDTClientParams->nVESBufCnt; i++)
            {
                eRetVal = OMX_UseBuffer(pVDTClientParams->hDecoder,
                                        &pBufferHdr,
                                        sParamPort.nPortIndex,
                                        NULL,
                                        pVDTClientParams->nVESBufSize,
                                        (OMX_U8 *)pVDTClientParams->pParserOutBufHdr[i]->pBuffer);
                if(eRetVal != OMX_ErrorNone)
                    return eRetVal;

                pVDTClientParams->pDecoderInBufHdr[i] = pBufferHdr;
            }

        } 
        else {
            pVDTClientParams->nYUVBufSize = sParamPort.nBufferSize;

            pBuffer = FSL_MALLOC(pVDTClientParams->nYUVBufSize);
            if(pBuffer == NULL)
                return OMX_ErrorInsufficientResources;
            fsl_osal_memset(pBuffer , 0, pVDTClientParams->nYUVBufSize);

            eRetVal = OMX_UseBuffer(pVDTClientParams->hDecoder,
                                    &pBufferHdr,
                                    sParamPort.nPortIndex,
                                    NULL,
                                    pVDTClientParams->nYUVBufSize,
                                    (OMX_U8 *)pBuffer);
            if(eRetVal != OMX_ErrorNone)
                return eRetVal;

            pVDTClientParams->pYUVBuf[0] = (OMX_U8 *)pBuffer;
            pVDTClientParams->pDecoderOutBufHdr[0] = pBufferHdr;
        }

    }

    DEBUG_PRINT("allocat video decoder buffer finished\n");

    /* decoder wait for idle */

    do {
        OMX_GetState(pVDTClientParams->hDecoder, &pVDTClientParams->sAppHandler.eState);
    } while (pVDTClientParams->sAppHandler.eState != OMX_StateIdle);

    DEBUG_PRINT("wait decoder to idle finished\n");

    /* decoder set to exec */

    eRetVal = OMX_SendCommand(pVDTClientParams->hDecoder,
                             OMX_CommandStateSet,
                             OMX_StateExecuting,
                             NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;


    /* decoder wait to exec */

    do {
        OMX_GetState(pVDTClientParams->hDecoder, &pVDTClientParams->sAppHandler.eState);
    } while (pVDTClientParams->sAppHandler.eState != OMX_StateExecuting);

    DEBUG_PRINT("wait decoder to exec finished\n");

    for(OMX_U32 i = 0; i < pVDTClientParams->nVESBufCnt; i++)
    {
        eRetVal = OMX_FillThisBuffer(pVDTClientParams->hParser,
                                     pVDTClientParams->pParserOutBufHdr[i]);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;
    }

    eRetVal = OMX_FillThisBuffer(pVDTClientParams->hDecoder,
                                 pVDTClientParams->pDecoderOutBufHdr[0]);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    return OMX_ErrorNone;

}


/*******************************************************************************
 Function       : VDT_UnLoadVideoDecoder

 Description    : This function is used to initialize video decoder component

 Returns        : OMX_ERRORTYPE

 Arguments      : pVDTClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE VDT_UnLoadVideoDecoder(VDT_CLIENT_PARAMS *pVDTClientParams)
{

    OMX_ERRORTYPE     eRetVal = OMX_ErrorNone;

    if(pVDTClientParams->hDecoder== NULL)
        return OMX_ErrorNone;

    eRetVal = OMX_SendCommand(pVDTClientParams->hDecoder,
                                OMX_CommandStateSet ,
                                OMX_StateIdle,
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    do
    {
        OMX_GetState(pVDTClientParams->hDecoder, &pVDTClientParams->sAppHandler.eState);
    }while (pVDTClientParams->sAppHandler.eState != OMX_StateIdle);

    eRetVal = OMX_SendCommand(pVDTClientParams->hDecoder,
                             OMX_CommandStateSet,
                             OMX_StateLoaded,
                             NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* Free Buffers for video dec */

    OMX_U32 i;    
    for(i=0; i<pVDTClientParams->nVESBufCnt; i++) {
        eRetVal = OMX_FreeBuffer(pVDTClientParams->hDecoder,
                                0, 
                                pVDTClientParams->pDecoderInBufHdr[i]);
        if(eRetVal != OMX_ErrorNone) {
            INFO_PRINT("%s free buffer for port %d failed.\n", "Decoder", 0);
            return eRetVal;
        }
    }

    for(i=0; i<pVDTClientParams->nYUVBufCnt; i++) {
        eRetVal = OMX_FreeBuffer(pVDTClientParams->hDecoder,
                                1, 
                                pVDTClientParams->pDecoderOutBufHdr[i]);
        if(eRetVal != OMX_ErrorNone) {
            INFO_PRINT("%s free buffer for port %d failed.\n", "Decoder", 1);
            return eRetVal;
        }
    }

    do
    {
        OMX_GetState(pVDTClientParams->hDecoder, &pVDTClientParams->sAppHandler.eState);
    }while (pVDTClientParams->sAppHandler.eState != OMX_StateLoaded);

    return OMX_ErrorNone;
}


/*******************************************************************************
 Function       : VDT_LoadVideoRender

 Description    : This function is used to initialize video render component

 Returns        : OMX_ERRORTYPE

 Arguments      : pVDTClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE VDT_LoadVideoRender(VDT_CLIENT_PARAMS *pVDTClientParams)
{
    OMX_ERRORTYPE           eRetVal = OMX_ErrorNone;
    OMX_S8                  cSinkComponentName[128];
    OMX_PARAM_PORTDEFINITIONTYPE sParamPort, sDownstreamParamPort;
    OMX_BUFFERHEADERTYPE    *pBufferHdr;
    OMX_U32                 nCounter;

    fsl_osal_strcpy((fsl_osal_char *)cSinkComponentName,(fsl_osal_char *)V4L_RENDER_NAME);
    eRetVal = OMX_GetHandle(&pVDTClientParams->hRender,
                            (OMX_STRING)cSinkComponentName,
                            &pVDTClientParams->sAppHandler,
                            &gVDTCallbacks);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;


    /* Get vido decoder outport format */
    OMX_INIT_STRUCT(&sParamPort, OMX_PARAM_PORTDEFINITIONTYPE);
    sParamPort.nPortIndex = 1;
    eRetVal = OMX_GetParameter(pVDTClientParams->hDecoder,
                               OMX_IndexParamPortDefinition,
                               &sParamPort);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    pVDTClientParams->nYUVBufSize = sParamPort.nBufferSize;
    pVDTClientParams->nYUVBufCnt = sParamPort.nBufferCountActual;

    /* set render inport format */
    OMX_INIT_STRUCT(&sDownstreamParamPort, OMX_PARAM_PORTDEFINITIONTYPE);
    sDownstreamParamPort.nPortIndex = 0;
    eRetVal = OMX_GetParameter(pVDTClientParams->hRender,
                               OMX_IndexParamPortDefinition,
                               &sDownstreamParamPort);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;
    sDownstreamParamPort.format.video = sParamPort.format.video; 
    sDownstreamParamPort.nBufferSize = sParamPort.nBufferSize; 
    sDownstreamParamPort.nBufferCountActual = sParamPort.nBufferCountActual; 
    sDownstreamParamPort.nBufferCountMin = sParamPort.nBufferCountMin; 
    eRetVal = OMX_SetParameter(pVDTClientParams->hRender,
                               OMX_IndexParamPortDefinition,
                               &sDownstreamParamPort);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* set render input crop */
    OMX_CONFIG_RECTTYPE sRect;
    OMX_INIT_STRUCT(&sRect, OMX_CONFIG_RECTTYPE);
    sRect.nPortIndex = 1;
    if(OMX_ErrorNone == OMX_GetConfig(pVDTClientParams->hDecoder, OMX_IndexConfigCommonOutputCrop, &sRect)) {
        sRect.nPortIndex = 0;
        OMX_SetConfig(pVDTClientParams->hRender, OMX_IndexConfigCommonInputCrop, &sRect);
    }

    /* set render display region */
    OMX_CONFIG_RECTTYPE sDispRect;
    OMX_INIT_STRUCT(&sDispRect, OMX_CONFIG_RECTTYPE);
    sDispRect.nPortIndex = 0;                                                                   
    sDispRect.nWidth = 1024;                                                                     
    sDispRect.nHeight = 768;                                                                    
    eRetVal = OMX_SetConfig(pVDTClientParams->hRender,                             
            OMX_IndexConfigCommonOutputCrop,                                      
            &sDispRect);                                                     
    if(eRetVal != OMX_ErrorNone)                                                                     
        return eRetVal;  

    /* set decoder to idle */
    eRetVal = OMX_SendCommand(pVDTClientParams->hRender, 
                                OMX_CommandStateSet, 
                                OMX_StateIdle, 
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* enable decoder outport */
    eRetVal = OMX_SendCommand(pVDTClientParams->hDecoder,
                                OMX_CommandPortEnable,
                                1,
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /*########################################*/
    /* allocate buffer for decoder component */
    /*########################################*/
    for(nCounter=0; nCounter<pVDTClientParams->nYUVBufCnt; nCounter++) {
        eRetVal = OMX_AllocateBuffer(pVDTClientParams->hRender,
                                     &pBufferHdr, 0, NULL, 
                                     pVDTClientParams->nYUVBufSize);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;

        pVDTClientParams->pRenderInBufHdr[nCounter] = pBufferHdr;

        eRetVal = OMX_UseBuffer(pVDTClientParams->hDecoder,
                                &pBufferHdr, 1, NULL,
                                pVDTClientParams->nYUVBufSize,
                                (OMX_U8 *)pBufferHdr->pBuffer);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;

        /* set the physicall address for buffer header */
        pBufferHdr->nOffset = pVDTClientParams->pRenderInBufHdr[nCounter]->nOffset;
        pVDTClientParams->pDecoderOutBufHdr[nCounter] = pBufferHdr;
    }

    do {
        OMX_GetState(pVDTClientParams->hRender, &pVDTClientParams->sAppHandler.eState);
    } while (pVDTClientParams->sAppHandler.eState != OMX_StateIdle);


    /* send command to render change to executing state */
    eRetVal = OMX_SendCommand(pVDTClientParams->hRender,
                                OMX_CommandStateSet,
                                OMX_StateExecuting,
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    do {
        OMX_GetState(pVDTClientParams->hRender, &pVDTClientParams->sAppHandler.eState);
    } while (pVDTClientParams->sAppHandler.eState != OMX_StateExecuting);

    return eRetVal;
}


/*******************************************************************************
 Function       : VDT_UnLoadVideoRender

 Description    : This function is used to initialize video render component

 Returns        : OMX_ERRORTYPE

 Arguments      : pVDTClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE VDT_UnLoadVideoRender(VDT_CLIENT_PARAMS *pVDTClientParams)
{
    OMX_ERRORTYPE   eRetVal = OMX_ErrorNone;

    if(pVDTClientParams->hRender== NULL)
        return OMX_ErrorNone;

    eRetVal = OMX_SendCommand(pVDTClientParams->hRender,
                                OMX_CommandStateSet,
                                OMX_StateIdle,
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    do
    {
        OMX_GetState(pVDTClientParams->hRender, &pVDTClientParams->sAppHandler.eState);
    }while (pVDTClientParams->sAppHandler.eState != OMX_StateIdle);

    eRetVal = OMX_SendCommand(pVDTClientParams->hRender,
                             OMX_CommandStateSet,
                             OMX_StateLoaded,
                             NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    
    /* Free Buffers for render */

    OMX_U32 i;
    for(i=0; i<pVDTClientParams->nYUVBufCnt; i++) {
        eRetVal = OMX_FreeBuffer(pVDTClientParams->hRender,
                                0,
                                pVDTClientParams->pRenderInBufHdr[i]);
        if(eRetVal != OMX_ErrorNone){
            INFO_PRINT("%s free buffer for port %d failed.\n", "Render", 0);
            return eRetVal;
        }
    }    


    do
    {
        OMX_GetState(pVDTClientParams->hRender, &pVDTClientParams->sAppHandler.eState);
    }while (pVDTClientParams->sAppHandler.eState != OMX_StateLoaded);

    return OMX_ErrorNone;
}


/*******************************************************************************
 Function       : VDT_FindBufferHeader

 Description    : This function is used to look for a matched buffer header
                    in a header list

 Returns        : OMX_S8
                    index of matched buffer header in header list
                    -1 - not found

 Arguments      : pHeaderList  - header list to be searched in
                  pBufferHdr   - header containing buffer that needs to be searched
                  num          - size of header list

********************************************************************************/
static OMX_S8 VDT_FindBufferHeader(
                    OMX_BUFFERHEADERTYPE **pHeaderList, 
                    OMX_BUFFERHEADERTYPE *pBufferHdr, 
                    OMX_S8 num)
{
    int count;

    for(count=0; count<num; count++) {
        if (pHeaderList[count]->pBuffer == pBufferHdr->pBuffer)
            return count;
    }

    INFO_PRINT("Can't find buffer header.\n");

    return -1;
}


/*******************************************************************************
  Function: VDT_SendEmptyBuffers

  Description: This function reads empty buffer queue
                and sends it to the component on receiving an EmptyBufferDone Callback

  Returns: OMX_ERRORTYPE

  Arguments: pVDTClientParams - Pointer to ILClient structure

 ********************************************************************************/

static OMX_ERRORTYPE VDT_SendEmptyBuffers(VDT_CLIENT_PARAMS *pVDTClientParams)
{
    OMX_ERRORTYPE       eRetVal = OMX_ErrorNone;
    Message             *pMsg = NULL;
    Message             sMsg;
    QUEUE_ERRORTYPE     eRetQueue = QUEUE_SUCCESS;

    pMsg = (Message *)&sMsg;

    eRetQueue = ReadQueue(pVDTClientParams->sAppHandler.EmptyBufQueue, pMsg, E_FSL_OSAL_TRUE);
    if(eRetQueue != QUEUE_SUCCESS) {
        INFO_PRINT("dequeue failed.\n");
        return OMX_ErrorUndefined;
    }

    /* receive EmptyBufferDone event from videodec */

    if (pMsg->hComponent == pVDTClientParams->hDecoder) {
        /*
        DEBUG_PRINT("receive EmptyBufferDone event from videodec\n");
        */
        int idx;

        idx = VDT_FindBufferHeader(pVDTClientParams->pParserOutBufHdr, pMsg->data.pBuffer, pVDTClientParams->nVESBufCnt);
        if(idx < 0)
            return OMX_ErrorUndefined;

        DEBUG_PRINT("Parser fill buffer\n");
        eRetVal = OMX_FillThisBuffer(pVDTClientParams->hParser,
                                     pVDTClientParams->pParserOutBufHdr[idx]);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;
    }

    /* receive EmptyBufferDone event from render */
    if (pMsg->hComponent == pVDTClientParams->hRender) {
        int idx;

        idx = VDT_FindBufferHeader(pVDTClientParams->pDecoderOutBufHdr, pMsg->data.pBuffer, pVDTClientParams->nYUVBufCnt);
        if(idx < 0)
            return OMX_ErrorUndefined;

        /*
        DEBUG_PRINT("receive EmptyBufferDone event from render\n");
        */
        DEBUG_PRINT("decoder fill buffer\n");

        eRetVal = OMX_FillThisBuffer(pVDTClientParams->hDecoder,
                                     pVDTClientParams->pDecoderOutBufHdr[idx]);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;
    }


	return eRetVal;
}


/*******************************************************************************
  Function: VDT_SendOutputBuffers

  Description: This function reads from filled buffer queue
                and sends it to the component on receiving a FillBufferDone Callback

  Returns: OMX_ERRORTYPE

  Arguments: *pVDTClientParams - Pointer to ILClient structure

********************************************************************************/

static OMX_ERRORTYPE VDT_SendFilledBuffers(VDT_CLIENT_PARAMS *pVDTClientParams)
{
    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;
    Message       *pMsg = NULL;
    Message       sMsg;
    QUEUE_ERRORTYPE eRetQueue;

    pMsg = (Message *)&sMsg;
    eRetQueue = ReadQueue(pVDTClientParams->sAppHandler.FilledBufQueue, pMsg, E_FSL_OSAL_TRUE);
    if(eRetQueue != QUEUE_SUCCESS) {
        INFO_PRINT("dequeue failed.\n");
        return OMX_ErrorUndefined;
    }

    /* receive FillBufferDone event from parser */
    
    if (pMsg->hComponent == pVDTClientParams->hParser) {
        /*
        DEBUG_PRINT("receive FillBufferDone event from parser\n");
        */

        if(pMsg->data.pBuffer->nFlags == OMX_BUFFERFLAG_EOS) {
            INFO_PRINT("Receive EOS...\n");
            assert(0);
            pVDTClientParams->bEOS = OMX_TRUE;
            return OMX_ErrorNone;
        }

        int idx;

        idx = VDT_FindBufferHeader(pVDTClientParams->pDecoderInBufHdr, pMsg->data.pBuffer, pVDTClientParams->nVESBufCnt);
        if(idx < 0)
            return OMX_ErrorUndefined;

        pVDTClientParams->pDecoderInBufHdr[idx]->nFilledLen = pMsg->data.pBuffer->nFilledLen;
        pVDTClientParams->pDecoderInBufHdr[idx]->nFlags = pMsg->data.pBuffer->nFlags;

        DEBUG_PRINT("FillBufferDone len %ld flag %lx\n", 
            pVDTClientParams->pDecoderInBufHdr[idx]->nFilledLen,
            pVDTClientParams->pDecoderInBufHdr[idx]->nFlags);

        assert(pVDTClientParams->pDecoderInBufHdr[idx]->nFilledLen > 0);

        DEBUG_PRINT("decoder empty this buffer \n");

        eRetVal = OMX_EmptyThisBuffer(pVDTClientParams->hDecoder,
                                      pVDTClientParams->pDecoderInBufHdr[idx]);

        if(eRetVal != OMX_ErrorNone) {
            DEBUG_PRINT("Decoder empty buffer failed.\n");
            return eRetVal;
        }

    }

    /* receive FillBufferDone event from videodec */
    
    if (pMsg->hComponent == pVDTClientParams->hDecoder) {
        /*
        DEBUG_PRINT("receive FillBufferDone event from videodec\n");
        */
        if(pVDTClientParams->decOutPortDisabled) {

            DEBUG_PRINT("decoder free first buffer\n");
            
            eRetVal = OMX_FreeBuffer(pVDTClientParams->hDecoder,
                                    1, 
                                    pVDTClientParams->pDecoderOutBufHdr[0]);
            if(eRetVal != OMX_ErrorNone)
                return eRetVal;
            
            FSL_FREE(pVDTClientParams->pYUVBuf[0]);
        }
        else {
            int idx;

            idx = VDT_FindBufferHeader(pVDTClientParams->pRenderInBufHdr, pMsg->data.pBuffer, pVDTClientParams->nYUVBufCnt);
            if(idx < 0)
                return OMX_ErrorUndefined;

            pVDTClientParams->pRenderInBufHdr[idx]->nFilledLen = pMsg->data.pBuffer->nFilledLen;
            pVDTClientParams->pRenderInBufHdr[idx]->nFlags = pMsg->data.pBuffer->nFlags;
            /*
            fsl_osal_sleep(33000);
            */
    
            DEBUG_PRINT("render empty buffer len %ld flag 0x%lx\n", 
                pVDTClientParams->pDecoderInBufHdr[idx]->nFilledLen,
                pVDTClientParams->pDecoderInBufHdr[idx]->nFlags);

            eRetVal = OMX_EmptyThisBuffer(pVDTClientParams->hRender,
                                          pVDTClientParams->pRenderInBufHdr[idx]);
            if(eRetVal != OMX_ErrorNone)
                return eRetVal;
        }
    }

	return eRetVal;
}


/*******************************************************************************
  Function: VDT_ProcessEventQueue

  Description: This function is used to process each entry obtained in the
                Event queue.

  Returns: OMX_ERRORTYPE

  Arguments: pMsg - Pointer to Entry in the event queue.
             *pVDTClientParams - Pointer to ILClient structure
********************************************************************************/
static OMX_ERRORTYPE VDT_ProcessEventQueue(VDT_CLIENT_PARAMS *pVDTClientParams, Message *pMsg)
{

    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;

    DEBUG_PRINT("VDT_ProcessEventQueue MsgType %d, Eventype %d\n", pMsg->MsgType, pMsg->event);

    switch(pMsg->MsgType)
    {
        case Eventype:
            if(pMsg->event == EmptyBufferDone)
            {
                DEBUG_PRINT("Received Emptybuffer done event\n");

                eRetVal = VDT_SendEmptyBuffers(pVDTClientParams);
                if(eRetVal != OMX_ErrorNone)
                {
                    DEBUG_PRINT("VDT_SendEmptyBuffers fail!!! %x\n", eRetVal);
                    return eRetVal;
                }
            }
            else if(pMsg->event == FillBufferDone)
            {
                DEBUG_PRINT("Received Fillbuffer done event\n");

                eRetVal = VDT_SendFilledBuffers(pVDTClientParams);
                if(eRetVal != OMX_ErrorNone)
                {
                    DEBUG_PRINT("VDT_SendFilledBuffers fail!!! %x\n", eRetVal);
                    return eRetVal;
                }
            }
            else if(pMsg->event == PortSettingsChanged) {
                DEBUG_PRINT("PortSettingsChanged %lx %lx\n", (unsigned long)pMsg->hComponent, (unsigned long)pVDTClientParams->hDecoder);
                if(pMsg->hComponent == pVDTClientParams->hDecoder) {
                    OMX_PARAM_PORTDEFINITIONTYPE sParamPort;
                    /* Get video decoder outport format */
                    OMX_INIT_STRUCT(&sParamPort, OMX_PARAM_PORTDEFINITIONTYPE);
                    sParamPort.nPortIndex = 1;
                    eRetVal = OMX_GetParameter(pVDTClientParams->hDecoder,
                                               OMX_IndexParamPortDefinition,
                                               &sParamPort);
                    if(eRetVal != OMX_ErrorNone)
                        return eRetVal;

                    DEBUG_PRINT("video foramt: width %d, height %d, framerate: %f\n",
                            (unsigned int)sParamPort.format.video.nFrameWidth,
                            (unsigned int)sParamPort.format.video.nFrameHeight,
                            (float)sParamPort.format.video.xFramerate);
                    DEBUG_PRINT("video required buffer: %d\n", (unsigned int)sParamPort.nBufferCountMin);

                    /* send command to disable video decoder outport */
                    eRetVal = OMX_SendCommand(pVDTClientParams->hDecoder,
                                                OMX_CommandPortDisable,
                                                1,
                                                NULL);
                    if(eRetVal != OMX_ErrorNone)
                        return eRetVal;

                    pVDTClientParams->decOutPortDisabled = OMX_TRUE;
                }
            }
            else if(pMsg->event == PortDisabled) {
                /* send command to enable video outport */
                if(pMsg->hComponent == pVDTClientParams->hDecoder) {
                    /* allocate buffer for video outport */

                    if (!pVDTClientParams->bsetting_changed)
                    {
                        eRetVal = VDT_LoadVideoRender(pVDTClientParams);
                        if(eRetVal != OMX_ErrorNone)
                            return eRetVal;
                        pVDTClientParams->bsetting_changed = OMX_TRUE;
                    }

                }

            }
            else if(pMsg->event == PortEnabled) {
                DEBUG_PRINT("PortEnabled.\n");
                if(pMsg->hComponent == pVDTClientParams->hDecoder 
                        && pVDTClientParams->decOutPortDisabled == OMX_TRUE) {
                    OMX_PARAM_PORTDEFINITIONTYPE sParamPort;
                    OMX_U32 nCounter;
                    /* Get vido decoder outport format */
                    OMX_INIT_STRUCT(&sParamPort, OMX_PARAM_PORTDEFINITIONTYPE);
                    sParamPort.nPortIndex = 1;
                    eRetVal = OMX_GetParameter(pVDTClientParams->hDecoder,
                            OMX_IndexParamPortDefinition,
                            &sParamPort);
                    if(eRetVal != OMX_ErrorNone)
                        return eRetVal;

                    for(nCounter=0; nCounter<sParamPort.nBufferCountActual; nCounter++) {
                        eRetVal = OMX_FillThisBuffer(pVDTClientParams->hDecoder,
                                                     pVDTClientParams->pDecoderOutBufHdr[nCounter]);
                        if(eRetVal != OMX_ErrorNone)
                            return eRetVal;
                    }

                    pVDTClientParams->decOutPortDisabled = OMX_FALSE;
                    DEBUG_PRINT("++++++++port enabled+++++++++++\n");
                }
            }
            else if(pMsg->event == PortFormatDetected) {
                DEBUG_PRINT("PortFormatDetected %lx %lx\n", (unsigned long)pMsg->hComponent, (unsigned long)pVDTClientParams->hParser);
                if(pMsg->hComponent == pVDTClientParams->hParser)
                {
                    OMX_VIDEO_PARAM_PORTFORMATTYPE sVideoPortFmt;

                    OMX_INIT_STRUCT(&sVideoPortFmt, OMX_VIDEO_PARAM_PORTFORMATTYPE);
                    sVideoPortFmt.nPortIndex = pVDTClientParams->nParserVideoPortIdx;
                    OMX_GetParameter(pVDTClientParams->hParser, 
                                     OMX_IndexParamVideoPortFormat, 
                                     &sVideoPortFmt);

                    if(pVDTClientParams->VideoFmt != sVideoPortFmt.eCompressionFormat)
                    {
                        pVDTClientParams->VideoFmt = sVideoPortFmt.eCompressionFormat;
                        DEBUG_PRINT("Video format detected: %x\n", pVDTClientParams->VideoFmt);

                        if((eRetVal = VDT_LoadVideoDecoder(pVDTClientParams)) != OMX_ErrorNone)
                        {
                            INFO_PRINT("VDT_LoadVideoDecoder fail %x!!!\n", eRetVal);
                        }
                    }
                } 
            }
            else if(pMsg->event == EndOfStream)
            {
                INFO_PRINT("event: EndOfStream...\n");
                pVDTClientParams->bEOS = OMX_TRUE;
            }
        default:
            break;
    }

	return eRetVal;
}


/*******************************************************************************

* The VDT_EventHandlerCallback method is used to notify the application when an
    event of interest occurs.  Events are defined in the OMX_EVENTTYPE
    enumeration.  Please see that enumeration for details of what will
    be returned for each type of event. Callbacks should not return
    an error to the component, so if an error occurs, the application
    shall handle it internally.  This is a blocking call.

    The application should return from this call within 5 msec to avoid
    blocking the component for an excessively long period of time.

    @param hComponent
        handle of the component to access.  This is the component
        handle returned by the call to the GetHandle function.
    @param pAppData
        pointer to an application defined value that was provided in the
        pAppData parameter to the OMX_GetHandle method for the component.
        This application defined value is provided so that the application
        can have a component specific context when receiving the callback.
    @param eEvent
        Event that the component wants to notify the application about.
    @param nData1
        nData will be the OMX_ERRORTYPE for an error event and will be
        an OMX_COMMANDTYPE for a command complete event and OMX_INDEXTYPE for a OMX_PortSettingsChanged event.
     @param nData2
        nData2 will hold further information related to the event. Can be OMX_STATETYPE for
        a OMX_StateChange command or port index for a OMX_PortSettingsCHanged event.
        Default value is 0 if not used. )
    @param pEventData
        Pointer to additional event-specific data (see spec for meaning).
********************************************************************************/

static OMX_ERRORTYPE VDT_EventHandlerCallback(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    AppHandler *pAppHandler = (AppHandler *)pAppData;
    Message *pMsg = NULL;

    DEBUG_PRINT("VDT_EventHandlerCallback %lx event type %d, nData1 %ld\n",(unsigned long)hComponent, eEvent, nData1);

    VDT_FrameMessage(&pMsg,Eventype);

    switch(eEvent)
    {
        case OMX_EventCmdComplete:
            pAppHandler->eState = (OMX_STATETYPE)nData1;
            break;
        case OMX_EventError:
            pAppHandler->eError = (OMX_ERRORTYPE)nData1;
            pMsg->event = Error;
            break;
        default:
            break;
    }

    if((eEvent == OMX_EventCmdComplete) && (nData1 == OMX_CommandPortDisable))
        pMsg->event = PortDisabled;
    else if((eEvent == OMX_EventCmdComplete) && (nData1 == OMX_CommandPortEnable))
        pMsg->event = PortEnabled;
    else if((eEvent == OMX_EventCmdComplete) && (nData1 == OMX_CommandFlush))
        pMsg->event = PortFlushed;
    else if((eEvent == OMX_EventCmdComplete) && (nData1 == OMX_CommandMarkBuffer))
        pMsg->event = BufferMarked;
    else if(eEvent == OMX_EventCmdComplete)
        pMsg->event = Statetransition;
    else if(eEvent == OMX_EventBufferFlag)
        pMsg->event = EndOfStream;
    else if(eEvent == OMX_EventPortSettingsChanged)
        pMsg->event = PortSettingsChanged;
    else if(eEvent == OMX_EventMark)
        pMsg->event = MarkEventGenerated;
    else if(eEvent == OMX_EventPortFormatDetected)
        pMsg->event = PortFormatDetected;

    pMsg->nData1 = nData1;
    pMsg->nData2 = nData2;

    pMsg->hComponent = hComponent;
    VDT_PostMessage(pAppHandler, pMsg);
    FSL_FREE(pMsg);
    return OMX_ErrorNone;
}


/*******************************************************************************

* The VDT_EmptyBufferDoneCallback method is used to return emptied buffers from an
    input port back to the application for reuse.  This is a blocking call
    so the application should not attempt to refill the buffers during this
    call, but should queue them and refill them in another thread.  There
    is no error return, so the application shall handle any errors generated
    internally.

    The application should return from this call within 5 msec.

    @param hComponent
        handle of the component to access.  This is the component
        handle returned by the call to the GetHandle function.
    @param pAppData
        pointer to an application defined value that was provided in the
        pAppData parameter to the OMX_GetHandle method for the component.
        This application defined value is provided so that the application
        can have a component specific context when receiving the callback.
    @param pBuffer
        pointer to an OMX_BUFFERHEADERTYPE structure allocated with UseBuffer
        or AllocateBuffer indicating the pBuffer that was emptied.
********************************************************************************/
static OMX_ERRORTYPE VDT_EmptyBufferDoneCallback(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    AppHandler *pAppHandler = (AppHandler *)pAppData;
    Message *pMsg = NULL;

    DEBUG_PRINT("VDT_EmptyBufferDoneCallback %lx\n",(unsigned long)hComponent);
    VDT_FrameMessage(&pMsg,DataBuffer);
    pMsg->data.type = EmptyBuffer;
    pMsg->data.pBuffer = pBuffer;
    pMsg->hComponent = hComponent;
    VDT_PostMessage(pAppHandler, pMsg);
    FSL_FREE(pMsg);

    VDT_FrameMessage(&pMsg,Eventype);
    pMsg->event = EmptyBufferDone;
    pMsg->hComponent = hComponent;
    VDT_PostMessage(pAppHandler, pMsg);
    FSL_FREE(pMsg);

    return OMX_ErrorNone;
}


/*******************************************************************************
** The VDT_FillBufferDoneCallback method is used to return filled buffers from an
    output port back to the application for emptying and then reuse.
    This is a blocking call so the application should not attempt to
    empty the buffers during this call, but should queue the buffers
    and empty them in another thread.  There is no error return, so
    the application shall handle any errors generated internally.  The
    application shall also update the pBuffer header to indicate the
    number of bytes placed into the pBuffer.

    The application should return from this call within 5 msec.

    @param hComponent
        handle of the component to access.  This is the component
        handle returned by the call to the GetHandle function.
    @param pAppData
        pointer to an application defined value that was provided in the
        pAppData parameter to the OMX_GetHandle method for the component.
        This application defined value is provided so that the application
        can have a component specific context when receiving the callback.
    @param pBuffer
        pointer to an OMX_BUFFERHEADERTYPE structure allocated with UseBuffer
        or AllocateBuffer indicating the pBuffer that was filled.
********************************************************************************/
static OMX_ERRORTYPE VDT_FillBufferDoneCallback(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    AppHandler *pAppHandler = (AppHandler *)pAppData;
    Message *pMsg = NULL;

    DEBUG_PRINT("VDT_FillBufferDoneCallback %lx, %lx, AllocLen %ld, FilledLen %ld Offset %ld OutPortIdx %ld InPortIdx %ld \n", 
                (unsigned long)hComponent, (unsigned long)pBuffer->pBuffer, pBuffer->nAllocLen, pBuffer->nFilledLen, 
                pBuffer->nOffset, pBuffer->nOutputPortIndex, pBuffer->nInputPortIndex);
    VDT_FrameMessage(&pMsg,DataBuffer);
    pMsg->data.type = FilledBuffer;
    pMsg->data.pBuffer = pBuffer;
    pMsg->hComponent = hComponent;
    VDT_PostMessage(pAppHandler, pMsg);
    FSL_FREE(pMsg);
    VDT_FrameMessage(&pMsg,Eventype);
    pMsg->event = FillBufferDone;
    pMsg->hComponent = hComponent;
    VDT_PostMessage(pAppHandler, pMsg);
    FSL_FREE(pMsg);

    return OMX_ErrorNone;
}


static OMX_BOOL VDT_CharTolower(OMX_STRING string)
{
    while(*string != '\0')
    {
        *string = tolower(*string);
        string++;
    }

    return OMX_TRUE;
}


/*******************************************************************************
 This method is the entry point of the program. The ILClient starts
 processing from this entry point.

 @param [in]    file
                file name of video stream 

 @return        OMX_S32
                It returns an integer value to the main function.
********************************************************************************/
static OMX_S32 VDT_ILClientMain(void * file)
{
    OMX_ERRORTYPE           eRetVal 	= OMX_ErrorNone;
    VDT_CLIENT_PARAMS       sVDTClientParams;
    	Message                 *pMsg = NULL;
   
    sVDTClientParams.pInfilename = (OMX_U8*)file;

    eRetVal = VDT_Initialize(&sVDTClientParams);
    if(eRetVal != OMX_ErrorNone)
        return -1;


    eRetVal = VDT_LoadParser(&sVDTClientParams);
    if(eRetVal != OMX_ErrorNone) {
        INFO_PRINT("\n\t\t Error in VDT_LoadParser \n");
        return -1;
    }

    /*################################*/
    /* playback event processing loop */
    /*################################*/

    pMsg = (Message *)FSL_MALLOC(sizeof(Message));
    
    do {
        VDT_ReadEventQueue(&sVDTClientParams.sAppHandler , pMsg);

        eRetVal = VDT_ProcessEventQueue(&sVDTClientParams, pMsg);

        if(eRetVal != OMX_ErrorNone)
            break;

        if (sVDTClientParams.bEOS == OMX_TRUE)
            break;

    } while(1);

    INFO_PRINT("Playback done.\n");

    FSL_FREE(pMsg);


    VDT_UnLoadParser(&sVDTClientParams);

    VDT_UnLoadVideoDecoder(&sVDTClientParams);

    VDT_UnLoadVideoRender(&sVDTClientParams);


    if(OMX_FreeHandle(sVDTClientParams.hParser) != OMX_ErrorNone)
        return -1;
    DEBUG_PRINT("Parser component deinited.\n");

    if(OMX_FreeHandle(sVDTClientParams.hDecoder) != OMX_ErrorNone)
        return -1;
    DEBUG_PRINT("vpu decoder component deinited.\n");


    if(OMX_FreeHandle(sVDTClientParams.hRender) != OMX_ErrorNone)
        return -1;
    DEBUG_PRINT("video render component deinited.\n");

    eRetVal = VDT_DeInitialize(&sVDTClientParams);
    if(eRetVal != OMX_ErrorNone)
    {
        return -1;
    }

    /* Return a success value to the main */
    return 0;
}


/*******************************************************************************
 This method is the entry point of the program. The ILClient starts
 executing from this entry point.

 @param [In] argc
            argc is the number of command line arguments.

 @param [In] argv
             It consists of the parameters passed from the command line

 @return int
         It returns an integer value to the OS.
********************************************************************************/
int main(int argc, char **argv)
{
    OMX_ERRORTYPE   eRetVal = OMX_ErrorNone;
    char            *file;
    OMX_STRING      p = NULL;
    fsl_osal_char   subfix[16];

    if(argc < 2) {
        INFO_PRINT("usage: ./video_decoder_test inputfile \n");
        return -1;
    }
    
    file = argv[1];

    p = fsl_osal_strrchr((fsl_osal_char*)file, '.');
    if (p == NULL || fsl_osal_strlen(p)>15)
    {
        INFO_PRINT("Unsupported file type [%s]!!!\n", file);
        return -1;
    }
    
    fsl_osal_strcpy(subfix, p);
    VDT_CharTolower(subfix);
    if (fsl_osal_strcmp(subfix, ".avi")!=0)
    {
        INFO_PRINT("Unsupported file type [%s]!!!\n", subfix);
        return -1;
    }
    

    eRetVal = OMX_Init();
    if (eRetVal != OMX_ErrorNone)
        return -1;

    VDT_ILClientMain(file);

    eRetVal = OMX_Deinit();
    if(eRetVal != OMX_ErrorNone)
        return -1;

    return 0;
}


/************************************ EOF ********************************/



