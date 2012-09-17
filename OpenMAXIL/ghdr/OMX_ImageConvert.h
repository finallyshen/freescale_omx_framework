/*!
 *  Copyright (c) 2009-2010,2012 Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 *  History :
 *  Date             Author       Version    Description
 *
 *  07/01/2009-2010       Li Jian      0.1     Created
 *
 * OMX_ImageConvert.h
 */

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_GraphManager.h"

typedef enum {
    NULL_RESIZE_MODE,
    KEEP_ORG_RESOLUTION,
    KEEP_DEST_RESOLUTION
}Resize_mode;

typedef enum {
    PROPERTY_NONE,
    PROPERTY_RESIZE_MODE,
    PROPERTY_OUT_FILE,
    PROPERTY_OUT_BUFFER
}IC_PROPERTY;

typedef struct _OMX_ImageConvert OMX_ImageConvert;
struct _OMX_ImageConvert
{
	OMX_BOOL (*resize)(OMX_ImageConvert *h, 
                           OMX_IMAGE_PORTDEFINITIONTYPE *in_format, OMX_CONFIG_RECTTYPE *pCropRect, OMX_U8 *in,
                           OMX_IMAGE_PORTDEFINITIONTYPE *out_format, OMX_U8 *out);
	OMX_BOOL (*jpeg_enc)(OMX_ImageConvert *h, OMX_IMAGE_PORTDEFINITIONTYPE *format, OMX_U8 *buf);
	OMX_BOOL (*raw_data)(OMX_ImageConvert *h, OMX_IMAGE_PORTDEFINITIONTYPE *format, OMX_U8 *buf);
        OMX_BOOL (*set_property)(OMX_ImageConvert *h, IC_PROPERTY property, OMX_PTR value);
        OMX_BOOL (*delete_it)(OMX_ImageConvert *h);
	void* pData;
};


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
OMX_ImageConvert* OMX_ImageConvertCreate();
#ifdef __cplusplus
}
#endif /* __cplusplus */

