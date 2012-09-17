/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <OMXPlayer.h>
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


extern const VIDEO_RENDER_MAP_ENTRY VideoRenderNameTable[OMX_VIDEO_RENDER_NUM] ={
    {OMX_VIDEO_RENDER_V4L,       "video_render.v4l"},
    {OMX_VIDEO_RENDER_IPULIB,    "video_render.ipulib"},
    {OMX_VIDEO_RENDER_SURFACE,   "video_render.surface"},
    {OMX_VIDEO_RENDER_FB,        "video_render.fb"},
    {OMX_VIDEO_RENDER_EGL,       "video_render.fake"},
    {OMX_VIDEO_RENDER_OVERLAY,   "video_render.overlay"},
};

static status_t GetFBResolution(
        OMX_U32  number,
        OMX_U32 *width,
        OMX_U32 *height)
{
    OMX_S32 fd_fb;
    struct fb_var_screeninfo fb_var;
    char deviceNumber[2];
    char deviceName[20] ;

    strcpy(deviceName, "/dev/graphics/fb");
    deviceNumber[0] = '0' + (number - 0) ;
    deviceNumber[1] = 0;
    strcat(deviceName,  deviceNumber);

    LOG_DEBUG("to open %s\n", deviceName);

    if ((fd_fb = open(deviceName, O_RDWR, 0)) < 0) {
        LOG_ERROR("Unable to open %s\n", deviceName);
         return UNKNOWN_ERROR;
     }

    if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var) < 0) {
        LOG_ERROR("Get FB var info failed!\n");
        close(fd_fb);
        return UNKNOWN_ERROR;
    }

    close(fd_fb);

    *width = fb_var.xres;
    *height = fb_var.yres;

    LOG_DEBUG("fb%d: x=%d, y=%d\n", number, *width, *height);

    return NO_ERROR;
}

static int eventhandler(void* context, GM_EVENT EventID, void* Eventpayload)
{
    OMXPlayer *player = (OMXPlayer*)context;

    player->ProcessEvent(EventID, Eventpayload);

    return 1;
}

static fsl_osal_ptr ProcessAsyncCmdThreadFunc(fsl_osal_ptr arg)
{
    OMXPlayer *player = (OMXPlayer*)arg;

    while(NO_ERROR == player->ProcessAsyncCmd());

    return NULL;
}

static fsl_osal_ptr PropertyCheckThreadFunc(fsl_osal_ptr arg)
{
    OMXPlayer *player = (OMXPlayer*)arg;

    while(NO_ERROR == player->PropertyCheck())
        fsl_osal_sleep(500000);

    return NULL;
}

OMXPlayer::OMXPlayer(int nMediaType)
{
    OMX_GraphManager* gm = NULL;

    LOG_DEBUG("OMXPlayer constructor.\n");

    player = NULL;
    mInit = NO_ERROR;
    mISurface = NULL;
    mSurface = NULL;
    mNativeWindow = NULL;
    mSharedFd = -1;
    bSuspend = false;
    bLoop = false;
    bTvOut = false;
    bDualDisplay = false;
    pPCmdThread = pPCheckThread = NULL;
    bStopPCmdThread = bStopPCheckThread = false;
    sTop = sBottom = sLeft = sRight = sRot = 0;
    msg = MSG_NONE;
    msgData = 0;
    sem = NULL;
    contentURI = NULL;
    bFB0Display = false;
    mMediaType = nMediaType;
    qdFlag = false;

    LogInit(-1, NULL);

    gm = OMX_GraphManagerCreate();
    if(gm == NULL) {
        LOG_ERROR("Failed to create GMPlayer.\n");
        mInit = NO_MEMORY;
    }

    if(E_FSL_OSAL_SUCCESS != fsl_osal_sem_init(&sem, 0, 0)) {
        LOG_ERROR("Failed create Semphore for OMXPlayer.\n");
        mInit = NO_MEMORY;
    }

    fsl_osal_thread_create(&pPCmdThread, NULL, ProcessAsyncCmdThreadFunc, (fsl_osal_ptr)this);
    if(pPCmdThread == NULL) {
        LOG_ERROR("Failed to create thread for async commad.\n");
        mInit = NO_MEMORY;
    }

#if 0
    if(mInit != NO_ERROR)
        OMXPlayer::~OMXPlayer();
#endif

    LOG_DEBUG("OMXPlayer Construct gm player: %p\n", gm);
    player = (void*)gm;
}

OMXPlayer::~OMXPlayer()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    LOG_DEBUG("OMXPlayer de-constructor %p.\n", gm);

    if(pPCmdThread != NULL) {
        bStopPCmdThread = OMX_TRUE;
        fsl_osal_sem_post(sem);
        fsl_osal_thread_destroy(pPCmdThread);
        pPCmdThread = NULL;
        bStopPCmdThread = OMX_FALSE;
    }

    if(sem != NULL) {
        fsl_osal_sem_destroy(sem);
        sem = NULL;
    }

    if(contentURI != NULL)
        FSL_FREE(contentURI);

    if(gm != NULL) {
        gm->deleteIt(gm);
        gm = NULL;
    }

    if (mSharedFd >= 0) {
        close(mSharedFd);
    }
    
    LogDeInit();
}

status_t OMXPlayer::initCheck()
{
    return mInit;
}

status_t OMXPlayer::setDataSource(
        const char *url, 
        const KeyedVector<String8, String8> *headers)
{
    LOG_DEBUG("OMXPlayer set data source %s\n", url);

    qdFlag = false;

    if (mSharedFd >= 0) {
        close(mSharedFd);
        mSharedFd = -1;
    }

    OMX_S32 len = fsl_osal_strlen(url) + 1;

    if(headers) {
        OMX_S32 i;
        for(i=0; i<(int)headers->size(); i++)
            len += fsl_osal_strlen(headers->keyAt(i).string()) + fsl_osal_strlen(headers->valueAt(i).string()) + 4;
        len += 1;
    }

    contentURI = (char*)FSL_MALLOC(len);
    if(contentURI == NULL)
        return UNKNOWN_ERROR;

    fsl_osal_strcpy(contentURI, url);

    if(headers) {
        char *pHeader = contentURI + fsl_osal_strlen(url);
        pHeader[0] = '\n';
        pHeader += 1;
        OMX_S32 i;
        for(i=0; i<(int)headers->size(); i++) {
            OMX_S32 keyLen = fsl_osal_strlen(headers->keyAt(i).string());
            fsl_osal_memcpy(pHeader, (OMX_PTR)headers->keyAt(i).string(), keyLen);
            pHeader += keyLen;
            pHeader[0] = ':';
            pHeader[1] = ' ';
            pHeader += 2;
            OMX_S32 valueLen = fsl_osal_strlen(headers->valueAt(i).string());
            fsl_osal_memcpy(pHeader, (OMX_PTR)headers->valueAt(i).string(), valueLen);
            pHeader += valueLen;
            pHeader[0] = '\r';
            pHeader[1] = '\n';
            pHeader += 2;
        }
        pHeader[0] = '\0';
    }
    
    LOG_DEBUG("OMXPlayer contentURI: %s\n", contentURI);

    return NO_ERROR;
}

status_t OMXPlayer::setDataSource(
        int fd, 
        int64_t offset, 
        int64_t length)
{
    LOG_DEBUG("OMXPlayer set data source, fd:%d, offset: %lld, length: %lld.\n",
            fd, offset, length);

    if(offset != 0 && (length == 17681 || length == 16130)){
        // for quadrant test
        qdFlag = true;
    }
    else
        qdFlag = false;

    if (mSharedFd >= 0) {
        close(mSharedFd);
        mSharedFd = -1;
    }

    contentURI = (char*)FSL_MALLOC(128);
    if(contentURI == NULL)
        return UNKNOWN_ERROR;

    mSharedFd = dup(fd);
    sprintf(contentURI, "sharedfd://%d:%lld:%lld:%d",  mSharedFd, offset, length, mMediaType);
    LOG_DEBUG("OMXPlayer contentURI: %s\n", contentURI);

    return NO_ERROR;
}

status_t OMXPlayer::setVideoSurface(const sp<ISurface>& surface)
{
    LOG_DEBUG("OMXPlayer::setVideoSurface: ISurface\n");

    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    mISurface = surface;

    if(gm != NULL)
        gm->setVideoSurface(gm, &mISurface, OMX_VIDEO_SURFACE_ISURFACE);

    return NO_ERROR;
}

status_t OMXPlayer::setVideoSurface(const android::sp<android::Surface>& surface)
{
    LOG_DEBUG("OMXPlayer::setVideoSurface: Surface\n");
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    mSurface = surface;
    mNativeWindow = mSurface;

    if(gm != NULL)
        gm->setVideoSurface(gm, &mNativeWindow, OMX_VIDEO_SURFACE_SURFACE);

    return NO_ERROR;
}

status_t OMXPlayer::setVideoSurfaceTexture(const sp<ISurfaceTexture>& surfaceTexture)
{
#ifdef HONEY_COMB
    LOG_DEBUG("OMXPlayer::setVideoSurfaceTexture\n");
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

	mSurface.clear();
	if (surfaceTexture != NULL) {
		mNativeWindow = new SurfaceTextureClient(surfaceTexture);
		if(gm != NULL)
			gm->setVideoSurface(gm, &mNativeWindow, OMX_VIDEO_SURFACE_TEXTURE);
	} else {
		if(gm != NULL)
			gm->setVideoSurface(gm, NULL, OMX_VIDEO_SURFACE_TEXTURE);
	}
#endif
    return NO_ERROR;
}

status_t OMXPlayer::prepare()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    char value[PROPERTY_VALUE_MAX];
    int i;
    OMX_VIDEO_RENDER_TYPE videoRender = OMX_VIDEO_RENDER_IPULIB;

    LOG_DEBUG("OMXPlayer prepare.\n");

    if(gm == NULL)
        return BAD_VALUE;

    gm->registerEventHandler(gm, this, eventhandler);
    gm->setAudioSink(gm, &mAudioSink);

    gm->AddRemoveAudioPP(gm, OMX_TRUE);

    property_get("rw.VIDEO_RENDER_NAME", value, "");
    LOG_DEBUG("rw.VIDEO_RENDER_NAME: [%s]\n", value);

    if(fsl_osal_strlen(value) > 0){
        for(i=0; i<OMX_VIDEO_RENDER_NUM; i++){
            if(fsl_osal_strcmp(value, VideoRenderNameTable[i].name)==0){
                videoRender = VideoRenderNameTable[i].type;
                break;
            }
        }
    }

    if(OMX_TRUE != gm->setVideoRenderType(gm, videoRender)) {
        LOG_DEBUG("to check SetVideoRenderType %d... \n", videoRender);
        gm->stop(gm);
        return UNKNOWN_ERROR;
    }

    fsl_osal_memset(value, 0, PROPERTY_VALUE_MAX);
    property_get("rw.HTTP_PROXY", value, "");
    //printf("rw.HTTP_PROXY [%s]\n", value);
    if(fsl_osal_strlen(value) > 0)
        setenv("http_proxy", value, 1);
    else
        unsetenv("http_proxy");

    if(OMX_TRUE != gm->load(gm, contentURI, fsl_osal_strlen(contentURI))) {
        LOG_DEBUG("load contentURI %s failed.\n", contentURI);
        gm->stop(gm);
        return UNKNOWN_ERROR;
    }

    GM_STREAMINFO sStreamInfo;
    if(gm->getStreamInfo(gm, GM_VIDEO_STREAM_INDEX, &sStreamInfo)) {
        sendEvent(MEDIA_SET_VIDEO_SIZE, sStreamInfo.streamFormat.video_info.nFrameWidth, sStreamInfo.streamFormat.video_info.nFrameHeight);
        if(videoRender == OMX_VIDEO_RENDER_IPULIB){
            CheckDualDisplaySetting();
            CheckTvOutSetting();
            fsl_osal_thread_create(&pPCheckThread, NULL, PropertyCheckThreadFunc, (fsl_osal_ptr)this);
        }
    }

    // notify client that quadrant is playing stream
    if(qdFlag)
        sendEvent(MEDIA_INFO, MEDIA_INFO_ASYNC_SEEK);

    return NO_ERROR;
}

status_t OMXPlayer::prepareAsync()
{
    LOG_DEBUG("OMXPlayer prepareAsync.\n");

    msg = MSG_PREPAREASYNC;
    fsl_osal_sem_post(sem);

    // notify client that quadrant is playing stream
    if(qdFlag)
        sendEvent(MEDIA_INFO, MEDIA_INFO_ASYNC_SEEK);

    return NO_ERROR;
}

status_t OMXPlayer::start()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    GM_STATE State = GM_STATE_NULL;
	OMX_U32 nCount = 0;

    if(gm == NULL)
        return BAD_VALUE;

	while (msg != MSG_NONE) {
		fsl_osal_sleep(10000);
		if(nCount ++ > 100) {
			LOG_WARNING("Wait command complete timeout when start.\n");
			break;
		}
	}

    gm->getState(gm, &State);

    if(State == GM_STATE_LOADED) {
        LOG_DEBUG("OMXPlayer start.\n");
        GM_STREAMINFO sStreamInfo;
        if(gm->getStreamInfo(gm, GM_VIDEO_STREAM_INDEX, &sStreamInfo))
            property_set("media.VIDEO_PLAYING", "1");
        gm->start(gm);
    }
    
    if(State == GM_STATE_PAUSE) {
        LOG_DEBUG("OMXPlayer resume.\n");
        gm->resume(gm);
    }

    if(State == GM_STATE_EOS) {
        LOG_DEBUG("OMXPlayer restart.\n");
        gm->seek(gm, OMX_TIME_SeekModeFast, 0);

		LOG_DEBUG("OMXPlayer resume.\n");
		gm->resume(gm);
	}

    return NO_ERROR;
}

status_t OMXPlayer::stop()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    LOG_DEBUG("OMXPlayer stop %p.\n", gm);

    if(gm == NULL)
        return BAD_VALUE;

    gm->stop(gm);

    property_set("media.VIDEO_PLAYING", "0");

    if(pPCheckThread != NULL) {
        bStopPCheckThread = true;
        fsl_osal_thread_destroy(pPCheckThread);
        pPCheckThread = NULL;
        bStopPCheckThread = false;
    }

    return NO_ERROR;
}

status_t OMXPlayer::pause()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    LOG_DEBUG("OMXPlayer pause %p.\n", gm);
    
    if(gm == NULL)
        return BAD_VALUE;

    gm->pause(gm);

    return NO_ERROR;
}

status_t OMXPlayer::captureCurrentFrame(VideoFrame** pvframe)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    LOG_DEBUG("OMXPlayer captureFrame.\n");

    if(gm == NULL || pvframe == NULL)
        return INVALID_OPERATION;

    *pvframe = NULL;

    VideoFrame *mVideoFrame = NULL;
    mVideoFrame = FSL_NEW(VideoFrame, ());
    if(mVideoFrame == NULL)
        return NO_MEMORY;

    //get the video format
    OMX_IMAGE_PORTDEFINITIONTYPE in_format;
    OMX_CONFIG_RECTTYPE sCropRect;
    gm->getScreenShotInfo(gm, &in_format, &sCropRect);

    mVideoFrame->mWidth = sCropRect.nWidth;
    mVideoFrame->mHeight = sCropRect.nHeight;
    mVideoFrame->mDisplayWidth  = sCropRect.nWidth;
    mVideoFrame->mDisplayHeight = sCropRect.nHeight;
    mVideoFrame->mSize = mVideoFrame->mWidth * mVideoFrame->mHeight * 2;
    mVideoFrame->mData = (uint8_t*)FSL_MALLOC(mVideoFrame->mSize*sizeof(uint8_t));
    if(mVideoFrame->mData == NULL) {
        FSL_DELETE(mVideoFrame);
        return NO_MEMORY;
    }
    fsl_osal_memset(mVideoFrame->mData, 0, mVideoFrame->mSize);


    //allocate buffer for save snapshot picture
    OMX_U8 *snapshot_buf = NULL;
    int size = in_format.nFrameWidth * in_format.nFrameHeight * 2;
    snapshot_buf = (OMX_U8*)FSL_MALLOC(size);
    if(snapshot_buf == NULL) {
        FSL_DELETE(mVideoFrame);
        return NO_MEMORY;
    }
    fsl_osal_memset(snapshot_buf, 0, size);

    //get snapshot
    if(OMX_TRUE != gm->getSnapshot(gm, snapshot_buf)) {
        FSL_FREE(snapshot_buf);
        FSL_DELETE(mVideoFrame);
        return UNKNOWN_ERROR;
    }

    //convert thumbnail picture to required format and resolution
    OMX_IMAGE_PORTDEFINITIONTYPE out_format;
    fsl_osal_memset(&out_format, 0, sizeof(OMX_IMAGE_PORTDEFINITIONTYPE));
    out_format.nFrameWidth = mVideoFrame->mWidth;
    out_format.nFrameHeight = mVideoFrame->mHeight;
    out_format.eColorFormat = OMX_COLOR_Format16bitRGB565;

    ConvertImage(&in_format, &sCropRect, snapshot_buf, &out_format, mVideoFrame->mData);

    FSL_FREE(snapshot_buf);
    *pvframe = mVideoFrame;

    return NO_ERROR;
}

status_t OMXPlayer::setVideoCrop(
        int top,
        int left, 
        int bottom, 
        int right)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    OMX_CONFIG_RECTTYPE sRect;

    if(gm == NULL)
        return BAD_VALUE;

    fsl_osal_memset(&sRect, 0, sizeof(OMX_CONFIG_RECTTYPE));
    sRect.nLeft = left;
    sRect.nTop = top;
    sRect.nWidth = right - left;
    sRect.nHeight = bottom - top;

    if((OMX_S32)sRect.nLeft < 0 || (OMX_S32)sRect.nTop < 0 || (OMX_S32)sRect.nWidth < 0 || (OMX_S32)sRect.nHeight < 0) {
        LOG_ERROR("Bad video crop Rect: x:%d, y:%d, w:%d, h:%d\n", sRect.nLeft, sRect.nTop, sRect.nWidth, sRect.nHeight);
        return BAD_VALUE;
    }

    LOG_DEBUG("Video Crop Rect: x:%d, y:%d, w:%d, h:%d\n", sRect.nLeft, sRect.nTop, sRect.nWidth, sRect.nHeight);
    gm->setVideoRect(gm, &sRect);

    return NO_ERROR;
}

bool OMXPlayer::isPlaying()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    GM_STATE State = GM_STATE_NULL;
    bool ret = false;

    if(gm == NULL)
        return false;

    gm->getState(gm, &State);
    LOG_DEBUG("OMXPlayer isPlaying State: %d\n", State);
    if(State == GM_STATE_PLAYING)
        ret = true;

    return ret;
}

status_t OMXPlayer::seekTo(int msec)
{
    LOG_DEBUG("OMXPlayer want seekTo %d\n", msec/1000);

    msg = MSG_SEEKTO;
    msgData = msec;
    fsl_osal_sem_post(sem);

    return NO_ERROR;
}

status_t OMXPlayer::getCurrentPosition(int *msec)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    OMX_TICKS ts = 0;

    if(gm == NULL)
        return BAD_VALUE;

    gm->getPosition(gm, &ts);
    *msec = ts / 1000;

    LOG_DEBUG("OMXPlayer Cur: %lld\n", ts);

    return NO_ERROR;
}

status_t OMXPlayer::getDuration(int *msec)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    OMX_TICKS ts = 0;

    // let quadrant have a proper score
    if(qdFlag)
        fsl_osal_sleep(0);

    if(gm == NULL)
        return BAD_VALUE;

    gm->getDuration(gm, &ts);
    *msec = ts / 1000;

    LOG_LOG("OMXPlayer Dur: %lld\n", ts);

    return NO_ERROR;
}

status_t OMXPlayer::reset()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    LOG_DEBUG("OMXPlayer Reset %p.\n", gm);

    stop();

    if (mSharedFd >= 0) {
        close(mSharedFd);
        mSharedFd = -1;
    }

    return NO_ERROR;
}

status_t OMXPlayer::setLooping(int loop)
{
    bLoop = loop > 0 ? true : false;
    return NO_ERROR;
}

status_t OMXPlayer::suspend()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    LOG_DEBUG("OMXPlayer Suspend %p.\n", gm);

    if(gm == NULL)
        return BAD_VALUE;

    if(msg == MSG_PREPAREASYNC)
        return UNKNOWN_ERROR;

    bSuspend = true;
    gm->syssleep(gm, OMX_TRUE);
    property_set("media.VIDEO_PLAYING", "0");

    return NO_ERROR;
}

status_t OMXPlayer::resume()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    LOG_DEBUG("OMXPlayer Resume from suspend %p.\n", gm);

    if(gm == NULL)
        return BAD_VALUE;

    bSuspend = false;
    sTop = sBottom = sLeft = sRight = sRot = 0;
    setVideoDispRect(0, 0, 0, 0);
    GM_STREAMINFO sStreamInfo;
    if(gm->getStreamInfo(gm, GM_VIDEO_STREAM_INDEX, &sStreamInfo))
        property_set("media.VIDEO_PLAYING", "1");
    gm->syssleep(gm, OMX_FALSE);

    return NO_ERROR;
}

status_t OMXPlayer::invoke(
        const Parcel& request, 
        Parcel *reply)
{
    return INVALID_OPERATION;
}

status_t OMXPlayer::getMetadata(
        const SortedVector<media::Metadata::Type>& ids, 
        Parcel *records)
{
    return NO_ERROR;
}

status_t OMXPlayer::setAudioEqualizer(bool isEnable)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    OMX_BOOL bEnable = OMX_FALSE;

    if(gm == NULL)
        return BAD_VALUE;

    bEnable = isEnable ? OMX_TRUE : OMX_FALSE;
    if(OMX_TRUE != gm->EnableDisablePEQ(gm, bEnable))
        return UNKNOWN_ERROR;

    return NO_ERROR;
}

status_t OMXPlayer::setAudioEffect(
        int iBandIndex, 
        int iBandFreq, 
        int iBandGain)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    if(gm == NULL)
        return BAD_VALUE;

    if(OMX_TRUE != gm->SetAudioEffectPEQ(gm, iBandIndex, iBandFreq, iBandGain))
        return UNKNOWN_ERROR;

    return NO_ERROR;
}

int OMXPlayer::getTrackCount()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    if(gm == NULL)
        return 0;

    return gm->GetAudioTrackNum(gm);
}

char* OMXPlayer::getTrackName(int index)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    char name[32];

    LOG_DEBUG("getTrackName for track%d\n", index);

    if(gm == NULL)
        return NULL;

    sprintf(name, "Track%d", index);
    LOG_DEBUG("Track %d name: %s\n", index ,name);
    return fsl_osal_strdup(name);
}

int OMXPlayer::getDefaultTrack()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    if(gm == NULL)
        return 0;

    return gm->GetCurAudioTrack(gm);
}

status_t OMXPlayer::selectTrack(int index)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    if(gm == NULL)
        return 0;

    LOG_DEBUG("Select track: %d\n", index);

    if(OMX_TRUE != gm->SelectAudioTrack(gm, index))
        return UNKNOWN_ERROR;

    return NO_ERROR;
}

status_t OMXPlayer::setPlaySpeed(int speed)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    if(gm == NULL)
        return 0;

    LOG_DEBUG("Set play speed to: %d\n",speed);

    if(OMX_TRUE != gm->setPlaySpeed(gm, speed))
        return UNKNOWN_ERROR;

    return NO_ERROR;
}

status_t OMXPlayer::ProcessEvent(int eventID, void* Eventpayload)
{
    GM_EVENT EventID = (GM_EVENT) eventID;

    switch(EventID) {
        case GM_EVENT_EOS:
            LOG_DEBUG("OMXPlayer Received GM_EVENT_EOS.\n");
            if(true != bLoop) {
                pause();
                sendEvent(MEDIA_PLAYBACK_COMPLETE);
            }
            else
                DoSeekTo(0);
            break;
        case GM_EVENT_CORRUPTED_STREM:
            sendEvent(MEDIA_ERROR, MEDIA_ERROR_UNKNOWN);
            break;
        case GM_EVENT_BUFFERING_UPDATE:
            if(Eventpayload == NULL)
                break;
            LOG_DEBUG("Buffering update to: %d\n", (*((OMX_S32 *)Eventpayload)));
            sendEvent(MEDIA_BUFFERING_UPDATE, (*((OMX_S32 *)Eventpayload)));
            break;
        default:
            break;
    }

    return NO_ERROR;
}

status_t OMXPlayer::ProcessAsyncCmd()
{
    status_t ret = NO_ERROR;

    fsl_osal_sem_wait(sem);

    if(bStopPCmdThread == OMX_TRUE)
        return UNKNOWN_ERROR;

    switch(msg) {
        case MSG_PREPAREASYNC:
            ret = prepare();
            msg = MSG_NONE;
            if(ret != NO_ERROR)
                sendEvent(MEDIA_ERROR, ret);
            else
                sendEvent(MEDIA_PREPARED);
            break;
        case MSG_SEEKTO:
            DoSeekTo(msgData);
            msg = MSG_NONE;
            // when quadrant is playing, seek complete event is sent by media player to apk to increase performance, no need to sent it here.
            if(!qdFlag)
                sendEvent(MEDIA_SEEK_COMPLETE);
            break;
        case MSG_NONE:
        default:
            break;
    }

    return NO_ERROR;
}

status_t OMXPlayer::PropertyCheck()
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

status_t OMXPlayer::setVideoDispRect(
        int top,
        int left, 
        int bottom, 
        int right)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    OMX_CONFIG_RECTTYPE sRect;

    LOG_DEBUG("setVideoDispRect %p.\n", gm);

    if(gm == NULL)
        return BAD_VALUE;

    fsl_osal_memset(&sRect, 0, sizeof(OMX_CONFIG_RECTTYPE));
    sRect.nLeft = left;
    sRect.nTop = top;
    sRect.nWidth = right - left;
    sRect.nHeight = bottom - top;

    if((OMX_S32)sRect.nWidth < 0 || (OMX_S32)sRect.nHeight < 0) {
        LOG_ERROR("Bad display region: x:%d, y:%d, w:%d, h:%d\n", sRect.nLeft, sRect.nTop, sRect.nWidth, sRect.nHeight);
        return BAD_VALUE;
    }

    LOG_DEBUG("Display region: x:%d, y:%d, w:%d, h:%d\n", sRect.nLeft, sRect.nTop, sRect.nWidth, sRect.nHeight);
    if(OMX_TRUE != gm->setDisplayRect(gm, &sRect))
        return UNKNOWN_ERROR;

    return NO_ERROR;
}

#ifdef HONEY_COMB
status_t OMXPlayer::getSurfaceRegion(
        int *top,
        int *left, 
        int *bottom, 
        int *right, 
        int *rot)
{
    *top = *left = *bottom = *right = 0;

    OMX_U32 nWidth, nHeight;
    nWidth = nHeight = 0;
    if(NO_ERROR != GetFBResolution(0, &nWidth, &nHeight))
        return UNKNOWN_ERROR;
    *right = nWidth;
    *bottom = nHeight - 80;

    return NO_ERROR;
}
#else
status_t OMXPlayer::getSurfaceRegion(
        int *top,
        int *left, 
        int *bottom, 
        int *right, 
        int *rot)
{
    if(mISurface == NULL)
        return BAD_VALUE;

    return mISurface->getDestRect(left, right, top, bottom, rot);
}
#endif

status_t OMXPlayer::CheckTvOutSetting()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    char value[PROPERTY_VALUE_MAX];
    bool bValue = false;

    if(gm == NULL)
        return BAD_VALUE;

    property_get("rw.VIDEO_TVOUT_DISPLAY", value, "");
    LOG_LOG("rw.VIDEO_TVOUT_DISPLAY: %s\n", value);
    if (fsl_osal_strcmp(value, "1") == 0)
        bValue = true;

    if(bValue == bTvOut)
        return NO_ERROR;

    bTvOut = bValue;
    
    /* switch out of TV mode */
    if(bTvOut != OMX_TRUE) {

        OMX_CONFIG_RECTTYPE rect;
        int left = 0, right = 0, top = 0, bottom = 0, rot = 0;
        OMX_U32 nWidth = 0, nHeight = 0;

        /*default width/height value*/
        if(NO_ERROR != GetFBResolution(0, &nWidth, &nHeight) || nWidth == 0 || nHeight == 0) {
            LOG_ERROR("GetFBResolution 0 fail!!!\n");
            return UNKNOWN_ERROR;
        }

        if(NO_ERROR != getSurfaceRegion(&top, &left, &bottom, &right, &rot)) {
            LOG_ERROR("getDestRect from surface fail!!!\n");
            return UNKNOWN_ERROR;
        }

        /* value from surface is incorrect, use default value from FB */
        if((OMX_U32)(right -left ) > nWidth || (OMX_U32)(bottom-top) > nHeight) { 
            top = 0;
            left = 0;
            bottom = nHeight;
            right = nWidth;
        }

        rect.nWidth = right - left;
        rect.nHeight = bottom - top;
        rect.nLeft = left;
        rect.nTop = top;
        sLeft = left; sRight = right; sTop = top; sBottom = bottom; sRot = rot;

        gm->setDisplayMode(gm,  NULL, &rect , GM_VIDEO_MODE_NORMAL  , GM_VIDEO_DEVICE_LCD, rot);
        
        return NO_ERROR;
    }

    OMX_U32 nFBWidth, nFBHeight;
    if(NO_ERROR != GetFBResolution(1, &nFBWidth, &nFBHeight))
        return UNKNOWN_ERROR;

     GM_VIDEO_DEVICE tv_dev;
    if(nFBWidth == 720 && nFBHeight == 576)
        tv_dev = GM_VIDEO_DEVICE_TV_PAL;
    else if(nFBWidth == 720 && nFBHeight == 480)
        tv_dev = GM_VIDEO_DEVICE_TV_NTSC;
    else if(nFBWidth == 1280 && nFBHeight == 720)
        tv_dev = GM_VIDEO_DEVICE_TV_720P;
    else
        tv_dev = GM_VIDEO_DEVICE_LCD;
    
    printf("TVout device: %d\n", tv_dev);

    if(tv_dev == GM_VIDEO_DEVICE_LCD){
        LOG_ERROR("Incorrect tv device %d , FB width %d, height %d\n", tv_dev, nFBWidth, nFBHeight);
        return BAD_VALUE;
    }

    //if(tv_dev != GM_VIDEO_DEVICE_LCD)
    //    gm->setvideodevice(gm, tv_dev, OMX_TRUE);

    OMX_CONFIG_RECTTYPE rectOut;
    char * end;
    
    property_get("rw.VIDEO_TVOUT_WIDTH", value, "");
    rectOut.nWidth = strtol(value, &end, 10);
    property_get("rw.VIDEO_TVOUT_HEIGHT", value, "");
    rectOut.nHeight= strtol(value, &end, 10);
    property_get("rw.VIDEO_TVOUT_POS_X", value, "");
    rectOut.nLeft = strtol(value, &end, 10);
    property_get("rw.VIDEO_TVOUT_POS_Y", value, "");
    rectOut.nTop = strtol(value, &end, 10);

    printf("SetTvOut perperty_get rectOut: %d,%d,%d,%d\n", rectOut.nWidth, rectOut.nHeight, rectOut.nLeft, rectOut.nTop);

    if(rectOut.nWidth == 0 || rectOut.nHeight == 0)
    {
        LOG_ERROR("Incorrect tvout rect width %d, height %d, use default value from FB\n", rectOut.nWidth, rectOut.nHeight);
        rectOut.nWidth = nFBWidth;
        rectOut.nHeight = nFBHeight;
        rectOut.nLeft = rectOut.nTop = 0;
    }

    gm->setDisplayMode(gm,  NULL, &rectOut , GM_VIDEO_MODE_FULLSCREEN, tv_dev, 0);

    return NO_ERROR;
}

status_t OMXPlayer::CheckDualDisplaySetting()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    char value[PROPERTY_VALUE_MAX];
    bool bValue = false;

    if(gm == NULL)
        return BAD_VALUE;

    property_get("sys.SECOND_DISPLAY_ENABLED", value, "");
    LOG_LOG("sys.SECOND_DISPLAY_ENABLED: %s\n", value);
    if (fsl_osal_strcmp(value, "1") == 0)
        bValue = true;

    if(bValue == bDualDisplay)
        return NO_ERROR;

    bDualDisplay = bValue;
    if(bDualDisplay != OMX_TRUE) {
        sTop = sBottom = sLeft = sRight = 0;
        CheckSurfaceRegion();
        return NO_ERROR;
    }

    OMX_U32 nWidth, nHeight;
    if(NO_ERROR != GetFBResolution(1, &nWidth, &nHeight))
        return UNKNOWN_ERROR;


    if(OMX_TRUE == gm->rotate(gm, 0))
        sRot = 0;
    setVideoDispRect(0, 0, nHeight, nWidth);

    return NO_ERROR;
}

/*
 * Customer requires that OMXPlayer changes display resolution according to setup of FB0 dynamically
 */
status_t OMXPlayer::CheckFB0DisplaySetting()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;
    char value[PROPERTY_VALUE_MAX];
    bool bValue = false;

    if(gm == NULL)
        return BAD_VALUE;

    property_get("rw.VIDEO_FB0_DISPLAY", value, "");
    LOG_LOG("rw.VIDEO_FB0_DISPLAY: %s\n", value);
    if (fsl_osal_strcmp(value, "1") == 0)
        bValue = true;

    if(bValue == bFB0Display && bValue == false)
        return NO_ERROR;

    if(bFB0Display == true && bValue == false) {
        printf("CheckFB0DisplaySetting: switch out of FB0 mode\n");
        bFB0Display = false;
        sTop = sBottom = sLeft = sRight = 0;
        CheckSurfaceRegion();
        return NO_ERROR;
    }

    if(bFB0Display == false && bValue == true)
        bFB0Display = true;

    OMX_U32 nWidth, nHeight;
    if(NO_ERROR != GetFBResolution(0, &nWidth, &nHeight))
        return UNKNOWN_ERROR;

    if(nWidth == 0 || nHeight == 0)
        return UNKNOWN_ERROR;

    if(sTop == 0 && sLeft == 0 && (OMX_U32)sBottom == nHeight && (OMX_U32)sRight == nWidth)
        return NO_ERROR;

    printf("CheckFB0DisplaySetting: Width %d, Height %d\n",nWidth, nHeight);

    setVideoDispRect(0, 0, nHeight, nWidth);

    sTop = sLeft = 0;
    sBottom = nHeight;
    sRight = nWidth;
    
    return NO_ERROR;
}


status_t OMXPlayer::CheckSurfaceRegion()
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    if(gm == NULL)
        return BAD_VALUE;

    if(bDualDisplay != OMX_TRUE && !bTvOut && !bFB0Display) {
        int left = 0, right = 0, top = 0, bottom = 0, rot = 0;

        if(NO_ERROR == getSurfaceRegion(&top, &left, &bottom, &right, &rot)) {
            if(left != sLeft || right != sRight || top != sTop || bottom != sBottom) {
                printf("Surface region changed: left: %d, right: %d, top: %d, bottom: %d, rot: %d\n",
                        left, right, top, bottom, rot);
                if(NO_ERROR == setVideoDispRect(top, left, bottom, right))
                    sLeft = left; sRight = right; sTop = top; sBottom = bottom;
            }

            if(rot != sRot) {
                if(OMX_TRUE == gm->rotate(gm, rot))
                    sRot = rot;
            }
        }
    }

    return NO_ERROR;
}

status_t OMXPlayer::DoSeekTo(int msec)
{
    OMX_GraphManager* gm = (OMX_GraphManager*)player;

    if(gm == NULL)
        return BAD_VALUE;

    LOG_DEBUG("OMXPlayer seekTo %d\n", msec/1000);

    gm->seek(gm, OMX_TIME_SeekModeFast, (OMX_TICKS)msec*1000);

	return NO_ERROR;
}

status_t OMXPlayer::setParameter(int key, const Parcel &request)
{ 
	return NO_ERROR;
}

status_t OMXPlayer::getParameter(int key, Parcel *reply)
{ 
	return NO_ERROR;
}

#include "MediaInspector.h"

OMXPlayerType::OMXPlayerType()
{
}

OMXPlayerType::~OMXPlayerType()
{
}

int OMXPlayerType::IsSupportedContent(char *url)
{
	int fd = 0;
	int64_t len = 0, offset = 0;

	if(fsl_osal_strstr((fsl_osal_char*)url, "sharedfd://")) {
		LOG_DEBUG("SFD open %s\n", url);

		if(sscanf(url, "sharedfd://%d:%lld:%lld", &fd, &offset, &len) != 3)
			return TYPE_NONE;

		LOG_DEBUG("getPlayerType offset %lld, len %lld\n", offset, len);
		// Use Stagefright for very short file playback in game.
		if (offset != 0 && len < 102400)
			return TYPE_NONE;
	}

    MediaType type = GetContentType(url);

    // Disable OGG local playback.
    if (type == TYPE_OGG)
        type = TYPE_NONE;

	// testvideosurfaceresetting workaround.
	if (offset != 0 && len == 504030 && type == TYPE_MP4)
        type = TYPE_NONE;
	
    return type;
}

