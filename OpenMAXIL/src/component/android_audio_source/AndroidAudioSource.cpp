/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "AndroidAudioSource.h"

AndroidAudioSource::AndroidAudioSource()
{ 
	fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_source.android.sw-based");
	ComponentVersion.s.nVersionMajor = 0x0;
	ComponentVersion.s.nVersionMinor = 0x1;
	ComponentVersion.s.nRevision = 0x0;
	ComponentVersion.s.nStep = 0x0;
	role_cnt = 1;
	role[0] = (OMX_STRING)"audio_source.android";

    nSourceFrames = 0;
    mMaxAmplitude = 0;
	nCurTS = 0;
    lock = NULL;
	mTrackMaxAmplitude = OMX_FALSE;
    mStarted = OMX_FALSE;
}

OMX_ERRORTYPE AndroidAudioSource::InitSourceComponent()
{
    if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal)) {
        LOG_ERROR("Create mutex for android device failed.\n");
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioSource::DeInitSourceComponent()
{
    if(lock)
        fsl_osal_mutex_destroy(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioSource::OpenDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    printf("sampleRate: %d, channels: %d", PcmMode.nSamplingRate, PcmMode.nChannels);
    if (!(PcmMode.nChannels == 1 || PcmMode.nChannels == 2)) {
		return OMX_ErrorBadParameter;
	}

    OMX_U32 flags = AudioRecord::RECORD_AGC_ENABLE |
                     AudioRecord::RECORD_NS_ENABLE  |
                     AudioRecord::RECORD_IIR_ENABLE;

	OMX_S32 inputSource = nAudioSource;
	printf("Audio source: %d\n", inputSource);
#ifdef ICS
    mRecord = FSL_NEW(AudioRecord, (
                inputSource, PcmMode.nSamplingRate, AUDIO_FORMAT_PCM_SUB_16_BIT,
                PcmMode.nChannels > 1? AUDIO_CHANNEL_IN_STEREO: AUDIO_CHANNEL_IN_MONO,
                4 * kMaxBufferSize / sizeof(OMX_S16), /* Enable ping-pong buffers */
                flags));
#else
    mRecord = FSL_NEW(AudioRecord, (
	            inputSource, PcmMode.nSamplingRate, AudioSystem::PCM_16_BIT,
		        PcmMode.nChannels > 1? AudioSystem::CHANNEL_IN_STEREO: AudioSystem::CHANNEL_IN_MONO,
                4 * kMaxBufferSize / sizeof(OMX_S16), /* Enable ping-pong buffers */
                flags));
#endif

    mInitCheck = mRecord->initCheck();
	if (mInitCheck != 0) {
		printf("AndroidAudioSource init fail.\n");
		return OMX_ErrorHardware;
	}

	nSampleSize = PcmMode.nChannels * 2;
	nPeriodSize = kMaxBufferSize/nSampleSize;
	nFadeInFadeOutProcessLen = PcmMode.nSamplingRate * FADEPROCESSTIME / 1000 * nSampleSize; 

	ret = SetDevice();
	if (ret != OMX_ErrorNone)
		return ret;

	return ret;
}

OMX_ERRORTYPE AndroidAudioSource::CloseDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    FSL_DELETE(mRecord);
    mRecord = NULL;

	return ret;
}

OMX_ERRORTYPE AndroidAudioSource::SetDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	return ret;
}

OMX_ERRORTYPE AndroidAudioSource::StartDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (mInitCheck != 0)
		return OMX_ErrorHardware;

    mRecord->start();
	mStarted = OMX_TRUE;

	return ret;
}

OMX_ERRORTYPE AndroidAudioSource::StopDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (mInitCheck != 0)
		return OMX_ErrorHardware;
    if (mStarted != OMX_TRUE)
        return OMX_ErrorNotReady;

    mRecord->stop();
    mStarted = OMX_FALSE;

	return ret;
}

void AndroidAudioSource::trackMaxAmplitude(OMX_S16 *data, OMX_S32 nSamples) {
    for (OMX_S32 i = nSamples; i > 0; --i) {
        OMX_S16 value = *data++;
        if (value < 0) {
            value = -value;
        }
        if (mMaxAmplitude < value) {
            mMaxAmplitude = value;
        }
    }
}

OMX_S16 AndroidAudioSource::getMaxAmplitude() {
    // First call activates the tracking.
    if (!mTrackMaxAmplitude) {
        mTrackMaxAmplitude = OMX_TRUE;
    }
    OMX_S16 value = mMaxAmplitude;
    mMaxAmplitude = 0;
    LOG_DEBUG("max amplitude since last call: %d", value);
    return value;
}

OMX_ERRORTYPE AndroidAudioSource::GetOneFrameFromDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U8 *pBuffer = pDeviceReadBuffer;
    OMX_U32 nReadSize = nPeriodSize * nSampleSize;

	if (mInitCheck != 0)
		return OMX_ErrorHardware;
    if (mStarted != OMX_TRUE)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);

	uint32_t numFramesRecorded;
	mRecord->getPosition(&numFramesRecorded);
	LOG_DEBUG("AudioRecord record frames: %d\n", numFramesRecorded);

	OMX_U32 numLostBytes = mRecord->getInputFramesLost() << 1;
	LOG_DEBUG("AudioRecord lost bytes: %d\n", numLostBytes);

	//static int nCnt = 0;
	//nCnt ++;
	//if (nCnt == 100)
	//	numLostBytes = 2048;
	if (numLostBytes) {
		nDeviceReadLen = numLostBytes;
		printf("AudioRecord lost bytes: %d\n", numLostBytes);
		// AudioRecord frame lost, need add zero and Fade in/out.
		fsl_osal_mutex_unlock(lock);
		return OMX_ErrorUndefined;
	}

	OMX_S32 n = mRecord->read(pBuffer, nReadSize);
	if (n <= 0) {
		LOG_ERROR("Read from AudioRecord returns: %ld", n);
		fsl_osal_mutex_unlock(lock);
		return OMX_ErrorHardware;
	}

	nDeviceReadLen = n;

	LOG_DEBUG("AudioRecord bytes: %d\n", nDeviceReadLen);
	if (mTrackMaxAmplitude) {
		trackMaxAmplitude((OMX_S16 *)pBuffer, n >> 1);
	}

	fsl_osal_mutex_unlock(lock);

	return ret;
}

OMX_ERRORTYPE AndroidAudioSource::GetDeviceDelay(OMX_U32 *nDelayLen)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	//  / 3 is workaround for can't get accurate latency.
	*nDelayLen = mRecord->latency() / 3 * PcmMode.nSamplingRate * nSampleSize / 1000;
	LOG_DEBUG("Audio device delay: %d\n", *nDelayLen);

	return ret;
}


/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE AndroidAudioSourceInit(OMX_IN OMX_HANDLETYPE pHandle)
		{
			OMX_ERRORTYPE ret = OMX_ErrorNone;
			AndroidAudioSource *obj = NULL;
			ComponentBase *base = NULL;

			obj = FSL_NEW(AndroidAudioSource, ());
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
