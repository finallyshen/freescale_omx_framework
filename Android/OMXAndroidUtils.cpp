/**
 *  Copyright (c) 2010,2012 Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "OMXAndroidUtils.h"

namespace android {

status_t ConvertImage(
        OMX_IMAGE_PORTDEFINITIONTYPE *pIn_fmt, 
        OMX_CONFIG_RECTTYPE *pCropRect, 
        OMX_U8 *in, 
        OMX_IMAGE_PORTDEFINITIONTYPE *pOut_fmt, 
        OMX_U8 *out)
{
    OMX_ImageConvert* ic;
    ic = OMX_ImageConvertCreate();
    if(ic == NULL)
        return UNKNOWN_ERROR;

    //resize image
    ic->resize(ic, pIn_fmt, pCropRect, in, pOut_fmt, out);
    ic->delete_it(ic);

    return NO_ERROR;
}


};

