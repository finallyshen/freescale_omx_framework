/**
 *  Copyright (c) 2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "fsl_osal.h"
#include "Mem.h"
#include "GMEgl.h"
#include "eglRender.h"

typedef enum {
    EglCreate = 0,
    EglDestroy,
    EglCreateImage,
    EglDrawImage,
}GMEglMsgType;

typedef struct {
    RenderContext EglContext;
    OMX_PTR pThread;
    fsl_osal_sem Sem;
    GMEglMsgType Msg;
    OMX_BOOL bMsgDone;
    OMX_ERRORTYPE MsgRet;
    OMX_PTR EglImage;
}GMEglContext;

static fsl_osal_ptr GMEglThreadFunc(fsl_osal_ptr arg)
{
    GMEglContext *pGmEglContext = (GMEglContext*) arg;
    OMX_BOOL bRunning = OMX_TRUE;

    while(bRunning == OMX_TRUE) {
        fsl_osal_sem_wait(pGmEglContext->Sem);
        pGmEglContext->MsgRet = OMX_ErrorNone;
        RenderContext *pContext = &pGmEglContext->EglContext;
        switch(pGmEglContext->Msg) {
            case EglCreate:
                if(eglOpen(pContext) < 0) {
                    pGmEglContext->MsgRet = OMX_ErrorHardware;
                    bRunning = OMX_FALSE;
                }
                break;
            case EglDestroy:
                if(pContext != NULL)
                    eglClose(pContext);
                bRunning = OMX_FALSE;
                break;
            case EglCreateImage:
                pGmEglContext->EglImage = CreateEGLImage(pContext);
                break;
            case EglDrawImage:
                Draw(pContext, pGmEglContext->EglImage);
                break;
            default:
                break;
        }
        pGmEglContext->bMsgDone = OMX_TRUE;
    }

    return NULL;
}

static OMX_ERRORTYPE GMEglProcessMsg(
        GMEglContext *pGmEglContext,
        GMEglMsgType Msg)
{
    pGmEglContext->bMsgDone = OMX_FALSE;
    pGmEglContext->Msg = Msg;
    fsl_osal_sem_post(pGmEglContext->Sem);
    while(1) {
        if(pGmEglContext->bMsgDone == OMX_TRUE)
            break;
        fsl_osal_sleep(1000);
    }

    return pGmEglContext->MsgRet;
}

OMX_ERRORTYPE GMCreateEglContext(
        OMX_PTR *pContext, 
        OMX_U32 nWidth, 
        OMX_U32 nHeight, 
        GMEglDisplayType DispType)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GMEglContext *pGmEglContext = NULL;

    *pContext = NULL;
    pGmEglContext = (GMEglContext*) FSL_MALLOC(sizeof(GMEglContext));
    if(pGmEglContext == NULL)
        return OMX_ErrorInsufficientResources;

    fsl_osal_memset(pGmEglContext, 0, sizeof(GMEglContext));

    if(E_FSL_OSAL_SUCCESS != fsl_osal_sem_init(&pGmEglContext->Sem, 0, 0)) {
        GMDestroyEglContext(pGmEglContext);
        return OMX_ErrorInsufficientResources;
    }

    if(E_FSL_OSAL_SUCCESS != fsl_osal_thread_create(&pGmEglContext->pThread, NULL, GMEglThreadFunc, pGmEglContext)) {
        GMDestroyEglContext(pGmEglContext);
        return OMX_ErrorInsufficientResources;
    }

    pGmEglContext->EglContext.width = nWidth;
    pGmEglContext->EglContext.height = nHeight;
    pGmEglContext->EglContext.disType = (DisplayType)DispType;

    ret = GMEglProcessMsg(pGmEglContext, EglCreate);
    if(ret != OMX_ErrorNone) {
        GMDestroyEglContext(pGmEglContext);
        return ret;
    }

    *pContext = pGmEglContext;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMDestroyEglContext(OMX_PTR Context)
{
    GMEglContext *pGmEglContext = (GMEglContext*)Context;

    if(pGmEglContext == NULL)
        return OMX_ErrorBadParameter;

    GMEglProcessMsg(pGmEglContext, EglDestroy);

    if(pGmEglContext->pThread != NULL)
        fsl_osal_thread_destroy(pGmEglContext->pThread);

    if(pGmEglContext->Sem != NULL)
        fsl_osal_sem_destroy(pGmEglContext->Sem);

    if(pGmEglContext != NULL)
        FSL_FREE(pGmEglContext);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMCreateEglImage(
        OMX_PTR Context, 
        OMX_PTR *pImage)
{
    GMEglContext *pGmEglContext = (GMEglContext*)Context;

    if(pGmEglContext == NULL)
        return OMX_ErrorBadParameter;

    pGmEglContext->EglImage = NULL;
    GMEglProcessMsg(pGmEglContext, EglCreateImage);
    *pImage = pGmEglContext->EglImage;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMEglDrawImage(
        OMX_PTR Context, 
        OMX_PTR Image)
{
    GMEglContext *pGmEglContext = (GMEglContext*)Context;

    if(pGmEglContext == NULL)
        return OMX_ErrorBadParameter;

    pGmEglContext->EglImage = Image;
    GMEglProcessMsg(pGmEglContext, EglDrawImage);

    return OMX_ErrorNone;
}
