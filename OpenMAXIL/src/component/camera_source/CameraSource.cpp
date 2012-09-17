/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#ifdef ICS
#include <cutils/log.h>
#include "gralloc_priv.h"
#include <android_natives.h>
#endif
#include <binder/IPCThreadState.h>
#include "CameraSource.h"

using android::IPCThreadState;

#define NO_ERROR 0

struct CameraSourceListener : public CameraListener {
    CameraSourceListener(CameraSource *source);

    virtual void notify(int msgType, int ext1, int ext2);
#ifdef ICS
    virtual void postData(int msgType, const sp<IMemory> &dataPtr, camera_frame_metadata_t *metadata);
#else
    virtual void postData(int msgType, const sp<IMemory> &dataPtr);
#endif

    virtual void postDataTimestamp(
            nsecs_t timestamp, int msgType, const sp<IMemory>& dataPtr);

    virtual ~CameraSourceListener();
protected:

private:
    CameraSource *mSource;

    CameraSourceListener(const CameraSourceListener &);
    CameraSourceListener &operator=(const CameraSourceListener &);
};

CameraSourceListener::CameraSourceListener(CameraSource *source) {
		mSource = source;
}

CameraSourceListener::~CameraSourceListener() {
}

void CameraSourceListener::notify(int msgType, int ext1, int ext2) {
    LOG_LOG("notify(%d, %d, %d)", msgType, ext1, ext2);
}

#ifdef ICS
void CameraSourceListener::postData(int msgType, const sp<IMemory> &dataPtr, camera_frame_metadata_t *metadata) {
	LOG_LOG("postData(%d, ptr:%p, size:%d)",
			msgType, dataPtr->pointer(), dataPtr->size());
}
#else
void CameraSourceListener::postData(int msgType, const sp<IMemory> &dataPtr) {
	LOG_LOG("postData(%d, ptr:%p, size:%d)",
			msgType, dataPtr->pointer(), dataPtr->size());
}
#endif

void CameraSourceListener::postDataTimestamp(
        nsecs_t timestamp, int msgType, const sp<IMemory>& dataPtr) {

	LOG_LOG("postData(%d, dataPtr:%p, ptr:%p, size:%d)",
			msgType, (OMX_PTR)(&dataPtr), dataPtr->pointer(), dataPtr->size());

	mSource->dataCallbackTimestamp((OMX_TICKS)timestamp/1000, (OMX_S32)msgType, (OMX_PTR)(&dataPtr));
}

static OMX_S32 getColorFormat(const char* colorFormat) {
	if (!fsl_osal_strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV422SP)) {
		return OMX_COLOR_FormatYUV422SemiPlanar;
	}

	if (!fsl_osal_strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV420SP)) {
		return OMX_COLOR_FormatYUV420SemiPlanar;
	}

	if (!fsl_osal_strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV422I)) {
		return OMX_COLOR_FormatYCbYCr;
	}

	if (!fsl_osal_strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_RGB565)) {
		return OMX_COLOR_Format16bitRGB565;
	}

	LOG_ERROR("Uknown color format (%s), please add it to "
			"CameraSource::getColorFormat", colorFormat);

	return OMX_COLOR_FormatUnused;
}

CameraSource::CameraSource()
{ 
	OMX_U32 i; 

	fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_source.camera.sw-based");
	ComponentVersion.s.nVersionMajor = 0x0;
	ComponentVersion.s.nVersionMinor = 0x1;
	ComponentVersion.s.nRevision = 0x0;
	ComponentVersion.s.nStep = 0x0;
	role_cnt = 1;
	role[0] = (OMX_STRING)"video_source.camera";

	nFrameBufferMin = 1;
	nFrameBufferActual = 1;
	nFrameBuffer = nFrameBufferActual;
	PreviewPortSupplierType = OMX_BufferSupplyInput;
	CapturePortSupplierType = OMX_BufferSupplyOutput; 
	fsl_osal_memset(&sVideoFmt, 0, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
	sVideoFmt.nFrameWidth = 176;
	sVideoFmt.nFrameHeight = 144;
	sVideoFmt.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
	sVideoFmt.eCompressionFormat = OMX_VIDEO_CodingUnused;

	OMX_INIT_STRUCT(&sRectIn, OMX_CONFIG_RECTTYPE);
	sRectIn.nWidth = sVideoFmt.nFrameWidth;
	sRectIn.nHeight = sVideoFmt.nFrameHeight;
	sRectIn.nTop = 0;
	sRectIn.nLeft = 0;
	OMX_INIT_STRUCT(&sRectOut, OMX_CONFIG_RECTTYPE);
	sRectOut.nWidth = sVideoFmt.nFrameWidth;
	sRectOut.nHeight = sVideoFmt.nFrameHeight;
	sRectOut.nTop = 0;
	sRectOut.nLeft = 0;

	for(i=0; i<FRAME_BUFFER_QUEUE_SIZE; i++)
		fsl_osal_memset(&fb_data[i], 0, sizeof(FB_DATA));
	for(i=0; i<FRAME_BUFFER_QUEUE_SIZE; i++) {
		mIMemory[i] = 0;
		BufferHdrs[i] = NULL;
	}

	mCamera = NULL;
	mCameraRecordingProxy = NULL;
	mPreviewSurface = NULL;
	mPreviewSurface2 = NULL;
	mDeathNotifier = NULL;
	mListenerPtr = NULL;
	mProxyListenerPtr = NULL;
	mStarted = OMX_FALSE; 
	bFrameBufferInit = OMX_FALSE;
	nAllocating = 0;
	nSourceFrames = 0;
	nIMemoryIndex = 0;
	nCurTS = 0;
	lock = NULL;
}

OMX_ERRORTYPE CameraSource::InitSourceComponent()
{
	if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal)) {
		LOG_ERROR("Create mutex for camera device failed.\n");
		return OMX_ErrorInsufficientResources;
	}

	/** Create queue for frame buffer. */
	FrameBufferQueue = FSL_NEW(Queue, ());
	if (FrameBufferQueue == NULL) {
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}

	if (FrameBufferQueue->Create(FRAME_BUFFER_QUEUE_SIZE, sizeof(OMX_BUFFERHEADERTYPE *), E_FSL_OSAL_TRUE) != QUEUE_SUCCESS) {
		LOG_ERROR("Can't create frame buffer queue.\n");
		return OMX_ErrorInsufficientResources;
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE CameraSource::DeInitSourceComponent()
{
	LOG_DEBUG("Frame buffer queue free\n");
	if(FrameBufferQueue != NULL)
		FrameBufferQueue->Free();
	LOG_DEBUG("Frame buffer queue delete\n");
	FSL_DELETE(FrameBufferQueue);

	if(lock)
		fsl_osal_mutex_destroy(lock);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE CameraSource::PortFormatChanged(OMX_U32 nPortIndex)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
	OMX_BOOL bFmtChanged = OMX_FALSE;

	OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	sPortDef.nPortIndex = nPortIndex;
	ports[nPortIndex]->GetPortDefinition(&sPortDef);

	sPortDef.nBufferSize = sPortDef.format.video.nFrameWidth
		* sPortDef.format.video.nFrameHeight
		* pxlfmt2bpp(sVideoFmt.eColorFormat) / 8;
	ret = ports[nPortIndex]->SetPortDefinition(&sPortDef);
	if (ret != OMX_ErrorNone)
		return ret;

	if(nPortIndex == CAPTURED_FRAME_PORT)
	{
		if(sVideoFmt.nFrameWidth != sPortDef.format.video.nFrameWidth) {
			sVideoFmt.nFrameWidth = sPortDef.format.video.nFrameWidth;
			bFmtChanged = OMX_TRUE;
		}
		if(sVideoFmt.nFrameHeight != sPortDef.format.video.nFrameHeight) {
			sVideoFmt.nFrameHeight = sPortDef.format.video.nFrameHeight;
			bFmtChanged = OMX_TRUE;
		}
		if(sVideoFmt.eColorFormat != sPortDef.format.video.eColorFormat) {
			sVideoFmt.eColorFormat = sPortDef.format.video.eColorFormat;
			bFmtChanged = OMX_TRUE;
		}
		if(nFrameBuffer != sPortDef.nBufferCountActual) {
			nFrameBuffer = sPortDef.nBufferCountActual;
		}

		sRectIn.nWidth = sVideoFmt.nFrameWidth;
		sRectIn.nHeight = sVideoFmt.nFrameHeight;
		sRectOut.nWidth = sVideoFmt.nFrameWidth;
		sRectOut.nHeight = sVideoFmt.nFrameHeight;
		sVideoFmt.xFramerate = sPortDef.format.video.xFramerate;

		printf("Set resolution: width: %d height: %d\n", sPortDef.format.video.nFrameWidth, \
				sPortDef.format.video.nFrameHeight);
	
		if(bFmtChanged && mCamera > 0) {
			CloseDevice();
			OpenDevice();
		}
	}

    return ret;
}

OMX_ERRORTYPE CameraSource::OpenDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	int64_t token = IPCThreadState::self()->clearCallingIdentity();
	if (cameraPtr == NULL) {
		mCamera = Camera::connect(nCameraId);
		if (mCamera == 0) {
			IPCThreadState::self()->restoreCallingIdentity(token);
			LOG_ERROR("Camera connection could not be established.");
			return OMX_ErrorHardware;
		}
		mFlags &= ~FLAGS_HOT_CAMERA;
		mCamera->lock();
	} else {
		sp<ICamera> *pCamera;
		pCamera = (sp<ICamera> *)cameraPtr;
		mFlags &= ~FLAGS_HOT_CAMERA;
		mCamera = Camera::create(*pCamera);
		if (mCamera == 0) {
			IPCThreadState::self()->restoreCallingIdentity(token);
			LOG_ERROR("Unable to connect to camera");
			return OMX_ErrorHardware;
		}
		mFlags |= FLAGS_HOT_CAMERA;

#ifdef ICS
		sp<ICameraRecordingProxy> *pCameraProxy;
		if (cameraProxyPtr == NULL) {
			LOG_ERROR("camera proxy is NULL");
			return OMX_ErrorHardware;
		}
		pCameraProxy = (sp<ICameraRecordingProxy> *)cameraProxyPtr;
		mCameraRecordingProxy = *pCameraProxy;
		mDeathNotifier = new DeathNotifier();
		mCameraRecordingProxy->asBinder()->linkToDeath(mDeathNotifier);
		mCamera->lock();
#endif
	}
	IPCThreadState::self()->restoreCallingIdentity(token);

	ret = SetDevice();
	if (ret != OMX_ErrorNone)
		return ret;

    ret = CameraBufferInit();
    if(ret != OMX_ErrorNone)
        return ret;

	return ret;
}

OMX_ERRORTYPE CameraSource::CloseDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    CameraBufferDeInit();

	if (mCamera != 0) {
		int64_t token = IPCThreadState::self()->clearCallingIdentity();
		LOG_LOG("Disconnect camera");
		if ((mFlags & FLAGS_HOT_CAMERA) == 0) {
			LOG_LOG("Camera was cold when we started, stopping preview");
			mCamera->stopPreview();
            mCamera->disconnect();
		}
		printf("unlock camera.\n");
		mCamera->unlock();
		mCamera.clear();
		mCamera = NULL;
		IPCThreadState::self()->restoreCallingIdentity(token);
	}

#ifdef ICS
	if (mCameraRecordingProxy != 0) {
		mCameraRecordingProxy->asBinder()->unlinkToDeath(mDeathNotifier);
		mCameraRecordingProxy.clear();
	}
#endif
	mFlags = 0;

	return ret;
}

OMX_ERRORTYPE CameraSource::SetDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	{
		// Set the actual video recording frame size
		int64_t token = IPCThreadState::self()->clearCallingIdentity();
		CameraParameters params(mCamera->getParameters());
		params.setPreviewSize(sRectIn.nWidth, sRectIn.nHeight);
		params.setPreviewFrameRate(sVideoFmt.xFramerate / Q16_SHIFT);
		String8 s = params.flatten();
		if (NO_ERROR != mCamera->setParameters(s)) {
			LOG_ERROR("Could not change settings."
					" Someone else is using camera %d?", nCameraId);
			IPCThreadState::self()->restoreCallingIdentity(token);
			return OMX_ErrorHardware;
		}
		CameraParameters newCameraParams(mCamera->getParameters());

		// Check on video frame size
		int frameWidth = 0, frameHeight = 0;
		newCameraParams.getPreviewSize(&frameWidth, &frameHeight);
		if (frameWidth  < 0 || frameWidth  != (int)sRectIn.nWidth ||
				frameHeight < 0 || frameHeight != (int)sRectIn.nHeight) {
			LOG_ERROR("Failed to set the video frame size to %dx%d",
					sRectIn.nWidth, sRectIn.nHeight);
			IPCThreadState::self()->restoreCallingIdentity(token);
			return OMX_ErrorHardware;
		}

		// Check on video frame rate
		OMX_S32 frameRate = newCameraParams.getPreviewFrameRate();
		if (frameRate < 0 || (frameRate - sVideoFmt.xFramerate / Q16_SHIFT) != 0) {
			LOG_ERROR("Failed to set frame rate to %d fps. The actual "
					"frame rate is %d", sVideoFmt.xFramerate / Q16_SHIFT, frameRate);
		}

		// This CHECK is good, since we just passed the lock/unlock
		// check earlier by calling mCamera->setParameters().
		printf("previewSurface: %p\n", previewSurface);
#ifdef HONEY_COMB 
		sp<Surface> *pSurface;
		pSurface = (sp<Surface> *)previewSurface;
		mPreviewSurface2 = *pSurface;
		if (NO_ERROR != mCamera->setPreviewDisplay(mPreviewSurface2)) {
			LOG_ERROR("setPreviewDisplay failed.\n");
		}
#else
		sp<ISurface> *pSurface;
		pSurface = (sp<ISurface> *)previewSurface;
		mPreviewSurface = *pSurface;
		if (NO_ERROR != mCamera->setPreviewDisplay(mPreviewSurface)) {
			LOG_ERROR("setPreviewDisplay failed.\n");
		}
#endif
		IPCThreadState::self()->restoreCallingIdentity(token);
	}

	{
		int64_t token = IPCThreadState::self()->clearCallingIdentity();
		String8 s = mCamera->getParameters();
		IPCThreadState::self()->restoreCallingIdentity(token);

		printf("params: \"%s\"\n", s.string());

		int width, height, stride, sliceHeight;
		CameraParameters params(s);
		params.getPreviewSize(&width, &height);

		OMX_S32 frameRate = params.getPreviewFrameRate();

		const char *colorFormatStr = params.get(CameraParameters::KEY_VIDEO_FRAME_FORMAT);
		if (colorFormatStr != NULL) {
			OMX_S32 colorFormat = getColorFormat(colorFormatStr);
		}
	}
	
	{
		int64_t token = IPCThreadState::self()->clearCallingIdentity();
#ifdef ICS
		if (mFlags & FLAGS_HOT_CAMERA) {
			ProxyListener *mProxyListener;
			mProxyListener = FSL_NEW(ProxyListener, (this));
			mProxyListenerPtr = (OMX_PTR)mProxyListener;

			if (NO_ERROR != mCamera->storeMetaDataInBuffers(true)) {
				LOG_WARNING("Camera set derect input fail.\n");
				return OMX_ErrorNone;
			}

			mCamera->unlock();
			mCamera.clear();

			mCameraRecordingProxy->startRecording(mProxyListener);
		} else {
#endif
			CameraSourceListener *mListener;
			mListener = FSL_NEW(CameraSourceListener, (this));
			mListenerPtr = (OMX_PTR)mListener;

			mCamera->setListener(mListener);
			if (NO_ERROR != mCamera->startRecording()) {
				LOG_ERROR("stopRecording failed.\n");
			}

			if (NO_ERROR != mCamera->storeMetaDataInBuffers(true)) {
				LOG_WARNING("Camera set derect input fail.\n");
				return OMX_ErrorNone;
			}
#ifdef ICS
		}
#endif
		IPCThreadState::self()->restoreCallingIdentity(token);

	}

	return ret;
}

OMX_ERRORTYPE CameraSource::SetSensorMode()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE CameraSource::SetVideoFormat()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE CameraSource::SetWhiteBalance()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE CameraSource::SetDigitalZoom()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE CameraSource::SetExposureValue()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE CameraSource::SetRotation()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	return ret;
}

OMX_ERRORTYPE CameraSource::DoAllocateBuffer(
        OMX_PTR *buffer, 
        OMX_U32 nSize,
        OMX_U32 nPortIndex)
{
	OMX_U32 i = 0;

	if (nPortIndex == CAPTURED_FRAME_PORT)
	{
		/* wait until camera device is opened */
		while(bFrameBufferInit != OMX_TRUE) {
			fsl_osal_sleep(10000);
			i++;
			printf("wait count: %d\n", i);
			if(i >= 10) {
				printf("Error, Camera hasn't ready.\n");
				return OMX_ErrorHardware;
			}
		}
		if(nAllocating >= nFrameBuffer) {
			LOG_ERROR("Requested buffer number beyond system can provide: [%d:%d]\n", \
					nAllocating, nFrameBuffer);
			return OMX_ErrorInsufficientResources;
		}

		*buffer = fb_data[nAllocating].pVirtualAddr;
		nAllocating ++;
		printf("Buffer count: %d\n", nAllocating);
		if(nAllocating == nFrameBuffer) {
			mStarted = OMX_TRUE; 
			for(i=0; i<FRAME_BUFFER_QUEUE_SIZE; i++) {
				BufferHdrs[i] = NULL;
			}
		}
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE CameraSource::DoFreeBuffer(
        OMX_PTR buffer,
        OMX_U32 nPortIndex)
{
    nAllocating --;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE CameraSource::StartDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (nAllocating != nFrameBuffer) {
		OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
		OMX_U32 nPortIndex = CAPTURED_FRAME_PORT;

		OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
		sPortDef.nPortIndex = nPortIndex;
		ports[nPortIndex]->GetPortDefinition(&sPortDef);

		sPortDef.nBufferCountActual = nFrameBuffer;
		ret = ports[nPortIndex]->SetPortDefinition(&sPortDef);
		if (ret != OMX_ErrorNone)
			return ret;

		SendEvent(OMX_EventPortSettingsChanged, CAPTURED_FRAME_PORT, 0, NULL);
	} else 
		mStarted = OMX_TRUE; 

	return ret;
}

void CameraSource::releaseRecordingFrame(const sp<IMemory>& frame) 
{
	VIDEOFRAME_BUFFER_PHY VideoBufferPhyTmp;

	fsl_osal_memcpy((void*)&VideoBufferPhyTmp, frame->pointer(),
			sizeof(VIDEOFRAME_BUFFER_PHY));
	LOG_DEBUG("Physic memory return to Camera HAL: %p\n", VideoBufferPhyTmp.phy_offset);

	LOGV("releaseRecordingFrame");
	if (mCameraRecordingProxy != NULL) {
#ifdef ICS
		mCameraRecordingProxy->releaseRecordingFrame(frame);
#endif
	} else if (mCamera != NULL) {
		int64_t token = IPCThreadState::self()->clearCallingIdentity();
		mCamera->releaseRecordingFrame(frame);
		IPCThreadState::self()->restoreCallingIdentity(token);
	}
}

OMX_ERRORTYPE CameraSource::releaseQueuedFrames()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nIndex = 0;

	for (nIndex = 0; nIndex < nAllocating; nIndex ++) {
		if (mIMemory[nIndex] != 0) {
			printf("releaseRecordingFrame index: %d\n", nIndex);
			if (mCameraRecordingProxy != NULL) {
#ifdef ICS
				mCameraRecordingProxy->releaseRecordingFrame(mIMemory[nIndex]);
#endif
			} else if (mCamera != NULL) {
				mCamera->releaseRecordingFrame(mIMemory[nIndex]);
			}
			mIMemory[nIndex] = 0;
		}
	}
	return ret;
}

OMX_ERRORTYPE CameraSource::StopDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    fsl_osal_mutex_lock(lock);
	if (mStarted == OMX_TRUE) {
		mStarted = OMX_FALSE; 

		int64_t token = IPCThreadState::self()->clearCallingIdentity();
		releaseQueuedFrames();
#ifdef ICS
		if (mFlags & FLAGS_HOT_CAMERA) {
			mCameraRecordingProxy->stopRecording();
		} else {
#endif
			mCamera->setListener(NULL);
			LOG_DEBUG("CameraSourceListener removed.\n");
			if (mListenerPtr) {
				CameraSourceListener *mListener = (CameraSourceListener *)mListenerPtr;
				mListenerPtr = NULL;
			}
			printf("stopRecording.\n");
			mCamera->stopRecording();
#ifdef ICS
		}
#endif
		IPCThreadState::self()->restoreCallingIdentity(token);
	}

    fsl_osal_mutex_unlock(lock);

	return ret;
}

OMX_ERRORTYPE CameraSource::SendBufferToDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nIndex = 0;

	SearchBuffer(pOutBufferHdr->pBuffer, &nIndex);
	BufferHdrs[nIndex] = pOutBufferHdr;

	if (mIMemory[nIndex] != 0) {
		LOG_DEBUG("releaseRecordingFrame index: %d\n", nIndex);
		releaseRecordingFrame(mIMemory[nIndex]);
		mIMemory[nIndex] = 0;
	}

	return ret;
}

OMX_ERRORTYPE CameraSource::GetOneFrameFromDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nIndex;

#ifdef ICS
	if (mFlags & FLAGS_HOT_CAMERA) {
		if(mCameraRecordingProxy <= 0)
			return OMX_ErrorNotReady;
	} else {
		if(mCamera <= 0)
			return OMX_ErrorNotReady;
	}
#else
    if(mCamera <= 0)
        return OMX_ErrorNotReady;
#endif

	if(FrameBufferQueue->Size() <= 0) {
		fsl_osal_sleep(10000);
		return OMX_ErrorNotReady;
	}

	if (FrameBufferQueue->Get(&pOutBufferHdr) != QUEUE_SUCCESS) {
		LOG_ERROR("Can't get output port buffer item.\n");
		return OMX_ErrorUndefined;
	}

	nCurTS = pOutBufferHdr->nTimeStamp;

	return ret;
}

OMX_TICKS CameraSource::GetDelayofFrame()
{
	int64_t readTimeUs = systemTime() / 1000;

	return readTimeUs - nCurTS;
}

OMX_TICKS CameraSource::GetFrameTimeStamp()
{
    return nCurTS;
}

void CameraSource::dataCallbackTimestamp(OMX_TICKS timestamp, OMX_S32 msgType, OMX_PTR dataPtr)
{
	VIDEOFRAME_BUFFER_PHY VideoBufferPhyTmp;
	OMX_BUFFERHEADERTYPE *pHdr = NULL;
	sp<IMemory> *pData;
	pData = (sp<IMemory> *)dataPtr;
    OMX_U32 nIndex = 0;

    fsl_osal_mutex_lock(lock);

	if (mStarted == OMX_FALSE) {
		printf("Haven't start, Drop frame.\n");
		releaseRecordingFrame(*pData);
		fsl_osal_mutex_unlock(lock);
		return;
	}

	fsl_osal_memcpy((void*)&VideoBufferPhyTmp, (*pData)->pointer(), sizeof(VIDEOFRAME_BUFFER_PHY));

	SearchBufferPhy((void*)VideoBufferPhyTmp.phy_offset, &nIndex);
	LOG_DEBUG("Got frame from Camera index: %d\n", nIndex);
	if (BufferHdrs[nIndex]) {
		mIMemory[nIndex] = *pData;

		BufferHdrs[nIndex]->nTimeStamp = timestamp;
		BufferHdrs[nIndex]->nOffset = 0;
		BufferHdrs[nIndex]->nFilledLen = VideoBufferPhyTmp.length;
		LOG_DEBUG("buffer len: %d\n", BufferHdrs[nIndex]->nFilledLen);

		if (FrameBufferQueue->Add(&BufferHdrs[nIndex]) != QUEUE_SUCCESS) {
			LOG_ERROR("Can't add frame buffer queue. Queue size: %d, max queue size: %d \n", \
					FrameBufferQueue->Size(), FRAME_BUFFER_QUEUE_SIZE);
		}
	} else {
		printf("No frame buffer, Drop one frame.\n");
		releaseRecordingFrame(*pData);
	}

    fsl_osal_mutex_unlock(lock);
}

OMX_ERRORTYPE CameraSource::CameraBufferInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	int64_t token = IPCThreadState::self()->clearCallingIdentity();

#ifdef ICS
	android_native_buffer_t *buf;
	private_handle_t *prvHandle;
	sp<ANativeWindow> nativeWindow = mPreviewSurface2;
	OMX_S32 nVideoBufNum;
	struct IndexBuffer {
		OMX_U32 nIndex;
		OMX_U32 bufPtr;
	};

	IndexBuffer kIndexBuffer;

	nativeWindow->query(nativeWindow.get(), NATIVE_WINDOW_GET_BUFFERS_COUNT, (int *)(&nVideoBufNum));
#else
	OMX_S32 nVideoBufNum = mCamera->getNumberOfVideoBuffers();
#endif
	printf("Physic buffer counter got from Camera HAL: %d\n", nVideoBufNum);
	for (OMX_S32 i = 0; i < nVideoBufNum; i ++)
	{
#ifdef ICS
		kIndexBuffer.nIndex = i;

		nativeWindow->query(nativeWindow.get(), NATIVE_WINDOW_GET_BUFFER, (int *)(&kIndexBuffer));
		buf = (android_native_buffer_t *)kIndexBuffer.bufPtr;
        prvHandle = (private_handle_t *)buf->handle;

		mVideoBufferPhy[i].length = prvHandle->size;
		mVideoBufferPhy[i].phy_offset = prvHandle->phys;
		fb_data[i].nIndex = i;
        fb_data[i].nLength = mVideoBufferPhy[i].length;
        fb_data[i].pPhyiscAddr = (OMX_PTR)mVideoBufferPhy[i].phy_offset;
        fb_data[i].pVirtualAddr = fb_data[i].pPhyiscAddr;
#else
		sp<IMemory> buffer = 0;
		buffer = mCamera->getVideoBuffer(i);

		fsl_osal_memcpy((void*)&mVideoBufferPhy[i], buffer->pointer(),
				sizeof(VIDEOFRAME_BUFFER_PHY));

        fb_data[i].nIndex = i;
        fb_data[i].nLength = mVideoBufferPhy[i].length;
        fb_data[i].pPhyiscAddr = (OMX_PTR)mVideoBufferPhy[i].phy_offset;
        fb_data[i].pVirtualAddr = fb_data[i].pPhyiscAddr;
#endif
		printf("Physic memory got from Camera HAL: %p\n", fb_data[i].pPhyiscAddr);
        if(fb_data[i].pVirtualAddr == NULL) {
            LOG_ERROR("mmap buffer: %d failed.\n", i);
            ret = OMX_ErrorHardware;
            break;
        }

        AddHwBuffer(fb_data[i].pPhyiscAddr, fb_data[i].pVirtualAddr);
	}

	IPCThreadState::self()->restoreCallingIdentity(token);

	nFrameBuffer = nVideoBufNum;
	bFrameBufferInit = OMX_TRUE;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE CameraSource::CameraBufferDeInit()
{
    for(OMX_U32 i=0; i<nFrameBuffer; i++) {
        if(fb_data[i].pVirtualAddr != NULL) {
            RemoveHwBuffer(fb_data[i].pVirtualAddr);
        }
    }

    bFrameBufferInit = OMX_FALSE;
    nAllocating = 0;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE CameraSource::SearchBuffer(
        OMX_PTR pAddr, 
        OMX_U32 *pIndex)
{
    OMX_U32 nIndex = 0;
    OMX_U32 i;
    OMX_BOOL bFound = OMX_FALSE;

    for(i=0; i<nFrameBuffer; i++) {
        if(fb_data[i].pVirtualAddr == pAddr) {
            nIndex = i;
            bFound = OMX_TRUE;
            break;
        }
    }

    if(bFound != OMX_TRUE)
        return OMX_ErrorUndefined;

    *pIndex = nIndex;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE CameraSource::SearchBufferPhy(
        OMX_PTR pAddr, 
        OMX_U32 *pIndex)
{
    OMX_U32 nIndex = 0;
    OMX_U32 i;
    OMX_BOOL bFound = OMX_FALSE;

    for(i=0; i<nFrameBuffer; i++) {
        if((OMX_PTR)(mVideoBufferPhy[i].phy_offset) == pAddr) {
            nIndex = i;
            bFound = OMX_TRUE;
            break;
        }
    }

    if(bFound != OMX_TRUE)
        return OMX_ErrorUndefined;

    *pIndex = nIndex;

    return OMX_ErrorNone;
}

#ifdef ICS
CameraSource::ProxyListener::ProxyListener(CameraSource *source) {
	mSource = source;
}

void CameraSource::ProxyListener::dataCallbackTimestamp(
		nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) {
	mSource->dataCallbackTimestamp((OMX_TICKS)timestamp/1000, (OMX_S32)msgType, (OMX_PTR)(&dataPtr));
}

void CameraSource::DeathNotifier::binderDied(const wp<IBinder>& who) {
	LOG_DEBUG("Camera recording proxy died");
}
#endif

/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE CameraSourceInit(OMX_IN OMX_HANDLETYPE pHandle)
		{
			OMX_ERRORTYPE ret = OMX_ErrorNone;
			CameraSource *obj = NULL;
			ComponentBase *base = NULL;

			obj = FSL_NEW(CameraSource, ());
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
