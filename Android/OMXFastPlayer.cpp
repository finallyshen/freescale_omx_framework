/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <OMXFastPlayer.h>
#include <ISurface.h>
#include <Surface.h>
#include <private/media/VideoFrame.h>
#include <cutils/properties.h>
#ifdef HONEY_COMB
#include <gui/ISurfaceTexture.h>
#include <gui/SurfaceTextureClient.h>
#endif

#include <fcntl.h>
#include <linux/mxcfb.h>

#include "OMX_GraphManager.h"
#include "OMXAndroidUtils.h"
#undef OMX_MEM_CHECK
#include "Mem.h"
#include "Log.h"
#include "OMX_Common.h"

using namespace android;
using android::Surface;

//#define LOG_DEBUG printf

OMXFastPlayer::OMXFastPlayer(int nMediaType)
{
    OMX_GraphManager* gm = NULL;

    LOG_DEBUG("OMXFastPlayer constructor.\n");

    LogInit(-1, NULL);

}

OMXFastPlayer::~OMXFastPlayer()
{
    LOG_DEBUG("OMXFastPlayer de-constructor.\n");
   
    LogDeInit();
}

status_t OMXFastPlayer::initCheck()
{
    return NO_ERROR;
}

status_t OMXFastPlayer::setDataSource(
        const char *url, 
        const KeyedVector<String8, String8> *headers)
{
    LOG_DEBUG("OMXFastPlayer set data source %s\n", url);
    return NO_ERROR;
}

status_t OMXFastPlayer::setDataSource(
        int fd, 
        int64_t offset, 
        int64_t length)
{
    LOG_DEBUG("OMXFastPlayer set data source, fd:%d, offset: %lld, length: %lld.\n",
            fd, offset, length);

    return NO_ERROR;
}

status_t OMXFastPlayer::setVideoSurface(const sp<ISurface>& surface)
{
    LOG_DEBUG("OMXFastPlayer::setVideoSurface: ISurface\n");
    return NO_ERROR;
}

status_t OMXFastPlayer::setVideoSurface(const android::sp<android::Surface>& surface)
{
    LOG_DEBUG("OMXFastPlayer::setVideoSurface: Surface\n");

    return NO_ERROR;
}

status_t OMXFastPlayer::setVideoSurfaceTexture(const sp<ISurfaceTexture>& surfaceTexture)
{
    return NO_ERROR;
}

status_t OMXFastPlayer::prepare()
{

    LOG_DEBUG("OMXFastPlayer prepare.\n");

    return NO_ERROR;
}

status_t OMXFastPlayer::prepareAsync()
{
    LOG_DEBUG("OMXFastPlayer prepareAsync.\n");
    sendEvent(MEDIA_PREPARED);

    return NO_ERROR;
}

status_t OMXFastPlayer::start()
{
    LOG_DEBUG("OMXFastPlayer start \n");
    return NO_ERROR;
}

status_t OMXFastPlayer::stop()
{
    LOG_DEBUG("OMXFastPlayer stop.\n");
    return NO_ERROR;
}

status_t OMXFastPlayer::pause()
{
    LOG_DEBUG("OMXFastPlayer pause .\n");
    
    return NO_ERROR;
}

status_t OMXFastPlayer::captureCurrentFrame(VideoFrame** pvframe)
{

    LOG_DEBUG("OMXFastPlayer captureFrame.\n");

    return NO_ERROR;
}

status_t OMXFastPlayer::setVideoCrop(
        int top,
        int left, 
        int bottom, 
        int right)
{
    return NO_ERROR;
}

bool OMXFastPlayer::isPlaying()
{
    LOG_DEBUG("OMXFastPlayer isPlaying State\n");

    return true;
}

status_t OMXFastPlayer::seekTo(int msec)
{
    LOG_DEBUG("OMXFastPlayer want seekTo %d\n", msec);
    if(msec > 4000)
        return BAD_VALUE;
    sendEvent(MEDIA_SEEK_COMPLETE);
    return NO_ERROR;
}

status_t OMXFastPlayer::getCurrentPosition(int *msec)
{
    *msec = 0;

    LOG_DEBUG("OMXFastPlayer Cur\n");

    return NO_ERROR;
}

status_t OMXFastPlayer::getDuration(int *msec)
{
    *msec = 4000;

    LOG_LOG("OMXFastPlayer Dur\n" );

    return NO_ERROR;
}

status_t OMXFastPlayer::reset()
{

    LOG_DEBUG("OMXFastPlayer Reset .\n");

    return NO_ERROR;
}

status_t OMXFastPlayer::setLooping(int loop)
{
    bLoop = loop > 0 ? true : false;
    return NO_ERROR;
}

status_t OMXFastPlayer::suspend()
{

    LOG_DEBUG("OMXFastPlayer Suspend.\n");

    return NO_ERROR;
}

status_t OMXFastPlayer::resume()
{

    LOG_DEBUG("OMXFastPlayer Resume from suspend .\n");

    return NO_ERROR;
}

status_t OMXFastPlayer::invoke(
        const Parcel& request, 
        Parcel *reply)
{
    return INVALID_OPERATION;
}

status_t OMXFastPlayer::getMetadata(
        const SortedVector<media::Metadata::Type>& ids, 
        Parcel *records)
{
    return NO_ERROR;
}

status_t OMXFastPlayer::setAudioEqualizer(bool isEnable)
{

    return NO_ERROR;
}

status_t OMXFastPlayer::setAudioEffect(
        int iBandIndex, 
        int iBandFreq, 
        int iBandGain)
{

    return NO_ERROR;
}

int OMXFastPlayer::getTrackCount()
{

    return 2;
}

char* OMXFastPlayer::getTrackName(int index)
{
    char name[32];

    LOG_DEBUG("getTrackName for track%d\n", index);


    sprintf(name, "Track%d", index);
    return fsl_osal_strdup(name);
}

int OMXFastPlayer::getDefaultTrack()
{
    return 1;
}

status_t OMXFastPlayer::selectTrack(int index)
{
    LOG_DEBUG("Select track: %d\n", index);

    return NO_ERROR;
}

status_t OMXFastPlayer::setPlaySpeed(int speed)
{

    LOG_DEBUG("Set play speed to: %d\n",speed);

    return NO_ERROR;
}

status_t OMXFastPlayer::ProcessEvent(int eventID, void* Eventpayload)
{
    GM_EVENT EventID = (GM_EVENT) eventID;

    switch(EventID) {
        default:
            break;
    }

    return NO_ERROR;
}

status_t OMXFastPlayer::ProcessAsyncCmd()
{
    status_t ret = NO_ERROR;

    fsl_osal_sem_wait(sem);

    if(bStopPCmdThread == OMX_TRUE)
        return UNKNOWN_ERROR;

    switch(msg) {
        default:
            break;
    }

    return NO_ERROR;
}

status_t OMXFastPlayer::PropertyCheck()
{
    if(bStopPCheckThread == true)
        return UNKNOWN_ERROR;

    if(bSuspend == true)
        return NO_ERROR;

    CheckTvOutSetting();
    if(bStopPCheckThread == true)
        return UNKNOWN_ERROR;

    CheckDualDisplaySetting();
    if(bStopPCheckThread == true)
        return UNKNOWN_ERROR;

    CheckFB0DisplaySetting();
    if(bStopPCheckThread == true)
        return UNKNOWN_ERROR;

    CheckSurfaceRegion();
    if(bStopPCheckThread == true)
        return UNKNOWN_ERROR;

    return NO_ERROR;
}

status_t OMXFastPlayer::setVideoDispRect(
        int top,
        int left, 
        int bottom, 
        int right)
{
    LOG_DEBUG("setVideoDispRect .\n");

    return NO_ERROR;
}

#ifdef HONEY_COMB
status_t OMXFastPlayer::getSurfaceRegion(
        int *top,
        int *left, 
        int *bottom, 
        int *right, 
        int *rot)
{
    return NO_ERROR;
}
#else
status_t OMXFastPlayer::getSurfaceRegion(
        int *top,
        int *left, 
        int *bottom, 
        int *right, 
        int *rot)
{
    return NO_ERROR;
}
#endif

status_t OMXFastPlayer::CheckTvOutSetting()
{
    LOG_LOG("rw.VIDEO_TVOUT_DISPLAY: \n");
    return NO_ERROR;
}

status_t OMXFastPlayer::CheckDualDisplaySetting()
{

    LOG_LOG("sys.SECOND_DISPLAY_ENABLED\n");

    return NO_ERROR;
}

/*
 * Customer requires that OMXFastPlayer changes display resolution according to setup of FB0 dynamically
 */
status_t OMXFastPlayer::CheckFB0DisplaySetting()
{
    LOG_LOG("rw.VIDEO_FB0_DISPLAY: \n");
    
    return NO_ERROR;
}


status_t OMXFastPlayer::CheckSurfaceRegion()
{
    return NO_ERROR;
}

status_t OMXFastPlayer::DoSeekTo(int msec)
{
    LOG_DEBUG("OMXFastPlayer seekTo %d\n", msec/1000);
	return NO_ERROR;
}

status_t OMXFastPlayer::setParameter(int key, const Parcel &request)
{ 
	return NO_ERROR;
}

status_t OMXFastPlayer::getParameter(int key, Parcel *reply)
{ 
	return NO_ERROR;
}



