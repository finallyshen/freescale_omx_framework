/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#ifndef FSL_RENDERER_H_

#define FSL_RENDERER_H_

#include <media/stagefright/VideoRenderer.h>
#include <utils/RefBase.h>

namespace android {

class ISurface;

class FSLRenderer : public VideoRenderer {

public:
    FSLRenderer(
            OMX_COLOR_FORMATTYPE colorFormat,
            const sp<ISurface> &surface,
            size_t displayWidth, size_t displayHeight,
            size_t decodedWidth, size_t decodedHeight);

    virtual ~FSLRenderer();

    virtual void render(
            const void *data, size_t size, void *platformPrivate);

private:
    OMX_COLOR_FORMATTYPE mColorFormat; 
	sp<ISurface> mISurface;
    size_t mDisplayWidth, mDisplayHeight;
    size_t mDecodedWidth, mDecodedHeight;
	ipu_lib_handle_t ipu_handle;


    FSLRenderer(const FSLRenderer &);
    FSLRenderer &operator=(const FSLRenderer &);
};

}  // namespace android

#endif  // FSL_RENDERER_H_

