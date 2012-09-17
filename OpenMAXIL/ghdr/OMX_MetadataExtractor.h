/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#ifndef _omx_metadataextractor_h_
#define _omx_metadataextractor_h_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "fsl_osal.h"
#include "OMX_Common.h"

/*struct OMX_KeyMap {
	const char *tag;
	int key;
};
const OMX_KeyMap sOMX_KeyMap[] = {
	{ "title", OMX_METADATA_TITLE },
	{ "language", OMX_METADATA_LANGUAGE },
	{ "genre", OMX_METADATA_GENRE },
	{ "artist", OMX_METADATA_ARTIST },
	{ "copyritht", OMX_METADATA_COPYRIGHT },
	{ "comments", OMX_METADATA_COMMENTS },
	{ "year", OMX_METADATA_CREATION_DATE },
	{ "rating", OMX_METADATA_RATING },
	{ "album", OMX_METADATA_ALBUM },
	{ "vcodec", OMX_METADATA_VCODECNAME },
	{ "acodec", OMX_METADATA_ACODECNAME },
	{ "duration", OMX_METADATA_DURATION },
	{ "tracknum", OMX_METADATA_TRACKNUM },
	{ "albumart", OMX_METADATA_ALBUMART },
};*/

typedef struct OMX_METADATA {
	OMX_METADATACHARSETTYPE eKeyCharset;
	OMX_U8 nKeySizeUsed;
	OMX_U8 nKey[128];
	OMX_METADATACHARSETTYPE eValueCharset;
	OMX_U32 nValueMaxSize;
	OMX_U32 nValueSizeUsed;
	OMX_U8 nValue[1];
} OMX_METADATA;

struct OMX_MetadataExtractor
{
	OMX_BOOL (*load)(OMX_MetadataExtractor *h, const char *filename, int length);
	OMX_BOOL (*unLoad)(OMX_MetadataExtractor *h);
	
	OMX_U32 (*getMetadataNum)(OMX_MetadataExtractor* h);
	OMX_U32 (*getMetadataSize)(OMX_MetadataExtractor* h, OMX_U32 index);
	OMX_BOOL (*getMetadata)(OMX_MetadataExtractor* h, OMX_U32 index, OMX_METADATA *pMetadata);
	OMX_BOOL (*getThumbnail)(OMX_MetadataExtractor* h, OMX_IMAGE_PORTDEFINITIONTYPE *pfmt, OMX_TICKS position, OMX_U8 **ppBuf);
	OMX_BOOL (*deleteIt)(OMX_MetadataExtractor* h);
        OMX_BOOL (*setVideoRenderType)(OMX_MetadataExtractor* h, OMX_VIDEO_RENDER_TYPE videoRender);
        OMX_BOOL (*setSurface)(OMX_MetadataExtractor* h, OMX_PTR surface);

	void* pData;
};

OMX_MetadataExtractor* OMX_MetadataExtractorCreate();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
