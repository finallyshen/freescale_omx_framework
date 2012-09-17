/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <OMXMediaScanner.h>
#include <media/mediametadataretriever.h>
#include <private/media/VideoFrame.h>
#undef OMX_MEM_CHECK
#include "Mem.h"
#include "Log.h"
// Sonivox includes
#include <libsonivox/eas.h>

namespace android {

OMXMediaScanner::OMXMediaScanner() {}

OMXMediaScanner::~OMXMediaScanner() {}

static bool FileHasAcceptableExtension(const char *extension) {
    static const char *kValidExtensions[] = {
		".avi", ".divx", ".mp3", ".aac", ".bsac", ".ac3", ".wmv",
		".wma", ".asf", ".rm", ".rmvb", ".ra", ".3gp", ".3gpp", ".3g2",
		".mp4", ".mov", ".m4v", ".m4a", ".flac", ".wav", ".mpg", ".ts",
		".vob", ".ogg", ".mkv", ".f4v", ".flv", ".webm", ".out",
        ".mid", ".smf", ".imy", ".midi", ".xmf", ".rtttl", ".rtx", ".ota", ".mxmf"
    };
    static const size_t kNumValidExtensions =
        sizeof(kValidExtensions) / sizeof(kValidExtensions[0]);

    for (size_t i = 0; i < kNumValidExtensions; ++i) {
        if (!strcasecmp(extension, kValidExtensions[i])) {
            return true;
        }
    }
	return false;
}
static MediaScanResult HandleMIDI(
        const char *filename, MediaScannerClient *client) {
    // get the library configuration and do sanity check
    const S_EAS_LIB_CONFIG* pLibConfig = EAS_Config();
    if ((pLibConfig == NULL) || (LIB_VERSION != pLibConfig->libVersion)) {
        LOG_ERROR("EAS library/header mismatch\n");
        return MEDIA_SCAN_RESULT_ERROR;
    }
    EAS_I32 temp;

    // spin up a new EAS engine
    EAS_DATA_HANDLE easData = NULL;
    EAS_HANDLE easHandle = NULL;
    EAS_RESULT result = EAS_Init(&easData);
    if (result == EAS_SUCCESS) {
        EAS_FILE file;
        file.path = filename;
        file.fd = 0;
        file.offset = 0;
        file.length = 0;
        result = EAS_OpenFile(easData, &file, &easHandle);
    }
    if (result == EAS_SUCCESS) {
        result = EAS_Prepare(easData, easHandle);
    }
    if (result == EAS_SUCCESS) {
        result = EAS_ParseMetaData(easData, easHandle, &temp);
    }
    if (easHandle) {
        EAS_CloseFile(easData, easHandle);
    }
    if (easData) {
        EAS_Shutdown(easData);
    }

    if (result != EAS_SUCCESS) {
        return MEDIA_SCAN_RESULT_SKIPPED;
    }

    char buffer[20];
    sprintf(buffer, "%ld", temp);
    status_t status = client->addStringTag("duration", buffer);
    if (status != OK) {
        return MEDIA_SCAN_RESULT_ERROR;
    }
    return MEDIA_SCAN_RESULT_OK;
}

#ifdef ICS
MediaScanResult OMXMediaScanner::processFile(
#else
status_t OMXMediaScanner::processFile(
#endif
		const char *path, const char *mimeType,
		MediaScannerClient &client) {
	LOG_DEBUG("processFile '%s'.", path);

	client.setLocale(locale());
    client.beginFile();

    const char *extension = strrchr(path, '.');

    if (!extension) {
#ifdef ICS
        return MEDIA_SCAN_RESULT_SKIPPED;
#else
		return UNKNOWN_ERROR;
#endif
    }

    if (!FileHasAcceptableExtension(extension)) {
        client.endFile();
#ifdef ICS
        return MEDIA_SCAN_RESULT_SKIPPED;
#else
		return UNKNOWN_ERROR;
#endif
    }

    if (!strcasecmp(extension, ".mid")
            || !strcasecmp(extension, ".smf")
            || !strcasecmp(extension, ".imy")
            || !strcasecmp(extension, ".midi")
            || !strcasecmp(extension, ".xmf")
            || !strcasecmp(extension, ".rtttl")
            || !strcasecmp(extension, ".rtx")
            || !strcasecmp(extension, ".ota")
            || !strcasecmp(extension, ".mxmf")) {
        LOG_DEBUG("OMXMediaScanner Handle MIDI file");
        return HandleMIDI(path, &client);
    }

	MediaMetadataRetriever *mRetriever = NULL; 
	mRetriever = FSL_NEW(MediaMetadataRetriever, ()); 
	if(mRetriever == NULL) {
        client.endFile();
#ifdef ICS
        return MEDIA_SCAN_RESULT_ERROR;
#else
		return NO_MEMORY;
#endif
	}

#ifdef MEDIA_SCAN_2_3_3_API
	if (mRetriever->setDataSource(path) == OK) {
#else
	if (mRetriever->setDataSource(path) == OK
            && mRetriever->setMode(
                METADATA_MODE_METADATA_RETRIEVAL_ONLY) == OK) {
#endif

		const char *value;
		if ((value = mRetriever->extractMetadata(
						METADATA_KEY_MIMETYPE)) != NULL) {
			printf("Key: mime\t Value: %s\n",  value);
			client.setMimeType(value);
		}

        struct KeyMap {
            const char *tag;
            int key;
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
            { "writer", METADATA_KEY_WRITER },
			{ "width", METADATA_KEY_VIDEO_WIDTH },
			{ "height", METADATA_KEY_VIDEO_HEIGHT },
			{ "frame_rate", METADATA_KEY_FRAME_RATE },
			{ "video_format", METADATA_KEY_VIDEO_FORMAT },
#ifdef ICS
			{ "location", METADATA_KEY_LOCATION},
#endif
        };
        static const size_t kNumEntries = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

        for (size_t i = 0; i < kNumEntries; ++i) {
            const char *value;
			if ((value = mRetriever->extractMetadata(kKeyMap[i].key)) != NULL) {
				LOG_DEBUG("Key: %s\t Value: %s\n", kKeyMap[i].tag, value);
				client.addStringTag(kKeyMap[i].tag, value);
			}
        }
	} else {
		printf("Key: mime\t Value: %s\n",  "FORMATUNKNOWN");
		//client.setMimeType("FORMATUNKNOWN");
	}

	FSL_DELETE(mRetriever);

    client.endFile();

#ifdef ICS
    return MEDIA_SCAN_RESULT_OK;
#else
	return OK;
#endif

}

char *OMXMediaScanner::extractAlbumArt(int fd) {
    LOG_DEBUG("extractAlbumArt %d", fd);

    off_t size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        return NULL;
    }
    lseek(fd, 0, SEEK_SET);

	MediaMetadataRetriever *mRetriever = NULL; 
	mRetriever = FSL_NEW(MediaMetadataRetriever, ()); 
	if(mRetriever == NULL)
        return NULL;

#ifdef MEDIA_SCAN_2_3_3_API
	if (mRetriever->setDataSource(fd, 0, size) == OK) {
#else
	if (mRetriever->setDataSource(fd, 0, size) == OK
            && mRetriever->setMode(
                METADATA_MODE_FRAME_CAPTURE_ONLY) == OK) {
#endif
        sp<IMemory> mem = mRetriever->extractAlbumArt();

        if (mem != NULL) {
            MediaAlbumArt *art = static_cast<MediaAlbumArt *>(mem->pointer());

            char *data = (char *)malloc(art->mSize + 4);
            *(int32_t *)data = art->mSize;
            memcpy(&data[4], &art[1], art->mSize);
			LOG_DEBUG("Key: AlbumArt\n");

			FSL_DELETE(mRetriever);

			return data;
		}
	}

	FSL_DELETE(mRetriever);

	return NULL;
}

}  // namespace android
