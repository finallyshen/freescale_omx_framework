/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file VideoSource.h
 *  @brief Class definition of VideoSource Component
 *  @ingroup VideoSource
 */

#ifndef VideoSource_h
#define VideoSource_h

#include "ComponentBase.h"

#define NUM_PORTS 3
#define PREVIEW_PORT 0
#define CAPTURED_FRAME_PORT 1
#define CLK_PORT 2

class VideoSource : public ComponentBase {
    public:
		VideoSource();
    protected:
		OMX_PTR cameraPtr;
		OMX_PTR cameraProxyPtr;
		OMX_S32 nCameraId;
		OMX_PTR previewSurface;
        ROTATION eRotation;
        OMX_U32 nFrameBufferMin;
        OMX_U32 nFrameBufferActual;
		OMX_BUFFERHEADERTYPE *pOutBufferHdr;
        OMX_BUFFERSUPPLIERTYPE CapturePortSupplierType;
        OMX_BUFFERSUPPLIERTYPE PreviewPortSupplierType;
        OMX_VIDEO_PORTDEFINITIONTYPE sVideoFmt;
		OMX_PARAM_SENSORMODETYPE SensorMode;
		OMX_VIDEO_PARAM_PORTFORMATTYPE PortFormat;
		OMX_CONFIG_WHITEBALCONTROLTYPE WhiteBalControl;
		OMX_CONFIG_SCALEFACTORTYPE ScaleFactor;
		OMX_CONFIG_EXPOSUREVALUETYPE ExposureValue;
		OMX_CONFIG_BOOLEANTYPE Capturing;
		OMX_CONFIG_BOOLEANTYPE AutoPauseAfterCapture;
		OMX_CONFIG_ROTATIONTYPE Rotation;
	private:
		OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE InstanceInit();
        OMX_ERRORTYPE InstanceDeInit();
        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
		OMX_ERRORTYPE DoLoaded2Idle();
		OMX_ERRORTYPE DoIdle2Loaded();
		OMX_ERRORTYPE ComponentReturnBuffer(OMX_U32 nPortIndex);
		OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex);
		OMX_ERRORTYPE ProcessDataBuffer();
		OMX_ERRORTYPE ProcessOutputBufferFlag();
		OMX_ERRORTYPE ProcessPreviewPort();
		OMX_ERRORTYPE SendOutputBuffer();
		OMX_ERRORTYPE ClockGetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
		OMX_ERRORTYPE ClockSetConfig(OMX_INDEXTYPE nParamIndex, \
				OMX_TIME_CONFIG_TIMESTAMPTYPE *pTimeStamp);
        OMX_ERRORTYPE ProcessClkBuffer();

		/** Video source device related */
        virtual OMX_ERRORTYPE InitSourceComponent() = 0;
        virtual OMX_ERRORTYPE DeInitSourceComponent() = 0;
        virtual OMX_ERRORTYPE OpenDevice() = 0;
		virtual OMX_ERRORTYPE CloseDevice() = 0;
		virtual OMX_ERRORTYPE SetSensorMode() = 0;
		virtual OMX_ERRORTYPE SetVideoFormat() = 0;
		virtual OMX_ERRORTYPE SetWhiteBalance() = 0;
		virtual OMX_ERRORTYPE SetDigitalZoom() = 0;
		virtual OMX_ERRORTYPE SetExposureValue() = 0;
		virtual OMX_ERRORTYPE SetRotation() = 0;
		virtual OMX_ERRORTYPE StartDevice() = 0;
		virtual OMX_ERRORTYPE StopDevice() = 0;
        virtual OMX_ERRORTYPE SendBufferToDevice() = 0;
        virtual OMX_ERRORTYPE GetOneFrameFromDevice() = 0;
        virtual OMX_TICKS GetDelayofFrame() = 0;
        virtual OMX_TICKS GetFrameTimeStamp() = 0;

		OMX_BOOL bFirstFrame;
		OMX_TICKS nFrameDelay;
		OMX_TICKS nBaseTime;
		OMX_TICKS nMediaTimestampPre;
		OMX_CONFIG_BOOLEANTYPE EOS;
		OMX_U32 nCaptureFrameCnt;
		OMX_BOOL bSendEOS;
		OMX_TICKS nMaxDuration;
		OMX_TICKS nTimeLapseUs;
		OMX_TICKS nNextLapseTS;
		OMX_TICKS nLastSendTS;
		OMX_TICKS nFrameInterval;
};

#endif
/* File EOF */
