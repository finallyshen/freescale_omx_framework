/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
 
#include "VideoSource.h"
 
#define CHECK_STRUCT(ps,t,r) \
    do { \
        OMX_CHECK_STRUCT(ps, t, r); \
        if(r != OMX_ErrorNone) \
        break; \
        if(ps->nPortIndex != CAPTURED_FRAME_PORT) { \
            r = OMX_ErrorBadPortIndex; \
            break; \
        } \
    } while(0);

VideoSource::VideoSource()
{
    bInContext = OMX_FALSE;
    nPorts = NUM_PORTS;
}

OMX_ERRORTYPE VideoSource::InitComponent()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = PREVIEW_PORT;
    sPortDef.eDir = OMX_DirOutput;
    sPortDef.eDomain = OMX_PortDomainVideo;
    fsl_osal_memcpy(&sPortDef.format.video, &sVideoFmt, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_FALSE;
    sPortDef.nBufferCountMin = nFrameBufferMin;
    sPortDef.nBufferCountActual = nFrameBufferActual;
    sPortDef.nBufferSize = sPortDef.format.video.nFrameWidth
        * sPortDef.format.video.nFrameHeight
        * pxlfmt2bpp(sVideoFmt.eColorFormat) / 8;
    ret = ports[PREVIEW_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for in port failed.\n");
        return ret;
    }

    ports[PREVIEW_PORT]->SetSupplierType(PreviewPortSupplierType);

    OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    sPortDef.nPortIndex = CAPTURED_FRAME_PORT;
    sPortDef.eDir = OMX_DirOutput;
    sPortDef.eDomain = OMX_PortDomainVideo;
    fsl_osal_memcpy(&sPortDef.format.video, &sVideoFmt, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_TRUE;
    sPortDef.nBufferCountMin = nFrameBufferMin;
    sPortDef.nBufferCountActual = nFrameBufferActual;
    sPortDef.nBufferSize = sPortDef.format.video.nFrameWidth
        * sPortDef.format.video.nFrameHeight
        * pxlfmt2bpp(sVideoFmt.eColorFormat) / 8;
    ret = ports[CAPTURED_FRAME_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for in port failed.\n");
        return ret;
    }

    ports[CAPTURED_FRAME_PORT]->SetSupplierType(CapturePortSupplierType);

    sPortDef.nPortIndex = CLK_PORT;
    sPortDef.eDir = OMX_DirInput;
    sPortDef.eDomain = OMX_PortDomainOther;
    sPortDef.format.other.eFormat = OMX_OTHER_FormatTime;
    sPortDef.bPopulated = OMX_FALSE;
    sPortDef.bEnabled = OMX_FALSE;
    sPortDef.nBufferCountMin = 1;
    sPortDef.nBufferCountActual = 1;
    sPortDef.nBufferSize = sizeof(OMX_TIME_MEDIATIMETYPE);
    ret = ports[CLK_PORT]->SetPortDefinition(&sPortDef);
    if(ret != OMX_ErrorNone) {
        LOG_ERROR("Set port definition for clk port failed.\n");
        return ret;
    }

	OMX_INIT_STRUCT(&SensorMode, OMX_PARAM_SENSORMODETYPE);

	SensorMode.nPortIndex = OMX_ALL;
	SensorMode.nFrameRate = 30 * Q16_SHIFT;
	SensorMode.bOneShot = OMX_FALSE;

	OMX_INIT_STRUCT(&PortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);

	PortFormat.nPortIndex = OMX_ALL;
	PortFormat.nIndex = 0;
	PortFormat.eCompressionFormat = OMX_VIDEO_CodingUnused;
	PortFormat.eColorFormat = OMX_COLOR_FormatYUV420Planar;
	PortFormat.xFramerate = 30 * Q16_SHIFT;

	OMX_INIT_STRUCT(&WhiteBalControl, OMX_CONFIG_WHITEBALCONTROLTYPE);

	WhiteBalControl.nPortIndex = OMX_ALL;
	WhiteBalControl.eWhiteBalControl = OMX_WhiteBalControlOff;

	OMX_INIT_STRUCT(&ScaleFactor, OMX_CONFIG_SCALEFACTORTYPE);

	ScaleFactor.nPortIndex = OMX_ALL;
	ScaleFactor.xWidth = 1.0 * Q16_SHIFT;
	ScaleFactor.xHeight = 1.0 * Q16_SHIFT;

	OMX_INIT_STRUCT(&ExposureValue, OMX_CONFIG_EXPOSUREVALUETYPE);

	ExposureValue.nPortIndex = OMX_ALL;
	ExposureValue.eMetering = OMX_MeteringModeAverage;
	ExposureValue.xEVCompensation = 1.0 * Q16_SHIFT;
	ExposureValue.nApertureFNumber = 1;
	ExposureValue.bAutoAperture = OMX_TRUE;
	ExposureValue.nShutterSpeedMsec = 33;
	ExposureValue.bAutoShutterSpeed = OMX_TRUE;
	ExposureValue.nSensitivity = 100;
	ExposureValue.bAutoSensitivity = OMX_TRUE;

	OMX_INIT_STRUCT(&Capturing, OMX_CONFIG_BOOLEANTYPE);

	Capturing.bEnabled = OMX_FALSE;

	OMX_INIT_STRUCT(&EOS, OMX_CONFIG_BOOLEANTYPE);

	EOS.bEnabled = OMX_FALSE;

	OMX_INIT_STRUCT(&AutoPauseAfterCapture, OMX_CONFIG_BOOLEANTYPE);

	AutoPauseAfterCapture.bEnabled = OMX_FALSE;

	OMX_INIT_STRUCT(&Rotation, OMX_CONFIG_ROTATIONTYPE);

	Rotation.nPortIndex = OMX_ALL;
	Rotation.nRotation = 0;

	bFirstFrame = OMX_TRUE;
	nFrameDelay = 0;
	nBaseTime = 0;
	nMediaTimestampPre = 0;
	pOutBufferHdr = NULL;
	nCaptureFrameCnt = 0;
	bSendEOS = OMX_FALSE;
	nMaxDuration = MAX_VALUE_S64;
	nTimeLapseUs = 0;
	nNextLapseTS = 0;
	nLastSendTS = 0;
	nFrameInterval = 0;
	cameraPtr = NULL;
	nCameraId = 0;
	previewSurface = NULL;

    ret = InitSourceComponent();
    if(ret != OMX_ErrorNone)
        return ret;

	return ret;
}

OMX_ERRORTYPE VideoSource::DeInitComponent()
{
    DeInitSourceComponent();
	return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoSource::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	
	nFrameInterval = (1000000LL / (sVideoFmt.xFramerate / Q16_SHIFT));
	StartDevice();

    return ret;
}

OMX_ERRORTYPE VideoSource::InstanceDeInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	StopDevice();

    return ret;
}

OMX_ERRORTYPE VideoSource::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
		case OMX_IndexParamCommonSensorMode:
            {
                OMX_PARAM_SENSORMODETYPE *pSensorMode;
                pSensorMode = (OMX_PARAM_SENSORMODETYPE*)pStructure;
                OMX_CHECK_STRUCT(pSensorMode, OMX_PARAM_SENSORMODETYPE, ret);
				fsl_osal_memcpy(pSensorMode, &SensorMode,	sizeof(OMX_PARAM_SENSORMODETYPE));
			}
			break;
		case OMX_IndexParamVideoPortFormat:
            {
				OMX_VIDEO_PARAM_PORTFORMATTYPE *pPortFormat;
				pPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)pStructure;
				OMX_CHECK_STRUCT(pPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE, ret);
				fsl_osal_memcpy(pPortFormat, &PortFormat,	sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
			}
			break;
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE VideoSource::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
		case OMX_IndexParamCommonSensorMode:
            {
                OMX_PARAM_SENSORMODETYPE *pSensorMode;
                pSensorMode = (OMX_PARAM_SENSORMODETYPE*)pStructure;
                OMX_CHECK_STRUCT(pSensorMode, OMX_PARAM_SENSORMODETYPE, ret);
				fsl_osal_memcpy(&SensorMode, pSensorMode, sizeof(OMX_PARAM_SENSORMODETYPE));
				SetSensorMode();
			}
			break;
		case OMX_IndexParamVideoPortFormat:
            {
				OMX_VIDEO_PARAM_PORTFORMATTYPE *pPortFormat;
				pPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)pStructure;
				OMX_CHECK_STRUCT(pPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE, ret);
				fsl_osal_memcpy(&PortFormat, pPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
				if (nParamIndex == CAPTURED_FRAME_PORT)
					SetVideoFormat();
			}
			break;
		case OMX_IndexParamVideoCamera:
            {
				 cameraPtr = (OMX_PTR)pStructure;
			}
			break;
		case OMX_IndexParamVideoCameraProxy:
            {
				 cameraProxyPtr = (OMX_PTR)pStructure;
			}
			break;
		case OMX_IndexParamVideoCameraId:
			{
				 nCameraId = *((OMX_S32*)pStructure);
			}
			break;
		case OMX_IndexParamVideoSurface:
			{
				previewSurface = (OMX_PTR)pStructure;
			}
			break;
		case OMX_IndexParamMaxFileDuration:
			{
				nMaxDuration = *((OMX_TICKS*)pStructure);
			}
			break;
		case OMX_IndexParamTimeLapseUs:
			{
				nTimeLapseUs = *((OMX_TICKS*)pStructure);
			}
			break;
		default:
			ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE VideoSource::GetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
		case OMX_IndexConfigCommonWhiteBalance:
			{
				OMX_CONFIG_WHITEBALCONTROLTYPE *pWhiteBalControl;
				pWhiteBalControl = (OMX_CONFIG_WHITEBALCONTROLTYPE*)pStructure;
				OMX_CHECK_STRUCT(pWhiteBalControl, OMX_CONFIG_WHITEBALCONTROLTYPE, ret);
				fsl_osal_memcpy(pWhiteBalControl, &WhiteBalControl, sizeof(OMX_CONFIG_WHITEBALCONTROLTYPE));
			}
            break;
        case OMX_IndexConfigCommonDigitalZoom:
             {
                OMX_CONFIG_SCALEFACTORTYPE *pScaleFactor;
                pScaleFactor = (OMX_CONFIG_SCALEFACTORTYPE*)pStructure;
                OMX_CHECK_STRUCT(pScaleFactor, OMX_CONFIG_SCALEFACTORTYPE, ret);
				fsl_osal_memcpy(pScaleFactor, &ScaleFactor, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
            }
            break;
        case OMX_IndexConfigCommonExposureValue:
             {
                OMX_CONFIG_EXPOSUREVALUETYPE *pExposureValue;
                pExposureValue = (OMX_CONFIG_EXPOSUREVALUETYPE*)pStructure;
                OMX_CHECK_STRUCT(pExposureValue, OMX_CONFIG_EXPOSUREVALUETYPE, ret);
				fsl_osal_memcpy(pExposureValue, &ExposureValue, sizeof(OMX_CONFIG_EXPOSUREVALUETYPE));
            }
            break;
        case OMX_IndexConfigCapturing:
             {
                OMX_CONFIG_BOOLEANTYPE *pCapturing;
                pCapturing = (OMX_CONFIG_BOOLEANTYPE*)pStructure;
                OMX_CHECK_STRUCT(pCapturing, OMX_CONFIG_BOOLEANTYPE, ret);
				fsl_osal_memcpy(pCapturing, &Capturing, sizeof(OMX_CONFIG_BOOLEANTYPE));
            }
            break;
        case OMX_IndexAutoPauseAfterCapture:
             {
                OMX_CONFIG_BOOLEANTYPE *pAutoPauseAfterCapture;
                pAutoPauseAfterCapture = (OMX_CONFIG_BOOLEANTYPE*)pStructure;
                OMX_CHECK_STRUCT(pAutoPauseAfterCapture, OMX_CONFIG_BOOLEANTYPE, ret);
				fsl_osal_memcpy(pAutoPauseAfterCapture, &AutoPauseAfterCapture, sizeof(OMX_CONFIG_BOOLEANTYPE));
            }
            break;
		case OMX_IndexConfigCommonRotate:
			 {
				 OMX_CONFIG_ROTATIONTYPE *pRotation;
				 pRotation = (OMX_CONFIG_ROTATIONTYPE*)pStructure;
				 OMX_CHECK_STRUCT(pRotation, OMX_CONFIG_ROTATIONTYPE, ret);
				 fsl_osal_memcpy(pRotation, &Rotation, sizeof(OMX_CONFIG_ROTATIONTYPE));
			 }
        default:
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

    return ret;
}

OMX_ERRORTYPE VideoSource::SetConfig(
        OMX_INDEXTYPE nParamIndex,
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    switch (nParamIndex) {
		case OMX_IndexConfigCommonWhiteBalance:
			{
				OMX_CONFIG_WHITEBALCONTROLTYPE *pWhiteBalControl;
				pWhiteBalControl = (OMX_CONFIG_WHITEBALCONTROLTYPE*)pStructure;
				OMX_CHECK_STRUCT(pWhiteBalControl, OMX_CONFIG_WHITEBALCONTROLTYPE, ret);
				fsl_osal_memcpy(&WhiteBalControl, pWhiteBalControl, sizeof(OMX_CONFIG_WHITEBALCONTROLTYPE));
				SetWhiteBalance();
			}
            break;
        case OMX_IndexConfigCommonDigitalZoom:
             {
                OMX_CONFIG_SCALEFACTORTYPE *pScaleFactor;
                pScaleFactor = (OMX_CONFIG_SCALEFACTORTYPE*)pStructure;
                OMX_CHECK_STRUCT(pScaleFactor, OMX_CONFIG_SCALEFACTORTYPE, ret);
				fsl_osal_memcpy(&ScaleFactor, pScaleFactor, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
				SetDigitalZoom();
            }
            break;
        case OMX_IndexConfigCommonExposureValue:
             {
                OMX_CONFIG_EXPOSUREVALUETYPE *pExposureValue;
                pExposureValue = (OMX_CONFIG_EXPOSUREVALUETYPE*)pStructure;
                OMX_CHECK_STRUCT(pExposureValue, OMX_CONFIG_EXPOSUREVALUETYPE, ret);
				fsl_osal_memcpy(&ExposureValue, pExposureValue, sizeof(OMX_CONFIG_EXPOSUREVALUETYPE));
				SetExposureValue();
            }
            break;
        case OMX_IndexConfigCapturing:
             {
                OMX_CONFIG_BOOLEANTYPE *pCapturing;
				OMX_TIME_CONFIG_TIMESTAMPTYPE StartTime;
				OMX_INIT_STRUCT(&StartTime, OMX_TIME_CONFIG_TIMESTAMPTYPE);
				StartTime.nTimestamp = 0;

				pCapturing = (OMX_CONFIG_BOOLEANTYPE*)pStructure;
				OMX_CHECK_STRUCT(pCapturing, OMX_CONFIG_BOOLEANTYPE, ret);
				fsl_osal_memcpy(&Capturing, pCapturing, sizeof(OMX_CONFIG_BOOLEANTYPE));

				if (Capturing.bEnabled == OMX_TRUE)
					ClockSetConfig(OMX_IndexConfigTimeClientStartTime, &StartTime);
			 }
			 break;
		case OMX_IndexAutoPauseAfterCapture:
			 {
				 OMX_CONFIG_BOOLEANTYPE *pAutoPauseAfterCapture;
				 pAutoPauseAfterCapture = (OMX_CONFIG_BOOLEANTYPE*)pStructure;
				 OMX_CHECK_STRUCT(pAutoPauseAfterCapture, OMX_CONFIG_BOOLEANTYPE, ret);
				 fsl_osal_memcpy(&AutoPauseAfterCapture, pAutoPauseAfterCapture, sizeof(OMX_CONFIG_BOOLEANTYPE));
			 }
			 break;
		case OMX_IndexConfigCommonRotate:
			 {
				 OMX_CONFIG_ROTATIONTYPE *pRotation;
				 pRotation = (OMX_CONFIG_ROTATIONTYPE*)pStructure;
				 OMX_CHECK_STRUCT(pRotation, OMX_CONFIG_ROTATIONTYPE, ret);
				 fsl_osal_memcpy(&Rotation, pRotation, sizeof(OMX_CONFIG_ROTATIONTYPE));
				 SetRotation();
			 }
			 break;
		case OMX_IndexConfigEOS:
			 {
				 OMX_CONFIG_BOOLEANTYPE *pEOS;
				 pEOS = (OMX_CONFIG_BOOLEANTYPE*)pStructure;
				 OMX_CHECK_STRUCT(pEOS, OMX_CONFIG_BOOLEANTYPE, ret);
				 fsl_osal_memcpy(&EOS, pEOS, sizeof(OMX_CONFIG_BOOLEANTYPE));
			 }
			 break;
		default:
			 ret = OMX_ErrorUnsupportedIndex;
			 break;
    }

    return ret;
}
 
OMX_ERRORTYPE VideoSource::ProcessDataBuffer()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	LOG_DEBUG("VideoSource port number: %d\n", ports[CAPTURED_FRAME_PORT]->BufferNum());
	if (bSendEOS == OMX_TRUE)
		return OMX_ErrorNoMore;

    while (ports[CAPTURED_FRAME_PORT]->BufferNum() > 0) {
		ports[CAPTURED_FRAME_PORT]->GetBuffer(&pOutBufferHdr);
		if(pOutBufferHdr != NULL) {
			ret = SendBufferToDevice();
			if (ret != OMX_ErrorNone)
			{
				LOG_ERROR("Send buffer to device fail.\n");
				return ret;
			}
		}
	}

	ret = GetOneFrameFromDevice();
	if (ret != OMX_ErrorNone) {
		if (ret == OMX_ErrorNotReady)
			return OMX_ErrorNone;
		else
			return ret;
	}

	ret = ProcessOutputBufferFlag();
	if (ret != OMX_ErrorNone)
		return ret;
	ret = ProcessPreviewPort();
	if (ret != OMX_ErrorNone)
		return ret;
	ret = SendOutputBuffer();
	if (ret != OMX_ErrorNone)
		return ret;

	return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoSource::ProcessOutputBufferFlag()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (bFirstFrame == OMX_TRUE) {
		nBaseTime = GetFrameTimeStamp();
		pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_STARTTIME;
		bFirstFrame = OMX_FALSE;
	}
	pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

	return ret;
}

OMX_ERRORTYPE VideoSource::ProcessPreviewPort()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_BUFFERHEADERTYPE *pPreviewBufferHdr;

	if (ports[PREVIEW_PORT]->IsEnabled() != OMX_TRUE)
        return ret;

    if (ports[PREVIEW_PORT]->BufferNum() > 0) {
		ports[PREVIEW_PORT]->GetBuffer(&pPreviewBufferHdr);
		if(pPreviewBufferHdr != NULL) {
			OMX_PTR pFrame = NULL;
			GetHwBuffer(pPreviewBufferHdr->pBuffer, &pFrame);
			if(pFrame == NULL)
				fsl_osal_memcpy(pPreviewBufferHdr->pBuffer, pOutBufferHdr->pBuffer, pOutBufferHdr->nFilledLen);
			pPreviewBufferHdr->nFilledLen = pOutBufferHdr->nFilledLen;
			pPreviewBufferHdr->nFlags = pOutBufferHdr->nFlags;
			ports[PREVIEW_PORT]->SendBuffer(pPreviewBufferHdr);
		}
	}
	
	return ret;
}

OMX_ERRORTYPE VideoSource::SendOutputBuffer()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_TICKS nMediaTimestamp;

	if (Capturing.bEnabled != OMX_TRUE)
		pOutBufferHdr->nFilledLen = 0;

	OMX_TIME_CONFIG_TIMESTAMPTYPE sCur;
	OMX_INIT_STRUCT(&sCur, OMX_TIME_CONFIG_TIMESTAMPTYPE);
	ClockGetConfig(OMX_IndexConfigTimeCurrentMediaTime, &sCur);

	nFrameDelay = GetDelayofFrame();

	nMediaTimestamp = sCur.nTimestamp - nFrameDelay;
	LOG_DEBUG("Media time diff: %lld\n", nMediaTimestamp - nMediaTimestampPre);
	if (nMediaTimestamp <= 0) {
		pOutBufferHdr->nFilledLen = 0;
		nMediaTimestamp = 0;
	}
	if (nMediaTimestamp < nMediaTimestampPre)
		nMediaTimestamp = nMediaTimestampPre + 1000;
	nMediaTimestampPre = nMediaTimestamp;

	pOutBufferHdr->nTimeStamp = nMediaTimestamp;
 
	if (EOS.bEnabled == OMX_TRUE || nMediaTimestamp > nMaxDuration) {
		bSendEOS = OMX_TRUE;
		pOutBufferHdr->nFlags |= OMX_BUFFERFLAG_EOS;
		pOutBufferHdr->nFilledLen = 0;
		SendEvent(OMX_EventBufferFlag, 0, OMX_BUFFERFLAG_MAX_DURATION, NULL);
		if (nMediaTimestamp)
			printf("Video frame rate: %f\n", ((OMX_S64)nCaptureFrameCnt) * \
					OMX_TICKS_PER_SECOND * 1000 / nMediaTimestamp / ((float)1000));
	} else {
		nCaptureFrameCnt ++;
		LOG_DEBUG("VideoSource time: %lld\n", pOutBufferHdr->nTimeStamp);
	}

	if (nTimeLapseUs > 0) {
		if (pOutBufferHdr->nTimeStamp > nNextLapseTS) {
			nNextLapseTS += nTimeLapseUs;
			pOutBufferHdr->nTimeStamp = nLastSendTS + nFrameInterval;
			nLastSendTS += nFrameInterval;
		} else {
			pOutBufferHdr->nFilledLen = 0;
			pOutBufferHdr->nTimeStamp = 0;
		}
	}

	LOG_DEBUG("VideoSource send nFilledLen: %d\n", pOutBufferHdr->nFilledLen);
	ports[CAPTURED_FRAME_PORT]->SendBuffer(pOutBufferHdr);
	
	return ret;
}


OMX_ERRORTYPE VideoSource::ClockGetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(ports[CLK_PORT]->IsEnabled() != OMX_TRUE)
	{
		switch (nParamIndex) {
			case OMX_IndexConfigTimeCurrentMediaTime:
				{
					OMX_TIME_CONFIG_TIMESTAMPTYPE *pCur = (OMX_TIME_CONFIG_TIMESTAMPTYPE *)pStructure;
					pCur->nTimestamp = GetFrameTimeStamp() - nBaseTime;
				}
				break;
			default :
				ret = OMX_ErrorUnsupportedIndex;
				break;
		}

		return ret;
	}

	TUNNEL_INFO TunnelInfo;
    ports[CLK_PORT]->GetTunneledInfo(&TunnelInfo);
	OMX_COMPONENTTYPE *hClockComp = (OMX_COMPONENTTYPE *)TunnelInfo.hTunneledComp;
    if(hClockComp == NULL)
	{
		switch (nParamIndex) {
			case OMX_IndexConfigTimeCurrentMediaTime:
				{
					OMX_TIME_CONFIG_TIMESTAMPTYPE *pCur = (OMX_TIME_CONFIG_TIMESTAMPTYPE *)pStructure;
					pCur->nTimestamp = GetFrameTimeStamp() - nBaseTime;
				}
				break;
			default :
				ret = OMX_ErrorUnsupportedIndex;
				break;
		}

		return ret;
	}

	switch (nParamIndex) {
		case OMX_IndexConfigTimeClockState:
			{
				OMX_TIME_CONFIG_CLOCKSTATETYPE *pClockState = (OMX_TIME_CONFIG_CLOCKSTATETYPE *)pStructure;
				ret = OMX_GetConfig(hClockComp, OMX_IndexConfigTimeClockState, pClockState);
			}
			break;
		case OMX_IndexConfigTimeCurrentMediaTime:
			{
				OMX_TIME_CONFIG_TIMESTAMPTYPE *pCur = (OMX_TIME_CONFIG_TIMESTAMPTYPE *)pStructure;
				pCur->nPortIndex = TunnelInfo.nTunneledPort;
				ret = OMX_GetConfig(hClockComp, OMX_IndexConfigTimeCurrentMediaTime, pCur);
			}
			break;
		default :
			ret = OMX_ErrorUnsupportedIndex;
			break;
	}

	return ret;
}

OMX_ERRORTYPE VideoSource::ClockSetConfig(OMX_INDEXTYPE nParamIndex, OMX_TIME_CONFIG_TIMESTAMPTYPE *pTimeStamp)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(ports[CLK_PORT]->IsEnabled() != OMX_TRUE)
		return ret;

	TUNNEL_INFO TunnelInfo;
    ports[CLK_PORT]->GetTunneledInfo(&TunnelInfo);
	OMX_COMPONENTTYPE *hClockComp = (OMX_COMPONENTTYPE *)TunnelInfo.hTunneledComp;
    if(hClockComp == NULL)
		return ret;

	pTimeStamp->nPortIndex = TunnelInfo.nTunneledPort;

    switch (nParamIndex) {
        case OMX_IndexConfigTimeClientStartTime:
			ret = OMX_SetConfig(hClockComp, OMX_IndexConfigTimeClientStartTime,pTimeStamp);
            break;
        case OMX_IndexConfigTimeCurrentVideoReference:
			ret = OMX_SetConfig(hClockComp, OMX_IndexConfigTimeCurrentVideoReference,pTimeStamp);
            break;
        default :
            ret = OMX_ErrorUnsupportedIndex;
            break;
    }

	return ret;
}

OMX_ERRORTYPE VideoSource::ProcessClkBuffer()
{
    if(ports[CLK_PORT]->IsEnabled() != OMX_TRUE)
        return OMX_ErrorNoMore;

	TUNNEL_INFO TunnelInfo;
    ports[CLK_PORT]->GetTunneledInfo(&TunnelInfo);
	OMX_COMPONENTTYPE *hClockComp = (OMX_COMPONENTTYPE *)TunnelInfo.hTunneledComp;
    if(hClockComp == NULL)
        return OMX_ErrorNoMore;

    if(ports[CLK_PORT]->BufferNum() == 0)
		return OMX_ErrorNoMore;

	OMX_BUFFERHEADERTYPE *pClockBufferHdr;
	ports[CLK_PORT]->GetBuffer(&pClockBufferHdr);
	OMX_TIME_MEDIATIMETYPE *pTimeBuffer = (OMX_TIME_MEDIATIMETYPE*) pClockBufferHdr->pBuffer;

	if ((pTimeBuffer->eUpdateType == OMX_TIME_UpdateClockStateChanged)
		&& (pTimeBuffer->eState == OMX_TIME_ClockStateRunning))
	{
	}
	else if (pTimeBuffer->eUpdateType == OMX_TIME_UpdateScaleChanged)
	{
	}
	else
	{
		LOG_WARNING("Unknow clock buffer.\n");
	}

	ports[CLK_PORT]->SendBuffer(pClockBufferHdr);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoSource::DoLoaded2Idle()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = OpenDevice();
    if(ret != OMX_ErrorNone) {
        CloseDevice();
        SendEvent(OMX_EventError, ret, 0, NULL);
        return ret;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoSource::DoIdle2Loaded()
{
 	CloseDevice();

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoSource::ComponentReturnBuffer(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VideoSource::FlushComponent(
        OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	return OMX_ErrorNone;
}

/* File EOF */
