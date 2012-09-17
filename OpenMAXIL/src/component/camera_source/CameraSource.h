/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file CameraSource.h
 *  @brief Class definition of CameraSource Component
 *  @ingroup CameraSource
 */

#ifndef CameraSource_h
#define CameraSource_h

#include <camera/Camera.h>
#ifdef ICS
#include <camera/ICameraRecordingProxyListener.h>
#endif
#include <camera/CameraParameters.h>
#include <surfaceflinger/ISurface.h>
#include <surfaceflinger/Surface.h>
#include <utils/String8.h>
#include "VideoSource.h"

using android::sp;
using android::wp;
using android::ISurface;
using android::Surface;
using android::Camera;
using android::ICamera;
#ifdef ICS
using android::ICameraRecordingProxy;
using android::BnCameraRecordingProxyListener;
using android::IBinder;
#endif
using android::CameraListener;
using android::IMemory;
using android::CameraParameters;
using android::String8;

// Need keep same as Camera HAL.
typedef struct {
    size_t phy_offset;
    unsigned int length;
}VIDEOFRAME_BUFFER_PHY;

/* capture framebuffer type */
typedef struct {
    OMX_U32 nIndex;
    OMX_U32 nLength;
    OMX_PTR pVirtualAddr;
    OMX_PTR pPhyiscAddr;
}FB_DATA;


#define FRAME_BUFFER_QUEUE_SIZE 32

class CameraSource : public VideoSource {
    public:
        CameraSource();
		void dataCallbackTimestamp(OMX_TICKS timestamp, OMX_S32 msgType, OMX_PTR dataPtr);
	private:
#ifdef ICS
		class ProxyListener: public BnCameraRecordingProxyListener {
			public:
				ProxyListener(CameraSource *source);
				virtual void dataCallbackTimestamp(int64_t timestampUs, int32_t msgType,
						const sp<IMemory> &data);

			private:
				CameraSource *mSource;
		};

		// isBinderAlive needs linkToDeath to work.
		class DeathNotifier: public IBinder::DeathRecipient {
			public:
				DeathNotifier() {}
				virtual void binderDied(const wp<IBinder>& who);
		};
#endif

		enum CameraFlags {
			FLAGS_SET_CAMERA = 1L << 0,
			FLAGS_HOT_CAMERA = 1L << 1,
		};

		OMX_ERRORTYPE InitSourceComponent();
        OMX_ERRORTYPE DeInitSourceComponent();
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
		OMX_ERRORTYPE DoAllocateBuffer(OMX_PTR *buffer, OMX_U32 nSize,OMX_U32 nPortIndex);
        OMX_ERRORTYPE DoFreeBuffer(OMX_PTR buffer,OMX_U32 nPortIndex);
        OMX_ERRORTYPE SetDevice();
		OMX_ERRORTYPE SetSensorMode();
		OMX_ERRORTYPE PortFormatChanged(OMX_U32 nPortIndex);
		OMX_ERRORTYPE SetVideoFormat();
		OMX_ERRORTYPE SetWhiteBalance();
		OMX_ERRORTYPE SetDigitalZoom();
		OMX_ERRORTYPE SetExposureValue();
		OMX_ERRORTYPE SetRotation();
		OMX_ERRORTYPE StartDevice();
		void releaseRecordingFrame(const sp<IMemory>& frame);
		OMX_ERRORTYPE releaseQueuedFrames();
        OMX_ERRORTYPE StopDevice();
        OMX_ERRORTYPE SendBufferToDevice();
        OMX_ERRORTYPE GetOneFrameFromDevice();
        OMX_TICKS GetDelayofFrame();
        OMX_TICKS GetFrameTimeStamp();
        OMX_ERRORTYPE SetDeviceRotation();
        OMX_ERRORTYPE SetDeviceInputCrop();
        OMX_ERRORTYPE SetDeviceDisplayRegion();

        OMX_ERRORTYPE CameraBufferInit();
        OMX_ERRORTYPE CameraBufferDeInit();
        OMX_ERRORTYPE SearchBuffer(OMX_PTR pAddr, OMX_U32 *pIndex);
        OMX_ERRORTYPE SearchBufferPhy(OMX_PTR pAddr, OMX_U32 *pIndex);

		sp<Camera> mCamera;
#ifdef ICS
		sp<ICameraRecordingProxy> mCameraRecordingProxy;
		sp<DeathNotifier> mDeathNotifier;
#else
		OMX_PTR mCameraRecordingProxy;
		OMX_PTR mDeathNotifier;
#endif
        sp<ISurface> mPreviewSurface;
        sp<Surface> mPreviewSurface2;
		List<sp<IMemory> > VideoFrameList;
		sp<IMemory> mIMemory[FRAME_BUFFER_QUEUE_SIZE];
		VIDEOFRAME_BUFFER_PHY mVideoBufferPhy[FRAME_BUFFER_QUEUE_SIZE];
        FB_DATA fb_data[FRAME_BUFFER_QUEUE_SIZE];
        OMX_BUFFERHEADERTYPE *BufferHdrs[FRAME_BUFFER_QUEUE_SIZE];
		Queue *FrameBufferQueue;
		OMX_PTR mListenerPtr;
		OMX_PTR mProxyListenerPtr;
		OMX_S32 mFlags;
		OMX_S32 nIMemoryIndex;
        OMX_CONFIG_RECTTYPE sRectIn;
        OMX_CONFIG_RECTTYPE sRectOut;
        OMX_BOOL bFrameBufferInit;
		OMX_BOOL bStreamOn;
		OMX_BOOL mStarted; 
        OMX_U32 nFrameBuffer;
        OMX_U32 nAllocating;
        OMX_U32 nQueuedBuffer;
        OMX_U32 nSourceFrames;
        fsl_osal_mutex lock;
		OMX_TICKS nCurTS;

};

#endif
/* File EOF */
