/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <OMXMetadataRetriever.h>
#include <ui/DisplayInfo.h>
/*
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurfaceComposer.h>
#include <surfaceflinger/SurfaceComposerClient.h>
*/
#include "OMX_MetadataExtractor.h"
#include "OMX_Implement.h"
#include "OMXAndroidUtils.h"
#undef OMX_MEM_CHECK
#include "Mem.h"
#include "Log.h"
#include <cutils/properties.h>
#include "OMX_Common.h"

using namespace android;

#define GET_METADATA (0x01 << 0)
#define GET_FRAME (0x01 << 1)

#define THUMB_W 96
#define THUMB_H 96

extern const VIDEO_RENDER_MAP_ENTRY VideoRenderNameTable[OMX_VIDEO_RENDER_NUM]; 

OMXMetadataRetriever::OMXMetadataRetriever(int nMediaType)
{
    OMX_MetadataExtractor* mExtractor = NULL;

    LOG_DEBUG("OMXMetadataRetriever constructor.\n");

    MetadataExtractor = NULL;
	mAlbumArt = NULL;
	mSharedFd = -1;
	mParsedMetaData = false;
	mMediaType = nMediaType;
    mExtractor = OMX_MetadataExtractorCreate();
    if(mExtractor == NULL)
        LOG_ERROR("Failed to create GMPlayer.\n");

    MetadataExtractor = (void*)mExtractor;

    sessionCreated = false;

    LOG_DEBUG("OMXMetadataRetriever Construct mExtractor MetadataExtractor: %p\n", mExtractor);
}

status_t ExtractorUnload(OMX_MetadataExtractor* mExtractor)
{
    if(OMX_TRUE != mExtractor->unLoad(mExtractor)) {
        //LOG_ERROR("unload failed.\n");
        return BAD_VALUE;
    }

    return NO_ERROR;
}

OMXMetadataRetriever::~OMXMetadataRetriever()
{
    OMX_MetadataExtractor* mExtractor = (OMX_MetadataExtractor*)MetadataExtractor;

    LOG_DEBUG("OMXMetadataRetriever de-constructor %p.\n", mExtractor);

	if(mExtractor != NULL) {
		ExtractorUnload(mExtractor);
		mExtractor->deleteIt(mExtractor);
        mExtractor = NULL;
    }

    if (mSharedFd >= 0) {
        close(mSharedFd);
    }

    if(sessionCreated)
        session->dispose();

}

status_t ExtractorLoad(OMX_MetadataExtractor* mExtractor, OMX_STRING contentURI)
{
    if(OMX_TRUE != mExtractor->load(mExtractor, contentURI, fsl_osal_strlen(contentURI))) {
        LOG_ERROR("load contentURI %s failed.\n", contentURI);
        return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t OMXMetadataRetriever::setDataSource(const char *url)
{
	OMX_MetadataExtractor* mExtractor = (OMX_MetadataExtractor*)MetadataExtractor;

	LOG_DEBUG("OMXMetadataRetriever set data source %s\n", url);

	mParsedMetaData = false;

    if (mSharedFd >= 0) {
        close(mSharedFd);
        mSharedFd = -1;
    }

	ExtractorUnload(mExtractor);

    return ExtractorLoad(mExtractor, (OMX_STRING)url);
}

status_t OMXMetadataRetriever::setDataSource(const char *url,
		const KeyedVector<String8, String8> *headers)
{
	OMX_MetadataExtractor* mExtractor = (OMX_MetadataExtractor*)MetadataExtractor;

	LOG_DEBUG("OMXMetadataRetriever set data source %s\n", url);

	mParsedMetaData = false;

    if (mSharedFd >= 0) {
        close(mSharedFd);
        mSharedFd = -1;
    }

	ExtractorUnload(mExtractor);

    return ExtractorLoad(mExtractor, (OMX_STRING)url);
}

status_t OMXMetadataRetriever::setDataSource(
        int fd, 
        int64_t offset, 
        int64_t length)
{
    LOG_DEBUG("OMXMetadataRetriever set data source, fd:%d, offset: %lld, length: %lld.\n",
            fd, offset, length);

	mParsedMetaData = false;

    if (mSharedFd >= 0) {
        close(mSharedFd);
        mSharedFd = -1;
    }

    mSharedFd = dup(fd);
    sprintf(contentURI, "sharedfd://%d:%lld:%lld:%d",  mSharedFd, offset, length, mMediaType);
    LOG_DEBUG("OMXMetadataRetriever contentURI: %s\n", contentURI);
    OMX_MetadataExtractor* mExtractor = (OMX_MetadataExtractor*)MetadataExtractor;

	ExtractorUnload(mExtractor);

    return ExtractorLoad(mExtractor, contentURI);
}

status_t OMXMetadataRetriever::setMode(int mode)
{
    LOG_DEBUG("OMXMetadataRetriever setMode: %d\n", mode);

#ifndef MEDIA_SCAN_2_3_3_API
    if (mode < METADATA_MODE_NOOP ||
            mode > METADATA_MODE_FRAME_CAPTURE_AND_METADATA_RETRIEVAL) {
        return BAD_VALUE;
    }

    mMode = mode;
#endif

    return NO_ERROR;
}

status_t OMXMetadataRetriever::getMode(int *mode) const
{

#ifndef MEDIA_SCAN_2_3_3_API
    *mode = mMode;
#endif

    return NO_ERROR;
}

VideoFrame *OMXMetadataRetriever::getFrameAtTime(int64_t timeUs, int option)
{
	LOG_DEBUG("getFrameAtTime: %lld option: %d\n", timeUs, option);
    return captureFrame();
}

VideoFrame *OMXMetadataRetriever::captureFrame()
{
    OMX_MetadataExtractor* mExtractor = (OMX_MetadataExtractor*)MetadataExtractor;
    char value[PROPERTY_VALUE_MAX];
    int i;
    OMX_VIDEO_RENDER_TYPE videoRender = OMX_VIDEO_RENDER_IPULIB;
                        
#ifndef MEDIA_SCAN_2_3_3_API
    if(!(mMode & GET_FRAME))
        return NULL;
#endif

    if(mExtractor == NULL)
        return NULL;

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

    if(videoRender == OMX_VIDEO_RENDER_SURFACE){

        LOG_DEBUG("retriever: video render type is %d\n", videoRender);
        if(OMX_TRUE != mExtractor->setVideoRenderType(mExtractor, videoRender)) {
            LOG_ERROR("setVideoRenderType fail %d... \n", videoRender);
            return NULL;
        }

        DisplayInfo dinfo;
        unsigned int panel_x,panel_y;

        session = new SurfaceComposerClient();
        sessionCreated = true;
        status_t status = session->getDisplayInfo(0, &dinfo);
        if (status)
            return NULL;

        panel_x=dinfo.w;
        panel_y=dinfo.h;

#ifdef ICS
        surfaceControl = session->createSurface(0, panel_x, panel_y, PIXEL_FORMAT_RGB_565);
#else
		surfaceControl = session->createSurface(getpid(), 0, panel_x, panel_y, PIXEL_FORMAT_RGB_565);
#endif
        surface = surfaceControl->getSurface();

        sp<ANativeWindow> nativeWindow = surface;

        mExtractor->setSurface(mExtractor, &nativeWindow);
    }
    else{
        // default, use ipulib render
        if(OMX_TRUE != mExtractor->setVideoRenderType(mExtractor, OMX_VIDEO_RENDER_IPULIB)) {
            LOG_ERROR("setVideoRenderType fail %d... \n", OMX_VIDEO_RENDER_IPULIB);
            return NULL;
        }
    }

    VideoFrame *mVideoFrame = NULL;
    mVideoFrame = FSL_NEW(VideoFrame, ());
    if(mVideoFrame == NULL)
        return NULL;

	mVideoFrame->mWidth = THUMB_W;
	mVideoFrame->mHeight = THUMB_H;
	mVideoFrame->mDisplayWidth  = THUMB_W;
	mVideoFrame->mDisplayHeight = THUMB_H;
#ifdef GINGER_BREAD
	mVideoFrame->mRotationAngle = 0;
#endif
#ifdef HONEY_COMB
	mVideoFrame->mRotationAngle = 0;
#endif

    OMX_IMAGE_PORTDEFINITIONTYPE out_format;
    fsl_osal_memset(&out_format, 0, sizeof(OMX_IMAGE_PORTDEFINITIONTYPE));
    out_format.eColorFormat = OMX_COLOR_Format16bitRGB565;

	if(OMX_TRUE != mExtractor->getThumbnail(mExtractor, &out_format, \
				(OMX_TICKS)(5*OMX_TICKS_PER_SECOND), &(mVideoFrame->mData))) {
		LOG_ERROR("Failed to get thumnail\n");
		mVideoFrame->mSize = mVideoFrame->mWidth * mVideoFrame->mHeight * 2;
		mVideoFrame->mData = (uint8_t*)FSL_MALLOC(mVideoFrame->mSize*sizeof(uint8_t));
		if(mVideoFrame->mData == NULL) {
			FSL_DELETE(mVideoFrame);
			return NULL;
		}
		fsl_osal_memset(mVideoFrame->mData, 0, mVideoFrame->mSize);

		return mVideoFrame;
	}

    mVideoFrame->mWidth = out_format.nFrameWidth;
    mVideoFrame->mHeight = out_format.nFrameHeight;
    mVideoFrame->mDisplayWidth  = out_format.nFrameWidth;
    mVideoFrame->mDisplayHeight = out_format.nFrameHeight;
    mVideoFrame->mSize = mVideoFrame->mWidth * mVideoFrame->mHeight * 2;

    return mVideoFrame;
}

void OMXMetadataRetriever::ExtractMetadata()
{
    OMX_MetadataExtractor* mExtractor = (OMX_MetadataExtractor*)MetadataExtractor;
    OMX_U32 nMetadataNum = mExtractor->getMetadataNum(mExtractor);
	OMX_METADATA *pMetadata = NULL;

	LOG_DEBUG("Metadata number: %d\n", nMetadataNum);
	for (OMX_U32 i = 0; i < nMetadataNum; i ++) {
		OMX_U32 nMetadataSize = mExtractor->getMetadataSize(mExtractor, i);

		pMetadata = (OMX_METADATA *)FSL_MALLOC(sizeof(OMX_METADATA) + nMetadataSize);;
		if (pMetadata == NULL) {
			LOG_ERROR("Can't get memory.\n");
			return;
		}
		fsl_osal_memset(pMetadata, 0, sizeof(OMX_METADATA) + nMetadataSize);

		mExtractor->getMetadata(mExtractor, i, pMetadata);

		struct KeyMap {
			const char *tag;
			int nKey;
		};
		static const KeyMap kKeyMap[] = {
			{ "tracknumber", METADATA_KEY_CD_TRACK_NUMBER },
			{ "discnumber", METADATA_KEY_DISC_NUMBER },
			{ "album", METADATA_KEY_ALBUM },
			{ "artist", METADATA_KEY_ARTIST },
			{ "albumartist", METADATA_KEY_ALBUMARTIST },
			{ "composer", METADATA_KEY_COMPOSER },
			{ "genre", METADATA_KEY_GENRE },
			{ "title", METADATA_KEY_TITLE },
			{ "year", METADATA_KEY_YEAR },
			{ "duration", METADATA_KEY_DURATION },
			{ "mime", METADATA_KEY_MIMETYPE },
			{ "albumart", 0 },
			{ "writer", METADATA_KEY_AUTHOR },
			{ "width", METADATA_KEY_VIDEO_WIDTH },
			{ "height", METADATA_KEY_VIDEO_HEIGHT },
			{ "frame_rate", METADATA_KEY_FRAME_RATE },
			{ "video_format", METADATA_KEY_VIDEO_FORMAT },
#ifdef ICS
			{ "location", METADATA_KEY_LOCATION},
#endif
		};

		static const size_t kNumMapEntries = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

		for (OMX_U32 j = 0; j < kNumMapEntries; j ++) {
			if (fsl_osal_strcmp((const fsl_osal_char*)pMetadata->nKey, kKeyMap[j].tag) == 0) {
				if (fsl_osal_strcmp((const fsl_osal_char*)pMetadata->nKey, "albumart") == 0) {
					mAlbumArt = FSL_NEW(MediaAlbumArt, ());
					mAlbumArt->mSize = pMetadata->nValueSizeUsed;
					mAlbumArt->mData = (OMX_U8*)FSL_MALLOC(pMetadata->nValueSizeUsed);
					memcpy(mAlbumArt->mData, &pMetadata->nValue, pMetadata->nValueSizeUsed);
				}
				else {
					char *value;
					value = (char *)pMetadata->nValue;
					LOG_DEBUG("Key: %s\t Value: %s\n", kKeyMap[j].tag, value);
					mMetaData.add(kKeyMap[j].nKey, String8(value));
				}
			}
		}

		FSL_FREE(pMetadata);
	}
}

MediaAlbumArt *OMXMetadataRetriever::extractAlbumArt()
{
    OMX_MetadataExtractor* mExtractor = (OMX_MetadataExtractor*)MetadataExtractor;

    LOG_DEBUG("OMXMetadataRetriever extractAlbumArt.\n");

#ifndef MEDIA_SCAN_2_3_3_API
    if (0 == (mMode & METADATA_MODE_METADATA_RETRIEVAL_ONLY)) {
	    LOG_DEBUG("extractAlbumArt/metadata retrieval disabled by mode");

	    return NULL;
	}
#endif

    if(mExtractor == NULL)
        return NULL;

	if (!mParsedMetaData) {
		ExtractMetadata();
		mParsedMetaData = true;
	}

	if (mAlbumArt) {
	    return new MediaAlbumArt(*mAlbumArt);
	}

    return NULL;
}

const char* OMXMetadataRetriever::extractMetadata(int keyCode)
{
    OMX_MetadataExtractor* mExtractor = (OMX_MetadataExtractor*)MetadataExtractor;

    LOG_DEBUG("OMXMetadataRetriever extractMetadata, keyCode: %d\n", keyCode);
    
#ifndef MEDIA_SCAN_2_3_3_API
    if (0 == (mMode & METADATA_MODE_METADATA_RETRIEVAL_ONLY)) {
	    LOG_DEBUG("extractAlbumArt/metadata retrieval disabled by mode");

	    return NULL;
	}
#endif

    if(mExtractor == NULL)
        return NULL;

	if (!mParsedMetaData) {
		ExtractMetadata();
		mParsedMetaData = true;
	}

	ssize_t index = mMetaData.indexOfKey(keyCode);

    if (index < 0) {
	     return NULL;
	}
	
	return strdup(mMetaData.valueAt(index).string());
}


