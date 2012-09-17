/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */


#ifndef MeidaInspector_h
#define MeidaInspector_h

#include "OMX_Types.h"
#include "OMX_ContentPipe.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    TYPE_NONE = 0,
    TYPE_AVI,
    TYPE_MP4,
    TYPE_RMVB,
    TYPE_MKV,
    TYPE_ASF,
    TYPE_FLV,
    TYPE_MPG2,
    TYPE_MP3,
    TYPE_FLAC,
    TYPE_AAC,
    TYPE_AC3,
    TYPE_WAV,
    TYPE_OGG,
    TYPE_HTTPLIVE
}MediaType;

MediaType GetContentType(OMX_STRING contentURI);
OMX_STRING MediaTypeConformByContent(OMX_STRING contentURI, OMX_STRING role);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
