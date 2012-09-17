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
#include "mp3_player.h"


/* Macro definitions */

#define DEBUG_PRINT(...) /*printf( __VA_ARGS__)*/
#define INFO_PRINT(...) printf( __VA_ARGS__)

/* Type definitions */

/* Function declarations */

static OMX_ERRORTYPE MP3_EventHandlerCallback(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData);

static OMX_ERRORTYPE MP3_EmptyBufferDoneCallback(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE MP3_FillBufferDoneCallback(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

/* Global variable definitions */

static OMX_CALLBACKTYPE gMP3Callbacks = {
    MP3_EventHandlerCallback,
    MP3_EmptyBufferDoneCallback,
    MP3_FillBufferDoneCallback
};

static const OMX_STRING MP3_PARSER_NAME = "OMX.Freescale.std.parser.mp3.sw-based";
static const OMX_STRING MP3_PARSER_ROLE = "parser.mp3";

static const OMX_STRING MP3_DECODER_NAME = "OMX.Freescale.std.audio_decoder.mp3.sw-based";
static const OMX_STRING MP3_DECODER_ROLE = "audio_decoder.mp3";

static const OMX_STRING AUDIO_RENDER_NAME = "OMX.Freescale.std.audio_render.alsa.sw-based";
static const OMX_STRING AUDIO_RENDER_ROLE = "audio_render.alsa";


/* Function definitions */


/*******************************************************************************
* Function: MP3_FrameMessage
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
OMX_ERRORTYPE MP3_FrameMessage(Message **ppMsg, Message_type type)
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
* Function: MP3_PostMessage
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
OMX_ERRORTYPE MP3_PostMessage(AppHandler *pAppHandler, Message *pMsg)
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
* Function: MP3_ReadEventQueue
*
* Description: This function returns an entry from the event queue.
*
* Returns:   void
*
* Arguments: AppHandler - Pointer to an application structure
*            Message *  - Pointer to a message
*
********************************************************************************/
void MP3_ReadEventQueue(AppHandler *pAppHandler, Message *pMsg)
{
    assert(pMsg != 0);

    fsl_osal_memset((fsl_osal_ptr)pMsg, 0, sizeof(Message));
    ReadQueue(pAppHandler->EventQueue, pMsg, E_FSL_OSAL_TRUE);

    return ;
}


/*******************************************************************************
  Function: MP3_Initialize

  Description: This function is used to initialize the parameters of
                IL Client structure.

  Returns: OMX_ERRORTYPE

  Arguments: pMP3ClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE MP3_Initialize(MP3_CLIENT_PARAMS *pMP3ClientParams)
{
    QUEUE_ERRORTYPE eRetQueue = QUEUE_SUCCESS;

    pMP3ClientParams->hDecoder = NULL;
    pMP3ClientParams->hParser = NULL;
    pMP3ClientParams->hRender = NULL;
    pMP3ClientParams->hContentPipe = NULL;

    pMP3ClientParams->bEOS = OMX_FALSE;

    pMP3ClientParams->nParserAudioPortIdx = 0;
    pMP3ClientParams->nParserVideoPortIdx = 1;

    eRetQueue = CreateQueue(&pMP3ClientParams->sAppHandler.EventQueue,
                            MAX_OMXQUEUE_SIZE,
                            E_FSL_OSAL_TRUE,
                            sizeof(Message));

    if(eRetQueue != QUEUE_SUCCESS)
        return OMX_ErrorInsufficientResources;

    eRetQueue = CreateQueue(&pMP3ClientParams->sAppHandler.EmptyBufQueue,
                            MAX_OMXQUEUE_SIZE,
                            E_FSL_OSAL_FALSE,
                            sizeof(Message));
    if(eRetQueue != QUEUE_SUCCESS)
        return OMX_ErrorInsufficientResources;

    eRetQueue = CreateQueue(&pMP3ClientParams->sAppHandler.FilledBufQueue,
                            MAX_OMXQUEUE_SIZE,
                            E_FSL_OSAL_FALSE,
                            sizeof(Message));
    if(eRetQueue != QUEUE_SUCCESS)
        return OMX_ErrorInsufficientResources;

    return OMX_ErrorNone;
}



/*******************************************************************************
  Function: MP3_DeInitialize

  Description: This function is used to de-initialize the parameters of
            IL Client structure.

  Returns: OMX_ERRORTYPE

  Arguments: pMP3ClientParams - Pointer to ILClient structure

********************************************************************************/

static OMX_ERRORTYPE MP3_DeInitialize(MP3_CLIENT_PARAMS *pMP3ClientParams)
{
    QUEUE_ERRORTYPE eRetVal = QUEUE_FAILURE;
    Message         sMsg;

    while(GetQueueSize(pMP3ClientParams->sAppHandler.EventQueue) > 0)
        ReadQueue(pMP3ClientParams->sAppHandler.EventQueue, &sMsg, E_FSL_OSAL_TRUE);

    while(GetQueueSize(pMP3ClientParams->sAppHandler.EmptyBufQueue) > 0)
        ReadQueue(pMP3ClientParams->sAppHandler.EmptyBufQueue, &sMsg, E_FSL_OSAL_TRUE);

    while(GetQueueSize(pMP3ClientParams->sAppHandler.FilledBufQueue) > 0)
        ReadQueue(pMP3ClientParams->sAppHandler.FilledBufQueue, &sMsg, E_FSL_OSAL_TRUE);

    eRetVal = DeleteQueue(pMP3ClientParams->sAppHandler.EventQueue);
    if(eRetVal != QUEUE_SUCCESS)
        return OMX_ErrorUndefined;

    eRetVal = DeleteQueue(pMP3ClientParams->sAppHandler.EmptyBufQueue);
    if(eRetVal != QUEUE_SUCCESS)
        return OMX_ErrorUndefined;

    eRetVal = DeleteQueue(pMP3ClientParams->sAppHandler.FilledBufQueue);
    if(eRetVal != QUEUE_SUCCESS)
        return OMX_ErrorUndefined;


    return OMX_ErrorNone;
}


/*******************************************************************************
 Function       : MP3_LoadParser

 Description    : This function is used to initialize parser component

 Returns        : OMX_ERRORTYPE

 Arguments      : pMP3ClientParams - Pointer to ILClient structure

********************************************************************************/

static OMX_ERRORTYPE MP3_LoadParser(MP3_CLIENT_PARAMS *pMP3ClientParams)
{
    OMX_ERRORTYPE           eRetVal = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE    *pBufferHdr;
    OMX_U8                  *ptr = NULL;
    OMX_S8                  cSourceComponentName[COMP_NAME_LENGTH];
    CP_PIPETYPE             *hPipe;
    OMX_PARAM_CONTENTURITYPE *Content;
    OMX_U8                  len;

    /* Load component */
    
    fsl_osal_strcpy((fsl_osal_char *)cSourceComponentName,(fsl_osal_char *)MP3_PARSER_NAME); 
    eRetVal = OMX_GetHandle(&pMP3ClientParams->hParser,
                            (OMX_STRING)cSourceComponentName,
                            &pMP3ClientParams->sAppHandler,
                            &gMP3Callbacks);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* set Parser role */

    OMX_STRING role = MP3_PARSER_ROLE;
    OMX_PARAM_COMPONENTROLETYPE CurRole;
    OMX_INIT_STRUCT(&CurRole, OMX_PARAM_COMPONENTROLETYPE);
    fsl_osal_memcpy(&CurRole.cRole, role, OMX_MAX_STRINGNAME_SIZE);
    OMX_SetParameter(pMP3ClientParams->hParser, 
                     OMX_IndexParamStandardComponentRole, 
                     &CurRole);

    DEBUG_PRINT("set parser role ok\n");

    /* set URI */
    
    len = fsl_osal_strlen((fsl_osal_char*)pMP3ClientParams->pInfilename) + 1;
    ptr = (OMX_U8*)FSL_MALLOC(sizeof(OMX_PARAM_CONTENTURITYPE) + len);
    if(ptr == NULL) {
        return OMX_ErrorInsufficientResources;
    }

    Content = (OMX_PARAM_CONTENTURITYPE*)ptr;
    OMX_INIT_STRUCT(Content, OMX_PARAM_CONTENTURITYPE);
    fsl_osal_strcpy((fsl_osal_char*)(&Content->contentURI), (fsl_osal_char*)pMP3ClientParams->pInfilename);

    DEBUG_PRINT("Loading content: %s\n", pMP3ClientParams->pInfilename);

    eRetVal = OMX_SetParameter(pMP3ClientParams->hParser, 
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
    eRetVal = OMX_SetParameter(pMP3ClientParams->hParser, 
                               OMX_IndexParamCustomContentPipe, 
                               &sPipe);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    DEBUG_PRINT("Parser GetContentPipe ok\n");

    /* disable video port */

    eRetVal = OMX_SendCommand(pMP3ClientParams->hParser, 
                              OMX_CommandPortDisable, 
                              pMP3ClientParams->nParserVideoPortIdx, 
                              NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    do {
        fsl_osal_sleep(100000);
    } while (pMP3ClientParams->sAppHandler.eState != (OMX_STATETYPE)OMX_CommandPortDisable);

    /* auto detect audio fmt */
    OMX_AUDIO_PARAM_PORTFORMATTYPE sAudioPortFmt;
    OMX_INIT_STRUCT(&sAudioPortFmt, OMX_AUDIO_PARAM_PORTFORMATTYPE);
    sAudioPortFmt.nPortIndex = pMP3ClientParams->nParserAudioPortIdx;
    
    eRetVal = OMX_GetParameter(pMP3ClientParams->hParser, 
                               OMX_IndexParamAudioPortFormat, 
                               &sAudioPortFmt);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    sAudioPortFmt.eEncoding = OMX_AUDIO_CodingAutoDetect;
    
    eRetVal = OMX_SetParameter(pMP3ClientParams->hParser, 
                               OMX_IndexParamAudioPortFormat, 
                               &sAudioPortFmt);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;
    pMP3ClientParams->AudioFmt = OMX_AUDIO_CodingAutoDetect;

    DEBUG_PRINT("Parser set auto detect ok\n");

    /* set parser to idle */

    eRetVal = OMX_SendCommand(pMP3ClientParams->hParser, 
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
    sPortDef.nPortIndex = pMP3ClientParams->nParserAudioPortIdx;
    eRetVal = OMX_GetParameter(pMP3ClientParams->hParser, 
                               OMX_IndexParamPortDefinition, 
                               &sPortDef);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    pMP3ClientParams->nAESBufSize = sPortDef.nBufferSize;
    pMP3ClientParams->nAESBufCnt  = sPortDef.nBufferCountActual;
    /*
    DEBUG_PRINT("Parser Port 1: size %d, Cnt %d, Dir %d, Enabled %d Populated %d\n",
                    sPortDef.nBufferSize, sPortDef.nBufferCountActual, sPortDef.eDir, sPortDef.bEnabled, sPortDef.bPopulated);
    */
    
    for(i=0; i<pMP3ClientParams->nAESBufCnt; i++) {
        pBufferHdr = NULL;
        eRetVal = OMX_AllocateBuffer(pMP3ClientParams->hParser, 
                                     &pBufferHdr, 
                                     pMP3ClientParams->nParserAudioPortIdx, 
                                     NULL, 
                                     pMP3ClientParams->nAESBufSize);
        if(eRetVal != OMX_ErrorNone) {
            INFO_PRINT("%s allocate buffer for port %d failed.\n", "Parser", 1);
            return eRetVal;
        }
        pMP3ClientParams->pParserOutBufHdr[i] = pBufferHdr;
        /*
        DEBUG_PRINT("%s allocate buffer %p:%p for port %d\n", "Parser", pBufferHdr, pBufferHdr->pBuffer, 1); 
        */
    }


    /* parser wait to idle */
    
    do {
        OMX_GetState(pMP3ClientParams->hParser, &pMP3ClientParams->sAppHandler.eState);
    } while (pMP3ClientParams->sAppHandler.eState != OMX_StateIdle);


    /* set parser to exec */

    eRetVal = OMX_SendCommand(pMP3ClientParams->hParser, 
                                OMX_CommandStateSet, 
                                OMX_StateExecuting, 
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;


    /* parser wait for exec */
    
    do {
        OMX_GetState(pMP3ClientParams->hParser, &pMP3ClientParams->sAppHandler.eState);
    } while (pMP3ClientParams->sAppHandler.eState != OMX_StateExecuting);

    DEBUG_PRINT("parser wait for exec OK!\n");

    /* parser set stream number */

    OMX_PARAM_U32TYPE u32Type;
    OMX_INIT_STRUCT(&u32Type, OMX_PARAM_U32TYPE);
    u32Type.nPortIndex = pMP3ClientParams->nParserAudioPortIdx;
    OMX_GetParameter(pMP3ClientParams->hParser, 
                     OMX_IndexParamNumAvailableStreams, 
                     &u32Type);
    DEBUG_PRINT("Total audio track number: %ld\n", u32Type.nU32);

    if(u32Type.nU32 > 0) {
        u32Type.nU32 = 0;
        OMX_SetParameter(pMP3ClientParams->hParser, 
                         OMX_IndexParamActiveStream, 
                         &u32Type);
    }

    
    /* free resources */


    FSL_FREE(Content);

    return OMX_ErrorNone;
}


/*******************************************************************************
 Function       : MP3_UnLoadParser

 Description    : This function is used to uninitialize parser component

 Returns        : OMX_ERRORTYPE

 Arguments      : pMP3ClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE MP3_UnLoadParser(MP3_CLIENT_PARAMS *pMP3ClientParams)
{
    OMX_ERRORTYPE       eRetVal = OMX_ErrorNone;

    if(pMP3ClientParams->hParser == NULL)
        return OMX_ErrorNone;
    
    eRetVal = OMX_SendCommand(pMP3ClientParams->hParser,
                                OMX_CommandStateSet,
                                OMX_StateIdle,
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    do
    {
        OMX_GetState(pMP3ClientParams->hParser, &pMP3ClientParams->sAppHandler.eState);
    }while (pMP3ClientParams->sAppHandler.eState != OMX_StateIdle);

    eRetVal = OMX_SendCommand(pMP3ClientParams->hParser,
                             OMX_CommandStateSet,
                             OMX_StateLoaded,
                             NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* Free Buffers for parser */

    OMX_U32 i;
    for(i=0; i<pMP3ClientParams->nAESBufCnt; i++) {
        eRetVal = OMX_FreeBuffer(pMP3ClientParams->hParser,
                                pMP3ClientParams->nParserAudioPortIdx, 
                                pMP3ClientParams->pParserOutBufHdr[i]);
        if(eRetVal != OMX_ErrorNone) {
            INFO_PRINT("%s free buffer for port 1 failed idx %ld retval %x.\n", "Parser", i, eRetVal);
            return eRetVal;
        }
    }

    do
    {
        OMX_GetState(pMP3ClientParams->hParser, &pMP3ClientParams->sAppHandler.eState);
    }while (pMP3ClientParams->sAppHandler.eState != OMX_StateLoaded);

    return OMX_ErrorNone;
    
}


/*******************************************************************************
 Function       : MP3_LoadAudioDecoder

 Description    : This function is used to initialize decoder component

 Returns        : OMX_ERRORTYPE

 Arguments      : pMP3ClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE MP3_LoadAudioDecoder(MP3_CLIENT_PARAMS *pMP3ClientParams)
{
    OMX_PARAM_PORTDEFINITIONTYPE    sParamPort;
    OMX_ERRORTYPE                   eRetVal = OMX_ErrorNone;
    OMX_U16                         nCounter = 0;
    OMX_BUFFERHEADERTYPE            *pBufferHdr;
    /*OMX_PTR                         pBuffer = NULL;*/
    OMX_S8                          cComponentName[COMP_NAME_LENGTH];
    OMX_S8                          role[COMP_NAME_LENGTH] ;
    
    switch(pMP3ClientParams->AudioFmt)
    {
        case OMX_AUDIO_CodingMP3:
            fsl_osal_strcpy((fsl_osal_char *)cComponentName,(fsl_osal_char *)MP3_DECODER_NAME);
            fsl_osal_strcpy((fsl_osal_char *)role,(fsl_osal_char *)MP3_DECODER_ROLE);
            break;

        default:
            INFO_PRINT("Unsupported audio format: %d\n", pMP3ClientParams->AudioFmt);
            return OMX_ErrorBadParameter;
    }

    eRetVal = OMX_GetHandle(&pMP3ClientParams->hDecoder,
                            (OMX_STRING)cComponentName,
                            &pMP3ClientParams->sAppHandler,
                            &gMP3Callbacks);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;
    
    OMX_PARAM_COMPONENTROLETYPE CurRole;
    OMX_INIT_STRUCT(&CurRole, OMX_PARAM_COMPONENTROLETYPE);
    fsl_osal_memcpy(&CurRole.cRole, role, OMX_MAX_STRINGNAME_SIZE);
    OMX_SetParameter(pMP3ClientParams->hDecoder, 
                     OMX_IndexParamStandardComponentRole, 
                     &CurRole);

    /* set audio format */
    
    OMX_INIT_STRUCT(&sParamPort, OMX_PARAM_PORTDEFINITIONTYPE);
    sParamPort.nPortIndex = 0;
    eRetVal = OMX_GetParameter(pMP3ClientParams->hDecoder, 
                               OMX_IndexParamPortDefinition, 
                               &sParamPort);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    sParamPort.format.audio.eEncoding = pMP3ClientParams->AudioFmt;
    sParamPort.nBufferCountActual = pMP3ClientParams->nAESBufCnt;
    sParamPort.nBufferSize = pMP3ClientParams->nAESBufSize;

    eRetVal = OMX_SetParameter(pMP3ClientParams->hDecoder, 
                               OMX_IndexParamPortDefinition, 
                               &sParamPort);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;
    
    DEBUG_PRINT("set audio format success\n");

    
    /* set decoder to idle */

    eRetVal = OMX_SendCommand(pMP3ClientParams->hDecoder, 
                                OMX_CommandStateSet, 
                                OMX_StateIdle, 
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* decoder allocate buffer */

    OMX_PORT_PARAM_TYPE sAudioPortInfo ;

    OMX_INIT_STRUCT(&sAudioPortInfo, OMX_PORT_PARAM_TYPE);
    eRetVal = OMX_GetParameter(pMP3ClientParams->hDecoder, 
                               OMX_IndexParamAudioInit, 
                               &sAudioPortInfo);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;
    
    for(nCounter = sAudioPortInfo.nStartPortNumber;
        nCounter < (sAudioPortInfo.nStartPortNumber + sAudioPortInfo.nPorts);
        nCounter++)
    {
        sParamPort.nPortIndex = nCounter;
        eRetVal = OMX_GetParameter(pMP3ClientParams->hDecoder, 
                                   OMX_IndexParamPortDefinition, 
                                   &sParamPort);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;

        if(sParamPort.eDir == OMX_DirInput) {
            OMX_U32 i = 0;
            for(i=0; i<pMP3ClientParams->nAESBufCnt; i++)
            {
                eRetVal = OMX_UseBuffer(pMP3ClientParams->hDecoder,
                                        &pBufferHdr,
                                        sParamPort.nPortIndex,
                                        NULL,
                                        pMP3ClientParams->nAESBufSize,
                                        (OMX_U8 *)pMP3ClientParams->pParserOutBufHdr[i]->pBuffer);
                if(eRetVal != OMX_ErrorNone)
                    return eRetVal;

                pMP3ClientParams->pDecoderInBufHdr[i] = pBufferHdr;
            }

        } 
        else {
            pMP3ClientParams->nPCMBufSize = sParamPort.nBufferSize;
            pMP3ClientParams->nPCMBufCnt = sParamPort.nBufferCountActual;

            for(OMX_U32 i=0; i<pMP3ClientParams->nPCMBufCnt; i++) {
                pBufferHdr = NULL;
                eRetVal = OMX_AllocateBuffer(pMP3ClientParams->hDecoder, 
                                             &pBufferHdr, 
                                             sParamPort.nPortIndex, 
                                             NULL, 
                                             pMP3ClientParams->nPCMBufSize);
                if(eRetVal != OMX_ErrorNone) {
                    INFO_PRINT("%s allocate buffer for port %ld failed.\n", "Decoder", sParamPort.nPortIndex);
                    return eRetVal;
                }
                pMP3ClientParams->pDecoderOutBufHdr[i] = pBufferHdr;
                /*
                DEBUG_PRINT("%s allocate buffer %p:%p for port %d\n", "Parser", pBufferHdr, pBufferHdr->pBuffer, 1); 
                */
            }

        }

    }

    DEBUG_PRINT("allocat audio decoder buffer finished\n");

    /* decoder wait for idle */

    do {
        OMX_GetState(pMP3ClientParams->hDecoder, &pMP3ClientParams->sAppHandler.eState);
    } while (pMP3ClientParams->sAppHandler.eState != OMX_StateIdle);

    DEBUG_PRINT("wait decoder to idle finished\n");

    /* decoder set to exec */

    eRetVal = OMX_SendCommand(pMP3ClientParams->hDecoder,
                             OMX_CommandStateSet,
                             OMX_StateExecuting,
                             NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;


    /* decoder wait to exec */

    do {
        OMX_GetState(pMP3ClientParams->hDecoder, &pMP3ClientParams->sAppHandler.eState);
    } while (pMP3ClientParams->sAppHandler.eState != OMX_StateExecuting);

    DEBUG_PRINT("wait decoder to exec finished\n");

/*
    OMX_INIT_STRUCT(&pMP3ClientParams->AudioDecPCMPara, OMX_AUDIO_PARAM_MP3TYPE);
    pMP3ClientParams->AudioDecPCMPara.nPortIndex = 1;
    eRetVal = OMX_GetParameter(pMP3ClientParams->hDecoder, 
                               OMX_IndexParamAudioPcm, 
                               &pMP3ClientParams->AudioDecPCMPara);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;
*/

    for(OMX_U32 i = 0; i < pMP3ClientParams->nAESBufCnt; i++)
    {
        eRetVal = OMX_FillThisBuffer(pMP3ClientParams->hParser,
                                     pMP3ClientParams->pParserOutBufHdr[i]);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;
    }

    for(OMX_U32 i = 0; i < pMP3ClientParams->nPCMBufCnt; i++)
    {
        eRetVal = OMX_FillThisBuffer(pMP3ClientParams->hDecoder,
                                     pMP3ClientParams->pDecoderOutBufHdr[i]);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;
    }
    return OMX_ErrorNone;

}


/*******************************************************************************
 Function       : MP3_UnLoadAudioDecoder

 Description    : This function is used to initialize video decoder component

 Returns        : OMX_ERRORTYPE

 Arguments      : pMP3ClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE MP3_UnLoadAudioDecoder(MP3_CLIENT_PARAMS *pMP3ClientParams)
{

    OMX_ERRORTYPE     eRetVal = OMX_ErrorNone;

    if(pMP3ClientParams->hDecoder== NULL)
        return OMX_ErrorNone;

    eRetVal = OMX_SendCommand(pMP3ClientParams->hDecoder,
                                OMX_CommandStateSet ,
                                OMX_StateIdle,
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    do
    {
        OMX_GetState(pMP3ClientParams->hDecoder, &pMP3ClientParams->sAppHandler.eState);
    }while (pMP3ClientParams->sAppHandler.eState != OMX_StateIdle);

    eRetVal = OMX_SendCommand(pMP3ClientParams->hDecoder,
                             OMX_CommandStateSet,
                             OMX_StateLoaded,
                             NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* Free Buffers for audio decoder */

    OMX_U32 i;    
    for(i=0; i<pMP3ClientParams->nAESBufCnt; i++) {
        eRetVal = OMX_FreeBuffer(pMP3ClientParams->hDecoder,
                                0, 
                                pMP3ClientParams->pDecoderInBufHdr[i]);
        if(eRetVal != OMX_ErrorNone) {
            INFO_PRINT("%s free buffer for port %d failed.\n", "Decoder", 0);
            return eRetVal;
        }
    }

    for(i=0; i<pMP3ClientParams->nPCMBufCnt; i++) {
        eRetVal = OMX_FreeBuffer(pMP3ClientParams->hDecoder,
                                1, 
                                pMP3ClientParams->pDecoderOutBufHdr[i]);
        if(eRetVal != OMX_ErrorNone) {
            INFO_PRINT("%s free buffer for port %d failed.\n", "Decoder", 1);
            return eRetVal;
        }
    }

    do
    {
        OMX_GetState(pMP3ClientParams->hDecoder, &pMP3ClientParams->sAppHandler.eState);
    }while (pMP3ClientParams->sAppHandler.eState != OMX_StateLoaded);

    return OMX_ErrorNone;
}


/*******************************************************************************
 Function       : MP3_LoadAudioRender

 Description    : This function is used to initialize video render component

 Returns        : OMX_ERRORTYPE

 Arguments      : pMP3ClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE MP3_LoadAudioRender(MP3_CLIENT_PARAMS *pMP3ClientParams)
{
    OMX_ERRORTYPE           eRetVal = OMX_ErrorNone;
    OMX_S8                  cSinkComponentName[COMP_NAME_LENGTH];
    OMX_PARAM_PORTDEFINITIONTYPE sParamPort, sDownstreamParamPort;
    OMX_BUFFERHEADERTYPE    *pBufferHdr;
    OMX_U32                 nCounter;

    DEBUG_PRINT("load audio render\n");

    fsl_osal_strcpy((fsl_osal_char *)cSinkComponentName,(fsl_osal_char *)AUDIO_RENDER_NAME);
    eRetVal = OMX_GetHandle(&pMP3ClientParams->hRender,
                            (OMX_STRING)cSinkComponentName,
                            &pMP3ClientParams->sAppHandler,
                            &gMP3Callbacks);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* set render role */

    OMX_STRING role = AUDIO_RENDER_ROLE;
    OMX_PARAM_COMPONENTROLETYPE CurRole;
    OMX_INIT_STRUCT(&CurRole, OMX_PARAM_COMPONENTROLETYPE);
    fsl_osal_memcpy(&CurRole.cRole, role, OMX_MAX_STRINGNAME_SIZE);
    OMX_SetParameter(pMP3ClientParams->hRender, 
                     OMX_IndexParamStandardComponentRole, 
                     &CurRole);

    DEBUG_PRINT("set render role ok\n");

    /* Get vido decoder outport format */
    OMX_INIT_STRUCT(&sParamPort, OMX_PARAM_PORTDEFINITIONTYPE);
    sParamPort.nPortIndex = 1;
    eRetVal = OMX_GetParameter(pMP3ClientParams->hDecoder,
                               OMX_IndexParamPortDefinition,
                               &sParamPort);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    /* set render inport format */
    OMX_INIT_STRUCT(&sDownstreamParamPort, OMX_PARAM_PORTDEFINITIONTYPE);
    sDownstreamParamPort.nPortIndex = 0;
    eRetVal = OMX_GetParameter(pMP3ClientParams->hRender,
                               OMX_IndexParamPortDefinition,
                               &sDownstreamParamPort);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;
    sDownstreamParamPort.format.audio = sParamPort.format.audio; 
    sDownstreamParamPort.nBufferSize = sParamPort.nBufferSize; 
    sDownstreamParamPort.nBufferCountActual = sParamPort.nBufferCountActual; 
    sDownstreamParamPort.nBufferCountMin = sParamPort.nBufferCountMin; 
    eRetVal = OMX_SetParameter(pMP3ClientParams->hRender,
                               OMX_IndexParamPortDefinition,
                               &sDownstreamParamPort);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    DEBUG_PRINT("set render input format ok\n");


    /* Set the values for the component extension structure */
    OMX_AUDIO_PARAM_PCMMODETYPE sFormatPCM;
    OMX_INIT_STRUCT(&sFormatPCM, OMX_AUDIO_PARAM_PCMMODETYPE);
    sFormatPCM.nPortIndex = 1;
    eRetVal = OMX_GetParameter(pMP3ClientParams->hDecoder,
                               OMX_IndexParamAudioPcm,
                               &sFormatPCM);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    //fsl_osal_memcpy(&pMP3ClientParams->AudioDecPCMPara, &sFormatPCM, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));

    sFormatPCM.nPortIndex = 0;
    eRetVal = OMX_SetParameter(pMP3ClientParams->hRender,
                               OMX_IndexParamAudioPcm,
                               &sFormatPCM);

    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    
    /* set render to idle */
    eRetVal = OMX_SendCommand(pMP3ClientParams->hRender, 
                                OMX_CommandStateSet, 
                                OMX_StateIdle, 
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

   
    /*########################################*/
    /* allocate buffer for render component */
    /*########################################*/
    for(nCounter=0; nCounter<pMP3ClientParams->nPCMBufCnt; nCounter++) {

        pBufferHdr = NULL;
        eRetVal = OMX_UseBuffer(pMP3ClientParams->hRender,
                                &pBufferHdr,
                                0, 
                                NULL,
                                pMP3ClientParams->nPCMBufSize,
                                (OMX_U8 *)pMP3ClientParams->pDecoderOutBufHdr[nCounter]->pBuffer);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;

        /* set the physicall address for buffer header */
        //pBufferHdr->nOffset = pMP3ClientParams->pRenderInBufHdr[nCounter]->nOffset;

        pMP3ClientParams->pRenderInBufHdr[nCounter] = pBufferHdr;
    }

    do {
        OMX_GetState(pMP3ClientParams->hRender, &pMP3ClientParams->sAppHandler.eState);
    } while (pMP3ClientParams->sAppHandler.eState != OMX_StateIdle);

    DEBUG_PRINT("wait render to idle  ok\n");

    /* send command to render change to executing state */
    eRetVal = OMX_SendCommand(pMP3ClientParams->hRender,
                                OMX_CommandStateSet,
                                OMX_StateExecuting,
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    do {
        OMX_GetState(pMP3ClientParams->hRender, &pMP3ClientParams->sAppHandler.eState);
    } while (pMP3ClientParams->sAppHandler.eState != OMX_StateExecuting);

    DEBUG_PRINT("wait render to exec  ok\n");

    return eRetVal;
}


/*******************************************************************************
 Function       : MP3_UnLoadAudioRender

 Description    : This function is used to initialize video render component

 Returns        : OMX_ERRORTYPE

 Arguments      : pMP3ClientParams - Pointer to ILClient structure

********************************************************************************/
static OMX_ERRORTYPE MP3_UnLoadAudioRender(MP3_CLIENT_PARAMS *pMP3ClientParams)
{
    OMX_ERRORTYPE   eRetVal = OMX_ErrorNone;

    if(pMP3ClientParams->hRender== NULL)
        return OMX_ErrorNone;

    eRetVal = OMX_SendCommand(pMP3ClientParams->hRender,
                                OMX_CommandStateSet,
                                OMX_StateIdle,
                                NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    do
    {
        OMX_GetState(pMP3ClientParams->hRender, &pMP3ClientParams->sAppHandler.eState);
    }while (pMP3ClientParams->sAppHandler.eState != OMX_StateIdle);

    eRetVal = OMX_SendCommand(pMP3ClientParams->hRender,
                             OMX_CommandStateSet,
                             OMX_StateLoaded,
                             NULL);
    if(eRetVal != OMX_ErrorNone)
        return eRetVal;

    
    /* Free Buffers for render */

    OMX_U32 i;
    for(i=0; i<pMP3ClientParams->nPCMBufCnt; i++) {
        eRetVal = OMX_FreeBuffer(pMP3ClientParams->hRender,
                                0,
                                pMP3ClientParams->pRenderInBufHdr[i]);
        if(eRetVal != OMX_ErrorNone){
            INFO_PRINT("%s free buffer for port %d failed.\n", "Render", 0);
            return eRetVal;
        }
    }    


    do
    {
        OMX_GetState(pMP3ClientParams->hRender, &pMP3ClientParams->sAppHandler.eState);
    }while (pMP3ClientParams->sAppHandler.eState != OMX_StateLoaded);

    return OMX_ErrorNone;
}


/*******************************************************************************
 Function       : MP3_FindBufferHeader

 Description    : This function is used to look for a matched buffer header
                    in a header list

 Returns        : OMX_S8
                    index of matched buffer header in header list
                    -1 - not found

 Arguments      : pHeaderList  - header list to be searched in
                  pBufferHdr   - header containing buffer that needs to be searched
                  num          - size of header list

********************************************************************************/
static OMX_S8 MP3_FindBufferHeader(
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
  Function: MP3_SendEmptyBuffers

  Description: This function reads empty buffer queue
                and sends it to the component on receiving an EmptyBufferDone Callback

  Returns: OMX_ERRORTYPE

  Arguments: pMP3ClientParams - Pointer to ILClient structure

 ********************************************************************************/

static OMX_ERRORTYPE MP3_SendEmptyBuffers(MP3_CLIENT_PARAMS *pMP3ClientParams)
{
    OMX_ERRORTYPE       eRetVal = OMX_ErrorNone;
    Message             *pMsg = NULL;
    Message             sMsg;
    QUEUE_ERRORTYPE     eRetQueue = QUEUE_SUCCESS;

    pMsg = (Message *)&sMsg;

    eRetQueue = ReadQueue(pMP3ClientParams->sAppHandler.EmptyBufQueue, pMsg, E_FSL_OSAL_TRUE);
    if(eRetQueue != QUEUE_SUCCESS) {
        INFO_PRINT("dequeue failed.\n");
        return OMX_ErrorUndefined;
    }

    /* receive EmptyBufferDone event from videodec */

    if (pMsg->hComponent == pMP3ClientParams->hDecoder) {
        
        DEBUG_PRINT("EmptyBufferDone from decoder\n");
        
        int idx;

        idx = MP3_FindBufferHeader(pMP3ClientParams->pParserOutBufHdr, pMsg->data.pBuffer, pMP3ClientParams->nAESBufCnt);
        if(idx < 0)
            return OMX_ErrorUndefined;

        DEBUG_PRINT("parser fill this buffer\n");
        eRetVal = OMX_FillThisBuffer(pMP3ClientParams->hParser,
                                     pMP3ClientParams->pParserOutBufHdr[idx]);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;
    }

    /* receive EmptyBufferDone event from render */
    if (pMsg->hComponent == pMP3ClientParams->hRender) {
        int idx;

        DEBUG_PRINT("EmptyBufferDone from render\n");

        idx = MP3_FindBufferHeader(pMP3ClientParams->pDecoderOutBufHdr, pMsg->data.pBuffer, pMP3ClientParams->nPCMBufCnt);
        if(idx < 0)
            return OMX_ErrorUndefined;

        /*
        DEBUG_PRINT("receive EmptyBufferDone event from render\n");
        */
        DEBUG_PRINT("decoder fill this buffer\n");

        eRetVal = OMX_FillThisBuffer(pMP3ClientParams->hDecoder,
                                     pMP3ClientParams->pDecoderOutBufHdr[idx]);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;
    }


    return eRetVal;
}


/*******************************************************************************
  Function: MP3_SendOutputBuffers

  Description: This function reads from filled buffer queue
                and sends it to the component on receiving a FillBufferDone Callback

  Returns: OMX_ERRORTYPE

  Arguments: *pMP3ClientParams - Pointer to ILClient structure

********************************************************************************/

static OMX_ERRORTYPE MP3_SendFilledBuffers(MP3_CLIENT_PARAMS *pMP3ClientParams)
{
    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;
    Message       *pMsg = NULL;
    Message       sMsg;
    QUEUE_ERRORTYPE eRetQueue;

    pMsg = (Message *)&sMsg;
    eRetQueue = ReadQueue(pMP3ClientParams->sAppHandler.FilledBufQueue, pMsg, E_FSL_OSAL_TRUE);
    if(eRetQueue != QUEUE_SUCCESS) {
        INFO_PRINT("dequeue failed.\n");
        return OMX_ErrorUndefined;
    }

    /* receive FillBufferDone event from parser */
    
    if (pMsg->hComponent == pMP3ClientParams->hParser) {
        
        DEBUG_PRINT("FillBufferDone from parser\n");
        
        if(pMsg->data.pBuffer->nFlags == OMX_BUFFERFLAG_EOS) {
            INFO_PRINT("Receive EOS...\n");
            assert(0);
            pMP3ClientParams->bEOS = OMX_TRUE;
            return OMX_ErrorNone;
        }

        int idx;

        idx = MP3_FindBufferHeader(pMP3ClientParams->pDecoderInBufHdr, pMsg->data.pBuffer, pMP3ClientParams->nAESBufCnt);
        if(idx < 0)
            return OMX_ErrorUndefined;

        pMP3ClientParams->pDecoderInBufHdr[idx]->nFilledLen = pMsg->data.pBuffer->nFilledLen;
        pMP3ClientParams->pDecoderInBufHdr[idx]->nOffset = pMsg->data.pBuffer->nOffset;
        pMP3ClientParams->pDecoderInBufHdr[idx]->nFlags =  0;

        /*
        DEBUG_PRINT("parser FillBufferDone len %ld flag %lx\n", 
            pMP3ClientParams->pDecoderInBufHdr[idx]->nFilledLen,
            pMP3ClientParams->pDecoderInBufHdr[idx]->nFlags);
        */
        assert(pMP3ClientParams->pDecoderInBufHdr[idx]->nFilledLen > 0);

        DEBUG_PRINT("decoder empty this buffer \n");

        eRetVal = OMX_EmptyThisBuffer(pMP3ClientParams->hDecoder,
                                      pMP3ClientParams->pDecoderInBufHdr[idx]);

        if(eRetVal != OMX_ErrorNone) {
            DEBUG_PRINT("Decoder empty buffer failed.\n");
            return eRetVal;
        }

    }

    /* receive FillBufferDone event from videodec */
    
    if (pMsg->hComponent == pMP3ClientParams->hDecoder) {
        
        DEBUG_PRINT("FillBufferDone from decoder\n");
        
        int idx;

        idx = MP3_FindBufferHeader(pMP3ClientParams->pRenderInBufHdr, pMsg->data.pBuffer, pMP3ClientParams->nPCMBufCnt);
        if(idx < 0)
            return OMX_ErrorUndefined;

        pMP3ClientParams->pRenderInBufHdr[idx]->nFilledLen = pMsg->data.pBuffer->nFilledLen;
        pMP3ClientParams->pRenderInBufHdr[idx]->nOffset = pMsg->data.pBuffer->nOffset;
        pMP3ClientParams->pRenderInBufHdr[idx]->nFlags = pMsg->data.pBuffer->nFlags;
        /*
        fsl_osal_sleep(33000);
        */

        DEBUG_PRINT("render empty this buffer len %ld flag 0x%lx\n", 
            pMP3ClientParams->pRenderInBufHdr[idx]->nFilledLen,
            pMP3ClientParams->pRenderInBufHdr[idx]->nFlags);

        eRetVal = OMX_EmptyThisBuffer(pMP3ClientParams->hRender,
                                      pMP3ClientParams->pRenderInBufHdr[idx]);
        if(eRetVal != OMX_ErrorNone)
            return eRetVal;

    }

    return eRetVal;
}


/*******************************************************************************
  Function: MP3_ProcessEventQueue

  Description: This function is used to process each entry obtained in the
                Event queue.

  Returns: OMX_ERRORTYPE

  Arguments: pMsg - Pointer to Entry in the event queue.
             *pMP3ClientParams - Pointer to ILClient structure
********************************************************************************/
static OMX_ERRORTYPE MP3_ProcessEventQueue(MP3_CLIENT_PARAMS *pMP3ClientParams, Message *pMsg)
{

    OMX_ERRORTYPE eRetVal = OMX_ErrorNone;

    //DEBUG_PRINT("MP3_ProcessEventQueue MsgType %d, Eventype %d\n", pMsg->MsgType, pMsg->event);

    switch(pMsg->MsgType)
    {
        case Eventype:
            if(pMsg->event == EmptyBufferDone)
            {
                /*
                DEBUG_PRINT("Received Emptybuffer done event\n");
                */

                eRetVal = MP3_SendEmptyBuffers(pMP3ClientParams);
                if(eRetVal != OMX_ErrorNone)
                {
                    DEBUG_PRINT("MP3_SendEmptyBuffers fail!!! %x\n", eRetVal);
                    return eRetVal;
                }
            }
            else if(pMsg->event == FillBufferDone)
            {
                /*
                DEBUG_PRINT("Received Fillbuffer done event\n");
                */

                eRetVal = MP3_SendFilledBuffers(pMP3ClientParams);
                if(eRetVal != OMX_ErrorNone)
                {
                    DEBUG_PRINT("MP3_SendFilledBuffers fail!!! %x\n", eRetVal);
                    return eRetVal;
                }
            }
            else if(pMsg->event == PortSettingsChanged) {
                if(pMsg->hComponent == pMP3ClientParams->hDecoder) {

                    DEBUG_PRINT("decoder PortSettingsChanged %ld %ld\n", pMsg->nData1, pMsg->nData2);

                
                    /* Get audio decoder outport format */

                    /*
                    OMX_PARAM_PORTDEFINITIONTYPE sParamPort;
                    OMX_INIT_STRUCT(&sParamPort, OMX_PARAM_PORTDEFINITIONTYPE);
                    sParamPort.nPortIndex = 1;
                    eRetVal = OMX_GetParameter(pMP3ClientParams->hDecoder,
                                               OMX_IndexParamPortDefinition,
                                               &sParamPort);
                    if(eRetVal != OMX_ErrorNone)
                        return eRetVal;

                    DEBUG_PRINT("audio eEncoding : %d\n", (unsigned int)sParamPort.format.audio.eEncoding);

                    */
                    if(pMP3ClientParams->hRender == NULL){
                        if((eRetVal = MP3_LoadAudioRender(pMP3ClientParams)) != OMX_ErrorNone)
                        {
                            INFO_PRINT("MP3_LoadAudioRender fail %x!!!\n", eRetVal);
                            break;
                        }
                    }
                    
                }
            }
            else if(pMsg->event == PortDisabled) {
                    DEBUG_PRINT("Port disabled\n");
            }
            else if(pMsg->event == PortEnabled) {
                    DEBUG_PRINT("Port enabled\n");
            }
            else if(pMsg->event == PortFormatDetected) {
                if(pMsg->hComponent == pMP3ClientParams->hParser)
                {

                    DEBUG_PRINT("Parser PortFormatDetected\n");

                    OMX_AUDIO_PARAM_PORTFORMATTYPE sAudioPortFmt;

                    OMX_INIT_STRUCT(&sAudioPortFmt, OMX_AUDIO_PARAM_PORTFORMATTYPE);
                    sAudioPortFmt.nPortIndex = pMP3ClientParams->nParserAudioPortIdx;
                    OMX_GetParameter(pMP3ClientParams->hParser, 
                                     OMX_IndexParamAudioPortFormat, 
                                     &sAudioPortFmt);

                    if(pMP3ClientParams->AudioFmt != sAudioPortFmt.eEncoding)
                    {
                        pMP3ClientParams->AudioFmt = sAudioPortFmt.eEncoding;
                        DEBUG_PRINT("Audio format detected: %x\n", pMP3ClientParams->AudioFmt);

                        if((eRetVal = MP3_LoadAudioDecoder(pMP3ClientParams)) != OMX_ErrorNone)
                        {
                            INFO_PRINT("MP3_LoadAudioDecoder fail %x!!!\n", eRetVal);
                            break;
                        }
                        
                        
                    }
                } 
            }
            else if(pMsg->event == EndOfStream)
            {
                INFO_PRINT("event: EndOfStream...\n");
                pMP3ClientParams->bEOS = OMX_TRUE;
            }
            break;
            
        default:
            break;
    }

    return eRetVal;
}


/*******************************************************************************

* The MP3_EventHandlerCallback method is used to notify the application when an
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

static OMX_ERRORTYPE MP3_EventHandlerCallback(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    AppHandler *pAppHandler = (AppHandler *)pAppData;
    Message *pMsg = NULL;

    /*
    DEBUG_PRINT("MP3_EventHandlerCallback %lx event type %d, nData1 %ld\n",(unsigned long)hComponent, eEvent, nData1);
    */

    MP3_FrameMessage(&pMsg,Eventype);

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
    else if(eEvent == OMX_EventPortFormatDetected)
        pMsg->event = PortFormatDetected;

    pMsg->nData1 = nData1;
    pMsg->nData2 = nData2;

    pMsg->hComponent = hComponent;
    MP3_PostMessage(pAppHandler, pMsg);
    FSL_FREE(pMsg);
    return OMX_ErrorNone;
}


/*******************************************************************************

* The MP3_EmptyBufferDoneCallback method is used to return emptied buffers from an
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
static OMX_ERRORTYPE MP3_EmptyBufferDoneCallback(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    AppHandler *pAppHandler = (AppHandler *)pAppData;
    Message *pMsg = NULL;

    /*
    DEBUG_PRINT("============MP3_EmptyBufferDoneCallback %lx\n",(unsigned long)hComponent);
    */
    MP3_FrameMessage(&pMsg,DataBuffer);
    pMsg->data.type = EmptyBuffer;
    pMsg->data.pBuffer = pBuffer;
    pMsg->hComponent = hComponent;
    MP3_PostMessage(pAppHandler, pMsg);
    FSL_FREE(pMsg);

    MP3_FrameMessage(&pMsg,Eventype);
    pMsg->event = EmptyBufferDone;
    pMsg->hComponent = hComponent;
    MP3_PostMessage(pAppHandler, pMsg);
    FSL_FREE(pMsg);

    return OMX_ErrorNone;
}


/*******************************************************************************
** The MP3_FillBufferDoneCallback method is used to return filled buffers from an
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
static OMX_ERRORTYPE MP3_FillBufferDoneCallback(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    AppHandler *pAppHandler = (AppHandler *)pAppData;
    Message *pMsg = NULL;
    /*
    DEBUG_PRINT("MP3_FillBufferDoneCallback %lx, %lx, AllocLen %ld, FilledLen %ld Offset %ld OutPortIdx %ld InPortIdx %ld \n", 
                (unsigned long)hComponent, (unsigned long)pBuffer->pBuffer, pBuffer->nAllocLen, pBuffer->nFilledLen, 
                pBuffer->nOffset, pBuffer->nOutputPortIndex, pBuffer->nInputPortIndex);
    */
    MP3_FrameMessage(&pMsg,DataBuffer);
    pMsg->data.type = FilledBuffer;
    pMsg->data.pBuffer = pBuffer;
    pMsg->hComponent = hComponent;
    MP3_PostMessage(pAppHandler, pMsg);
    FSL_FREE(pMsg);
    MP3_FrameMessage(&pMsg,Eventype);
    pMsg->event = FillBufferDone;
    pMsg->hComponent = hComponent;
    MP3_PostMessage(pAppHandler, pMsg);
    FSL_FREE(pMsg);

    return OMX_ErrorNone;
}


static OMX_BOOL MP3_CharTolower(OMX_STRING string)
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
static OMX_S32 MP3_ILClientMain(void * file)
{
    OMX_ERRORTYPE           eRetVal 	= OMX_ErrorNone;
    MP3_CLIENT_PARAMS       sVDTClientParams;
    	Message                 *pMsg = NULL;
   
    sVDTClientParams.pInfilename = (OMX_U8*)file;

    eRetVal = MP3_Initialize(&sVDTClientParams);
    if(eRetVal != OMX_ErrorNone)
        return -1;


    eRetVal = MP3_LoadParser(&sVDTClientParams);
    if(eRetVal != OMX_ErrorNone) {
        INFO_PRINT("\n\t\t Error in MP3_LoadParser \n");
        return -1;
    }

    /*################################*/
    /* playback event processing loop */
    /*################################*/

    pMsg = (Message *)FSL_MALLOC(sizeof(Message));
    
    do {
        MP3_ReadEventQueue(&sVDTClientParams.sAppHandler , pMsg);

        eRetVal = MP3_ProcessEventQueue(&sVDTClientParams, pMsg);

        if(eRetVal != OMX_ErrorNone)
            break;

        if (sVDTClientParams.bEOS == OMX_TRUE)
            break;

    } while(1);

    INFO_PRINT("Playback done.\n");

    FSL_FREE(pMsg);


    MP3_UnLoadParser(&sVDTClientParams);

    MP3_UnLoadAudioDecoder(&sVDTClientParams);

    MP3_UnLoadAudioRender(&sVDTClientParams);


    if(OMX_FreeHandle(sVDTClientParams.hParser) != OMX_ErrorNone)
        return -1;
    DEBUG_PRINT("Parser component deinited.\n");

    if(OMX_FreeHandle(sVDTClientParams.hDecoder) != OMX_ErrorNone)
        return -1;
    DEBUG_PRINT("vpu decoder component deinited.\n");


    if(OMX_FreeHandle(sVDTClientParams.hRender) != OMX_ErrorNone)
        return -1;
    DEBUG_PRINT("video render component deinited.\n");

    eRetVal = MP3_DeInitialize(&sVDTClientParams);
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
    MP3_CharTolower(subfix);
    if (fsl_osal_strcmp(subfix, ".mp3")!=0)
    {
        INFO_PRINT("Unsupported file type [%s]!!!\n", subfix);
        return -1;
    }
    

    eRetVal = OMX_Init();
    if (eRetVal != OMX_ErrorNone)
        return -1;

    MP3_ILClientMain(file);

    eRetVal = OMX_Deinit();
    if(eRetVal != OMX_ErrorNone)
        return -1;

    return 0;
}


/************************************ EOF ********************************/



