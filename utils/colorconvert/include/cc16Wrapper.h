/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file cc16Wrapper.h
 *  @brief 
 *  @ingroup 
 */

#ifndef CC16WRAPPER_h
#define CC16WRAPPER_h

#include "OMX_Types.h"
#include "OMX_IVCommon.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void * cc16WrapperCreate(void);
OMX_S32 cc16WrapperInit(void * pWrapper, OMX_S32 Src_width, OMX_S32 Src_height, OMX_S32 Src_pitch, OMX_CONFIG_RECTTYPE *Src_crop, OMX_COLOR_FORMATTYPE Src_colorFormat, OMX_S32 Dst_width, OMX_S32 Dst_height, OMX_S32 Dst_pitch, OMX_S32 nRotation);
OMX_S32 cc16WrapperSetMode(void *pWrapper, OMX_S32 nMode); //nMode : 0 Off, 1 On
OMX_S32 cc16WrapperConvert(void *pWrapper, OMX_U8*srcBuf, OMX_U8 *destBuf);
void cc16WrapperDelete(void **pWrapper);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
