/**
 *  Copyright (c) 2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

typedef enum {
    FMT_NONE,
    YUV420Planar,
    YUV420PackedPlanar,
    YUV420SemiPlanar,
    YUV422Planar,
    YUV422PackedPlanar,
    YUV422SemiPlanar
} VIDEO_FMT;

typedef struct {
    void * (*init)();
    int (*deinit)(void *handle);
    int (*report)(void *handle, int idx, int width, int height, VIDEO_FMT fmt, char *data);
} VFV_INTERFACE;

extern "C" {
int CreateVFVInterface(VFV_INTERFACE *interface);
}

