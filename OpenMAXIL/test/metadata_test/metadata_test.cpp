/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "OMX_MetadataExtractor.h"
#include "OMX_Implement.h"
#include "Mem.h"
#include "Log.h"

#define THUMB_W 200
#define THUMB_H 200

// A simple buffer to hold binary data
class MediaAlbumArt
{
	public:
		MediaAlbumArt(): mSize(0), mData(0) {}

		explicit MediaAlbumArt(const char* url) {
			mSize = 0;
			mData = NULL;
			FILE *in = fopen(url, "r");
			if (!in) {
				return;
			}
			fseek(in, 0, SEEK_END);
			mSize = ftell(in);  // Allocating buffer of size equals to the external file size.
			if (mSize == 0 || (mData = new OMX_U8[mSize]) == NULL) {
				fclose(in);
				if (mSize != 0) {
					mSize = 0;
				}
				return;
			}
			rewind(in);
			if (fread(mData, 1, mSize, in) != mSize) {  // Read failed.
				delete[] mData;
				mData = NULL;
				mSize = 0;
				return;
			}
			fclose(in);
		}

		MediaAlbumArt(const MediaAlbumArt& copy) {
			mSize = copy.mSize;
			mData = NULL;  // initialize it first
			if (mSize > 0 && copy.mData != NULL) {
				mData = new OMX_U8[copy.mSize];
				if (mData != NULL) {
					memcpy(mData, copy.mData, mSize);
				} else {
					mSize = 0;
				}
			}
		}

		~MediaAlbumArt() {
			if (mData != 0) {
				delete[] mData;
			}
		}
		// Intentional public access modifier:
		// We have to know the internal structure in order to share it between
		// processes?
		OMX_U32 mSize;            // Number of bytes in mData
		OMX_U8* mData;            // Actual binary data
};

// Represents a color converted (RGB-based) video frame
// with bitmap pixels stored in FrameBuffer
class VideoFrame
{
	public:
		VideoFrame(): mWidth(0), mHeight(0), mDisplayWidth(0), mDisplayHeight(0), mSize(0), mData(0) {}

		VideoFrame(const VideoFrame& copy) {
			mWidth = copy.mWidth;
			mHeight = copy.mHeight;
			mDisplayWidth = copy.mDisplayWidth;
			mDisplayHeight = copy.mDisplayHeight;
			mSize = copy.mSize;
			mData = NULL;  // initialize it first
			if (mSize > 0 && copy.mData != NULL) {
				mData = new OMX_U8[mSize];
				if (mData != NULL) {
					memcpy(mData, copy.mData, mSize);
				} else {
					mSize = 0;
				}
			}
		}

		~VideoFrame() {
			if (mData != 0) {
				delete[] mData;
			}
		}

		// Intentional public access modifier:
		OMX_U32 mWidth;
		OMX_U32 mHeight;
		OMX_U32 mDisplayWidth;
		OMX_U32 mDisplayHeight;
		OMX_U32 mSize;            // Number of bytes in mData
		OMX_U8* mData;            // Actual binary data
};

MediaAlbumArt *mAlbumArt = NULL;

OMX_ERRORTYPE MakeThumbnailFileNameJPG(char *media_file_name, char *thumbnail_file_name, OMX_S32 index)
{
    OMX_STRING thumb_path = NULL;
    char * file_name = thumbnail_file_name;

    thumb_path = fsl_osal_getenv_new("FSL_OMX_METADATA_PATH");
    if(thumb_path != NULL) {
        OMX_STRING ptr, ptr1;
        ptr = fsl_osal_strrchr(media_file_name, '/') + 1;
        if ((OMX_STRING)0x1 == ptr)
            ptr = media_file_name;
        strcpy(file_name, thumb_path);
        ptr1 = file_name + strlen(file_name);
        ptr1[0] = '/';
        fsl_osal_strcpy(ptr1+1, ptr);
        ptr1 = fsl_osal_strrchr(file_name, '.');
        sprintf(ptr1, "%d", index);
        fsl_osal_strcpy(ptr1+1, ".jpg");
    }
    else {
        /* get idx file from the same folder of media file */
        OMX_STRING ptr, ptr1;
        strcpy(file_name, media_file_name);
        ptr = fsl_osal_strrchr(media_file_name, '/') + 1;
        if ((OMX_STRING)0x1 == ptr)
            ptr = media_file_name;

        ptr1 = fsl_osal_strrchr(file_name, '/') + 1;
        if ((OMX_STRING)0x1 == ptr1)
            ptr1 = file_name;

        fsl_osal_strcpy(ptr1+1, ptr);
        ptr1 = fsl_osal_strrchr(file_name, '.');
        sprintf(ptr1, "%d", index);
        fsl_osal_strcpy(ptr1+1, ".jpg");
    }

    return OMX_ErrorNone;
}
OMX_ERRORTYPE MakeThumbnailFileNameRGBA(char *media_file_name, char *thumbnail_file_name, OMX_S32 index)
{
    OMX_STRING thumb_path = NULL;
    char * file_name = thumbnail_file_name;

    thumb_path = fsl_osal_getenv_new("FSL_OMX_METADATA_PATH");
    if(thumb_path != NULL) {
        OMX_STRING ptr, ptr1;
        ptr = fsl_osal_strrchr(media_file_name, '/') + 1;
        if ((OMX_STRING)0x1 == ptr)
            ptr = media_file_name;
        strcpy(file_name, thumb_path);
        ptr1 = file_name + strlen(file_name);
        ptr1[0] = '/';
        fsl_osal_strcpy(ptr1+1, ptr);
        ptr1 = fsl_osal_strrchr(file_name, '.');
        sprintf(ptr1, "%d", index);
        fsl_osal_strcpy(ptr1+1, ".rgba");
    }
    else {
        /* get idx file from the same folder of media file */
        OMX_STRING ptr, ptr1;
        strcpy(file_name, media_file_name);
        ptr = fsl_osal_strrchr(media_file_name, '/') + 1;
        if ((OMX_STRING)0x1 == ptr)
            ptr = media_file_name;

        ptr1 = fsl_osal_strrchr(file_name, '/') + 1;
        if ((OMX_STRING)0x1 == ptr1)
            ptr1 = file_name;

        fsl_osal_strcpy(ptr1+1, ptr);
        ptr1 = fsl_osal_strrchr(file_name, '.');
        sprintf(ptr1, "%d", index);
        fsl_osal_strcpy(ptr1+1, ".rgba");
    }

    return OMX_ErrorNone;
}

VideoFrame *captureFrame(OMX_MetadataExtractor* mExtractor)
{
    LOG_DEBUG("OMXMetadataRetriever captureFrame.\n");

    if(mExtractor == NULL)
        return NULL;

    VideoFrame *mVideoFrame = NULL;
    mVideoFrame = FSL_NEW(VideoFrame, ());
    if(mVideoFrame == NULL)
        return NULL;

    mVideoFrame->mWidth = THUMB_W;
    mVideoFrame->mHeight = THUMB_H;
    mVideoFrame->mDisplayWidth  = THUMB_W;
    mVideoFrame->mDisplayHeight = THUMB_H;
    mVideoFrame->mSize = mVideoFrame->mWidth * mVideoFrame->mHeight * 2;
    mVideoFrame->mSize = mVideoFrame->mWidth * mVideoFrame->mHeight * 4;
    mVideoFrame->mData = (OMX_U8 *)FSL_MALLOC(mVideoFrame->mSize);
    if(mVideoFrame->mData == NULL) {
        FSL_DELETE(mVideoFrame);
        return NULL;
    }
    fsl_osal_memset(mVideoFrame->mData, 0, mVideoFrame->mSize);

    OMX_IMAGE_PORTDEFINITIONTYPE out_format;
    fsl_osal_memset(&out_format, 0, sizeof(OMX_IMAGE_PORTDEFINITIONTYPE));
    out_format.nFrameWidth = THUMB_W;
    out_format.nFrameHeight = THUMB_H;
    out_format.eColorFormat = OMX_COLOR_Format16bitRGB565;
	out_format.eColorFormat = OMX_COLOR_Format32bitARGB8888;

    if(OMX_TRUE != mExtractor->getThumbnail(mExtractor, &out_format, (OMX_TICKS)(5*OMX_TICKS_PER_SECOND), &(mVideoFrame->mData))) {
        LOG_ERROR("Failed to get thumnail\n");
		FSL_FREE(mVideoFrame->mData);
        FSL_DELETE(mVideoFrame);
        return NULL;
    }

    return mVideoFrame;
}

void ExtractMetadata(OMX_MetadataExtractor* mExtractor)
{
    OMX_U32 nMetadataNum = mExtractor->getMetadataNum(mExtractor);
	OMX_METADATA *pMetadata = NULL;

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
		};
		static const KeyMap kKeyMap[] = {
			{ "tracknumber" },
			{ "discnumber" },
			{ "album" },
			{ "artist" },
			{ "albumartist" },
			{ "composer" },
			{ "genre" },
			{ "title" },
			{ "year" },
			{ "duration" },
			{ "mime" },
			{ "albumart" },
			{ "writer" },
			{ "width" },
			{ "height" },
			{ "frame_rate" },
			{ "video_format" },
		};

		static const size_t kNumMapEntries = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

		for (OMX_U32 j = 0; j < kNumMapEntries; j ++) {
			if (fsl_osal_strcmp((const fsl_osal_char*)pMetadata->nKey, kKeyMap[j].tag) == 0) {
				if (fsl_osal_strcmp((const fsl_osal_char*)pMetadata->nKey, "albumart") == 0) {
					mAlbumArt = FSL_NEW(MediaAlbumArt, ());
					mAlbumArt->mSize = pMetadata->nValueSizeUsed;
					mAlbumArt->mData = (OMX_U8*)FSL_MALLOC(pMetadata->nValueSizeUsed);
					memcpy(mAlbumArt->mData, &pMetadata->nValue, pMetadata->nValueSizeUsed);
					printf("Have Albumart.\n");
				}
				else {
					//mMetaData.add(kKeyMap[j].tag, String8(pMetadata->nValue));
					printf("Key: %s\t Value: %s\n", kKeyMap[j].tag, pMetadata->nValue);
				}
			}
		}

		FSL_FREE(pMetadata);
	}
}

int nThumbCnt = 0;

void SaveVideoFrame(char *file_name, char *buf, int size)
{
    OMX_U8 cFileName[256] = {0};
    FILE *fp_thumb;
    if(OMX_ErrorNone != MakeThumbnailFileNameRGBA(file_name, (char *)(&cFileName), nThumbCnt++)) {
        return;
    }

    fp_thumb = fopen((const char *)cFileName,"wb");
    if(fp_thumb == NULL) {
        printf("Open File Error\n");
        return;
    }

	fwrite(buf, sizeof(char), size, fp_thumb);

    fclose(fp_thumb);

}

void SaveAlbumArt(char *file_name, char *buf, int size)
{
    OMX_U8 cFileName[256] = {0};
    FILE *fp_thumb;
    if(OMX_ErrorNone != MakeThumbnailFileNameJPG(file_name, (char *)(&cFileName), nThumbCnt++)) {
        return;
    }

    fp_thumb = fopen((const char *)cFileName,"wb");
    if(fp_thumb == NULL) {
        printf("Open File Error\n");
        return;
    }

	fwrite(buf, sizeof(char), size, fp_thumb);

    fclose(fp_thumb);
}

int main(int argc, char *argv[])
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(argc < 2)
	{
        printf("Usage: ./bin <in_file>\n");
        return 0;
    }

    OMX_MetadataExtractor* mExtractor = NULL;

    LOG_DEBUG("OMXMetadataRetriever constructor.\n");

	struct timeval tv, tv1;
	gettimeofday (&tv, NULL);

    mExtractor = OMX_MetadataExtractorCreate();
    if(mExtractor == NULL)
        LOG_ERROR("Failed to create GMPlayer.\n");

    LOG_DEBUG("OMXMetadataRetriever Construct mExtractor MetadataExtractor: %p\n", mExtractor);

gettimeofday (&tv1, NULL);
	printf("ExtractMetadata create Time: %d\n", (tv1.tv_sec-tv.tv_sec)*1000+(tv1.tv_usec-tv.tv_usec)/1000);


	gettimeofday (&tv, NULL);
    if(OMX_TRUE != mExtractor->load(mExtractor, argv[1], fsl_osal_strlen(argv[1]))) {
        LOG_ERROR("load argv[1] %s failed.\n", argv[1]);
        return 0;
    }

	gettimeofday (&tv1, NULL);
	printf("ExtractMetadata load Time: %d\n", (tv1.tv_sec-tv.tv_sec)*1000+(tv1.tv_usec-tv.tv_usec)/1000);


	gettimeofday (&tv, NULL);

	ExtractMetadata(mExtractor);
	gettimeofday (&tv1, NULL);
	printf("ExtractMetadata Time: %d\n", (tv1.tv_sec-tv.tv_sec)*1000+(tv1.tv_usec-tv.tv_usec)/1000);


	gettimeofday (&tv, NULL);
	VideoFrame *mVideoFrame = captureFrame(mExtractor);

	if (mAlbumArt && mAlbumArt->mSize != 0) {
		SaveAlbumArt(argv[1], (char *)mAlbumArt->mData, mAlbumArt->mSize);
		FSL_FREE(mAlbumArt->mData);
        FSL_DELETE(mAlbumArt);
	}
	if (mVideoFrame && mVideoFrame->mSize != 0) {
		SaveVideoFrame(argv[1], (char *)mVideoFrame->mData, mVideoFrame->mSize);
		FSL_FREE(mVideoFrame->mData);
        FSL_DELETE(mVideoFrame);
	}
 
    if(OMX_TRUE != mExtractor->unLoad(mExtractor)) {
        LOG_ERROR("unload failed.\n");
        return 0;
    }

	gettimeofday (&tv1, NULL);
	printf("ExtractMetadata write data and free Time: %d\n", (tv1.tv_sec-tv.tv_sec)*1000+(tv1.tv_usec-tv.tv_usec)/1000);


	if(mExtractor != NULL) {
		mExtractor->deleteIt(mExtractor);
        mExtractor = NULL;
    }

    return 1;
}

/* File EOF */
