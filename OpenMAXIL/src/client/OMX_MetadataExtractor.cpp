/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "OMX_MetadataExtractor.h"
#include "GMPlayer.h"


static OMX_BOOL extractor_load(
        OMX_MetadataExtractor *h, 
        const char *filename, 
        int length)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if(OMX_ErrorNone != Player->loadMetadataExtractor((char *)filename))
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL extractor_unLoad(
        OMX_MetadataExtractor *h)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if(OMX_ErrorNone != Player->unloadMetadataExtractor())
        return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_U32 extractor_getMetadataNum(
        OMX_MetadataExtractor *h)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    
    return Player->getMetadataNum();
}

static OMX_U32 extractor_getMetadataSize(
        OMX_MetadataExtractor* h,
		OMX_U32 index)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    
    return Player->getMetadataSize(index);
}

static OMX_BOOL extractor_getMetadata(
        OMX_MetadataExtractor* h, 
		OMX_U32 index, 
		OMX_METADATA *pMetadata)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    Player->getMetadata(index, pMetadata);

    return OMX_TRUE;
}

static OMX_BOOL extractor_setVideoRenderType(
        OMX_MetadataExtractor* h, 
        OMX_VIDEO_RENDER_TYPE videoRender)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if (Player->SetVideoRenderType(videoRender) != OMX_ErrorNone) 
		return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL extractor_setSurface(
        OMX_MetadataExtractor* h, 
        OMX_PTR surface)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if (Player->SetVideoSurface(surface, OMX_VIDEO_SURFACE_SURFACE) != OMX_ErrorNone) 
		return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL extractor_getThumbnail(
        OMX_MetadataExtractor* h, 
		OMX_IMAGE_PORTDEFINITIONTYPE *pfmt, 
		OMX_TICKS position, 
		OMX_U8 **ppBuf)
{
    GMPlayer *Player = NULL;

    Player = (GMPlayer*)h->pData;
    if (Player->getThumbnail(pfmt, position, ppBuf) != OMX_ErrorNone) 
		return OMX_FALSE;

    return OMX_TRUE;
}

static OMX_BOOL extractor_deleteIt(
        OMX_MetadataExtractor* h)
{
    GMPlayer *Player = NULL;
    Player = (GMPlayer*)h->pData;
    Player->DeInit();
    FSL_DELETE(Player);
    FSL_FREE(h);
    OMX_Deinit();

    return OMX_TRUE;
}

OMX_MetadataExtractor* OMX_MetadataExtractorCreate()
{
    OMX_MetadataExtractor *mExtractor = NULL;
    GMPlayer *Player = NULL;

    if (OMX_Init()!=OMX_ErrorNone) {
        LOG_ERROR("can not init OMX core!!!\n");
        return NULL;
    }

    mExtractor = (OMX_MetadataExtractor*)FSL_MALLOC(sizeof(OMX_MetadataExtractor));
    if(mExtractor == NULL)
        return NULL;

    Player = FSL_NEW(GMPlayer, ());
    if(Player == NULL) {
        FSL_FREE(mExtractor);
        LOG_ERROR("Can't create player.\n");
        return NULL;
    }

    Player->Init();
    mExtractor->pData = (OMX_PTR) Player;

    mExtractor->load = extractor_load;
    mExtractor->unLoad = extractor_unLoad;
    mExtractor->getMetadataNum = extractor_getMetadataNum;
    mExtractor->getMetadataSize = extractor_getMetadataSize;
    mExtractor->getMetadata = extractor_getMetadata;
    mExtractor->getThumbnail = extractor_getThumbnail;
    mExtractor->deleteIt = extractor_deleteIt;
    mExtractor->setVideoRenderType = extractor_setVideoRenderType;
    mExtractor->setSurface = extractor_setSurface;

    return mExtractor;
}


