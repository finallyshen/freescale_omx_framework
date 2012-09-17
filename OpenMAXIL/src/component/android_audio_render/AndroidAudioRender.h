/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file AndroidAudioRender.h
 *  @brief Class definition of AndroidAudioRender Component
 *  @ingroup AndroidAudioRender
 */

#ifndef AlsaRender_h
#define AlsaRender_h

#include <Errors.h>
#include <AudioTrack.h>
#include <MediaPlayerInterface.h>
#include <MediaPlayerService.h>
#include "ComponentBase.h"
#include "AudioRender.h"

namespace android {

class AndroidAudioRender : public AudioRender {
    public:
        AndroidAudioRender();
    private:
        OMX_ERRORTYPE SelectDevice(OMX_PTR device);
        OMX_ERRORTYPE OpenDevice();
        OMX_ERRORTYPE CloseDevice();
        OMX_ERRORTYPE SetDevice();
        OMX_ERRORTYPE ResetDevice();
        OMX_ERRORTYPE DrainDevice();
        OMX_ERRORTYPE DeviceDelay(OMX_U32 *nDelayLen);
        OMX_ERRORTYPE WriteDevice(OMX_U8 *pBuffer, OMX_U32 nActuralLen);
        OMX_ERRORTYPE DoExec2Pause();
        OMX_ERRORTYPE DoPause2Exec();
        OMX_BOOL bNativeDevice;
        sp<MediaPlayerBase::AudioSink> mAudioSink;

        OMX_U32 nWrited;
        OMX_U32 nBufferSize;
};

}

#endif
/* File EOF */
