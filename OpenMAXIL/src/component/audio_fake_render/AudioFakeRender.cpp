/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <sys/time.h>
#include "AudioFakeRender.h"
  
AudioFakeRender::AudioFakeRender()
{ 
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_render.fake.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = (OMX_STRING)"audio_render.fake";
    bInContext = OMX_FALSE;
    nPorts = NUM_PORTS;
}

OMX_ERRORTYPE AudioFakeRender::OpenDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	return ret;
}

OMX_ERRORTYPE AudioFakeRender::CloseDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	return ret;
}

OMX_ERRORTYPE AudioFakeRender::SetDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	nSampleSize = PcmMode.nChannels*PcmMode.nBitPerSample>>3;
	nPeriodSize = nSampleSize * 1024;
	nFadeInFadeOutProcessLen = 0;

	return ret;
}

OMX_ERRORTYPE AudioFakeRender::WriteDevice(OMX_U8 *pBuffer, OMX_U32 nActuralLen)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	nActuralLen /= nSampleSize;
   
	usleep(nActuralLen*1000000000/PcmMode.nSamplingRate);
   
	return ret;
}

OMX_ERRORTYPE AudioFakeRender::DrainDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	return ret;
}

OMX_ERRORTYPE AudioFakeRender::DeviceDelay(OMX_U32 *nDelayLen)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	*nDelayLen = 0;

	return ret;
}

OMX_ERRORTYPE AudioFakeRender::ResetDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	return ret;
}

/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE AudioFakeRenderInit(OMX_IN OMX_HANDLETYPE pHandle)
		{
			OMX_ERRORTYPE ret = OMX_ErrorNone;
			AudioFakeRender *obj = NULL;
			ComponentBase *base = NULL;

			obj = FSL_NEW(AudioFakeRender, ());
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
