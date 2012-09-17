/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "State.h"
#include <string.h>

State::State(ComponentBase *pBase) : base(pBase)
{
    if(base->bInContext == OMX_TRUE)
        Process = &ComponentBase::ProcessInContext;
    else
        Process = &ComponentBase::ProcessOutContext;
}

OMX_ERRORTYPE State::ProcessCmd() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    CMD_MSG sMsg;
    OMX_U32 i;

    if(base->pCmdQueue->Size() == 0)
        return OMX_ErrorNoMore;

    base->pCmdQueue->Get(&sMsg);

    LOG_DEBUG("Processing Command[cmd:%d, param: %d, data: %d] in Component[%s].\n",
            sMsg.Cmd, sMsg.nParam, sMsg.pCmdData, base->name);

    switch (sMsg.Cmd)
    {
        case OMX_CommandStateSet:
            switch (sMsg.nParam)
            {
                case OMX_StateInvalid:
                    ret = ToInvalid();
                    break;
                case OMX_StateLoaded:
                    ret = ToLoaded();
                    break;
                case OMX_StateIdle:
                    ret = ToIdle();
                    break;
                case OMX_StateExecuting:
                    ret = ToExecuting();
                    break;
                case OMX_StatePause:
                    ret = ToPause();
                    break;
                case OMX_StateWaitForResources:
                    ret = ToWaitForResources();
                    break;
                default:
                    LOG_ERROR("Invalid state transation.\n");
                    ret = OMX_ErrorBadParameter;
                    break;
            }
            break;
        case OMX_CommandFlush:
            if(sMsg.nParam == OMX_ALL) {
                for( i=0 ; i<base->nPorts ; i++ ) {
                    ret = base->ports[i]->Flush();
                    if(ret != OMX_ErrorNone)
                        break;
                }
            }
            else {
                if(sMsg.nParam >= base->nPorts) {
                    LOG_ERROR("Port number[%d] is not correct for flush command.\n", sMsg.nParam);
                    ret = OMX_ErrorBadParameter;
                    break;
                }
                ret = base->ports[sMsg.nParam]->Flush();
            }
            break;
        case OMX_CommandPortDisable:
            if(sMsg.nParam == OMX_ALL) {
                for( i=0 ; i<base->nPorts ; i++ ) {
                    ret = base->ports[i]->Disable();
                }
            }
            else {
                if(sMsg.nParam >= base->nPorts) {
                    LOG_ERROR("Port number[%d] is not correct for PortDisable command.\n", sMsg.nParam);
                    ret = OMX_ErrorBadParameter;
                    break;
                }
                ret = base->ports[sMsg.nParam]->Disable();
            }
            break;
        case OMX_CommandPortEnable:
            if(sMsg.nParam == OMX_ALL) {
                for( i=0 ; i<base->nPorts ; i++ ) {
                    ret = base->ports[i]->Enable();
                }
            }
            else {
                if(sMsg.nParam >= base->nPorts) {
                    LOG_ERROR("Port number[%d] is not correct for PortEnable command.\n", sMsg.nParam);
                    ret = OMX_ErrorBadParameter;
                    break;
                }
                ret = base->ports[sMsg.nParam]->Enable();
            }
            break;
        case OMX_CommandMarkBuffer:
            if(sMsg.nParam >= base->nPorts) {
                LOG_ERROR("Port number[%d] is not correct for MarkBuffer command.\n", sMsg.nParam);
                ret = OMX_ErrorBadParameter;
                break;
            }

            LOG_DEBUG("%s,%d,mark buffer on port %d.\n",__FUNCTION__,__LINE__ ,sMsg.nParam);
            ret = base->ports[sMsg.nParam]->MarkBuffer((OMX_MARKTYPE*)(sMsg.pCmdData));
            break;
        default:
            LOG_ERROR("Invalid command.\n");
            ret = OMX_ErrorBadParameter;
            break;
    }

    base->pendingCmd.bProcessed = OMX_TRUE;

    if(ret == OMX_ErrorNone) {
        LOG_DEBUG("Process Command [cmd:%d, param: %d, data: %d] finished.\n",
                sMsg.Cmd, sMsg.nParam, sMsg.pCmdData);
        base->SendEvent(OMX_EventCmdComplete, sMsg.Cmd, sMsg.nParam, sMsg.pCmdData);
    }
    else if(ret == OMX_ErrorNotComplete)
        ret = OMX_ErrorNone;
    else {
        LOG_DEBUG("%s Process Command [cmd:%d, param: %d, data: %d] failed, ret = %x.\n",
                base->name, sMsg.Cmd, sMsg.nParam, sMsg.pCmdData, ret);
        base->pendingCmd.cmd = OMX_CommandMax;
        base->SendEvent(OMX_EventError, ret, sMsg.Cmd, NULL);
    }

    return ret;
}

OMX_ERRORTYPE State::DoGetComponentVersion(
        OMX_STRING pComponentName, 
        OMX_VERSIONTYPE* pComponentVersion, 
        OMX_VERSIONTYPE* pSpecVersion, 
        OMX_UUIDTYPE* pComponentUUID) 
{
    if(pComponentName == NULL || pComponentVersion == NULL
            || pSpecVersion == NULL || pComponentUUID == NULL)
        return OMX_ErrorBadParameter;

    fsl_osal_strcpy(pComponentName, (OMX_STRING)(base->name));
    fsl_osal_memcpy(pComponentVersion, &(base->ComponentVersion), sizeof(OMX_VERSIONTYPE));
    fsl_osal_memcpy(pSpecVersion, &(base->SpecVersion), sizeof(OMX_VERSIONTYPE));

    return OMX_ErrorNone;
}

OMX_ERRORTYPE State::DoSendCommand(
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam, 
        OMX_PTR pCmdData) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    CMD_MSG sMsg;

    /* reset Cmd pool if pendingCmd is dead */
    if(Cmd == OMX_CommandMax) {
        base->pendingCmd.cmd = OMX_CommandMax;
        return OMX_ErrorNone;
    }

    if(base->pendingCmd.cmd != OMX_CommandMax)
        return OMX_ErrorNotReady;

    LOG_DEBUG("Sending Command[cmd:%d, param: %d, data: %d] to Component[%s].\n",
            Cmd, nParam, pCmdData, base->name);

    ret = CheckCommand(Cmd, nParam, pCmdData);
    if(ret != OMX_ErrorNone)
        return ret;

    base->pendingCmd.cmd = Cmd;
    base->pendingCmd.nParam = nParam;
    base->pendingCmd.pCmdData = pCmdData;
    base->pendingCmd.bProcessed = OMX_FALSE;

    sMsg.Cmd = Cmd;
    sMsg.nParam = nParam;
    sMsg.pCmdData = pCmdData;
    base->pCmdQueue->Add(&sMsg);

    LOG_DEBUG("Sending Command[cmd:%d, param: %d, data: %d] to Component[%s] done.\n",
            Cmd, nParam, pCmdData, base->name);

    ret = (base->*Process)(COMMAND);


    return ret;
}

OMX_ERRORTYPE State::DoEmptyThisBuffer(
        OMX_BUFFERHEADERTYPE* pBuffer) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nPortIndex;

    LOG_DEBUG("%s EmptyBuffer %p.\n", base->name, pBuffer);

    if(pBuffer == NULL)
        return OMX_ErrorBadParameter;

    /**< check buffer structure version*/
    OMX_CHECK_STRUCT(pBuffer, OMX_BUFFERHEADERTYPE, ret);
    if(ret != OMX_ErrorNone)
        return ret;

    nPortIndex = pBuffer->nInputPortIndex;
    if(nPortIndex >= base->nPorts
            || base->ports[nPortIndex]->GetPortDir() != OMX_DirInput)
        return OMX_ErrorBadPortIndex;

    if(base->ports[nPortIndex]->IsEnabled() != OMX_TRUE) {
        LOG_WARNING("%s Port %d is not populated.\n", base->name, nPortIndex);
        return OMX_ErrorPortUnpopulated;
    }

    LOG_DEBUG("%s Sending Input buffer[%p]\n", base->name, pBuffer);
    if (NULL != pBuffer->hMarkTargetComponent)
    {
        OMX_MARKTYPE sMarkData;
        sMarkData.hMarkTargetComponent = pBuffer->hMarkTargetComponent;
        sMarkData.pMarkData = pBuffer->pMarkData;
        if(pBuffer->hMarkTargetComponent != base->GetComponentHandle()) {
            base->MarkOtherPortsBuffer(&sMarkData);
            LOG_DEBUG("%s,%d,in component %s relay mark target %x to output port.\n",__FUNCTION__,__LINE__,base->name,sMarkData.hMarkTargetComponent);
        }
        else {
            base->ports[nPortIndex]->MarkBuffer(&sMarkData);
            LOG_DEBUG("%s,%d,in component %s mark target %x to input port.\n",__FUNCTION__,__LINE__,base->name,pBuffer->hMarkTargetComponent);
        }
    }

    base->ports[nPortIndex]->AddBuffer(pBuffer);
    if(base->ports[nPortIndex]->NeedReturnBuffers() == OMX_TRUE)
        return ret;

    ret = (base->*Process)(BUFFER);

    return ret;
}

OMX_ERRORTYPE State::DoFillThisBuffer(
        OMX_BUFFERHEADERTYPE* pBuffer) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nPortIndex;

    LOG_DEBUG("%s FillBuffer %p.\n", base->name, pBuffer);

    if(pBuffer == NULL)
        return OMX_ErrorBadParameter;

    /**< check buffer structure version*/
    OMX_CHECK_STRUCT(pBuffer, OMX_BUFFERHEADERTYPE, ret);
    if(ret != OMX_ErrorNone)
        return ret;

    nPortIndex = pBuffer->nOutputPortIndex;
    if(nPortIndex >= base->nPorts
            || base->ports[nPortIndex]->GetPortDir() != OMX_DirOutput)
        return OMX_ErrorBadPortIndex;

    if(base->ports[nPortIndex]->IsEnabled() != OMX_TRUE) {
        LOG_WARNING("%s Port %d is not populated.\n", base->name, nPortIndex);
        return OMX_ErrorPortUnpopulated;
    }


    LOG_DEBUG("%s Sending Output buffer[%p]\n", base->name, pBuffer);
    base->ports[nPortIndex]->AddBuffer(pBuffer);
    if(base->ports[nPortIndex]->NeedReturnBuffers() == OMX_TRUE)
        return ret;
        
    ret = (base->*Process)(BUFFER);

    return ret;
}

OMX_ERRORTYPE State::DoGetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(pStructure == NULL)
        return OMX_ErrorBadParameter;

    switch(nParamIndex) {
        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;

                pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPortDef, OMX_PARAM_PORTDEFINITIONTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;

                if(pPortDef->nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                ret = base->ports[pPortDef->nPortIndex]->GetPortDefinition(pPortDef);
            }
            break;
        case OMX_IndexParamCompBufferSupplier:
            {
                OMX_PARAM_BUFFERSUPPLIERTYPE *pBufferSupplierType;
                OMX_U32 nPortIndex;

                pBufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE*) pStructure;
                OMX_CHECK_STRUCT(pBufferSupplierType, OMX_PARAM_BUFFERSUPPLIERTYPE, ret);
                if(ret != OMX_ErrorNone)
                    return ret;

                nPortIndex = pBufferSupplierType->nPortIndex;
                if(nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                ret = base->ports[nPortIndex]->GetSupplierType(&(pBufferSupplierType->eBufferSupplier));
            }
            break;
       case OMX_IndexParamAudioInit:
            {
                OMX_PORT_PARAM_TYPE *pPortPara;

                pPortPara = (OMX_PORT_PARAM_TYPE*) pStructure;
                OMX_CHECK_STRUCT(pPortPara, OMX_PORT_PARAM_TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;

                pPortPara->nPorts = base->PortParam[AudioPortParamIdx].nPorts;
                pPortPara->nStartPortNumber = base->PortParam[AudioPortParamIdx].nStartPortNumber;
            }
            break;
        case OMX_IndexParamVideoInit:
            {
                OMX_PORT_PARAM_TYPE *pPortPara;

                pPortPara = (OMX_PORT_PARAM_TYPE*) pStructure;
                OMX_CHECK_STRUCT(pPortPara, OMX_PORT_PARAM_TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;

                pPortPara->nPorts = base->PortParam[VideoPortParamIdx].nPorts;
                pPortPara->nStartPortNumber = base->PortParam[VideoPortParamIdx].nStartPortNumber;
            }
            break;
        case OMX_IndexParamImageInit:
            {
                OMX_PORT_PARAM_TYPE *pPortPara;

                pPortPara = (OMX_PORT_PARAM_TYPE*) pStructure;
                OMX_CHECK_STRUCT(pPortPara, OMX_PORT_PARAM_TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;

                pPortPara->nPorts = base->PortParam[ImagePortParamIdx].nPorts;
                pPortPara->nStartPortNumber = base->PortParam[ImagePortParamIdx].nStartPortNumber;
            }
            break;
        case OMX_IndexParamOtherInit:
            {
                OMX_PORT_PARAM_TYPE *pPortPara;

                pPortPara = (OMX_PORT_PARAM_TYPE*) pStructure;
                OMX_CHECK_STRUCT(pPortPara, OMX_PORT_PARAM_TYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;

                pPortPara->nPorts = base->PortParam[OtherPortParamIdx].nPorts;
                pPortPara->nStartPortNumber = base->PortParam[OtherPortParamIdx].nStartPortNumber;
            }
            break;
        case OMX_IndexParamAudioPortFormat:
            {
                OMX_AUDIO_PARAM_PORTFORMATTYPE *pPortFmt;
                OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
                OMX_U32 nParamIndex;

                pPortFmt = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPortFmt, OMX_AUDIO_PARAM_PORTFORMATTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                if(pPortFmt->nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
                sPortDef.nPortIndex = pPortFmt->nPortIndex;
                base->ports[pPortFmt->nPortIndex]->GetPortDefinition(&sPortDef);
                if(sPortDef.eDomain != OMX_PortDomainAudio)
                    return OMX_ErrorBadPortIndex;
                pPortFmt->eEncoding = sPortDef.format.audio.eEncoding;
            }
            break;
        case OMX_IndexParamVideoPortFormat:
            {
                OMX_VIDEO_PARAM_PORTFORMATTYPE *pPortFmt;
                OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
                OMX_U32 nParamIndex;

                pPortFmt = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPortFmt, OMX_VIDEO_PARAM_PORTFORMATTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                if(pPortFmt->nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;
				if (pPortFmt->nIndex > 0) 
					return OMX_ErrorNoMore;

                OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
                sPortDef.nPortIndex = pPortFmt->nPortIndex;
                base->ports[pPortFmt->nPortIndex]->GetPortDefinition(&sPortDef);
                if(sPortDef.eDomain != OMX_PortDomainVideo)
                    return OMX_ErrorBadPortIndex;
                pPortFmt->eCompressionFormat = sPortDef.format.video.eCompressionFormat;
                pPortFmt->eColorFormat = sPortDef.format.video.eColorFormat;
                pPortFmt->xFramerate = sPortDef.format.video.xFramerate;
            }
            break;
        case OMX_IndexParamImagePortFormat:
            {
                OMX_IMAGE_PARAM_PORTFORMATTYPE *pPortFmt;
                OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
                OMX_U32 nParamIndex;

                pPortFmt = (OMX_IMAGE_PARAM_PORTFORMATTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPortFmt, OMX_IMAGE_PARAM_PORTFORMATTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                if(pPortFmt->nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
                sPortDef.nPortIndex = pPortFmt->nPortIndex;
                base->ports[pPortFmt->nPortIndex]->GetPortDefinition(&sPortDef);
                if(sPortDef.eDomain != OMX_PortDomainImage)
                    return OMX_ErrorBadPortIndex;
                pPortFmt->eCompressionFormat = sPortDef.format.image.eCompressionFormat;
                pPortFmt->eColorFormat = sPortDef.format.image.eColorFormat;
            }
            break;
        case OMX_IndexParamOtherPortFormat:
            {
                OMX_OTHER_PARAM_PORTFORMATTYPE *pPortFmt;
                OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
                OMX_U32 nParamIndex;

                pPortFmt = (OMX_OTHER_PARAM_PORTFORMATTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPortFmt, OMX_OTHER_PARAM_PORTFORMATTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                if(pPortFmt->nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
                sPortDef.nPortIndex = pPortFmt->nPortIndex;
                base->ports[pPortFmt->nPortIndex]->GetPortDefinition(&sPortDef);
                if(sPortDef.eDomain != OMX_PortDomainOther)
                    return OMX_ErrorBadPortIndex;
                pPortFmt->eFormat = sPortDef.format.other.eFormat;
            }
            break;
        default:
            ret = base->GetParameter(nParamIndex, pStructure);
            break;
    }

    return ret;
}

OMX_ERRORTYPE State::DoSetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(pStructure == NULL)
        return OMX_ErrorBadParameter;

    switch(nParamIndex) {
        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;

                pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPortDef, OMX_PARAM_PORTDEFINITIONTYPE, ret);
                if(ret != OMX_ErrorNone)
                    return ret;
                if(pPortDef->nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                ret = base->ports[pPortDef->nPortIndex]->SetPortDefinition(pPortDef);
                if(ret == OMX_ErrorNone)
                    base->PortFormatChanged(pPortDef->nPortIndex);
            }
            break;
        case OMX_IndexParamCompBufferSupplier:
            {
                OMX_PARAM_BUFFERSUPPLIERTYPE *pBufferSupplierType;
                OMX_U32 nPortIndex;

                pBufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE*) pStructure;
                OMX_CHECK_STRUCT(pBufferSupplierType, OMX_PARAM_BUFFERSUPPLIERTYPE, ret);
                if(ret != OMX_ErrorNone)
                    return ret;

                nPortIndex = pBufferSupplierType->nPortIndex;
                if(nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                base->ports[nPortIndex]->SetSupplierType(pBufferSupplierType->eBufferSupplier);
            }
            break;
        case OMX_IndexParamAudioPortFormat:
            {
                OMX_AUDIO_PARAM_PORTFORMATTYPE *pPortFmt;
                OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
                OMX_U32 nParamIndex;

                pPortFmt = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPortFmt, OMX_AUDIO_PARAM_PORTFORMATTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                if(pPortFmt->nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
                sPortDef.nPortIndex = pPortFmt->nPortIndex;
                base->ports[pPortFmt->nPortIndex]->GetPortDefinition(&sPortDef);
                if(sPortDef.eDomain != OMX_PortDomainAudio)
                    return OMX_ErrorBadPortIndex;
                sPortDef.format.audio.eEncoding = pPortFmt->eEncoding;
                base->ports[pPortFmt->nPortIndex]->SetPortDefinition(&sPortDef);
            }
            break;
        case OMX_IndexParamVideoPortFormat:
            {
                OMX_VIDEO_PARAM_PORTFORMATTYPE *pPortFmt;
                OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
                OMX_U32 nParamIndex;

                pPortFmt = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPortFmt, OMX_VIDEO_PARAM_PORTFORMATTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                if(pPortFmt->nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
                sPortDef.nPortIndex = pPortFmt->nPortIndex;
                base->ports[pPortFmt->nPortIndex]->GetPortDefinition(&sPortDef);
                if(sPortDef.eDomain != OMX_PortDomainVideo)
                    return OMX_ErrorBadPortIndex;
                sPortDef.format.video.eCompressionFormat = pPortFmt->eCompressionFormat;
                sPortDef.format.video.eColorFormat = pPortFmt->eColorFormat;
                sPortDef.format.video.xFramerate = pPortFmt->xFramerate;
                base->ports[pPortFmt->nPortIndex]->SetPortDefinition(&sPortDef);
            }
            break;
        case OMX_IndexParamImagePortFormat:
            {
                OMX_IMAGE_PARAM_PORTFORMATTYPE *pPortFmt;
                OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
                OMX_U32 nParamIndex;

                pPortFmt = (OMX_IMAGE_PARAM_PORTFORMATTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPortFmt, OMX_IMAGE_PARAM_PORTFORMATTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                if(pPortFmt->nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
                sPortDef.nPortIndex = pPortFmt->nPortIndex;
                base->ports[pPortFmt->nPortIndex]->GetPortDefinition(&sPortDef);
                if(sPortDef.eDomain != OMX_PortDomainImage)
                    return OMX_ErrorBadPortIndex;
                sPortDef.format.image.eCompressionFormat = pPortFmt->eCompressionFormat;
                sPortDef.format.image.eColorFormat = pPortFmt->eColorFormat;
                base->ports[pPortFmt->nPortIndex]->SetPortDefinition(&sPortDef);
            }
            break;
        case OMX_IndexParamOtherPortFormat:
            {
                OMX_OTHER_PARAM_PORTFORMATTYPE *pPortFmt;
                OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
                OMX_U32 nParamIndex;

                pPortFmt = (OMX_OTHER_PARAM_PORTFORMATTYPE*)pStructure;
                OMX_CHECK_STRUCT(pPortFmt, OMX_OTHER_PARAM_PORTFORMATTYPE, ret);
                if(ret != OMX_ErrorNone)
                    break;
                if(pPortFmt->nPortIndex >= base->nPorts)
                    return OMX_ErrorBadPortIndex;

                OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
                sPortDef.nPortIndex = pPortFmt->nPortIndex;
                base->ports[pPortFmt->nPortIndex]->GetPortDefinition(&sPortDef);
                if(sPortDef.eDomain != OMX_PortDomainOther)
                    return OMX_ErrorBadPortIndex;
                sPortDef.format.other.eFormat = pPortFmt->eFormat;
                base->ports[pPortFmt->nPortIndex]->SetPortDefinition(&sPortDef);
            }
            break;
        default:
            ret = base->SetParameter(nParamIndex, pStructure);
    }

    return ret;
}

OMX_ERRORTYPE State::DoGetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure) 
{
    if(pStructure == NULL)
        return OMX_ErrorBadParameter;

    return base->GetConfig(nParamIndex, pStructure);
}

OMX_ERRORTYPE State::DoSetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure) 
{
    if(pStructure == NULL)
        return OMX_ErrorBadParameter;

    return base->SetConfig(nParamIndex, pStructure);
}

OMX_ERRORTYPE State::DoGetExtensionIndex(
        OMX_STRING cParameterName, 
        OMX_INDEXTYPE* pIndexType) 
{
    return base->GetExtensionIndex(cParameterName, pIndexType);
}

OMX_ERRORTYPE State::DoUseBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes, 
        OMX_U8* pBuffer) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(pBuffer == NULL || nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    ret = base->ports[nPortIndex]->UseBuffer(
            ppBufferHdr, pAppPrivate, nSizeBytes, pBuffer);

    return ret;
}

OMX_ERRORTYPE State::DoAllocateBuffer(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        OMX_U32 nSizeBytes) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    ret = base->ports[nPortIndex]->AllocateBuffer(
            ppBufferHdr, pAppPrivate, nSizeBytes);

    return ret;
}

OMX_ERRORTYPE State::DoFreeBuffer(
        OMX_U32 nPortIndex, 
        OMX_BUFFERHEADERTYPE* pBufferHdr) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    ret = base->ports[nPortIndex]->FreeBuffer(pBufferHdr);

    return ret;
}

OMX_ERRORTYPE State::DoTunnelRequest(
        OMX_U32 nPortIndex, 
        OMX_HANDLETYPE hTunneledComp,
        OMX_U32 nTunneledPort, 
        OMX_TUNNELSETUPTYPE* pTunnelSetup) 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    ret = base->ports[nPortIndex]->TunnelRequest(
            hTunneledComp,nTunneledPort,pTunnelSetup);

    return ret;
}

OMX_ERRORTYPE State::DoUseEGLImage(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex,
        OMX_PTR pAppPrivate, 
        void* eglImage)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(nPortIndex >= base->nPorts)
        return OMX_ErrorBadParameter;

    ret = base->ports[nPortIndex]->UseEGLImage(
            ppBufferHdr, pAppPrivate, eglImage);

    return ret;
}

OMX_ERRORTYPE State::CheckCommand(
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam, 
        OMX_PTR pCmdData)
{
    OMX_STATETYPE eState = OMX_StateInvalid;
    OMX_U32 i;

    if(Cmd == OMX_CommandStateSet) {
        base->GetState(&eState);
        if((OMX_U32)eState == nParam)
            return OMX_ErrorSameState;
    }
    else if(Cmd == OMX_CommandPortDisable) {
        if(nParam == OMX_ALL) {
            OMX_BOOL bAllDisabled = OMX_TRUE;
            for(i=0; i<base->nPorts; i++) {
                if(base->ports[i]->IsEnabled() == OMX_TRUE) {
                    bAllDisabled = OMX_FALSE;
                    break;
                }
            }
            if(bAllDisabled == OMX_TRUE)
                return OMX_ErrorSameState;
        }
        else {
            if(nParam >= base->nPorts)
                return OMX_ErrorBadParameter;
            if(base->ports[nParam]->IsEnabled() != OMX_TRUE)
                return OMX_ErrorSameState;
        }
    }
    else if(Cmd == OMX_CommandPortEnable) {
        if(nParam == OMX_ALL) {
            OMX_BOOL bAllEnabled = OMX_TRUE;
            for(i=0; i<base->nPorts; i++) {
                if(base->ports[i]->IsEnabled() != OMX_TRUE) {
                    bAllEnabled = OMX_FALSE;
                    break;
                }
            }
            if(bAllEnabled == OMX_TRUE)
                return OMX_ErrorSameState;
        }
        else {
            if(nParam >= base->nPorts)
                return OMX_ErrorBadParameter;
            if(base->ports[nParam]->IsEnabled() == OMX_TRUE)
                return OMX_ErrorSameState;
        }
    }

    return OMX_ErrorNone;
}

/* File EOF */
