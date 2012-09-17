/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
#include "GMComponent.h"

static OMX_ERRORTYPE GMEventHandler(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    GMComponent *pComponent = NULL;

    pComponent = (GMComponent*)pAppData;
    pComponent->CbEventHandler(eEvent, nData1, nData2, pEventData);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE GMEmptyBufferDone(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    GMComponent *pComponent = NULL;

    pComponent = (GMComponent*)pAppData;
    pComponent->CbEmptyBufferDone(pBuffer);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE GMFillBufferDone(
        OMX_OUT OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_PTR pAppData,
        OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer)
{
    GMComponent *pComponent = NULL;

    pComponent = (GMComponent*)pAppData;
    pComponent->CbFillBufferDone(pBuffer);

    return OMX_ErrorNone;
}

static OMX_CALLBACKTYPE GMCallBacks = {
    GMEventHandler,
    GMEmptyBufferDone,
    GMFillBufferDone
};

static OMX_BOOL IntersectPortDef(
        OMX_PARAM_PORTDEFINITIONTYPE *pDef1,
        OMX_PARAM_PORTDEFINITIONTYPE *pDef2)
{
    OMX_U32 nBufferCount, nBufferSize, nBufferAlignment;

    if(pDef1->eDomain != pDef2->eDomain)
        return OMX_FALSE;
    if(pDef1->eDomain == OMX_PortDomainAudio)
        if(pDef1->format.audio.eEncoding != pDef2->format.audio.eEncoding)
            return OMX_FALSE;
    if(pDef1->eDomain == OMX_PortDomainVideo)
        if(pDef1->format.video.eCompressionFormat != pDef2->format.video.eCompressionFormat)
            return OMX_FALSE;
    if(pDef1->eDomain == OMX_PortDomainImage)
        if(pDef1->format.image.eCompressionFormat != pDef2->format.image.eCompressionFormat)
            return OMX_FALSE;

    nBufferCount = MAX(pDef1->nBufferCountActual, pDef2->nBufferCountActual);
    pDef1->nBufferCountActual = pDef2->nBufferCountActual = nBufferCount;
    nBufferSize = MAX(pDef1->nBufferSize, pDef2->nBufferSize);
    pDef1->nBufferSize = pDef2->nBufferSize = nBufferSize;
    nBufferAlignment = MAX(pDef1->nBufferAlignment, pDef2->nBufferAlignment);
    pDef1->nBufferAlignment = pDef2->nBufferAlignment = nBufferAlignment;

    return OMX_TRUE;
}


OMX_ERRORTYPE GMComponent::Load(
        OMX_STRING role)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_U32 NumComps = 0;
    OMX_U8** compNames = NULL;
    OMX_U8 * pCompName = NULL;;
    OMX_BOOL bLoaded = OMX_FALSE;
    OMX_U32 i;

    ret = OMX_GetComponentsOfRole(role, &NumComps, NULL);
    if(ret != OMX_ErrorNone)
        return ret;

    if(NumComps == 0)
        return OMX_ErrorInvalidComponentName;

    compNames = (OMX_U8**)FSL_MALLOC(NumComps * sizeof(OMX_U8*));
    if(compNames == NULL)
        return OMX_ErrorInsufficientResources;

    pCompName = (OMX_U8*)FSL_MALLOC(NumComps * COMPONENT_NAME_LEN);
    if(pCompName == NULL)
        return OMX_ErrorInsufficientResources;

    for(i=0; i<NumComps; i++) {
        compNames[i]= pCompName + i * COMPONENT_NAME_LEN;
    }
    
    ret = OMX_GetComponentsOfRole(role, &NumComps, (OMX_U8**)compNames);
    if(ret != OMX_ErrorNone) {
        FSL_FREE(pCompName);
        FSL_FREE(compNames);
        return ret;
    }

    i = 0;
    if(!fsl_osal_strcmp(role, "video_decoder.avc")) {
        for(i=0; i<NumComps; i++) {
            if(fsl_osal_strstr((OMX_STRING)compNames[i], "v3"))
                break;
        }

        if(i == NumComps)
            i = 0;
    }

    for(; i<NumComps; i++) {
        ret = OMX_GetHandle(&hComponent, (OMX_STRING)compNames[i], (OMX_PTR)this, &GMCallBacks);
        if(ret == OMX_ErrorNone) {
            bLoaded = OMX_TRUE;
            fsl_osal_strcpy((fsl_osal_char*)name,(fsl_osal_char*)compNames[i]);       
            LOG_INFO("Component %s is loaded.\n", compNames[i]);
            break;
        }
    }

    FSL_FREE(pCompName);
    FSL_FREE(compNames);
    
    if(bLoaded != OMX_TRUE) {
        LOG_ERROR("Can't load component which role is %s\n", role);
        return OMX_ErrorInvalidComponentName;
    }

    OMX_PARAM_COMPONENTROLETYPE CurRole;
    OMX_INIT_STRUCT(&CurRole, OMX_PARAM_COMPONENTROLETYPE);
    fsl_osal_memcpy(&CurRole.cRole, role, OMX_MAX_STRINGNAME_SIZE);
    OMX_SetParameter(hComponent, OMX_IndexParamStandardComponentRole, &CurRole);

    GetComponentPortsPara();
    if(nPorts == 0) {
        UnLoad();
        return OMX_ErrorInvalidComponent;
    }

    ProcessingCmd.Cmd = OMX_CommandMax;
    DoneCmd.Cmd = OMX_CommandMax;
    bError = OMX_FALSE;
    bResetTimeout = OMX_FALSE;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    fsl_osal_memset(PortsInfo, 0, sizeof(GM_PORTINFO)*MAX_PORT_NUM);
    for(i=0; i<nPorts; i++) {
        sPortDef.nPortIndex = i;
        OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
        PortsInfo[i].bEnabled = sPortDef.bEnabled;
        PortsInfo[i].eDir = sPortDef.eDir;
        if(PortsInfo[i].eDir == OMX_DirOutput)
            PortsInfo[i].Allocater = GM_SELF_ALLOC;
        else
            PortsInfo[i].Allocater = GM_PEER_ALLOC;
        PortsInfo[i].eDomain = sPortDef.eDomain;
        if(PortsInfo[i].eDomain != OMX_PortDomainAudio
                && PortsInfo[i].eDomain != OMX_PortDomainVideo)
            PortDisable(i);

        PortsInfo[i].nBuffers = sPortDef.nBufferCountActual;
        PortsInfo[i].nBufferSize = sPortDef.nBufferSize;
        fsl_osal_mutex_init(&PortsInfo[i].HoldBufferListLock, fsl_osal_mutex_normal);
    }


    SysBufferHandler = NULL;
    SysBufferAppData = NULL;
    pBufferCallback = NULL;
    pBufAppData = NULL;

    return ret;
}

OMX_ERRORTYPE GMComponent::UnLoad()
{
    OMX_U32 i;

    OMX_FreeHandle(hComponent);

	for(i=0; i<nPorts; i++) {
		if(PortsInfo[i].HoldBufferListLock != NULL)
			fsl_osal_mutex_destroy(PortsInfo[i].HoldBufferListLock);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::Link(
        OMX_U32 nPortIndex, 
        GMComponent *pPeer, 
        OMX_U32 nPeerPortIndex)
{
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef1, sPortDef2;

    OMX_INIT_STRUCT(&sPortDef1, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef1.nPortIndex = nPortIndex;
    OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef1);

    OMX_INIT_STRUCT(&sPortDef2, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef2.nPortIndex = nPeerPortIndex;
    OMX_GetParameter(pPeer->hComponent, OMX_IndexParamPortDefinition, &sPortDef2);

    if(OMX_TRUE != IntersectPortDef(&sPortDef1, &sPortDef2))
        return OMX_ErrorInvalidComponent;

    OMX_SetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef1);
    sPortDef2.format = sPortDef1.format;
    OMX_SetParameter(pPeer->hComponent, OMX_IndexParamPortDefinition, &sPortDef2);

    PortsInfo[nPortIndex].peer.pComponent = pPeer;
    PortsInfo[nPortIndex].peer.nPortIndex = nPeerPortIndex;
    PortsInfo[nPortIndex].nBuffers = sPortDef1.nBufferCountActual;
    PortsInfo[nPortIndex].nBufferSize = sPortDef1.nBufferSize;
    PortsInfo[nPortIndex].bSettingChanged = OMX_FALSE;
    PortsInfo[nPortIndex].bPortResourceChanged = OMX_FALSE;
    PortsInfo[nPortIndex].bOutputed = OMX_FALSE;
    PortsInfo[nPortIndex].bHoldBuffers = OMX_FALSE;
    PortsInfo[nPortIndex].bFreeBuffers = OMX_FALSE;
    PortsInfo[nPortIndex].nBufferHolded = 0;

    if(PortsInfo[nPortIndex].eDir == OMX_DirOutput)
        SetPeerPortMediaFormat(nPortIndex);

    LOG_DEBUG("Link result: [%s:%d:%d].\n", name, sPortDef1.nBufferCountActual, sPortDef1.nBufferSize);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::UnLink(
        OMX_U32 nPortIndex)
{
    PortsInfo[nPortIndex].peer.pComponent = NULL;
    PortsInfo[nPortIndex].peer.nPortIndex = 0;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::StateTransUpWard(
        OMX_STATETYPE eState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_STATETYPE eCurState = OMX_StateInvalid;

    OMX_GetState(hComponent, &eCurState);

    if(eState == eCurState) 
        return OMX_ErrorNone;

    /* Loaded->Idle */
    if(eCurState == OMX_StateLoaded && eState == OMX_StateIdle)
        ret = DoLoaded2Idle();
    /* Idle->Exec/Idle->Pause done */
	else if(eCurState == OMX_StateIdle && (eState == OMX_StateExecuting || eState == OMX_StatePause)) {
		struct timeval tv, tv1;
		gettimeofday (&tv, NULL);

		/* Component may cost more time when idle->exec */
		ret = DoStateTrans(eState, 7*OMX_TICKS_PER_SECOND);

		gettimeofday (&tv1, NULL);
		LOG_DEBUG("Idle to executing Time: %d\n", (tv1.tv_sec-tv.tv_sec)*1000+(tv1.tv_usec-tv.tv_usec)/1000);
	}
	/* Pause->Exec */
	else if(eCurState == OMX_StatePause && eState == OMX_StateExecuting)
        ret = DoStateTrans(eState, 2*OMX_TICKS_PER_SECOND);
    else {
        LOG_ERROR("%s Invalid state transitioin: %d -> %d.\n", name, eCurState, eState);
        ret = OMX_ErrorIncorrectStateTransition;
    }

    return ret;
}

OMX_ERRORTYPE GMComponent::StateTransDownWard(
        OMX_STATETYPE eState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_STATETYPE eCurState = OMX_StateInvalid;

    OMX_GetState(hComponent, &eCurState);

    if(eState == eCurState)
        return OMX_ErrorNone;

    /* Idle->Loaded */
    if(eCurState == OMX_StateIdle && eState == OMX_StateLoaded)
        ret = DoIdle2Loaded();
    /* Exec->Idle/Pause->Idle */
    else if((eCurState == OMX_StateExecuting || eCurState == OMX_StatePause) && eState == OMX_StateIdle)
        ret = DoExec2Idle(eState);
    /* Exec->Pause */
    else if(eCurState == OMX_StateExecuting && eState == OMX_StatePause)
        ret = DoStateTrans(eState, 2*OMX_TICKS_PER_SECOND);
    else {
        LOG_ERROR("%s Invalid state transitioin: %d -> %d.\n", name, eCurState, eState);
        ret = OMX_ErrorIncorrectStateTransition;
    }

    return ret;
}

OMX_BOOL GMComponent::IsPortEnabled(
		OMX_U32 nPortIndex)
{
	return PortsInfo[nPortIndex].bEnabled;
}

OMX_ERRORTYPE GMComponent::WaitAndFreePortBuffer(
		OMX_U32 nPortIndex)

{
	OMX_STATETYPE eState = OMX_StateInvalid;
	OMX_STATETYPE eStatePeer = OMX_StateInvalid;
	OMX_U32 nCount = 0;
    GMComponent *peer = NULL;
    OMX_U32 nPeerPortIndex;

    peer = PortsInfo[nPortIndex].peer.pComponent;
    nPeerPortIndex = PortsInfo[nPortIndex].peer.nPortIndex;
 
	while (1) {
		LOG_DEBUG("Component: %s on port: %d, hold buffer number: %d\n", name, nPortIndex, \
				PortsInfo[nPortIndex].nBufferHolded);
		OMX_GetState(hComponent, &eState);
		if (peer != NULL) {
			if (peer->hComponent != NULL) {
				OMX_GetState(peer->hComponent, &eStatePeer);
			}
		}
		if (eState != OMX_StateExecuting \
				|| peer == NULL \
				|| peer->IsPortEnabled(nPeerPortIndex) != OMX_TRUE \
				|| eStatePeer != OMX_StateExecuting \
				|| PortsInfo[nPortIndex].nBufferHolded == PortsInfo[nPortIndex].nBuffers) {
			LOG_DEBUG("Free component: %s on port: %d\n", name, nPortIndex);
			PortFreeBuffers(nPortIndex);
			break;
		}

		fsl_osal_sleep(5000);
		if(nCount ++ > 1000) {
			LOG_ERROR("Wait hold buffer timeout when port disable.\n");
			break;
		}
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::PortDisable(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(PortsInfo[nPortIndex].bEnabled != OMX_TRUE)
        return OMX_ErrorNone;

    PortsInfo[nPortIndex].bHoldBuffers = OMX_TRUE;
    PortsInfo[nPortIndex].bFreeBuffers = OMX_TRUE;
    PortsInfo[nPortIndex].nBufferHolded = 0;

    ret = SendCommand(OMX_CommandPortDisable, nPortIndex, NULL);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Disable port %d failed.\n", nPortIndex);
        return ret;
    }

    OMX_STATETYPE eState = OMX_StateInvalid;

    OMX_GetState(hComponent, &eState);

	if(!(eState == OMX_StateLoaded)) {
		if(PortsInfo[nPortIndex].Allocater == GM_SELF_ALLOC)
			WaitAndFreePortBuffer(nPortIndex);
		else
			PortFreeBuffers(nPortIndex);
	}

    ret = WaitCommand(OMX_CommandPortDisable, nPortIndex, NULL, OMX_TICKS_PER_SECOND);
    if(ret != OMX_ErrorNone)
        return ret;

    PortsInfo[nPortIndex].bEnabled = OMX_FALSE;
    PortsInfo[nPortIndex].bHoldBuffers = OMX_FALSE;
    PortsInfo[nPortIndex].bFreeBuffers = OMX_FALSE;
    PortsInfo[nPortIndex].nBufferHolded = 0;

	/* Clear hold buffer list */
    OMX_BUFFERHEADERTYPE *pBuffer;
    while(PortsInfo[nPortIndex].HoldBufferList.GetNodeCnt() > 0) {
        pBuffer = NULL;
        pBuffer = PortsInfo[nPortIndex].HoldBufferList.GetNode(0);
        LOG_DEBUG("%s:%d get buffer %p from hold buffer list.\n", name, nPortIndex, pBuffer);
        if(pBuffer == NULL) {
            LOG_ERROR("%s get null buffer from holding list.\n", name);
            continue;
        }
        PortsInfo[nPortIndex].HoldBufferList.Remove(pBuffer);
   }
 
    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::PortEnable(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(PortsInfo[nPortIndex].bEnabled != OMX_FALSE)
        return OMX_ErrorNone;

    ret = SendCommand(OMX_CommandPortEnable, nPortIndex, NULL);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Enable port %d failed.\n", nPortIndex);
        return ret;
    }

    OMX_STATETYPE eState = OMX_StateInvalid;

    OMX_GetState(hComponent, &eState);

    if(!(eState == OMX_StateLoaded)) {
        if(PortsInfo[nPortIndex].Allocater == GM_SELF_ALLOC)
            ret = PortAllocateBuffers(nPortIndex);
        if(PortsInfo[nPortIndex].Allocater == GM_PEER_ALLOC)
            ret = PortUsePeerBuffers(nPortIndex);
        if(PortsInfo[nPortIndex].Allocater == GM_CLIENT_ALLOC)
            ret = PortUseClientBuffers(nPortIndex);
        if(ret != OMX_ErrorNone) {
            ProcessingCmd.Cmd = OMX_CommandMax;
            SendCommand(OMX_CommandMax, nPortIndex, NULL);
            PortFreeBuffers(nPortIndex);
            return ret;
        }
    }

    ret = WaitCommand(OMX_CommandPortEnable, nPortIndex, NULL, OMX_TICKS_PER_SECOND);
    if(ret != OMX_ErrorNone)
        return ret;

    PortsInfo[nPortIndex].bEnabled = OMX_TRUE;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::PortFlush(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(PortsInfo[nPortIndex].bEnabled != OMX_TRUE)
        return OMX_ErrorNone;

    ret = SendCommand(OMX_CommandFlush, nPortIndex, NULL);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Flush port %d failed.\n", nPortIndex);
        return ret;
    }

    ret = WaitCommand(OMX_CommandFlush, nPortIndex, NULL, 2*OMX_TICKS_PER_SECOND);
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::PortMarkBuffer(
        OMX_U32 nPortIndex,
        OMX_PTR pMarkData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(PortsInfo[nPortIndex].bEnabled != OMX_TRUE)
        return OMX_ErrorNone;

    ret = SendCommand(OMX_CommandMarkBuffer, nPortIndex, pMarkData);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Enable port %d failed.\n", nPortIndex);
        return ret;
    }

    ret = WaitCommand(OMX_CommandMarkBuffer, nPortIndex, pMarkData, 2*OMX_TICKS_PER_SECOND);
    if(ret != OMX_ErrorNone)
        return ret;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::TunneledPortDisable(
        OMX_U32 nPortIndex)
{
    return SendCommand(OMX_CommandPortDisable, nPortIndex, NULL);
}

OMX_ERRORTYPE GMComponent::WaitTunneledPortDisable(
        OMX_U32 nPortIndex)
{
    return WaitCommand(OMX_CommandPortDisable, nPortIndex, NULL, OMX_TICKS_PER_SECOND);
}

OMX_ERRORTYPE GMComponent::TunneledPortEnable(
        OMX_U32 nPortIndex)
{
    return SendCommand(OMX_CommandPortEnable, nPortIndex, NULL);
}

OMX_ERRORTYPE GMComponent::WaitTunneledPortEnable(
        OMX_U32 nPortIndex)
{
    return WaitCommand(OMX_CommandPortEnable, nPortIndex, NULL, OMX_TICKS_PER_SECOND);
}

OMX_ERRORTYPE GMComponent::PassOutBufferToPeer(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GM_PORTINFO *pPortInfo = NULL;
    GMComponent *peer = NULL;
    OMX_BUFFERHEADERTYPE *pBuffer, *pPeerBuffer;
    OMX_U32 i;

    pPortInfo = &PortsInfo[nPortIndex];
    if(pPortInfo->eDir != OMX_DirOutput)
        return OMX_ErrorBadPortIndex;

    peer = PortsInfo[nPortIndex].peer.pComponent;
    fsl_osal_mutex_lock(PortsInfo[nPortIndex].HoldBufferListLock);
    while(PortsInfo[nPortIndex].HoldBufferList.GetNodeCnt() > 0) {
        pBuffer = NULL;
        pBuffer = PortsInfo[nPortIndex].HoldBufferList.GetNode(0);
        LOG_DEBUG("%s:%d get buffer %p from hold buffer list.\n", name, nPortIndex, pBuffer);
        if(pBuffer == NULL) {
            LOG_ERROR("%s get null buffer from holding list.\n", name);
            continue;
        }
        PortsInfo[nPortIndex].HoldBufferList.Remove(pBuffer);

        pPeerBuffer = (OMX_BUFFERHEADERTYPE*)(pBuffer->pAppPrivate);
        if(pPeerBuffer == NULL) {
            LOG_ERROR("%s:%d peer buffer is NULL.\n", name, nPortIndex);
            continue;
        }
        pPeerBuffer->nFilledLen = pBuffer->nFilledLen;
        pPeerBuffer->nOffset = pBuffer->nOffset;
        pPeerBuffer->nFlags = pBuffer->nFlags;
        pPeerBuffer->nTimeStamp = pBuffer->nTimeStamp;
        pPeerBuffer->hMarkTargetComponent = pBuffer->hMarkTargetComponent;
        pPeerBuffer->pMarkData = pBuffer->pMarkData;
        ret = OMX_EmptyThisBuffer(peer->hComponent, pPeerBuffer);
        //if(fsl_osal_strcmp((fsl_osal_char*)name, (fsl_osal_char*)"OMX.Freescale.std.video_decoder.avc.sw-based") == 0)
        //    printf("Vpu pass buffer %p:%p:%d to render %p:%p:%d, ret=%x\n", pBuffer, pBuffer->pBuffer, pBuffer->nFlags, pPeerBuffer, pPeerBuffer->pBuffer, pPeerBuffer->nFlags, ret);
    }
    fsl_osal_mutex_unlock(PortsInfo[nPortIndex].HoldBufferListLock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::GetComponentPortsPara()
{
    OMX_PORT_PARAM_TYPE *pPortPara = NULL;

    pPortPara = &PortPara[AUDIO_PORT_PARA];
    OMX_INIT_STRUCT(pPortPara, OMX_PORT_PARAM_TYPE);
    OMX_GetParameter(hComponent, OMX_IndexParamAudioInit, pPortPara);

    pPortPara = &PortPara[VIDEO_PORT_PARA];
    OMX_INIT_STRUCT(pPortPara, OMX_PORT_PARAM_TYPE);
    OMX_GetParameter(hComponent, OMX_IndexParamVideoInit, pPortPara);

    pPortPara = &PortPara[IMAGE_PORT_PARA];
    OMX_INIT_STRUCT(pPortPara, OMX_PORT_PARAM_TYPE);
    OMX_GetParameter(hComponent, OMX_IndexParamImageInit, pPortPara);

    pPortPara = &PortPara[OTHER_PORT_PARA];
    OMX_INIT_STRUCT(pPortPara, OMX_PORT_PARAM_TYPE);
    OMX_GetParameter(hComponent, OMX_IndexParamOtherInit, pPortPara);

    nPorts = PortPara[AUDIO_PORT_PARA].nPorts
        + PortPara[VIDEO_PORT_PARA].nPorts
        + PortPara[IMAGE_PORT_PARA].nPorts
        + PortPara[OTHER_PORT_PARA].nPorts;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::RegistSysCallback(
        SYSCALLBACKTYPE sysCb, 
        OMX_PTR pAppData)
{
    SysEventHandler = sysCb;
    SysEventAppData = pAppData;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::RegistSysBufferCallback(
        SYSBUFFERCB sysBufferCb, 
        OMX_PTR pAppData)
{
    SysBufferHandler = sysBufferCb;
    SysBufferAppData = pAppData;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::RegistBufferCallback(
        GM_BUFFERCB *pBufferCb, 
        OMX_PTR pAppData)
{
    pBufferCallback = pBufferCb;
    pBufAppData = pAppData;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::CbEventHandler(
        OMX_EVENTTYPE eEvent, 
        OMX_U32 nData1, 
        OMX_U32 nData2, 
        OMX_PTR pEventData)
{
    switch (eEvent) {
        case OMX_EventCmdComplete:
            LOG_DEBUG("%s Get Cmd [%x:%d] complelte Event.\n", name, nData1, nData2);
            if(nData1 != (OMX_U32)ProcessingCmd.Cmd) {
                LOG_WARNING("%s Get invalid CmdComplete event: %x, want: %x.\n", name, nData1, ProcessingCmd.Cmd);
                break;
            }

            DoneCmd.Cmd = (OMX_COMMANDTYPE)nData1;
            DoneCmd.nParam1 = nData2;
            DoneCmd.pCmdData = pEventData;
            break;
        case OMX_EventPortSettingsChanged:
            {
                LOG_DEBUG("%s Get PortSettingsChanged Event in port %d.\n", name, nData1);
                OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
                OMX_U32 nPortIndex = nData1;

                PortsInfo[nPortIndex].bSettingChanged = OMX_TRUE;
                OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
                sPortDef.nPortIndex = nPortIndex;
                OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
                if(sPortDef.nBufferSize > PortsInfo[nPortIndex].nBufferSize
                        || sPortDef.nBufferCountActual > PortsInfo[nPortIndex].nBuffers) {
                    LOG_DEBUG("%s port %d resource changed from [%d:%d] to [%d:%d].\n",
                            name, nPortIndex,
                            PortsInfo[nPortIndex].nBuffers, PortsInfo[nPortIndex].nBufferSize,
                            sPortDef.nBufferCountActual, sPortDef.nBufferSize);
                    PortsInfo[nPortIndex].bPortResourceChanged = OMX_TRUE;
                    //PortsInfo[nPortIndex].nBuffers = sPortDef.nBufferCountActual;
                    //PortsInfo[nPortIndex].nBufferSize = sPortDef.nBufferSize;
                }
            }
            break;
        case OMX_EventError:
            if((OMX_S32)nData1 == OMX_ErrorSameState) {
                DoneCmd.Cmd = ProcessingCmd.Cmd;
                DoneCmd.nParam1 = ProcessingCmd.nParam1;
                DoneCmd.pCmdData = ProcessingCmd.pCmdData;
                break;
            }
            bError = OMX_TRUE;
        case OMX_EventPortFormatDetected:
            LOG_DEBUG("%s Get PortFormatDetected Event in port %d.\n", name, nData1);
        case OMX_EventBufferFlag:
        case OMX_EventMark:
        case OMX_EventBufferingUpdate:
        case OMX_EventStreamSkipped:
            if(SysEventHandler)
                (*SysEventHandler)(this, SysEventAppData, eEvent, nData1, nData2, pEventData);
            break;
        case OMX_EventBufferingData:
            bResetTimeout = OMX_TRUE;
            if(SysEventHandler)
                (*SysEventHandler)(this, SysEventAppData, eEvent, nData1, nData2, pEventData);
            break;
        case OMX_EventBufferingDone:
            bResetTimeout = OMX_FALSE;
            if(SysEventHandler)
                (*SysEventHandler)(this, SysEventAppData, eEvent, nData1, nData2, pEventData);
            break;
        default :
            break;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::HoldBufferCheck(
		OMX_U32 nPortIndex)
{
	if(PortsInfo[nPortIndex].bHoldBuffers == OMX_TRUE) { 
		if(PortsInfo[nPortIndex].bFreeBuffers == OMX_TRUE) { 
			PortsInfo[nPortIndex].nBufferHolded ++; 
		} 
		return OMX_ErrorUndefined; 
	} 

	return OMX_ErrorNone; 
}

OMX_ERRORTYPE GMComponent::CbEmptyBufferDone(
        OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pPeerBuffer = NULL;
    GMComponent *peer = NULL;
    OMX_U32 nPortIndex;
    OMX_U32 nPeerPortIndex;

    if(pBuffer == NULL) {
        //LOG_ERROR("%s EmptyBufferDone with NULL buffer.\n", name);
        return OMX_ErrorBadParameter;
    }

    nPortIndex = pBuffer->nInputPortIndex;
    if(SysBufferHandler != NULL) {
        if(pBuffer->nTimeStamp >= 0)
            (*SysBufferHandler)(this, pBuffer, nPortIndex, SysBufferAppData);
    }

    peer = PortsInfo[nPortIndex].peer.pComponent;
    nPeerPortIndex = PortsInfo[nPortIndex].peer.nPortIndex;
    if(peer == NULL) {
        LOG_ERROR("%s EmptyBufferDone has no peer for port %d.\n", name, nPortIndex);
        return OMX_ErrorUndefined;
    }

    pPeerBuffer = (OMX_BUFFERHEADERTYPE*)pBuffer->pAppPrivate;
    if(pPeerBuffer == NULL) {
        LOG_ERROR("%s:%d peer buffer is NULL.\n", name, nPortIndex);
        return OMX_ErrorUndefined;
    }

    pPeerBuffer->nFilledLen = 0;
    pPeerBuffer->nOffset = 0;
    pPeerBuffer->nFlags = 0;
    pPeerBuffer->nTimeStamp = 0;
    pPeerBuffer->hMarkTargetComponent = NULL;
    pPeerBuffer->pMarkData = NULL;

    LOG_DEBUG("%s pass buffer to %s.\n", name, peer->name);

	if(PortsInfo[nPortIndex].Allocater == GM_SELF_ALLOC) {
		ret = HoldBufferCheck(nPortIndex);
		if(ret != OMX_ErrorNone)
			return OMX_ErrorNone;
	}
	else if(PortsInfo[nPortIndex].Allocater == GM_PEER_ALLOC) {
		ret = peer->HoldBufferCheck(nPeerPortIndex);
		if(ret != OMX_ErrorNone)
			return OMX_ErrorNone;
	}

	ret = OMX_FillThisBuffer(peer->hComponent, pPeerBuffer);
	if(ret != OMX_ErrorNone) {
		if(PortsInfo[nPortIndex].Allocater == GM_SELF_ALLOC) {
			ret = HoldBufferCheck(nPortIndex);
			if(ret != OMX_ErrorNone)
				return OMX_ErrorNone;
		}
		else if(PortsInfo[nPortIndex].Allocater == GM_PEER_ALLOC) {
			ret = peer->HoldBufferCheck(nPeerPortIndex);
			if(ret != OMX_ErrorNone)
				return OMX_ErrorNone;
		}
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::CbFillBufferDone(
        OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_BUFFERHEADERTYPE *pPeerBuffer = NULL;
    OMX_STATETYPE eState = OMX_StateInvalid;
    GMComponent *peer = NULL;
    OMX_U32 nPortIndex;
    OMX_U32 nPeerPortIndex;

    if(pBuffer == NULL) {
        LOG_ERROR("%s FillBufferDone with NULL buffer.\n", name);
        return OMX_ErrorBadParameter;
    }

    nPortIndex = pBuffer->nOutputPortIndex;
    if(SysBufferHandler != NULL)
        (*SysBufferHandler)(this, pBuffer, nPortIndex, SysBufferAppData);

    peer = PortsInfo[nPortIndex].peer.pComponent;
    nPeerPortIndex = PortsInfo[nPortIndex].peer.nPortIndex;
    if(peer == NULL) {
        LOG_ERROR("%s FillBufferDone has no peer for port %d.\n", name, nPortIndex);
        return OMX_ErrorUndefined;
    }

    PortsInfo[nPortIndex].bOutputed = OMX_TRUE;
    OMX_GetState(peer->hComponent, &eState);
    if(eState != OMX_StateExecuting && eState != OMX_StatePause) {
        HoldingOutPortBuffer(pBuffer);
        return OMX_ErrorNone;
    }

    if(peer->IsPortEnabled(nPeerPortIndex) != OMX_TRUE) {
        HoldingOutPortBuffer(pBuffer);
        return OMX_ErrorNone;
    }

    PassOutBufferToPeer(nPortIndex);

    pPeerBuffer = (OMX_BUFFERHEADERTYPE*)pBuffer->pAppPrivate;
    if(pPeerBuffer == NULL) {
        LOG_ERROR("%s:%d peer buffer is NULL.\n", name, nPortIndex);
        return OMX_ErrorUndefined;
    }

    pPeerBuffer->nFilledLen = pBuffer->nFilledLen;
    pPeerBuffer->nOffset = pBuffer->nOffset;
    pPeerBuffer->nFlags = pBuffer->nFlags;
    pPeerBuffer->nTimeStamp = pBuffer->nTimeStamp;
    pPeerBuffer->hMarkTargetComponent = pBuffer->hMarkTargetComponent;
    pPeerBuffer->pMarkData = pBuffer->pMarkData;
    
    LOG_DEBUG("%s pass buffer to %s.\n", name, peer->name);

    //if(pBuffer->hMarkTargetComponent != NULL)
    //    printf("%s pass buffer to %s, flag: %x.\n", name, peer->name, pBuffer->nFlags);
    
    OMX_EmptyThisBuffer(peer->hComponent, pPeerBuffer);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::SendCommand(
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam1, 
        OMX_PTR pCmdData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nCount = 0;

    LOG_DEBUG("%s send command[%d:%d]\n", name, Cmd, nParam1);

    while(ProcessingCmd.Cmd != OMX_CommandMax) {
        fsl_osal_sleep(10000);
        if(nCount ++ > 50) {
            LOG_ERROR("%s Time out for Sending Command [%d:%d], Busy on Command [%d:%d].\n", 
                    name, Cmd, nParam1, ProcessingCmd.Cmd, ProcessingCmd.nParam1);
            return OMX_ErrorTimeout;
        }
    }

    nCount = 0;

    bError = OMX_FALSE;
    ProcessingCmd.Cmd = Cmd;
    ProcessingCmd.nParam1 = nParam1;
    ProcessingCmd.pCmdData = pCmdData;
    DoneCmd.Cmd = OMX_CommandMax;
    while(1) {
        ret = OMX_SendCommand(hComponent, Cmd, nParam1, pCmdData);
        if(ret == OMX_ErrorNone)
            break;
        else if(ret == OMX_ErrorNotReady) {
            fsl_osal_sleep(10000);
            if(nCount ++ > 50) {
                LOG_ERROR("%s send command[%d:%d] timeout.\n", name, Cmd, nParam1);
                ret = OMX_ErrorTimeout;
                break;
            }
            continue;
        }
        else if(ret == OMX_ErrorSameState) {
            DoneCmd.Cmd = ProcessingCmd.Cmd;
            DoneCmd.nParam1 = ProcessingCmd.nParam1;
            DoneCmd.pCmdData = ProcessingCmd.pCmdData;
            ret = OMX_ErrorNone;
            break;
        }
        else { 
            LOG_ERROR("%s send command[%d:%d] failed, ret = %x\n", name, Cmd, nParam1, ret);
            ProcessingCmd.Cmd = OMX_CommandMax;
            break;
        }
    }

    return ret;
}

OMX_ERRORTYPE GMComponent::WaitCommand(
        OMX_COMMANDTYPE Cmd, 
        OMX_U32 nParam1, 
        OMX_PTR pCmdData,
        OMX_TICKS timeout)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_S32 nCount = 0, TimeOutCnt = 0;

    if(Cmd != ProcessingCmd.Cmd) {
        LOG_ERROR("%s Wait incorrect Command[%d:%d], ProcessingCmd is [%d:%d].\n",
                name, Cmd, nParam1, ProcessingCmd.Cmd, ProcessingCmd.nParam1);
        return OMX_ErrorUndefined;
    }

    TimeOutCnt = timeout / 10000;
    while(1) {
        if(ProcessingCmd.Cmd == DoneCmd.Cmd 
                && ProcessingCmd.nParam1 == DoneCmd.nParam1
                && ProcessingCmd.pCmdData == DoneCmd.pCmdData)
            break;
        else if(bError == OMX_TRUE) {
            ret = OMX_ErrorUndefined;
            break;
        }
        else {
            fsl_osal_sleep(10000);
            if(bResetTimeout == OMX_TRUE)
                nCount = 0;

            if(TimeOutCnt > 0) {
                nCount ++;
                if(nCount >= TimeOutCnt) {
                    LOG_ERROR("%s wait command[%d:%d] timeout.\n", name, Cmd, nParam1);
                    ret = OMX_ErrorTimeout;
                    break;
                }
            }
        }
    }

    ProcessingCmd.Cmd = OMX_CommandMax;

    return ret;
}

OMX_ERRORTYPE GMComponent::DoLoaded2Idle()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i;

    for(i=0; i<nPorts; i++)
        if(OMX_TRUE == PortsInfo[i].bSettingChanged) {
            ret = DoPortSettingChanged(i);
            if(ret != OMX_ErrorNone)
                return ret;
            PortsInfo[i].bSettingChanged = OMX_FALSE;
        }

    ret = SendCommand(OMX_CommandStateSet, OMX_StateIdle, NULL);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("State trans to Idle failed.\n");
        return ret;
    }

    for(i=0; i<nPorts; i++) {
        if(PortsInfo[i].bEnabled != OMX_TRUE)
            continue;
        if(PortsInfo[i].Allocater == GM_SELF_ALLOC)
            ret = PortAllocateBuffers(i);
        else if(PortsInfo[i].Allocater == GM_PEER_ALLOC ||
                PortsInfo[i].Allocater == GM_PEER_BUFFER)
            ret = PortUsePeerBuffers(i);
        else if(PortsInfo[i].Allocater == GM_CLIENT_ALLOC)
            ret = PortUseClientBuffers(i);
        else if(PortsInfo[i].Allocater == GM_USE_EGLIMAGE)
            ret = PortUseEglImage(i);
        if(ret != OMX_ErrorNone)
            break;
    }

    if(ret != OMX_ErrorNone) {
        /* port allocate resource failed, need to free already allocated resources */
        for(i=0; i<nPorts; i++) {
            if(PortsInfo[i].bEnabled != OMX_TRUE)
                continue;
            PortFreeBuffers(i);
        }
        return ret;
    }

    ret = WaitCommand(OMX_CommandStateSet, OMX_StateIdle, NULL, OMX_TICKS_PER_SECOND);
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::DoIdle2Loaded()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i;

    ret = SendCommand(OMX_CommandStateSet, OMX_StateLoaded, NULL);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("State trans to Loaded failed.\n");
        return ret;
    }

    for(i=0; i<nPorts; i++) {
        if(PortsInfo[i].bEnabled != OMX_TRUE)
            continue;
        PortFreeBuffers(i);
    }

    ret = WaitCommand(OMX_CommandStateSet, OMX_StateLoaded, NULL, 5*OMX_TICKS_PER_SECOND);
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::DoStateTrans(
        OMX_STATETYPE eState,
        OMX_TICKS timeout)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = SendCommand(OMX_CommandStateSet, eState, NULL);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("State trans to %d failed.\n", eState);
        return ret;
    }

    ret = WaitCommand(OMX_CommandStateSet, eState, NULL, timeout);
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::DoExec2Idle(
        OMX_STATETYPE eState)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i;

    for(i=0; i<nPorts; i++) {
        if(PortsInfo[i].bEnabled != OMX_TRUE)
            continue;
        PortsInfo[i].bHoldBuffers = OMX_TRUE;
    }

    ret = SendCommand(OMX_CommandStateSet,eState,NULL);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("State trans to %d failed.\n", eState);
        return ret;
    }

    ret = WaitCommand(OMX_CommandStateSet, eState, NULL, OMX_TICKS_PER_SECOND);
    if(ret != OMX_ErrorNone)
        return ret;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::SetPortBufferAllocateType(
        OMX_U32 nPortIndex, 
        GM_BUFFERALLOCATER eAllocater)
{
    PortsInfo[nPortIndex].Allocater = eAllocater;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::PortAllocateBuffers(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    OMX_U32 i;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
    PortsInfo[nPortIndex].nBuffers = sPortDef.nBufferCountActual;
    PortsInfo[nPortIndex].nBufferSize = sPortDef.nBufferSize;

    for(i=0; i<PortsInfo[nPortIndex].nBuffers; i++) {
        pBufferHdr = NULL;
        ret = OMX_AllocateBuffer(hComponent, &pBufferHdr, nPortIndex, NULL, PortsInfo[nPortIndex].nBufferSize);
        if(ret != OMX_ErrorNone) {
            LOG_ERROR("%s allocate buffer for port %d failed.\n", name, nPortIndex);
            break;
        }
        PortsInfo[nPortIndex].pBufferHdr[i] = pBufferHdr;
        //printf("%s allocate buffer %p:%p for port %d\n", name, pBufferHdr, pBufferHdr->pBuffer, nPortIndex);
    }

    PortsInfo[nPortIndex].Allocated = PortsInfo[nPortIndex].Allocater;

    return ret;
}

OMX_ERRORTYPE GMComponent::PortUsePeerBuffers(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GMComponent *peer = NULL;
    OMX_U32 nPeerPortIndex;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL, *pPeerBufferHdr = NULL;
    OMX_U8* pBuffer = NULL;
    OMX_STATETYPE eState = OMX_StateInvalid;
    OMX_U32 i;

    peer = PortsInfo[nPortIndex].peer.pComponent;
    nPeerPortIndex = PortsInfo[nPortIndex].peer.nPortIndex;

    OMX_GetState(peer->hComponent, &eState);
    if(eState == OMX_StateLoaded) {
        ret = peer->StateTransUpWard(OMX_StateIdle);
        if(ret != OMX_ErrorNone) {
            LOG_ERROR("%s request peer %s trans to idle failed when PortUsePeerBuffers.\n", name, peer->name);
            return ret;
        }
    }

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
    PortsInfo[nPortIndex].nBuffers = sPortDef.nBufferCountActual;
    PortsInfo[nPortIndex].nBufferSize = sPortDef.nBufferSize;

    for(i=0; i<PortsInfo[nPortIndex].nBuffers; i++) {
        pBufferHdr = NULL;
        pPeerBufferHdr = NULL;
        peer->GetPortBuffer(nPeerPortIndex, i, &pPeerBufferHdr);
        if(PortsInfo[nPortIndex].Allocater == GM_PEER_ALLOC)
            pBuffer = pPeerBufferHdr->pBuffer;
        if(PortsInfo[nPortIndex].Allocater == GM_PEER_BUFFER)
            pBuffer = (OMX_U8*) pPeerBufferHdr;
        ret = OMX_UseBuffer(hComponent, &pBufferHdr, nPortIndex, NULL, PortsInfo[nPortIndex].nBufferSize, pBuffer);
        if(ret != OMX_ErrorNone) {
            LOG_ERROR("%s use buffer for port %d failed.\n", name, nPortIndex);
            break;
        }
        pBufferHdr->pAppPrivate = pPeerBufferHdr;
        pPeerBufferHdr->pAppPrivate = pBufferHdr;
        PortsInfo[nPortIndex].pBufferHdr[i] = pBufferHdr;
        //printf("%s use buffer %p:%p for port %d, peer buffer %p:%p\n", name, pBufferHdr, pBufferHdr->pBuffer, nPortIndex,
        //        pPeerBufferHdr, pPeerBufferHdr->pBuffer);
    }

    PortsInfo[nPortIndex].Allocated = PortsInfo[nPortIndex].Allocater;

    return ret;
}

OMX_ERRORTYPE GMComponent::PortUseClientBuffers(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    GMBUFFERINFO sBufferInfo;
    OMX_PTR pBuffer;
    OMX_U32 i;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
    PortsInfo[nPortIndex].nBuffers = sPortDef.nBufferCountActual;
    PortsInfo[nPortIndex].nBufferSize = sPortDef.nBufferSize;

    sBufferInfo.type = GM_PHYSIC;
    sBufferInfo.nSize = PortsInfo[nPortIndex].nBufferSize;
    sBufferInfo.nNum = PortsInfo[nPortIndex].nBuffers;

    for(i=0; i<PortsInfo[nPortIndex].nBuffers; i++) {
        pBufferHdr = NULL;
        pBuffer = NULL;
        ret = pBufferCallback->GetBuffer(hComponent, nPortIndex, &pBuffer, &sBufferInfo, pBufAppData);
        if(ret != OMX_ErrorNone) {
            LOG_ERROR("%s Get Client Buffer for port %d failed.\n", name, nPortIndex);
            break;
        }

        ret = OMX_UseBuffer(hComponent, &pBufferHdr, nPortIndex, NULL, PortsInfo[nPortIndex].nBufferSize, (OMX_U8*)pBuffer);
        if(ret != OMX_ErrorNone) {
            LOG_ERROR("%s use buffer for port %d failed.\n", name, nPortIndex);
            break;
        }

        PortsInfo[nPortIndex].pBufferHdr[i] = pBufferHdr;
    }

    PortsInfo[nPortIndex].Allocated = PortsInfo[nPortIndex].Allocater;

    return ret;
}

OMX_ERRORTYPE GMComponent::PortUseShareBuffers(
        OMX_U32 nPortIndex)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::PortFreeBuffers(
        OMX_U32 nPortIndex)
{
    OMX_U32 i;

    for(i=0; i<PortsInfo[nPortIndex].nBuffers; i++) {
        if(PortsInfo[nPortIndex].pBufferHdr[i] != NULL) {
            if(PortsInfo[nPortIndex].Allocated == GM_CLIENT_ALLOC) {
                OMX_PTR pBuffer = PortsInfo[nPortIndex].pBufferHdr[i]->pBuffer;
                GMBUFFERINFO sBufferInfo;
                sBufferInfo.type = GM_PHYSIC;
                sBufferInfo.nSize = PortsInfo[nPortIndex].nBufferSize;
                sBufferInfo.nNum = PortsInfo[nPortIndex].nBuffers;
                pBufferCallback->FreeBuffer(hComponent, nPortIndex, pBuffer, &sBufferInfo, pBufAppData);
            }
            OMX_FreeBuffer(hComponent, nPortIndex, PortsInfo[nPortIndex].pBufferHdr[i]);
            PortsInfo[nPortIndex].pBufferHdr[i] = NULL;
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::PortUseEglImage(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_BUFFERHEADERTYPE *pBufferHdr = NULL;
    GMBUFFERINFO sBufferInfo;
    OMX_PTR pEglImage = NULL;
    OMX_U32 i;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);
    PortsInfo[nPortIndex].nBuffers = sPortDef.nBufferCountActual;
    PortsInfo[nPortIndex].nBufferSize = sPortDef.nBufferSize;

    sBufferInfo.type = GM_EGLIMAGE;
    sBufferInfo.nSize = PortsInfo[nPortIndex].nBufferSize;
    sBufferInfo.nNum = PortsInfo[nPortIndex].nBuffers;

    for(i=0; i<PortsInfo[nPortIndex].nBuffers; i++) {
        pBufferHdr = NULL;
        pEglImage = NULL;

        ret = pBufferCallback->GetBuffer(hComponent, nPortIndex, &pEglImage, &sBufferInfo, pBufAppData);
        if(ret != OMX_ErrorNone) {
            LOG_ERROR("%s Get EglImage for port %d failed.\n", name, nPortIndex);
            break;
        }

        if(pEglImage == NULL) {
            LOG_ERROR("%s get NULL eglImage for port %d.\n", name, nPortIndex);
            break;
        }

        ret = OMX_UseEGLImage(hComponent, &pBufferHdr, nPortIndex, NULL, pEglImage);
        if(ret != OMX_ErrorNone) {
            LOG_ERROR("%s Use EglImage for port %d failed.\n", name, nPortIndex);
            break;
        }

        PortsInfo[nPortIndex].pBufferHdr[i] = pBufferHdr;
    }

    PortsInfo[nPortIndex].Allocated = PortsInfo[nPortIndex].Allocater;

    return ret;
}

OMX_ERRORTYPE GMComponent::GetPortBuffer(
        OMX_U32 nPortIndex, 
        OMX_U32 nBufferIndex, 
        OMX_BUFFERHEADERTYPE **ppBufferHdr)
{
    *ppBufferHdr = PortsInfo[nPortIndex].pBufferHdr[nBufferIndex];
    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::StartProcessNoWait(OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i,j;

    for(i=0; i<nPorts; i++) {
		if(nPortIndex != OMX_ALL)
			if(nPortIndex != i)
				continue;
		if(PortsInfo[i].bEnabled != OMX_TRUE)
            continue;
        if(OMX_DirOutput != PortsInfo[i].eDir)
            continue;

        for(j=0; j<PortsInfo[i].nBuffers; j++)
            OMX_FillThisBuffer(hComponent, PortsInfo[i].pBufferHdr[j]);

        if(OMX_TRUE == PortsInfo[i].bSettingChanged) {
            ret = DoPortSettingChanged(i);
            if(ret != OMX_ErrorNone)
                break;
            PortsInfo[i].bSettingChanged = OMX_FALSE;
        }
    }

    return ret;
}

OMX_ERRORTYPE GMComponent::StartProcess(OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 i,j;

    for(i=0; i<nPorts; i++) {
        if(nPortIndex != OMX_ALL)
            if(nPortIndex != i)
                continue;
        if(PortsInfo[i].bEnabled != OMX_TRUE)
            continue;
        if(OMX_DirOutput != PortsInfo[i].eDir)
            continue;

        for(j=0; j<PortsInfo[i].nBuffers; j++)
            OMX_FillThisBuffer(hComponent, PortsInfo[i].pBufferHdr[j]);

        ret = WaitForOutput(i);
        if(ret != OMX_ErrorNone)
            break;

        if(OMX_TRUE == PortsInfo[i].bSettingChanged) {
            ret = DoPortSettingChanged(i);
            if(ret != OMX_ErrorNone)
                break;
            PortsInfo[i].bSettingChanged = OMX_FALSE;
        }
    }

    return ret;
}

OMX_ERRORTYPE GMComponent::WaitForOutput(OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    while(1) {
        if(PortsInfo[nPortIndex].bSettingChanged == OMX_TRUE
                || PortsInfo[nPortIndex].bOutputed == OMX_TRUE)
            break;
        if(bError == OMX_TRUE) {
            ret = OMX_ErrorUndefined;
            break;
        }
        fsl_osal_sleep(50000);
    };

    return ret;
}

OMX_ERRORTYPE GMComponent::HoldingOutPortBuffer(
        OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    GM_PORTINFO *pPortInfo = NULL;
    OMX_U32 nPortIndex;

    nPortIndex = pBufferHdr->nOutputPortIndex;
    pPortInfo = &PortsInfo[nPortIndex];
    if(pPortInfo->eDir != OMX_DirOutput)
        return OMX_ErrorBadPortIndex;

    fsl_osal_mutex_lock(PortsInfo[nPortIndex].HoldBufferListLock);
    LOG_DEBUG("%s:%d add buffer %p to hold buffer list.\n", name, nPortIndex, pBufferHdr);
    PortsInfo[nPortIndex].HoldBufferList.Add(pBufferHdr);
    fsl_osal_mutex_unlock(PortsInfo[nPortIndex].HoldBufferListLock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::DoPortSettingChanged(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GMComponent *pPeer = NULL;
    OMX_U32 nPeerPortIndex;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef1, sPortDef2;

    pPeer = PortsInfo[nPortIndex].peer.pComponent;
    nPeerPortIndex = PortsInfo[nPortIndex].peer.nPortIndex;

    OMX_INIT_STRUCT(&sPortDef1, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef1.nPortIndex = nPortIndex;
    OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef1);

    OMX_INIT_STRUCT(&sPortDef2, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef2.nPortIndex = nPeerPortIndex;
    OMX_GetParameter(pPeer->hComponent, OMX_IndexParamPortDefinition, &sPortDef2);

    sPortDef2.format = sPortDef1.format;
    sPortDef2.nBufferSize = sPortDef1.nBufferSize;
    sPortDef2.nBufferCountActual = sPortDef1.nBufferCountActual; 
    ret = OMX_SetParameter(pPeer->hComponent, OMX_IndexParamPortDefinition, &sPortDef2);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("[%s:%d] set peer port definition [%s:%d] failed.\n", name, nPortIndex, pPeer->name, nPeerPortIndex);
        return ret;
    }

    SetPeerPortMediaFormat(nPortIndex);

    if(PortsInfo[nPortIndex].bPortResourceChanged == OMX_TRUE) {
        LOG_DEBUG("%s Port %d resource changed.\n", name, nPortIndex);

        ret = PortDisable(nPortIndex);
        if(ret != OMX_ErrorNone)
            return ret;

        PortsInfo[nPortIndex].nBuffers = sPortDef1.nBufferCountActual;
        PortsInfo[nPortIndex].nBufferSize = sPortDef1.nBufferSize;

        ret = PortEnable(nPortIndex);
        if(ret != OMX_ErrorNone)
            return ret;

        OMX_STATETYPE eState = OMX_StateInvalid;
        OMX_U32 j;
        OMX_GetState(hComponent, &eState);
        if(eState == OMX_StateExecuting) {
            for(j=0; j<PortsInfo[nPortIndex].nBuffers; j++)
                OMX_FillThisBuffer(hComponent, PortsInfo[nPortIndex].pBufferHdr[j]);
        }

        PortsInfo[nPortIndex].bPortResourceChanged = OMX_FALSE;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::SetPeerPortMediaFormat(
        OMX_U32 nPortIndex)
{
    OMX_HANDLETYPE hPeer = NULL;
    OMX_U32 nPeerPortIndex;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    hPeer = PortsInfo[nPortIndex].peer.pComponent->hComponent;
    nPeerPortIndex = PortsInfo[nPortIndex].peer.nPortIndex;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = nPortIndex;
    OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &sPortDef);

    switch(sPortDef.eDomain) {
        case OMX_PortDomainAudio:
            switch(sPortDef.format.audio.eEncoding) {
                case OMX_AUDIO_CodingPCM:
                    {
                        OMX_AUDIO_PARAM_PCMMODETYPE PcmPara;
                        OMX_INIT_STRUCT(&PcmPara, OMX_AUDIO_PARAM_PCMMODETYPE);
                        PcmPara.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamAudioPcm, &PcmPara)) {
                            PcmPara.nPortIndex = nPeerPortIndex;
                            OMX_SetParameter(hPeer, OMX_IndexParamAudioPcm, &PcmPara);
                        }
                    }
                    break;
                case OMX_AUDIO_CodingMP3:
                    {
                        OMX_AUDIO_PARAM_MP3TYPE Mp3Para;
                        OMX_INIT_STRUCT(&Mp3Para, OMX_AUDIO_PARAM_MP3TYPE);
                        Mp3Para.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamAudioMp3, &Mp3Para)) {
                            Mp3Para.nPortIndex = nPeerPortIndex;
                            OMX_SetParameter(hPeer, OMX_IndexParamAudioMp3, &Mp3Para);
                        }
                    }
                    break;
                case OMX_AUDIO_CodingAMR:
                    {
                        OMX_AUDIO_PARAM_AMRTYPE AmrPara;
                        OMX_INIT_STRUCT(&AmrPara, OMX_AUDIO_PARAM_AMRTYPE);
                        AmrPara.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamAudioAmr, &AmrPara)) {
                            AmrPara.nPortIndex = nPeerPortIndex;
                            OMX_SetParameter(hPeer, OMX_IndexParamAudioAmr, &AmrPara);
                        }
                    }
                    break;
                case OMX_AUDIO_CodingAAC:
                    {
                        OMX_AUDIO_PARAM_AACPROFILETYPE AacProfile;
                        OMX_INIT_STRUCT(&AacProfile, OMX_AUDIO_PARAM_AACPROFILETYPE);
                        AacProfile.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamAudioAac, &AacProfile)) {
                            AacProfile.nPortIndex = nPeerPortIndex;
                            OMX_SetParameter(hPeer, OMX_IndexParamAudioAac, &AacProfile);
                        }
                    }
                    break;
                case OMX_AUDIO_CodingAC3:
                    {
                        OMX_AUDIO_PARAM_AC3TYPE Ac3;
                        OMX_INIT_STRUCT(&Ac3, OMX_AUDIO_PARAM_AC3TYPE);
                        Ac3.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamAudioAc3, &Ac3)) {
                            Ac3.nPortIndex = nPeerPortIndex;
                            OMX_SetParameter(hPeer, OMX_IndexParamAudioAc3, &Ac3);
                        }
                    }
                    break;
                case OMX_AUDIO_CodingVORBIS:
                    {
                        OMX_AUDIO_PARAM_VORBISTYPE Vorbis;
                        OMX_INIT_STRUCT(&Vorbis, OMX_AUDIO_PARAM_VORBISTYPE);
                        Vorbis.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamAudioVorbis, &Vorbis)) {
                            Vorbis.nPortIndex = nPeerPortIndex;
                            OMX_SetParameter(hPeer, OMX_IndexParamAudioVorbis, &Vorbis);
                        }
                    }
                    break;
                case OMX_AUDIO_CodingWMA:
                    {
                        OMX_AUDIO_PARAM_WMATYPE WmaPara;
                        OMX_INIT_STRUCT(&WmaPara, OMX_AUDIO_PARAM_WMATYPE);
                        WmaPara.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamAudioWma, &WmaPara)) {
                            WmaPara.nPortIndex = nPeerPortIndex;
                            OMX_SetParameter(hPeer, OMX_IndexParamAudioWma, &WmaPara);
                        }
                        OMX_AUDIO_PARAM_WMATYPE_EXT WmaParaExt;
                        OMX_INIT_STRUCT(&WmaParaExt, OMX_AUDIO_PARAM_WMATYPE_EXT);
                        WmaParaExt.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamAudioWmaExt, &WmaParaExt)) {
                            WmaParaExt.nPortIndex = nPeerPortIndex;
                            OMX_SetParameter(hPeer, OMX_IndexParamAudioWmaExt, &WmaParaExt);
                        }
                    }
                    break;
                case OMX_AUDIO_CodingRA:
                    {
                        OMX_AUDIO_PARAM_RATYPE RaPara;
                        OMX_INIT_STRUCT(&RaPara, OMX_AUDIO_PARAM_RATYPE);
                        RaPara.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetParameter(hComponent, OMX_IndexParamAudioRa, &RaPara)) {
                            RaPara.nPortIndex = nPeerPortIndex;
                            OMX_SetParameter(hPeer, OMX_IndexParamAudioRa, &RaPara);
                        }
                    }
                    break;
                default:
                    break;
            }
            break;
        case OMX_PortDomainVideo:
            switch(sPortDef.format.video.eCompressionFormat) {
                case OMX_VIDEO_CodingUnused:
                    {
                        OMX_CONFIG_RECTTYPE sRect;
                        OMX_INIT_STRUCT(&sRect, OMX_CONFIG_RECTTYPE);
                        sRect.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetConfig(hComponent, OMX_IndexConfigCommonOutputCrop, &sRect)) {
                            sRect.nPortIndex = nPeerPortIndex;
                            OMX_SetConfig(hPeer, OMX_IndexConfigCommonInputCrop, &sRect);
                        }
                    }
                    break; 
                case OMX_VIDEO_CodingWMV:
                case OMX_VIDEO_CodingWMV9:
                    {
                        OMX_VIDEO_PARAM_WMVTYPE wmv_type; 
                        OMX_INIT_STRUCT(&wmv_type, OMX_VIDEO_PARAM_WMVTYPE);
                        wmv_type.nPortIndex = nPortIndex;
                        if(OMX_ErrorNone == OMX_GetParameter(hComponent,OMX_IndexParamVideoWmv, &wmv_type)) {
                            wmv_type.nPortIndex = nPeerPortIndex;
                            OMX_SetParameter(hPeer, OMX_IndexParamVideoWmv, &wmv_type);
                        }

                    }
                    break;

                default:
                    break;
            }
            break;
        default:
            break;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE GMComponent::PeerNeedBuffer()
{
    return OMX_ErrorNone;
}

