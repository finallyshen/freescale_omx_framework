/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/videodev.h>
#include <linux/mxc_v4l2.h>
#include "V4lSource.h"

V4lSource::V4lSource()
{ 
	OMX_U32 i; 

	fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_source.v4l.sw-based");
	ComponentVersion.s.nVersionMajor = 0x0;
	ComponentVersion.s.nVersionMinor = 0x1;
	ComponentVersion.s.nRevision = 0x0;
	ComponentVersion.s.nStep = 0x0;
	role_cnt = 1;
	role[0] = "video_source.v4l";

	nFrameBufferMin = 1;
	nFrameBufferActual = 5;
	nFrameBuffer = nFrameBufferActual;
	PreviewPortSupplierType = OMX_BufferSupplyInput;
	CapturePortSupplierType = OMX_BufferSupplyOutput;
	fsl_osal_memset(&sVideoFmt, 0, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
	sVideoFmt.nFrameWidth = 176;
	sVideoFmt.nFrameHeight = 144;
	sVideoFmt.eColorFormat = OMX_COLOR_FormatYUV420Planar;
	sVideoFmt.eCompressionFormat = OMX_VIDEO_CodingUnused;

	OMX_INIT_STRUCT(&sRectIn, OMX_CONFIG_RECTTYPE);
	sRectIn.nWidth = sVideoFmt.nFrameWidth;
	sRectIn.nHeight = sVideoFmt.nFrameHeight;
	sRectIn.nTop = 0;
	sRectIn.nLeft = 0;
	OMX_INIT_STRUCT(&sRectOut, OMX_CONFIG_RECTTYPE);
	sRectOut.nWidth = sVideoFmt.nFrameWidth;
	sRectOut.nHeight = sVideoFmt.nFrameHeight;
	sRectOut.nTop = 0;
	sRectOut.nLeft = 0;

	for(i=0; i<MAX_PORT_BUFFER; i++)
		fsl_osal_memset(&fb_data[i], 0, sizeof(FB_DATA));
	for(i=0; i<MAX_V4L_BUFFER; i++) {
		fsl_osal_memset(&v4lbufs[i], 0, sizeof(struct v4l2_buffer));
		BufferHdrs[i] = NULL;
	}

	fd_v4l = 0;
	g_extra_pixel = 0;
	g_input = 0;
    bStreamOn = OMX_FALSE;
    bFrameBufferInit = OMX_FALSE;
    nAllocating = 0;
    nQueuedBuffer = 0;
    nSourceFrames = 0;
	nCurTS = 0;
    lock = NULL;
}

OMX_ERRORTYPE V4lSource::InitSourceComponent()
{
    if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal)) {
        LOG_ERROR("Create mutex for v4l device failed.\n");
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::DeInitSourceComponent()
{
    if(lock)
        fsl_osal_mutex_destroy(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::PortFormatChanged(OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
	OMX_BOOL bFmtChanged = OMX_FALSE;

	OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	sPortDef.nPortIndex = nPortIndex;
	ports[nPortIndex]->GetPortDefinition(&sPortDef);

	sPortDef.nBufferSize = sPortDef.format.video.nFrameWidth
		* sPortDef.format.video.nFrameHeight
		* pxlfmt2bpp(sVideoFmt.eColorFormat) / 8;
	ret = ports[nPortIndex]->SetPortDefinition(&sPortDef);
	if (ret != OMX_ErrorNone)
		return ret;

	if(nPortIndex == CAPTURED_FRAME_PORT)
	{
		if(sVideoFmt.nFrameWidth != sPortDef.format.video.nFrameWidth) {
			sVideoFmt.nFrameWidth = sPortDef.format.video.nFrameWidth;
			bFmtChanged = OMX_TRUE;
		}
		if(sVideoFmt.nFrameHeight != sPortDef.format.video.nFrameHeight) {
			sVideoFmt.nFrameHeight = sPortDef.format.video.nFrameHeight;
			bFmtChanged = OMX_TRUE;
		}
		if(sVideoFmt.eColorFormat != sPortDef.format.video.eColorFormat) {
			sVideoFmt.eColorFormat = sPortDef.format.video.eColorFormat;
			bFmtChanged = OMX_TRUE;
		}
		if(nFrameBuffer != sPortDef.nBufferCountActual) {
			nFrameBuffer = sPortDef.nBufferCountActual;
		}

		sRectIn.nWidth = sVideoFmt.nFrameWidth;
		sRectIn.nHeight = sVideoFmt.nFrameHeight;
		sRectOut.nWidth = sVideoFmt.nFrameWidth;
		sRectOut.nHeight = sVideoFmt.nFrameHeight;
		sVideoFmt.xFramerate = sPortDef.format.video.xFramerate;

		printf("Set resolution: width: %d height: %d\n", sPortDef.format.video.nFrameWidth, \
				sPortDef.format.video.nFrameHeight);
	
		if(bFmtChanged && fd_v4l > 0) {
			CloseDevice();
			OpenDevice();
		}
	}

    return ret;
}

OMX_ERRORTYPE V4lSource::OpenDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	char v4l_device[100] = "/dev/video0";

	if ((fd_v4l = open(v4l_device, O_RDWR, 0)) < 0)
	{
		LOG_ERROR("Unable to open %s\n", v4l_device);
		return OMX_ErrorHardware;
	}

	ret = SetDevice();
	if (ret != OMX_ErrorNone)
		return ret;

    ret = V4lBufferInit();
    if(ret != OMX_ErrorNone)
        return ret;

	return ret;
}

OMX_ERRORTYPE V4lSource::CloseDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    V4lBufferDeInit();
	if (fd_v4l > 0)
		close(fd_v4l);
	fd_v4l = 0;

	return ret;
}

OMX_ERRORTYPE V4lSource::SetDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	struct v4l2_format fmt;
	struct v4l2_control ctrl;
	struct v4l2_streamparm parm;
	struct v4l2_crop crop;
	struct v4l2_mxc_offset off;

	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	printf("frame rate: %d\n", sVideoFmt.xFramerate / Q16_SHIFT);
	parm.parm.capture.timeperframe.denominator = sVideoFmt.xFramerate / Q16_SHIFT;
	parm.parm.capture.capturemode = 0;

	if (ioctl(fd_v4l, VIDIOC_S_PARM, &parm) < 0)
	{
		LOG_ERROR("VIDIOC_S_PARM failed\n");
		return OMX_ErrorBadParameter;
	}

	if (ioctl(fd_v4l, VIDIOC_S_INPUT, &g_input) < 0)
	{
		LOG_ERROR("VIDIOC_S_INPUT failed\n");
		return OMX_ErrorBadParameter;
	}

	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd_v4l, VIDIOC_G_CROP, &crop) < 0)
	{
		LOG_ERROR("VIDIOC_G_CROP failed\n");
		return OMX_ErrorBadParameter;
	}

	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c.width = sRectIn.nWidth;
	crop.c.height = sRectIn.nHeight;
	crop.c.top = sRectIn.nTop;
	crop.c.left = sRectIn.nLeft;
	if (ioctl(fd_v4l, VIDIOC_S_CROP, &crop) < 0)
	{
		LOG_ERROR("VIDIOC_S_CROP failed\n");
		return OMX_ErrorBadParameter;
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.width = sRectOut.nWidth;
	fmt.fmt.pix.height = sRectOut.nHeight;
	if (g_extra_pixel){
		off.u_offset = (2 * g_extra_pixel + sRectOut.nWidth) * (sRectOut.nHeight + g_extra_pixel)
			- g_extra_pixel + (g_extra_pixel / 2) * ((sRectOut.nWidth / 2)
					+ g_extra_pixel) + g_extra_pixel / 2;
		off.v_offset = off.u_offset + (g_extra_pixel + sRectOut.nWidth / 2) *
			((sRectOut.nHeight / 2) + g_extra_pixel);
		fmt.fmt.pix.bytesperline = sRectOut.nWidth + g_extra_pixel * 2;
		fmt.fmt.pix.priv = (uint32_t) &off;
		fmt.fmt.pix.sizeimage = (sRectOut.nWidth + g_extra_pixel * 2 )
			* (sRectOut.nHeight + g_extra_pixel * 2) * 3 / 2;
	} else {
		fmt.fmt.pix.bytesperline = sRectOut.nWidth;
		fmt.fmt.pix.priv = 0;
		fmt.fmt.pix.sizeimage = 0;
	}

	if (ioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0)
	{
		LOG_ERROR("set format failed\n");
		return OMX_ErrorBadParameter;
	}

	// Set rotation
	ctrl.id = V4L2_CID_PRIVATE_BASE + 0;
	ctrl.value = Rotation.nRotation;
	printf("Rotation: %d\n", Rotation.nRotation);
	if (Rotation.nRotation == 90)
		ctrl.value = 4;
	else if (Rotation.nRotation == 180)
		ctrl.value = 3;
	else if (Rotation.nRotation == 270)
		ctrl.value = 7;
	
	if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0)
	{
		LOG_ERROR("set ctrl failed\n");
		return OMX_ErrorBadParameter;
	}

	return ret;
}

OMX_ERRORTYPE V4lSource::SetSensorMode()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE V4lSource::SetVideoFormat()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE V4lSource::SetWhiteBalance()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE V4lSource::SetDigitalZoom()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE V4lSource::SetExposureValue()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE V4lSource::SetRotation()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	return ret;
}

OMX_ERRORTYPE V4lSource::DoAllocateBuffer(
        OMX_PTR *buffer, 
        OMX_U32 nSize,
        OMX_U32 nPortIndex)
{
	OMX_U32 i = 0;

	if (nPortIndex == CAPTURED_FRAME_PORT)
	{
		/* wait until v4l device is opened */
		while(bFrameBufferInit != OMX_TRUE) {
			usleep(10000);
			i++;
			if(i >= 10)
				return OMX_ErrorHardware;
		}

		if(nAllocating >= nFrameBuffer) {
			LOG_ERROR("Requested buffer number beyond system can provide: [%:%]\n", nAllocating, nFrameBuffer);
			return OMX_ErrorInsufficientResources;
		}

		*buffer = fb_data[nAllocating].pVirtualAddr;
		nAllocating ++;
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::DoFreeBuffer(
        OMX_PTR buffer,
        OMX_U32 nPortIndex)
{
    nAllocating --;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::StartDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	return ret;
}

OMX_ERRORTYPE V4lSource::StopDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    V4lStreamOff(OMX_FALSE);

	return ret;
}

OMX_ERRORTYPE V4lSource::SendBufferToDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nIndex;

    if(nAllocating > 0) {
        V4lSearchBuffer(pOutBufferHdr->pBuffer, &nIndex);
        BufferHdrs[nIndex] = pOutBufferHdr;
    }
    else {
        nIndex = (nSourceFrames % nFrameBuffer);
        BufferHdrs[nIndex] = pOutBufferHdr;
    }

    V4lQueueBuffer(nIndex);

	return ret;
}

OMX_ERRORTYPE V4lSource::GetOneFrameFromDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 nIndex;

    if(fd_v4l <= 0)
        return OMX_ErrorNotReady;

    if(bStreamOn != OMX_TRUE)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);
    nIndex = 0;
    ret = V4lDeQueueBuffer(&nIndex);
    if(ret == OMX_ErrorNone) {
        OMX_BUFFERHEADERTYPE *pHdr = NULL;
        pOutBufferHdr = BufferHdrs[nIndex];
		pOutBufferHdr->nFilledLen = sRectOut.nWidth
        * sRectOut.nHeight
        * pxlfmt2bpp(sVideoFmt.eColorFormat) / 8;
		LOG_DEBUG("Output buffer len: %d\n", pOutBufferHdr->nFilledLen);
        BufferHdrs[nIndex] = NULL;
    }
    fsl_osal_mutex_unlock(lock);

	return ret;
}

OMX_TICKS V4lSource::GetDelayofFrame()
{
	struct timeval now;
	gettimeofday (&now, NULL);

	return (now.tv_sec*1000000+now.tv_usec) - nCurTS;
}

OMX_TICKS V4lSource::GetFrameTimeStamp()
{
    return nCurTS;
}



























OMX_ERRORTYPE V4lSource::SetDeviceRotation()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(fd_v4l <= 0)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);
    V4lStreamOff(OMX_FALSE);
    ret = V4lSetRotation();
    if(ret != OMX_ErrorNone) {
        fsl_osal_mutex_unlock(lock);
        return ret;
    }
    fsl_osal_mutex_unlock(lock);

    return ret;
}

OMX_ERRORTYPE V4lSource::SetDeviceInputCrop()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(fd_v4l <= 0)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);
    V4lStreamOff(OMX_FALSE);
    ret = V4lSetInputCrop();
    if(ret != OMX_ErrorNone) {
        fsl_osal_mutex_unlock(lock);
        return ret;
    }
    fsl_osal_mutex_unlock(lock);

    return ret;
}

OMX_ERRORTYPE V4lSource::SetDeviceDisplayRegion()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(fd_v4l <= 0)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);
    V4lStreamOff(OMX_FALSE);
    ret = V4lSetOutputCrop();
    if(ret != OMX_ErrorNone) {
        fsl_osal_mutex_unlock(lock);
        return ret;
    }
    fsl_osal_mutex_unlock(lock);

    return ret;
}

OMX_ERRORTYPE V4lSource::V4lBufferInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    struct v4l2_requestbuffers buf_req;
    struct v4l2_buffer *pBuffer = NULL;
    OMX_S32 cr_left, cr_top, cr_right, cr_bottom;
    OMX_BOOL bSetCrop = OMX_FALSE;
    OMX_U32 i;

    fsl_osal_memset(&buf_req, 0, sizeof(buf_req));
    buf_req.count = nFrameBuffer;
    buf_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf_req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd_v4l, VIDIOC_REQBUFS, &buf_req) < 0) {
        LOG_ERROR("request buffers #%d failed.\n", nFrameBuffer);
        return OMX_ErrorInsufficientResources;
    }

    cr_left   = sRectIn.nLeft;
    cr_top    = sRectIn.nTop;
    cr_right  = sVideoFmt.nFrameWidth - (sRectIn.nLeft + sRectIn.nWidth);
    cr_bottom = sVideoFmt.nFrameHeight - (sRectIn.nTop + sRectIn.nHeight);
    if(cr_left != 0 || cr_right != 0 || cr_top != 0)
        bSetCrop = OMX_TRUE;

    for(i=0; i<nFrameBuffer; i++)
        fb_data[i].pVirtualAddr = NULL;

    for(i=0; i<nFrameBuffer; i++) {
        pBuffer = &v4lbufs[i];
        fsl_osal_memset(pBuffer, 0, sizeof(struct v4l2_buffer));
        pBuffer->index = i;
        pBuffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        pBuffer->memory = V4L2_MEMORY_MMAP;
        if (ioctl(fd_v4l, VIDIOC_QUERYBUF, pBuffer) < 0) {
            LOG_ERROR("VIDIOC_QUERYBUF failed %d\n", pBuffer->index);
            ret = OMX_ErrorHardware;
            break;
        }

        fb_data[i].nIndex = i;
        fb_data[i].nLength = pBuffer->length;
        fb_data[i].pPhyiscAddr = (OMX_PTR)pBuffer->m.offset;
        fb_data[i].pVirtualAddr = mmap(
                NULL, 
                pBuffer->length,
                PROT_READ | PROT_WRITE, MAP_SHARED,
                fd_v4l,
                pBuffer->m.offset);
        if(fb_data[i].pVirtualAddr == NULL) {
            LOG_ERROR("mmap buffer: %d failed.\n", i);
            ret = OMX_ErrorHardware;
            break;
        }

        AddHwBuffer(fb_data[i].pPhyiscAddr, fb_data[i].pVirtualAddr);

        if(bSetCrop == OMX_TRUE)
            pBuffer->m.offset = pBuffer->m.offset + (cr_top * sVideoFmt.nFrameWidth) + cr_left;
        LOG_DEBUG("%s,%d,v4l buf offset %x, base %x, cr_top %d, frame width %d, cr_left %d.\n",__FUNCTION__,__LINE__,
                pBuffer->m.offset,
                fb_data[i].pPhyiscAddr,
                cr_top,
                sVideoFmt.nFrameWidth,
                cr_left);

    }

    if(ret != OMX_ErrorNone) {
        V4lBufferDeInit();
        return ret;
    }

    bFrameBufferInit = OMX_TRUE;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::V4lBufferDeInit()
{
    OMX_U32 i;

    for(i=0; i<nFrameBuffer; i++) {
        if(fb_data[i].pVirtualAddr != NULL) {
            RemoveHwBuffer(fb_data[i].pVirtualAddr);
            munmap(fb_data[i].pVirtualAddr, fb_data[i].nLength);
        }
    }

    bFrameBufferInit = OMX_FALSE;
    nAllocating = 0;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::V4lSearchBuffer(
        OMX_PTR pVirtualAddr, 
        OMX_U32 *pIndex)
{
    OMX_U32 nIndex = 0;
    OMX_U32 i;
    OMX_BOOL bFound = OMX_FALSE;

    for(i=0; i<nFrameBuffer; i++) {
        if(fb_data[i].pVirtualAddr == pVirtualAddr) {
            nIndex = i;
            bFound = OMX_TRUE;
            break;
        }
    }

    if(bFound != OMX_TRUE)
        return OMX_ErrorUndefined;

    *pIndex = nIndex;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::V4lQueueBuffer(
        OMX_U32 nIndex)
{
    struct v4l2_buffer *pBuffer;

    pBuffer = &v4lbufs[nIndex];
    if(ioctl(fd_v4l, VIDIOC_QBUF, pBuffer) < 0) {
        LOG_ERROR("Queue buffer #%d failed.\n", nIndex);
        SendEvent(OMX_EventError, OMX_ErrorHardware, 0, NULL);
        return OMX_ErrorHardware;
    }
    EnqueueBufferIdx = nIndex;

    nQueuedBuffer ++;
    nSourceFrames ++;
    if(nQueuedBuffer >= 2 && bStreamOn != OMX_TRUE) {
        OMX_S32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd_v4l, VIDIOC_STREAMON, &type) < 0) {
            LOG_ERROR("Stream On failed.\n");
            SendEvent(OMX_EventError, OMX_ErrorHardware, 0, NULL);
            return OMX_ErrorHardware;
        }
        bStreamOn = OMX_TRUE;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::V4lDeQueueBuffer(
        OMX_U32 *pIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    struct v4l2_buffer v4lbuf;
    OMX_U32 i;

    if(nQueuedBuffer < 2)
        return OMX_ErrorNotReady;

    for(i=0; i<BUFFER_NEW_RETRY_MAX; i++) {
        fsl_osal_memset(&v4lbuf, 0, sizeof(struct v4l2_buffer));
        v4lbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4lbuf.memory = V4L2_MEMORY_MMAP;
        if(ioctl(fd_v4l, VIDIOC_DQBUF, &v4lbuf) < 0) {
            usleep(1000);
            ret = OMX_ErrorNotReady;
            continue;
        }
        nQueuedBuffer --;
        *pIndex = v4lbuf.index;
        nCurTS = (OMX_TICKS)(v4lbuf.timestamp.tv_sec*1000000+v4lbuf.timestamp.tv_usec);
        ret = OMX_ErrorNone;
        break;
    }

    return ret;
}

OMX_ERRORTYPE V4lSource::V4lStreamOff(OMX_BOOL bKeepLast)
{
    OMX_S32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    OMX_U32 i;

    if(bStreamOn != OMX_FALSE) {
        ioctl(fd_v4l, VIDIOC_STREAMOFF, &type);
        for(i=0; i<nFrameBuffer; i++) {
            if(BufferHdrs[i] != NULL) {
                if(bKeepLast == OMX_TRUE && i == EnqueueBufferIdx)
                    continue;
                ports[CAPTURED_FRAME_PORT]->SendBuffer(BufferHdrs[i]);
            }
        }

        bStreamOn = OMX_FALSE;
        nQueuedBuffer = 0;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::V4lStreamOnInPause()
{
    OMX_U32 nIndex;

    if(EnqueueBufferIdx < 0)
        return OMX_ErrorNone;

    V4lQueueBuffer(EnqueueBufferIdx);
    V4lQueueBuffer(EnqueueBufferIdx);
    V4lDeQueueBuffer(&nIndex);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::V4lSetInputCrop()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    struct v4l2_format fmt;
    struct v4l2_mxc_offset off;
    OMX_S32 in_width, in_height;
    OMX_S32 cr_left, cr_top, cr_right, cr_bottom;
    OMX_S32 in_width_chroma, in_height_chroma;
    OMX_S32 crop_left_chroma, crop_right_chroma, crop_top_chroma, crop_bottom_chroma;
    OMX_U32 i;
    OMX_CONFIG_RECTTYPE sRectSetup = sRectIn;
    sRectSetup.nWidth = sRectSetup.nWidth/8*8;
    sRectSetup.nLeft = (sRectSetup.nLeft + 7)/8*8;
    in_width = sRectSetup.nWidth;
    in_height = sRectSetup.nHeight;

    fsl_osal_memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = in_width;
    fmt.fmt.pix.height = in_height;
    if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV420Planar) {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
        LOG_DEBUG("Set V4L in format to YUVI420.\n");
    }
    if(sVideoFmt.eColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
        LOG_DEBUG("Set V4L in format to NV12.\n");
    }

    cr_left   = sRectSetup.nLeft;
    cr_top    = sRectSetup.nTop;
    cr_right  = sVideoFmt.nFrameWidth - (sRectSetup.nLeft + sRectSetup.nWidth);
    cr_bottom = sVideoFmt.nFrameHeight - (sRectSetup.nTop + sRectSetup.nHeight);

    in_width_chroma = in_width / 2;
    in_height_chroma = in_height / 2;

    crop_left_chroma = cr_left / 2;
    crop_right_chroma = cr_right / 2;
    crop_top_chroma = cr_top / 2;
    crop_bottom_chroma = cr_bottom / 2;

    if ((cr_left!=0) || (cr_top!=0) || (cr_right!=0) || (cr_bottom!=0)) {
        off.u_offset = ((cr_left + cr_right + in_width) * (in_height + cr_bottom)) - cr_left
            + (crop_top_chroma * (in_width_chroma + crop_left_chroma + crop_right_chroma))
            + crop_left_chroma;
        off.v_offset = off.u_offset +
            (crop_left_chroma + crop_right_chroma + in_width_chroma)
            * ( in_height_chroma  + crop_bottom_chroma + crop_top_chroma);

        fmt.fmt.pix.bytesperline = in_width + cr_left + cr_right;
        fmt.fmt.pix.priv = (OMX_S32) & off;
        fmt.fmt.pix.sizeimage = (in_width + cr_left + cr_right)
            * (in_height + cr_top + cr_bottom) * 3 / 2;

    }
    else {
        fmt.fmt.pix.bytesperline = in_width;
        fmt.fmt.pix.priv = 0;
        fmt.fmt.pix.sizeimage = 0;
    }

    LOG_DEBUG("%s,%d,\n\ 
                      in_width %d,in_height %d\n\
                      cr_left %d, cr_top %d, cr_right %d cr_bottom %d\n\
                      u_offset %d, v_offset %d\n",__FUNCTION__,__LINE__,
                      in_width,in_height,
                      cr_left,cr_top,cr_right,cr_bottom,
                      off.u_offset,off.v_offset);

    if (ioctl(fd_v4l, VIDIOC_S_FMT, &fmt) < 0) {
        LOG_ERROR("set format failed\n");
        return OMX_ErrorHardware;
    }

    if (ioctl(fd_v4l, VIDIOC_G_FMT, &fmt) < 0) {
        LOG_ERROR("get format failed\n");
        return OMX_ErrorHardware;
    }

    /* set v4l buffers corp */
    for(i=0; i<nFrameBuffer; i++) {
        struct v4l2_buffer *pBuffer = NULL;
        pBuffer = &v4lbufs[i];
        if(pBuffer != NULL)
            pBuffer->m.offset = (OMX_U32)fb_data[i].pPhyiscAddr + (cr_top * sVideoFmt.nFrameWidth) + cr_left;
        LOG_DEBUG("%s,%d,v4l buf offset %x, base %x, cr_top %d, frame width %d, cr_left %d.\n",__FUNCTION__,__LINE__,
                pBuffer->m.offset,
                fb_data[i].pPhyiscAddr,
                cr_top,
                sVideoFmt.nFrameWidth,
                cr_left);
    }

    return ret;
}

OMX_ERRORTYPE V4lSource::V4lSetOutputCrop()
{
    struct v4l2_crop crop;

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    crop.c.width = sRectOut.nWidth;
    crop.c.height = sRectOut.nHeight;
    crop.c.top = sRectOut.nTop;
    crop.c.left = sRectOut.nLeft;

    LOG_DEBUG("%s,%d, width %d,height %d,top %d, left %d\n",__FUNCTION__,__LINE__,
            crop.c.width,crop.c.height,crop.c.top,crop.c.left);

    if (ioctl(fd_v4l, VIDIOC_S_CROP, &crop) < 0) {
        LOG_ERROR("set crop failed\n");
        return OMX_ErrorHardware;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE V4lSource::V4lSetRotation()
{
    struct v4l2_control ctrl;

    ctrl.id = V4L2_CID_PRIVATE_BASE;
    ctrl.value = (OMX_S32)Rotation.nRotation;
	printf("Rotation: %d\n", Rotation.nRotation);
    if (ioctl(fd_v4l, VIDIOC_S_CTRL, &ctrl) < 0) {
        LOG_ERROR("set rotate failed\n");
        return OMX_ErrorHardware;
    }

    LOG_DEBUG("%s,%d, set screen rotate to %d\n",__FUNCTION__,__LINE__,eRotation);
    return OMX_ErrorNone;
}

/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE V4lSourceInit(OMX_IN OMX_HANDLETYPE pHandle)
		{
			OMX_ERRORTYPE ret = OMX_ErrorNone;
			V4lSource *obj = NULL;
			ComponentBase *base = NULL;

			obj = FSL_NEW(V4lSource, ());
			if(obj == NULL)
				return OMX_ErrorInsufficientResources;

			base = (ComponentBase*)obj;
			ret = base->ConstructComponent(pHandle);
			if(ret != OMX_ErrorNone)
				return ret;

			return ret;
		}
	}

/* File EOF */
