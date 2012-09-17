/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Template.h"

Template::Template()
{
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.template.sw-based");
    ComponentVersion.s.nVersionMajor = 0x0;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x0;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = "component.template";
    bInContext = OMX_FALSE;
    nPorts = 2;
    pInBufferHdr = pOutBufferHdr = NULL; 
}

OMX_ERRORTYPE Template::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = IN_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
    sPortDef.nBufferSize = 1000;
    ret = ports[IN_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for port[%d] failed.\n", IN_PORT);
        return ret;
    }

    sPortDef.nPortIndex = OUT_PORT;
    sPortDef.eDir = OMX_DirOutput;
    sPortDef.eDomain = OMX_PortDomainAudio;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 3;
    sPortDef.nBufferSize = 128;
    ret = ports[OUT_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for port[%d] failed.\n", 0);
        return ret;
    }

    return ret;
}

OMX_ERRORTYPE Template::DeInitComponent()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Template::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE Template::ProcessDataBuffer()
{
    LOG_DEBUG("In: %d, Out: %d\n", ports[IN_PORT]->BufferNum(), ports[OUT_PORT]->BufferNum());

    if((ports[IN_PORT]->BufferNum() == 0 && pInBufferHdr == NULL )
            || ports[OUT_PORT]->BufferNum() == 0)
        return OMX_ErrorNoMore;

    ports[OUT_PORT]->GetBuffer(&pOutBufferHdr);
    if(pOutBufferHdr == NULL)
        return OMX_ErrorUnderflow;

    if(pInBufferHdr == NULL) {
        ports[IN_PORT]->GetBuffer(&pInBufferHdr);
        if(pInBufferHdr == NULL)
            return OMX_ErrorUnderflow;
    }

    if(pInBufferHdr->nFilledLen <= pOutBufferHdr->nAllocLen) {
        fsl_osal_memcpy(pOutBufferHdr->pBuffer, 
                pInBufferHdr->pBuffer + pInBufferHdr->nOffset, 
                pInBufferHdr->nFilledLen);
        pOutBufferHdr->nFilledLen = pInBufferHdr->nFilledLen;
        pOutBufferHdr->nOffset = 0;
        if(pInBufferHdr->nFlags & OMX_BUFFERFLAG_EOS)
            pOutBufferHdr->nFlags = OMX_BUFFERFLAG_EOS;

        pInBufferHdr->nFilledLen = 0;
        ports[IN_PORT]->SendBuffer(pInBufferHdr);
        pInBufferHdr = NULL;
    }
    else {
        fsl_osal_memcpy(pOutBufferHdr->pBuffer, 
                pInBufferHdr->pBuffer + pInBufferHdr->nOffset, 
                pOutBufferHdr->nAllocLen);
        pInBufferHdr->nOffset += pOutBufferHdr->nAllocLen;
        pInBufferHdr->nFilledLen -= pOutBufferHdr->nAllocLen;
        pOutBufferHdr->nFilledLen = pOutBufferHdr->nAllocLen;
        pOutBufferHdr->nOffset = 0;
    }

    ports[OUT_PORT]->SendBuffer(pOutBufferHdr);
    pOutBufferHdr = NULL;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Template::ComponentReturnBuffer(
        OMX_U32 nPortIndex)
{
    if(nPortIndex == IN_PORT && pInBufferHdr != NULL) {
        ports[IN_PORT]->SendBuffer(pInBufferHdr);
        pInBufferHdr = NULL;
    }

    if(nPortIndex == OUT_PORT && pOutBufferHdr != NULL) {
        ports[OUT_PORT]->SendBuffer(pOutBufferHdr);
        pOutBufferHdr = NULL;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE Template::FlushComponent(
        OMX_U32 nPortIndex)
{
    if(nPortIndex == IN_PORT && pInBufferHdr != NULL) {
        ports[IN_PORT]->SendBuffer(pInBufferHdr);
        pInBufferHdr = NULL;
    }

    if(nPortIndex == OUT_PORT && pOutBufferHdr != NULL) {
        ports[OUT_PORT]->SendBuffer(pOutBufferHdr);
        pOutBufferHdr = NULL;
    }

    return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE TemplateInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        Template *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(Template, ());
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
