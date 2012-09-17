/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <AudioSystem.h>
#include "AndroidAudioRender.h"

using namespace android;

AndroidAudioRender::AndroidAudioRender()
{ 
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_render.android.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_render.android";
    bInContext = OMX_FALSE;
    nPorts = NUM_PORTS;
    mAudioSink = NULL;
    bNativeDevice = OMX_FALSE;
    nWrited = 0;
}

OMX_ERRORTYPE AndroidAudioRender::SelectDevice(OMX_PTR device)
{
    sp<MediaPlayerBase::AudioSink> *pSink;

    pSink = (sp<MediaPlayerBase::AudioSink> *)device;
    mAudioSink = *pSink;
    LOG_DEBUG("AndroidAudioRender Set AudioSink %p\n", &mAudioSink);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioRender::OpenDevice()
{
    status_t ret = NO_ERROR;

    LOG_DEBUG("AndroidAudioRender OpenDevice\n");

    if(mAudioSink == NULL) {
#ifdef RUN_WITH_GM
        /* this is used for run with oxmgm, 
         * you need to define MediaPlayerService::AudioOutput
         * as public class in file MediaPlayerService.h */
        mAudioSink = FSL_NEW(MediaPlayerService::AudioOutput, ());
        if(mAudioSink == NULL) {
            LOG_ERROR("Failed to create AudioSink.\n");
            return OMX_ErrorInsufficientResources;
        }
        bNativeDevice = OMX_TRUE;
#else
        LOG_ERROR("Can't open mAudioSink device.\n");
        return OMX_ErrorHardware;
#endif

    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioRender::CloseDevice()
{
    LOG_DEBUG("AndroidAudioRender CloseDevice.\n");

    if(mAudioSink == NULL)
        return OMX_ErrorBadParameter;

    mAudioSink->stop();
    mAudioSink->close();

    if(bNativeDevice == OMX_TRUE) {
        LOG_DEBUG("clear mAudioSink.\n");
        mAudioSink.clear();
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioRender::SetDevice()
{
    status_t status = NO_ERROR;
    OMX_S32 PcmBit;
    OMX_U32 nSamplingRate;

    if(mAudioSink == NULL)
        return OMX_ErrorBadParameter;

    LOG_DEBUG("SetDevice: SampleRate: %d, Channels: %d, PcmBits: %d, nClockScale %x\n", 
            PcmMode.nSamplingRate, PcmMode.nChannels, PcmMode.nBitPerSample, nClockScale);

    nSamplingRate = (OMX_U64)PcmMode.nSamplingRate * nClockScale / Q16_SHIFT;


    switch(PcmMode.nBitPerSample) {
        case 8:
#ifdef ICS
            PcmBit = AUDIO_FORMAT_PCM_SUB_8_BIT;
#else
            PcmBit = AudioSystem::PCM_8_BIT;
#endif
            break;
        case 16:
#ifdef ICS
            PcmBit = AUDIO_FORMAT_PCM_SUB_16_BIT;
#else
            PcmBit = AudioSystem::PCM_16_BIT;
#endif
            break;
        default:
            return OMX_ErrorBadParameter;
    }

    if(NO_ERROR != mAudioSink->open(nSamplingRate, PcmMode.nChannels, 
                PcmBit, DEFAULT_AUDIOSINK_BUFFERCOUNT)) {
        LOG_ERROR("Failed to open AudioOutput device.\n");
        return OMX_ErrorHardware;
    }

    mAudioSink->start();

    LOG_DEBUG("buffersize: %d, frameSize: %d, frameCount: %d\n", 
            mAudioSink->bufferSize(), mAudioSink->frameSize(), mAudioSink->frameCount());

    nBufferSize = mAudioSink->bufferSize();
    nSampleSize = mAudioSink->frameSize();
    nPeriodSize = nBufferSize/nSampleSize/8;

    nFadeInFadeOutProcessLen = nSamplingRate * FADEPROCESSTIME / 1000 * nSampleSize; 

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioRender::WriteDevice(OMX_U8 *pBuffer, OMX_U32 nActuralLen)
{
    if(mAudioSink == NULL)
        return OMX_ErrorBadParameter;

    LOG_DEBUG("AndroidAudioRender write: %d\n", nActuralLen);
	if(PcmMode.nBitPerSample == 8)
	{
		// Convert to U8
		OMX_S32 i,Len;
		OMX_U8 *pDst =(OMX_U8 *)pBuffer;
		OMX_S8 *pSrc =(OMX_S8 *)pBuffer;
		Len = nActuralLen;
		for(i=0;i<Len;i++)
		{
				*pDst++ = *pSrc++ + 128;
		}
	}

    mAudioSink->write(pBuffer, nActuralLen);
    nWrited += nActuralLen;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioRender::DrainDevice()
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioRender::DeviceDelay(OMX_U32 *nDelayLen)
{
    OMX_U32 nLatency;
    OMX_U32 nPSize = nPeriodSize * nSampleSize;

    if(nWrited > nBufferSize + 3 * nPSize)
        *nDelayLen = nBufferSize + 3 * nPSize;
    else
        *nDelayLen = nWrited;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioRender::ResetDevice()
{
    if(mAudioSink == NULL)
        return OMX_ErrorBadParameter;

    mAudioSink->flush();
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioRender::DoExec2Pause()
{
    if(mAudioSink == NULL)
        return OMX_ErrorBadParameter;

	if (bSendEOS == OMX_TRUE) {
#ifdef FROYO
		OMX_S64 latency = mAudioSink->latency() * 1000;
		printf("Add latency: %lld\n", latency);
		fsl_osal_sleep(latency);
#endif
		mAudioSink->stop();
	} else {
		mAudioSink->pause();
	}
    return OMX_ErrorNone;
}

OMX_ERRORTYPE AndroidAudioRender::DoPause2Exec()
{
    if(mAudioSink == NULL)
        return OMX_ErrorBadParameter;

    mAudioSink->start();
    return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
    OMX_ERRORTYPE AndroidAudioRenderInit(OMX_IN OMX_HANDLETYPE pHandle)
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        AndroidAudioRender *obj = NULL;
        ComponentBase *base = NULL;

        obj = FSL_NEW(AndroidAudioRender, ());
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
