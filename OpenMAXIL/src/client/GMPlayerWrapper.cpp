/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "OMX_GraphManager.h"
#include "GMPlayer.h"

static OMX_BOOL gm_registerEventHandler(
        OMX_GraphManager* h, 
        void* context, 
        GM_EVENTHANDLER handler)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->RegistAppCallback((OMX_PTR)context, handler);

    return OMX_TRUE;
}

static OMX_BOOL gm_setdivxdrmcallback(
        OMX_GraphManager* h, 
        void* context, 
        divxdrmcallback handler,
        void *pfunc)
{
    return OMX_TRUE;
}

static OMX_BOOL gm_load(
        OMX_GraphManager *h, 
        const char *filename, 
        int length)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if(OMX_ErrorNone != Player->Load((OMX_STRING)filename))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL gm_unLoad(
        OMX_GraphManager *h)
{
    return OMX_TRUE;
}

static OMX_BOOL gm_start(
        OMX_GraphManager *h)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->Start();

    return OMX_TRUE;
}

static OMX_BOOL gm_stop(
        OMX_GraphManager* h)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->Stop();

    return OMX_TRUE;
}

static OMX_BOOL gm_pause(
        OMX_GraphManager* h)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->Pause();

    return OMX_TRUE;
}

static OMX_BOOL gm_resume(
        OMX_GraphManager* h)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->Resume();

    return OMX_TRUE;
}

static OMX_BOOL gm_seek(
        OMX_GraphManager* h, 
        OMX_TIME_SEEKMODETYPE mode, 
        OMX_TICKS position)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->Seek(mode, position);

    return OMX_TRUE;
}

static OMX_BOOL gm_setPlaySpeed(
        OMX_GraphManager* h, 
        int Scale)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->SetPlaySpeed(Scale);

    return OMX_TRUE;
}

static OMX_BOOL gm_setVolume(
        OMX_GraphManager* h, 
        OMX_BOOL up)
{
    return OMX_TRUE;
}

static OMX_BOOL gm_disableStream(
        OMX_GraphManager* h, 
        GM_STREAM_INDEX stream_index)
{
    GMPlayer *Player = NULL;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    Player = (GMPlayer*)h->pData;
    Player->DisableStream(stream_index);

    return OMX_TRUE;
}

static OMX_BOOL gm_getStreamInfo(
        OMX_GraphManager* h, 
        int streamIndex, 
        GM_STREAMINFO* streamInfo)
{
    GMPlayer *Player = NULL;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    Player = (GMPlayer*)h->pData;
    ret = Player->GetStreamInfo(streamIndex, streamInfo);
    if(ret != OMX_ErrorNone)
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL gm_getDuration(
        OMX_GraphManager* h, 
        OMX_TICKS* pDuration)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->GetMediaDuration(pDuration);

    return OMX_TRUE;
}

static OMX_BOOL gm_getState(
        OMX_GraphManager* h, 
        GM_STATE *state)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->GetState(state);

    return OMX_TRUE;
}

static OMX_BOOL gm_getPosition(
        OMX_GraphManager* h, 
        OMX_TICKS* pCur)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->GetMediaPosition(pCur);

    return OMX_TRUE;
}



static OMX_BOOL gm_deleteIt(
        OMX_GraphManager* h)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->DeInit();
    FSL_DELETE(Player);
    FSL_FREE(h);
    OMX_Deinit();

    return OMX_TRUE;
}
static OMX_BOOL gm_rotate(OMX_GraphManager* h, OMX_S32 nRotate)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->Rotate(nRotate);
    return ret;
}

static OMX_BOOL gm_fullscreen(OMX_GraphManager* h, OMX_BOOL bzoomin, OMX_BOOL bkeepratio)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->FullScreen(bzoomin,bkeepratio);	
	return ret;
}

static OMX_BOOL gm_AdjustAudioVolume(OMX_GraphManager* h, OMX_BOOL bVolumeUp)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->AdjustAudioVolume(bVolumeUp);	
	return ret;
}

static OMX_U32 gm_GetCurAudioTrack(OMX_GraphManager* h)
{
    OMX_U32 nCurAudioTrack;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	nCurAudioTrack = Player->GetCurAudioTrack();	
	return nCurAudioTrack;
}


static OMX_U32 gm_GetAudioTrackNum(OMX_GraphManager* h)
{
    OMX_U32 nAudioTrackNum;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	nAudioTrackNum = Player->GetAudioTrackNum();	
	return nAudioTrackNum;
}

static OMX_BOOL gm_SelectAudioTrack(OMX_GraphManager* h, OMX_U32 nSelectedTrack)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->SelectAudioTrack(nSelectedTrack);	
	return ret;
}

static OMX_U32 gm_GetBandNumPEQ(OMX_GraphManager* h)
{
    OMX_U32 nPEQBandNum;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	nPEQBandNum = Player->GetBandNumPEQ();	
	return nPEQBandNum;
}

static OMX_BOOL gm_SetAudioEffectPEQ(OMX_GraphManager* h, OMX_U32 nBindIndex, OMX_U32 nFC, OMX_S32 nGain)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->SetAudioEffectPEQ(nBindIndex, nFC, nGain);	
	return ret;
}

static OMX_BOOL gm_EnableDisablePEQ(OMX_GraphManager* h, OMX_BOOL bAudioEffect)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->EnableDisablePEQ(bAudioEffect);	
	return ret;
}

static OMX_BOOL gm_AddRemoveAudioPP(OMX_GraphManager* h, OMX_BOOL bAddComponent)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->AddRemoveAudioPP(bAddComponent);	
	return ret;
}

static OMX_BOOL gm_set_video_mode(OMX_GraphManager* h, GM_VIDEO_MODE vmode)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->SetVideoMode(vmode);	
    return ret;
}
static OMX_BOOL gm_set_video_device(OMX_GraphManager* h, GM_VIDEO_DEVICE vdevice, OMX_BOOL bBlockMode)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->SetVideoDevice(vdevice,bBlockMode);	
    return ret;
}

static OMX_BOOL
gm_getScreenShotInfo(OMX_GraphManager* h, OMX_IMAGE_PORTDEFINITIONTYPE *pfmt, OMX_CONFIG_RECTTYPE *pCropRect)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->GetScreenShotInfo(pfmt,pCropRect);	
    return ret;
}

static OMX_BOOL
gm_getThumbnail(OMX_GraphManager* h, OMX_TICKS pos, OMX_U8 *pBuf)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->GetThumbNail(pos,pBuf);	
    return ret;
}

static OMX_BOOL
gm_getSnapshot(OMX_GraphManager* h, OMX_U8 *buf)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
	ret = Player->GetSnapShot(buf);	
    return ret;
}
static OMX_BOOL gm_syssleep(OMX_GraphManager* h, OMX_BOOL bsleep)
{
    OMX_BOOL ret = OMX_TRUE;
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    ret = Player->SysSleep(bsleep);	
    return ret;
}

static OMX_BOOL gm_setAudioSink(OMX_GraphManager *h, OMX_PTR sink)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->SetAudioSink(sink);	

    return OMX_TRUE;
}

static OMX_BOOL gm_setDisplayRect(OMX_GraphManager *h, OMX_CONFIG_RECTTYPE *pRect)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if(OMX_ErrorNone != Player->SetDisplayRect(pRect))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL gm_setVideoRect(OMX_GraphManager *h, OMX_CONFIG_RECTTYPE *pRect)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if(OMX_ErrorNone != Player->SetVideoRect(pRect))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL gm_setDisplayMode(OMX_GraphManager *h, OMX_CONFIG_RECTTYPE *pInRect, OMX_CONFIG_RECTTYPE *pOutRect, GM_VIDEO_MODE eVideoMode,  GM_VIDEO_DEVICE tv_dev, OMX_U32 nRotate)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if(OMX_ErrorNone != Player->SetDisplayMode(pInRect, pOutRect, eVideoMode, tv_dev, nRotate))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL gm_setVideoSurface(OMX_GraphManager *h,  OMX_PTR surface, OMX_VIDEO_SURFACE_TYPE type)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if(OMX_ErrorNone != Player->SetVideoSurface(surface, type))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL gm_setVideoRenderType(OMX_GraphManager *h, OMX_VIDEO_RENDER_TYPE type)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if(OMX_ErrorNone != Player->SetVideoRenderType(type))
        return OMX_FALSE;

    return OMX_TRUE;
}

OMX_GraphManager* OMX_GraphManagerCreate()
{
    OMX_GraphManager *Gm = NULL;
    GMPlayer *Player = NULL;

    if (OMX_Init()!=OMX_ErrorNone) {
        LOG_ERROR("can not init OMX core!!!\n");
        return NULL;
    }

    Gm = (OMX_GraphManager*)FSL_MALLOC(sizeof(OMX_GraphManager));
    if(Gm == NULL)
        return NULL;

    Player = FSL_NEW(GMPlayer, ());
    if(Player == NULL) {
        FSL_FREE(Gm);
        LOG_ERROR("Can't create player.\n");
        return NULL;
    }

    Player->Init();
    Gm->pData = (OMX_PTR) Player;

    Gm->deleteIt = gm_deleteIt;
    Gm->registerEventHandler = gm_registerEventHandler;
    Gm->setdivxdrmcallback = gm_setdivxdrmcallback;
    Gm->load = gm_load;
    Gm->start = gm_start;
    Gm->stop = gm_stop;
    Gm->getDuration = gm_getDuration;
    Gm->getState = gm_getState;
    Gm->getPosition = gm_getPosition;
    Gm->getStreamInfo = gm_getStreamInfo;
    Gm->pause = gm_pause;
    Gm->resume = gm_resume;
    Gm->seek = gm_seek;
    Gm->setPlaySpeed = gm_setPlaySpeed;
    Gm->disableStream = gm_disableStream;
	
	Gm->rotate = gm_rotate;
	Gm->fullscreen = gm_fullscreen;
	Gm->AdjustAudioVolume = gm_AdjustAudioVolume;
	Gm->AddRemoveAudioPP = gm_AddRemoveAudioPP;
	Gm->GetAudioTrackNum = gm_GetAudioTrackNum;
	Gm->GetCurAudioTrack = gm_GetCurAudioTrack;
	Gm->SelectAudioTrack = gm_SelectAudioTrack;
	Gm->GetBandNumPEQ = gm_GetBandNumPEQ;
	Gm->SetAudioEffectPEQ = gm_SetAudioEffectPEQ;
	Gm->EnableDisablePEQ = gm_EnableDisablePEQ;

    Gm->setvideodevice=gm_set_video_device;
    Gm->setvideomode=gm_set_video_mode;
	
    Gm->getScreenShotInfo = gm_getScreenShotInfo;
    Gm->getThumbnail = gm_getThumbnail;
    Gm->getSnapshot = gm_getSnapshot;	
    Gm->syssleep = gm_syssleep;
    Gm->setAudioSink = gm_setAudioSink;
    Gm->setDisplayRect = gm_setDisplayRect;
    Gm->setVideoRect = gm_setVideoRect;
    Gm->setDisplayMode = gm_setDisplayMode;
    Gm->setVideoSurface = gm_setVideoSurface;
    Gm->setVideoRenderType = gm_setVideoRenderType;

    return Gm;
}


