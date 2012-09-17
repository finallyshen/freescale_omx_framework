/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor, Inc. 
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "ComponentBase.h"
#include "InvalidState.h"
#include "LoadedState.h"
#include "IdleState.h"
#include "ExecutingState.h"
#include "PauseState.h"
#include "WaitForResourcesState.h"
#include <string.h>
/**< internally used functions */

#define OMX_COMP_DBG
#ifdef OMX_COMP_DBG
#define OMX_COMP_DBG_LEVEL	LOG_LEVEL_APIINFO
//#define OMX_COMP_API_LOG(...)   LOG(-1/*always logout*/, __VA_ARGS__) 
#define OMX_COMP_API_LOG(name,...)   {if((nLogLevel >=OMX_COMP_DBG_LEVEL)){printf("OMXCOMP(%s): ",name); printf(__VA_ARGS__);}}
#define OMX_COMP_CALLBACK_LOG(name,...)	 {if((nLogLevel >=OMX_COMP_DBG_LEVEL)){printf("OMXCALLBACK(%s): ",name); printf(__VA_ARGS__);}}
#else
#define OMX_COMP_API_LOG(...)
#define OMX_COMP_CALLBACK_LOG(...)
#endif

#ifdef OMX_COMP_DBG

typedef struct {
	OMX_U32 value;
	OMX_U8 str[32];
}OMX_MAP_NODE;

#define OMX_MAP_INVALID_VALUE	0xFFFFFFFF

OMX_MAP_NODE omx_map_cmd_str[]={
	{OMX_CommandStateSet,"StateSet"},
	{OMX_CommandFlush,"Flush"},
	{OMX_CommandPortDisable,"PortDisable"},
	{OMX_CommandPortEnable,"PortEnable"},
	{OMX_CommandMarkBuffer,"MarkBuffer"},
	{OMX_CommandKhronosExtensions,"KhronosExtensions"},
	{OMX_CommandVendorStartUnused,"VendorStartUnused"},
	{OMX_CommandMax,"CommandMax"},
	{OMX_MAP_INVALID_VALUE,"CommandUnspecified"},
};

OMX_MAP_NODE omx_map_state_str[]={
	{OMX_StateInvalid,"StateInvalid"},
	{OMX_StateLoaded,"StateLoaded"},
	{OMX_StateIdle,"StateIdle"},
	{OMX_StateExecuting,"StateExecuting"},
	{OMX_StatePause,"StatePause"},
	{OMX_StateWaitForResources,"StateWaitForResources"},
	{OMX_StateKhronosExtensions,"StateKhronosExtensions"},
	{OMX_StateVendorStartUnused,"StateVendorStartUnused"},
	{OMX_StateMax,"StateVendorStartUnused"},
	{OMX_MAP_INVALID_VALUE,"StateUnspecified"},
};

 OMX_MAP_NODE omx_map_event_str[]={
	{OMX_EventCmdComplete,"CmdComplete"},
	{OMX_EventError,"Error"},
	{OMX_EventMark,"Mark"},
	{OMX_EventPortSettingsChanged,"PortSettingsChanged"},
	{OMX_EventBufferFlag,"BufferFlag"},
	{OMX_EventResourcesAcquired,"ResourcesAcquired"},
	{OMX_EventComponentResumed,"ComponentResumed"},
	{OMX_EventDynamicResourcesAvailable,"DynamicResourcesAvailable"},
	{OMX_EventPortFormatDetected,"PortFormatDetected"},
	{OMX_EventKhronosExtensions,"KhronosExtensions"},
	{OMX_EventVendorStartUnused,"VendorStartUnused"},
	{OMX_EventMax,"Max"},
	{OMX_MAP_INVALID_VALUE,"EventUnspecified"},
};



OMX_MAP_NODE omx_map_index_str[]={	/*only one sub-set of index*/
	{OMX_IndexParamAudioInit,"AudioInit"},
	{OMX_IndexParamImageInit,"ImageInit"},
	{OMX_IndexParamVideoInit,"VideoInit"},
	{OMX_IndexParamOtherInit,"OtherInit"},
	{OMX_IndexParamNumAvailableStreams,"NumAvailableStreams"},
	{OMX_IndexParamActiveStream,"ActiveStream"},
	{OMX_IndexParamContentURI,"ContentURI"},
	{OMX_IndexParamCustomContentPipe,"CustomContentPipe"},
	{OMX_IndexConfigMetadataItemCount,"MetadataItemCount"},
	{OMX_IndexConfigMetadataItem,"MetadataItem"},
	{OMX_IndexParamStandardComponentRole,"StandardComponentRole"},
	{OMX_IndexParamPortDefinition,"PortDefinition"},
	{OMX_IndexParamCompBufferSupplier,"CompBufferSupplier"},
	{OMX_IndexParamAudioPortFormat,"AudioPortFormat"},
	{OMX_IndexParamAudioPcm,"AudioPcm"},
	{OMX_IndexParamAudioAac,"AudioAac"},
	{OMX_IndexParamAudioRa,"AudioRa"},
	{OMX_IndexParamAudioMp3,"AudioMp3"},
	{OMX_IndexParamAudioAdpcm,"AudioAdpcm"},
	{OMX_IndexParamAudioG723,"AudioG723"},
	{OMX_IndexParamAudioG729,"AudioG729"},
	{OMX_IndexParamAudioAmr,"AudioAmr"},
	{OMX_IndexParamAudioWma,"AudioWma"},
	{OMX_IndexParamAudioVorbis,"AudioVorbis"},
	{OMX_IndexConfigAudioVolume,"AudioVolume"},
	{OMX_IndexConfigAudioEqualizer,"AudioEqualizer"},
	{OMX_IndexParamVideoPortFormat,"VideoPortFormat"},
	{OMX_IndexParamVideoQuantization,"VideoQuantization"},
	{OMX_IndexParamVideoFastUpdate,"VideoFastUpdate"},
	{OMX_IndexParamVideoBitrate,"VideoBitrate"},
	{OMX_IndexParamVideoMotionVector,"VideoMotionVector"},
	{OMX_IndexParamVideoIntraRefresh,"VideoIntraRefresh"},
	{OMX_IndexParamVideoMpeg2,"VideoMpeg2"},
	{OMX_IndexParamVideoMpeg4,"VideoMpeg4"},
	{OMX_IndexParamVideoWmv,"VideoWmv"},
	{OMX_IndexParamVideoRv,"VideoRv"},
	{OMX_IndexParamVideoAvc,"VideoAvc"},
	{OMX_IndexParamVideoH263,"VideoH263"},
	{OMX_IndexParamVideoProfileLevelQuerySupported,"VideoProfileLevelQuerySupported"},
	{OMX_IndexParamVideoProfileLevelCurrent,"VideoProfileLevelCurrent"},
	{OMX_IndexConfigVideoBitrate,"VideoBitrate"},
	{OMX_IndexConfigVideoFramerate,"VideoFramerate"},
	{OMX_IndexConfigVideoIntraVOPRefresh,"VideoIntraVOPRefresh"},
	{OMX_IndexConfigVideoAVCIntraPeriod,"VideoAVCIntraPeriod"},
	{OMX_IndexParamCommonDeblocking,"CommonDeblocking"},
	{OMX_IndexParamCommonSensorMode,"CommonSensorMode"},
	{OMX_IndexParamCommonInterleave,"CommonInterleave"},
	{OMX_IndexConfigCommonColorFormatConversion,"CommonColorFormatConversion"},
	{OMX_IndexConfigCommonScale,"CommonScale"},
	{OMX_IndexConfigCommonRotate,"CommonRotate"},
	{OMX_IndexConfigCommonMirror,"CommonMirror"},
	{OMX_IndexConfigCommonOutputPosition,"CommonOutputPosition"},
	{OMX_IndexConfigCommonInputCrop,"CommonInputCrop"},
	{OMX_IndexConfigCommonOutputCrop,"CommonOutputCrop"},
	{OMX_IndexConfigCommonDigitalZoom,"CommonDigitalZoom"},
	{OMX_IndexConfigCommonOpticalZoom,"CommonOpticalZoom"},
	{OMX_IndexConfigCommonWhiteBalance,"CommonWhiteBalance"},
	{OMX_IndexConfigCommonExposureValue,"CommonExposureValue"},
	{OMX_IndexParamOtherPortFormat,"OtherPortFormat"},
	{OMX_IndexConfigOtherPower,"OtherPower"},
	{OMX_IndexConfigTimeScale,"TimeScale"},
	{OMX_IndexConfigTimeClockState,"TimeClockState"},
	{OMX_IndexConfigTimeActiveRefClock,"TimeActiveRefClock"},
	{OMX_IndexConfigTimeCurrentMediaTime,"TimeCurrentMediaTime"},
	{OMX_IndexConfigTimeCurrentWallTime,"TimeCurrentWallTime"},
	{OMX_IndexConfigTimeCurrentAudioReference,"TimeCurrentAudioReference"},
	{OMX_IndexConfigTimeCurrentVideoReference,"TimeCurrentVideoReference"},
	{OMX_IndexConfigTimeMediaTimeRequest,"TimeMediaTimeRequest"},
	{OMX_IndexConfigTimeClientStartTime,"TimeClientStartTime"},
	{OMX_IndexConfigTimePosition,"TimePosition"},
	{OMX_IndexConfigTimeSeekMode,"TimeSeekMode"},
	{OMX_IndexKhronosExtensions,"KhronosExtensions"},
	{OMX_IndexVendorStartUnused,"VendorStartUnused"},
	{OMX_IndexParamMediaSeekable,"FSL_MediaSeekable"},
	{OMX_IndexParamMediaDuration,"FSL_MediaDuration"},
	{OMX_IndexParamTrackDuration,"FSL_TrackDuration"},
	{OMX_IndexConfigParserSendAudioFirst,"FSL_SendAudioFirst"},
	{OMX_IndexConfigCaptureFrame,"FSL_CaptureFrame"},
	{OMX_IndexOutputMode,"FSL_OutputMode"},
	{OMX_IndexSysSleep,"FSL_SysSleep"},
	{OMX_IndexParamAudioAc3,"FSL_AudioAc3"},
	{OMX_IndexConfigAudioPostProcess,"FSL_AudioPostProcess"},
	{OMX_IndexParamAudioSink,"FSL_AudioSink"},
	{OMX_IndexConfigClock,"FSL_Clock"},
	{OMX_IndexConfigVideoOutBufPhyAddr,"FSL_VideoOutBufPhyAddr"},
	{OMX_IndexParamAudioFlac,"FSL_AudioFlac"},
	{OMX_IndexParamMemOperator,"FSL_MemOperator"},
	{OMX_IndexConfigAbortBuffering,"FSL_AbortBuffering"},
	{OMX_IndexParamVideoCamera,"FSL_VideoCamera"},
	{OMX_IndexParamVideoCameraId,"FSL_VideoCameraId"},
	{OMX_IndexParamVideoSurface,"FSL_VideoSurface"},
	{OMX_IndexParamMaxFileDuration,"FSL_MaxFileDuration"},
	{OMX_IndexParamMaxFileSize,"FSL_MaxFileSize"},
	{OMX_IndexParamInterleaveUs,"FSL_InterleaveUs"},
	{OMX_IndexParamIsGetMetadata,"FSL_IsGetMetadata"},
	{OMX_IndexParamSurface,"FSL_Surface"},
	{OMX_IndexConfigEOS,"FSL_EOS"},
	{OMX_IndexParamAudioSource,"FSL_AudioSource"},
	{OMX_IndexConfigMaxAmplitude,"FSL_MaxAmplitude"},
	{OMX_IndexParamDecoderPlayMode,"FSL_DecoderPlayMode"},
	{OMX_IndexParamTimeLapseUs,"FSL_TimeLapseUs"},
	{OMX_IndexParamAudioWmaExt,"FSL_AudioWmaExt"},
	{OMX_IndexParamVideoDecChromaAlign,"FSL_VideoDecChromaAlign"},
	{OMX_IndexParamVideoCameraProxy,"FSL_VideoCameraProxy"},
	{OMX_IndexMax,"Max"},
	{OMX_MAP_INVALID_VALUE,"Index***"},
};

static OMX_STRING DBG_MAP_STR(OMX_U32 nValue,OMX_MAP_NODE* pNodeBase)
{
	OMX_MAP_NODE* pNode=pNodeBase;
	do{
		if(pNode->value==OMX_MAP_INVALID_VALUE)
			break;
		if(nValue==pNode->value)
			break;
		pNode++;
	}while(1);
	return (OMX_STRING)pNode->str;
}
#endif

static OMX_ERRORTYPE BaseGetComponentVersion(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STRING pComponentName,
        OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
        OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
        OMX_OUT OMX_UUIDTYPE* pComponentUUID)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"GetComponentVersion: ComponentName: %s \r\n",pComponentName);
    return base->CurState->GetVersion(pComponentName, pComponentVersion, pSpecVersion, pComponentUUID);
}


static OMX_ERRORTYPE BaseSendCommand(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_COMMANDTYPE Cmd,
            OMX_IN  OMX_U32 nParam1,
            OMX_IN  OMX_PTR pCmdData)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"SendCommand: Cmd: 0x%X(%s), Param: 0x%X, CmdData: 0x%X \r\n",Cmd,DBG_MAP_STR((OMX_U32)Cmd,&omx_map_cmd_str[0]),nParam1,pCmdData);
    return base->CurState->SendCommand(Cmd, nParam1, pCmdData);
}


static OMX_ERRORTYPE BaseGetParameter(
            OMX_IN  OMX_HANDLETYPE hComponent, 
            OMX_IN  OMX_INDEXTYPE nParamIndex,  
            OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    if(nParamIndex==OMX_IndexParamPortDefinition)
    {
        OMX_COMP_API_LOG(base->name,"GetParameter: ParamIndex: 0x%X(%s), PortIndex: %d \r\n",nParamIndex,DBG_MAP_STR((OMX_U32)nParamIndex,&omx_map_index_str[0]),((OMX_PARAM_PORTDEFINITIONTYPE*)pComponentParameterStructure)->nPortIndex);
    }
    else
    {
        OMX_COMP_API_LOG(base->name,"GetParameter: ParamIndex: 0x%X(%s) \r\n",nParamIndex,DBG_MAP_STR((OMX_U32)nParamIndex,&omx_map_index_str[0]));
    }
    return base->CurState->GetParameter(nParamIndex, pComponentParameterStructure);
}


static OMX_ERRORTYPE BaseSetParameter(
            OMX_IN  OMX_HANDLETYPE hComponent, 
            OMX_IN  OMX_INDEXTYPE nIndex,
            OMX_IN  OMX_PTR pComponentParameterStructure)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    if(nIndex==OMX_IndexParamPortDefinition)
    {
        OMX_COMP_API_LOG(base->name,"SetParameter: Index: 0x%X(%s), PortIndex: %d \r\n",nIndex,DBG_MAP_STR((OMX_U32)nIndex,&omx_map_index_str[0]),((OMX_PARAM_PORTDEFINITIONTYPE*)pComponentParameterStructure)->nPortIndex);
    }
    else
    {
        OMX_COMP_API_LOG(base->name,"SetParameter: Index: 0x%X(%s) \r\n",nIndex,DBG_MAP_STR((OMX_U32)nIndex,&omx_map_index_str[0]));
    }
    return base->CurState->SetParameter(nIndex, pComponentParameterStructure);
}


static OMX_ERRORTYPE BaseGetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex, 
            OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"GetConfig: Index: 0x%X(%s) \r\n",nIndex,DBG_MAP_STR((OMX_U32)nIndex,&omx_map_index_str[0]));
    return base->CurState->GetConfig(nIndex, pComponentConfigStructure);
}


static OMX_ERRORTYPE BaseSetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex, 
            OMX_IN  OMX_PTR pComponentConfigStructure)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"SetConfig: Index: 0x%X(%s) \r\n",nIndex,DBG_MAP_STR((OMX_U32)nIndex,&omx_map_index_str[0]));	
    return base->CurState->SetConfig(nIndex, pComponentConfigStructure);
}


static OMX_ERRORTYPE BaseGetExtensionIndex(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"GetExtensionIndex: ParamName: %s \r\n",cParameterName);
    return base->CurState->GetExtensionIndex(cParameterName, pIndexType);
}


static OMX_ERRORTYPE BaseGetState(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_OUT OMX_STATETYPE* pState)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"GetState: \r\n");
    return base->GetState(pState);
}

static OMX_ERRORTYPE BaseComponentTunnelRequest(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_U32 nPort,
        OMX_IN  OMX_HANDLETYPE hTunneledComp,
        OMX_IN  OMX_U32 nTunneledPort,
        OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    if(hTunneledComp==NULL){
        OMX_COMP_API_LOG(base->name,"ComponentTunnelRequest: TunneledComponent: NULL, Port: 0x%X, TunnelFlag: 0x%X, TunnelSupplier: 0x%X \r\n",
            nTunneledPort,pTunnelSetup->nTunnelFlags,pTunnelSetup->eSupplier);
    }
    else{
        OMX_COMP_API_LOG(base->name,"ComponentTunnelRequest: Port: 0x%X, TunneledComponent: %s, TunneledPort: 0x%X, TunnelFlag: 0x%X, TunnelSupplier: 0x%X \r\n",nPort,
            ((ComponentBase*)(((OMX_COMPONENTTYPE*)hTunneledComp)->pComponentPrivate))->name,nTunneledPort,pTunnelSetup->nTunnelFlags,pTunnelSetup->eSupplier);
    }
    return base->CurState->TunnelRequest(nPort, hTunneledComp, nTunneledPort, pTunnelSetup);
}


static OMX_ERRORTYPE BaseUseBuffer(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"UseBuffer: Port: 0x%X, Buffer: 0x%X, Size: 0x%X \r\n",nPortIndex,pBuffer,nSizeBytes);
    return base->CurState->UseBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
}


static OMX_ERRORTYPE BaseAllocateBuffer(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"AllocateBuffer: Port: 0x%X, Size: 0x%X \r\n",nPortIndex,nSizeBytes);
    return base->CurState->AllocateBuffer(ppBuffer, nPortIndex, pAppPrivate, nSizeBytes);
}


static OMX_ERRORTYPE BaseFreeBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"FreeBuffer: Port: 0x%X, Buffer: 0x%X, Size: 0x%X \r\n",nPortIndex,pBuffer->pBuffer,pBuffer->nAllocLen);
    return base->CurState->FreeBuffer(nPortIndex, pBuffer);
}


static OMX_ERRORTYPE BaseEmptyThisBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"EmptyThisBuffer: Buffer: 0x%X, Offset: 0x%X, FilledLen: 0x%X(%d), Flag: 0x%X \r\n",pBuffer->pBuffer,pBuffer->nOffset,pBuffer->nFilledLen,pBuffer->nFilledLen,pBuffer->nFlags);
    return base->CurState->EmptyThisBuffer(pBuffer);
}


static OMX_ERRORTYPE BaseFillThisBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"FillThisBuffer: Buffer: 0x%X, AllocLen: 0x%X \r\n",pBuffer->pBuffer,pBuffer->nAllocLen);
    return base->CurState->FillThisBuffer(pBuffer);
}


static OMX_ERRORTYPE BaseSetCallbacks(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_CALLBACKTYPE* pCbs, 
            OMX_IN  OMX_PTR pAppData)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"SetCallbacks: \r\n");
    return base->SetCallbacks(pCbs, pAppData);
}


static OMX_ERRORTYPE BaseComponentDeInit(
            OMX_IN  OMX_HANDLETYPE hComponent)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"ComponentDeInit: \r\n");
    return base->DestructComponent();
}


static OMX_ERRORTYPE BaseUseEGLImage(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN void* eglImage)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"UseEGLImage: Port: 0x%X, EGLImage: 0x%X \r\n",nPortIndex,eglImage);
    return base->UseEGLImage(ppBufferHdr, nPortIndex, pAppPrivate, eglImage);
}


static OMX_ERRORTYPE BaseComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
		OMX_OUT OMX_U8 *cRole,
		OMX_IN OMX_U32 nIndex)
{
    ComponentBase *base = NULL;
    OMX_COMPONENTTYPE *hComp = NULL;

    if(hComponent == NULL)
        return OMX_ErrorInvalidComponent;

    hComp = (OMX_COMPONENTTYPE*)hComponent;
    base = (ComponentBase*)(hComp->pComponentPrivate);
    OMX_COMP_API_LOG(base->name,"ComponentRoleEnum: Index: 0x%X \r\n",nIndex);
    return base->ComponentRoleEnum(cRole, nIndex);
}


void *DoThread(void *arg) 
{
    ComponentBase *base = (ComponentBase*)arg;
    OMX_ERRORTYPE ret1, ret2;

    while(1) {
        base->Down();
        if(base->bStopThread == OMX_TRUE)
            break;
        ret1 = ret2 = OMX_ErrorNone;
        LOG_DEBUG("Thread processing.\n");
        while(ret1 == OMX_ErrorNone || ret2 == OMX_ErrorNone) {
            ret1 = base->CurState->ProcessCmd();
            ret2 = base->CurState->ProcessBuffer();
        }
        LOG_DEBUG("Thread processing done.\n");
    }

    LOG_DEBUG("Thread is stoped.\n");

    return NULL;
}

/**< done internally used functions */

ComponentBase::ComponentBase()
{
    OMX_U32 i;

    pCmdQueue = NULL;
    for(i=0; i<MAX_PORT_NUM; i++)
        ports[i] = NULL;
    states[InvalidStateIdx] = NULL;
    states[LoadedStateIdx] = NULL;
    states[IdleStateIdx] = NULL;
    states[ExecutingStateIdx] = NULL;
    states[PauseStateIdx] = NULL;
    states[WaitForResourcesStateIdx] = NULL;
    inContextMutex = NULL;
    pThreadId = NULL;
    outContextSem = NULL;
    SemLock = NULL;
}

OMX_ERRORTYPE ComponentBase::ConstructComponent(
        OMX_HANDLETYPE pHandle)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
    OMX_U32 i;

    if(pHandle == NULL)
        return OMX_ErrorBadParameter;

    SpecVersion.s.nVersionMajor = 0x1;
    SpecVersion.s.nVersionMinor = 0x1;
    SpecVersion.s.nRevision = 0x2;
    SpecVersion.s.nStep = 0x0;

    hComponent = (OMX_COMPONENTTYPE*)pHandle;
    OMX_CHECK_STRUCT(hComponent, OMX_COMPONENTTYPE, ret);
    if(ret != OMX_ErrorNone)
        return ret;

    hComponent->pComponentPrivate = this;
    hComponent->GetComponentVersion = BaseGetComponentVersion;
    hComponent->SendCommand = BaseSendCommand;
    hComponent->GetParameter = BaseGetParameter;
    hComponent->SetParameter = BaseSetParameter;
    hComponent->GetConfig = BaseGetConfig;
    hComponent->SetConfig = BaseSetConfig;
    hComponent->GetExtensionIndex = BaseGetExtensionIndex;
    hComponent->GetState = BaseGetState;
    hComponent->ComponentTunnelRequest = BaseComponentTunnelRequest;
    hComponent->UseBuffer = BaseUseBuffer;
    hComponent->AllocateBuffer = BaseAllocateBuffer;
    hComponent->FreeBuffer = BaseFreeBuffer;
    hComponent->EmptyThisBuffer = BaseEmptyThisBuffer;
    hComponent->FillThisBuffer = BaseFillThisBuffer;
    hComponent->SetCallbacks = BaseSetCallbacks;
    hComponent->ComponentDeInit = BaseComponentDeInit;
    hComponent->UseEGLImage = BaseUseEGLImage;
    hComponent->ComponentRoleEnum = BaseComponentRoleEnum;

    pendingCmd.cmd = OMX_CommandMax;

    /**< setup cmd queue */
    pCmdQueue = FSL_NEW(Queue, ());
    if(pCmdQueue == NULL) {
        LOG_ERROR("New cmd queue failed.\n");
        ret = OMX_ErrorInsufficientResources;
        goto err;
    }

    if(pCmdQueue->Create(32, sizeof(CMD_MSG), E_FSL_OSAL_TRUE) != QUEUE_SUCCESS) {
        LOG_ERROR("Init cmd queue failed.\n");
        ret = OMX_ErrorUndefined;
        goto err;
    }

    /**< setup ports */
    if(nPorts > MAX_PORT_NUM) {
        LOG_ERROR("Requried port number: %d more than defined: %d\n", nPorts, MAX_PORT_NUM);
        ret = OMX_ErrorBadParameter;
        goto err;
    }

    for(i=0; i<nPorts; i++) {
        ports[i] = FSL_NEW(Port, (this, i));
        if(ports[i] == NULL) {
            LOG_ERROR("Create ports[%d] failed.\n", i);
            ret = OMX_ErrorInsufficientResources;
            goto err;
        }

        ret = ports[i]->Init();
        if(ret != OMX_ErrorNone) {
            LOG_ERROR("Init port [%d] failed\n", i);
            goto err;
        }
    }

    /**< setup states */
    states[InvalidStateIdx] = FSL_NEW(InvalidState, (this));
    states[LoadedStateIdx] = FSL_NEW(LoadedState, (this));
    states[IdleStateIdx] = FSL_NEW(IdleState, (this));
    states[ExecutingStateIdx] = FSL_NEW(ExecutingState, (this));
    states[PauseStateIdx] = FSL_NEW(PauseState, (this));
    states[WaitForResourcesStateIdx] = FSL_NEW(WaitForResourcesState, (this));
    if(states[InvalidStateIdx] == NULL 
            || states[LoadedStateIdx] == NULL
            || states[IdleStateIdx] == NULL 
            || states[ExecutingStateIdx] == NULL
            || states[PauseStateIdx] == NULL
            || states[WaitForResourcesStateIdx] == NULL) {
        LOG_ERROR("Create states failed.\n");
        ret = OMX_ErrorInsufficientResources;
        goto err;
    }

    /**< setup mutex and thread for message processing */
    if(bInContext == OMX_TRUE) {
        if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&inContextMutex, fsl_osal_mutex_normal)) {
            LOG_ERROR("Create inContext Mutex failed.\n");
            ret = OMX_ErrorInsufficientResources;
            goto err;
        }
    }
    else {
        SemCnt = 0;
        bStopThread = OMX_FALSE;

        if(E_FSL_OSAL_SUCCESS != fsl_osal_sem_init(&outContextSem, 0, 0)) {
            LOG_ERROR("Create outContext Semphore failed.\n");
            ret = OMX_ErrorInsufficientResources;
            goto err;
        }

        if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&SemLock, fsl_osal_mutex_normal)) {
            LOG_ERROR("Create Semphore Mutex failed.\n");
            ret = OMX_ErrorInsufficientResources;
            goto err;
        }

        if(E_FSL_OSAL_SUCCESS != fsl_osal_thread_create(&pThreadId, NULL, DoThread, this)) {
            LOG_ERROR("Create outContext Thread failed.\n");
            ret = OMX_ErrorInsufficientResources;
            goto err;
        }
    }

    ret = InitComponent();
    if(ret != OMX_ErrorNone)
        goto err;

    for(i=0; i<TOTAL_PORT_PARM; i++) {
        OMX_INIT_STRUCT(&PortParam[i], OMX_PORT_PARAM_TYPE);
        PortParam[i].nStartPortNumber = OMX_ALL;
    }

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    for(i=0; i<nPorts; i++) {
        sPortDef.nPortIndex = i;
        ports[i]->GetPortDefinition(&sPortDef);
        switch(sPortDef.eDomain) {
            case OMX_PortDomainAudio:
                PortParam[AudioPortParamIdx].nPorts ++;
                if(PortParam[AudioPortParamIdx].nStartPortNumber == OMX_ALL)
                    PortParam[AudioPortParamIdx].nStartPortNumber = i;
                break;
            case OMX_PortDomainVideo:
                PortParam[VideoPortParamIdx].nPorts ++;
                if(PortParam[VideoPortParamIdx].nStartPortNumber == OMX_ALL)
                    PortParam[VideoPortParamIdx].nStartPortNumber = i;
                break;
            case OMX_PortDomainImage: 
                PortParam[ImagePortParamIdx].nPorts ++;
                if(PortParam[ImagePortParamIdx].nStartPortNumber == OMX_ALL)
                    PortParam[ImagePortParamIdx].nStartPortNumber = i;
                break;
            case OMX_PortDomainOther:
                PortParam[OtherPortParamIdx].nPorts ++;
                if(PortParam[OtherPortParamIdx].nStartPortNumber == OMX_ALL)
                    PortParam[OtherPortParamIdx].nStartPortNumber = i;
                break;
            default:
                break;
        }
    }

    /**< set state to loaded */
    eCurState = OMX_StateLoaded;
    CurState = states[LoadedStateIdx];
    LOG_DEBUG("Component %s is loaded.\n", name);

    return ret;
err:
    DestructComponent();
    return ret;
}

OMX_ERRORTYPE ComponentBase::DestructComponent()
{
    OMX_U32 i;

    /**< free resources allocated by components */
    DeInitComponent();

    /**< free mutex and thread for message processing*/
    if(bInContext == OMX_TRUE) {
        if(inContextMutex != NULL)
            fsl_osal_mutex_destroy(inContextMutex);
    }
    else {
        if(pThreadId != NULL) {
            bStopThread = OMX_TRUE;
            Up();
            fsl_osal_thread_destroy(pThreadId);
        }
        if(outContextSem != NULL)
            fsl_osal_sem_destroy(outContextSem);
        if(SemLock != NULL)
            fsl_osal_mutex_destroy(SemLock);
    }

    /**< free cmd queue */
    if(pCmdQueue != NULL) {
        pCmdQueue->Free();
        FSL_DELETE(pCmdQueue);
    }

    /**< free ports */
    for(i=0; i<nPorts; i++) {
        if(ports[i] != NULL) {
            ports[i]->DeInit();
            FSL_DELETE(ports[i]);
        }
    }

    /**< free states */
    if(states[InvalidStateIdx] != NULL)
        FSL_DELETE(states[InvalidStateIdx]);
    if(states[LoadedStateIdx] != NULL)
        FSL_DELETE(states[LoadedStateIdx]);
    if(states[IdleStateIdx] != NULL)
        FSL_DELETE(states[IdleStateIdx]);
    if(states[ExecutingStateIdx] != NULL)
        FSL_DELETE(states[ExecutingStateIdx]);
    if(states[PauseStateIdx] != NULL)
        FSL_DELETE(states[PauseStateIdx]);
    if(states[WaitForResourcesStateIdx] != NULL)
        FSL_DELETE(states[WaitForResourcesStateIdx]);

    LOG_DEBUG("%s destroyed.\n", name);

    ComponentBase *own = this;
    FSL_DELETE(own);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE ComponentBase::SetCallbacks(
        OMX_CALLBACKTYPE* pCbs, 
        OMX_PTR pAppData)
{
    pCallbacks = pCbs;
    hComponent->pApplicationPrivate = pAppData;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ComponentRoleEnum(
        OMX_U8 *cRole, OMX_U32 nIndex)
{
    if(nIndex >= role_cnt)
        return OMX_ErrorBadParameter;

    fsl_osal_memcpy(cRole, role[nIndex], MAX_ROLE_NAME_LEN);

    return OMX_ErrorNone;
}

OMX_HANDLETYPE ComponentBase::GetComponentHandle()
{
    return (OMX_HANDLETYPE) hComponent;
}

OMX_ERRORTYPE ComponentBase::GetState(
        OMX_STATETYPE *pState)
{
    if(pState == NULL) {
        LOG_ERROR("pState is null in funtion GetState.\n");
        return OMX_ErrorBadParameter;
    }

    *pState = eCurState;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::SetState(
        OMX_STATETYPE eState)
{
    eCurState = eState;
    CurState = states[eState - OMX_StateInvalid];
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::PortNotify(OMX_U32 nPortIndex, PORT_NOTIFY_TYPE ePortNotify)
{
    OMX_BOOL bCmdComplete = OMX_FALSE;

    /**< if Port is populated*/
    if(ePortNotify == PORT_ON) {
        if(pendingCmd.cmd == OMX_CommandStateSet && pendingCmd.nParam == OMX_StateIdle) {
            if(OMX_TRUE == CheckAllPortsState(PORT_ON))
                bCmdComplete = OMX_TRUE;
        }

        if(pendingCmd.cmd == OMX_CommandPortEnable) {
            if(pendingCmd.nParam == OMX_ALL) {
                if(OMX_TRUE == CheckAllPortsState(PORT_ON))
                    bCmdComplete = OMX_TRUE;
            }
            else if(pendingCmd.nParam == nPortIndex) {
                bCmdComplete = OMX_TRUE;
            }
        }
    }
    /**< if Port is un-populated*/
    else if(ePortNotify == PORT_OFF) {
        if(pendingCmd.cmd == OMX_CommandStateSet && pendingCmd.nParam == OMX_StateLoaded) {
            if(OMX_TRUE == CheckAllPortsState(PORT_OFF)) {
                bCmdComplete = OMX_TRUE;
                DoIdle2Loaded();
            }
        }

        /* Failed to switch Loaded to Idle */
        if(pendingCmd.cmd == OMX_CommandStateSet && pendingCmd.nParam == OMX_StateIdle && eCurState == OMX_StateLoaded) {
            if(OMX_TRUE == CheckAllPortsState(PORT_OFF)) {
                DoIdle2Loaded();
                pendingCmd.cmd = OMX_CommandMax;
            }
        }

        if(pendingCmd.cmd == OMX_CommandPortDisable) {
            if(pendingCmd.nParam == OMX_ALL) {
                if(OMX_TRUE == CheckAllPortsState(PORT_ON))
                    bCmdComplete = OMX_TRUE;
            }
            else if(pendingCmd.nParam == nPortIndex) {
                bCmdComplete = OMX_TRUE;
            }
        }
    }
    else if(ePortNotify == BUFFER_RETURNED)
        bCmdComplete = OMX_TRUE;

    if(bCmdComplete == OMX_TRUE) {
        SendEvent(OMX_EventCmdComplete, pendingCmd.cmd, pendingCmd.nParam, pendingCmd.pCmdData);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::MarkOtherPortsBuffer(
        OMX_MARKTYPE *pMarkData)
{
    OMX_U32 i;

    for(i=0; i<nPorts; i++) {
        if(OMX_DirOutput == ports[i]->GetPortDir())
            ports[i]->MarkBuffer(pMarkData);
    }

    return OMX_ErrorNone;
}

OMX_BOOL ComponentBase::bHasCmdToProcess()
{
    if(pCmdQueue == NULL)
        return OMX_FALSE;

    if(pCmdQueue->Size() > 0)
        return OMX_TRUE;
    else
        return OMX_FALSE;
}

OMX_BOOL ComponentBase::CheckAllPortsState(
        PORT_NOTIFY_TYPE ePortNotify)
{
    OMX_BOOL ret = OMX_TRUE;
    OMX_U32 i;

    for(i=0; i<nPorts; i++) {
        if(ports[i]->IsEnabled() != OMX_TRUE)
            continue;

        if(ePortNotify == PORT_ON) {
            if(ports[i]->IsPopulated() == OMX_FALSE) {
                ret = OMX_FALSE;
                break;
            }
        }
        else if(ePortNotify == PORT_OFF) {
            if(ports[i]->IsPopulated() == OMX_TRUE) {
                ret = OMX_FALSE;
                break;
            }
        }
    }

    return ret;
}

OMX_ERRORTYPE ComponentBase::FlushComponent(
        OMX_U32 nPortIndex) 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ComponentReturnBuffer(
        OMX_U32 nPortIndex)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::UseEGLImage(
        OMX_BUFFERHEADERTYPE** ppBufferHdr, 
        OMX_U32 nPortIndex, 
        OMX_PTR pAppPrivate, 
        void* eglImage)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::SendEvent(
        OMX_EVENTTYPE eEvent, 
        OMX_U32 nData1, 
        OMX_U32 nData2, 
        OMX_PTR pEventData)
{
    if(eEvent == OMX_EventCmdComplete) {
        pendingCmd.cmd = OMX_CommandMax;
        if(nData1 == OMX_CommandStateSet)
            SetState((OMX_STATETYPE)nData2);
    }

    if(eEvent == OMX_EventError) {
        /* Failed to switch Loaded to Idle */
        if(pendingCmd.cmd == OMX_CommandStateSet && pendingCmd.nParam == OMX_StateIdle && eCurState == OMX_StateLoaded) {
            if(OMX_TRUE == CheckAllPortsState(PORT_OFF)) {
                DoIdle2Loaded();
                pendingCmd.cmd = OMX_CommandMax;
            }
        }
    }

    pCallbacks->EventHandler(hComponent, hComponent->pApplicationPrivate, 
            eEvent, nData1, nData2, pEventData);

    LOG_DEBUG("SendEvent to client, [eEvent: %d, nData1: %d, nData2: %d]\n", eEvent, nData1, nData2);
    OMX_COMP_CALLBACK_LOG(((ComponentBase*)(hComponent->pComponentPrivate))->name,"SendEvent to client, [eEvent: %d(%s), nData1: %d, nData2: %d] \r\n", eEvent,DBG_MAP_STR((OMX_U32)eEvent,&omx_map_event_str[0]), nData1, nData2);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::EmptyDone(
        OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    pCallbacks->EmptyBufferDone(hComponent, hComponent->pApplicationPrivate, pBufferHdr);

    LOG_DEBUG("EmptyDone to client, buffer[%p]\n", pBufferHdr);
    OMX_COMP_CALLBACK_LOG(((ComponentBase*)(hComponent->pComponentPrivate))->name,"EmptyDone to buffer: 0x%X \r\n", pBufferHdr->pBuffer);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::FillDone(
        OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    pCallbacks->FillBufferDone(hComponent, hComponent->pApplicationPrivate, pBufferHdr);

    LOG_DEBUG("FillDone to client, buffer[%p]\n", pBufferHdr);
    OMX_COMP_CALLBACK_LOG(((ComponentBase*)(hComponent->pComponentPrivate))->name,"FillDone to buffer: 0x%X \r\n", pBufferHdr->pBuffer);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    return OMX_ErrorUnsupportedIndex;
}

OMX_ERRORTYPE ComponentBase::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    return OMX_ErrorUnsupportedIndex;
}

OMX_ERRORTYPE ComponentBase::GetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    return OMX_ErrorUnsupportedIndex;
}

OMX_ERRORTYPE ComponentBase::SetConfig(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pComponentParameterStructure)
{
    return OMX_ErrorUnsupportedIndex;
}

OMX_ERRORTYPE ComponentBase::GetExtensionIndex(
        OMX_STRING cParameterName, 
        OMX_INDEXTYPE* pIndexType)
{
    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::PortFormatChanged(
        OMX_U32 nPortIndex)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::DoAllocateBuffer(
        OMX_PTR *buffer,
        OMX_U32 nSize,
        OMX_U32 nPortIndex)
{
    OMX_PTR ptr = NULL;

    if(buffer == NULL) {
        LOG_ERROR("buffer pointer is null in function DoAllocateBuffer.\n");
        return OMX_ErrorBadParameter;
    }

    ptr = FSL_MALLOC(nSize);
    if(ptr == NULL) {
        LOG_ERROR("Allocate memory failed, size: %d\n", nSize);
        return OMX_ErrorInsufficientResources;
    }

    fsl_osal_memset(ptr, 0, nSize);
    *buffer = ptr;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::DoFreeBuffer(
        OMX_PTR buffer,
        OMX_U32 nPortIndex) 
{
    FSL_FREE(buffer);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::DoLoaded2Idle() 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::DoIdle2Loaded() 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::InstanceInit() 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::InstanceDeInit() 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::DoExec2Pause() 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::DoPause2Exec() 
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessInContext(
        MSG_TYPE msg)
{
    OMX_ERRORTYPE ret1, ret2;

    if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_trylock(inContextMutex)) {
        fsl_osal_u32 id = 0;
        fsl_osal_thread_self(&id);
        if(id == InContextThreadId) {
            DelayedInContextMsgList.Add(&msg);
            return OMX_ErrorNone;
        }
        else
            fsl_osal_mutex_lock(inContextMutex);
    }

    fsl_osal_thread_self(&InContextThreadId);

    while(1) {
        if(msg == COMMAND)
            CurState->ProcessCmd();
        else if(msg == BUFFER)
            CurState->ProcessBuffer();

        if(DelayedInContextMsgList.GetNodeCnt() == 0)
            break;

        MSG_TYPE *pMsg = NULL;
        pMsg = DelayedInContextMsgList.GetNode(0);
        msg = *pMsg;
        DelayedInContextMsgList.Remove(pMsg);
    };

    fsl_osal_mutex_unlock(inContextMutex);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessOutContext(
        MSG_TYPE msg) 
{
    Up();
        
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::Up() 
{
    fsl_osal_mutex_lock(SemLock);
    if(SemCnt == 0) {
        SemCnt = 1;
        fsl_osal_sem_post(outContextSem);
    }
    fsl_osal_mutex_unlock(SemLock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::Down() 
{
    fsl_osal_sem_wait(outContextSem);
    fsl_osal_mutex_lock(SemLock);
    SemCnt --;
    fsl_osal_mutex_unlock(SemLock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessClkBuffer() 
{
    return OMX_ErrorNoMore;
}


/* File EOF */
