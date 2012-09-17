/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <sys/time.h>
#include "AlsaRender.h"
  
AlsaRender::AlsaRender()
{ 
    fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_render.alsa.sw-based");
    ComponentVersion.s.nVersionMajor = 0x1;
    ComponentVersion.s.nVersionMinor = 0x1;
    ComponentVersion.s.nRevision = 0x2;
    ComponentVersion.s.nStep = 0x0;
    role_cnt = 1;
    role[0] = "audio_render.alsa";
    bInContext = OMX_FALSE;
    nPorts = NUM_PORTS;
	fsl_osal_strcpy((fsl_osal_char *)Device, (fsl_osal_char *)"default");
}

OMX_ERRORTYPE AlsaRender::OpenDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 eRetVal;

	struct timeval tv, tv1;
	gettimeofday (&tv, NULL);

	/* Allocate the playback handle and the hardware parameter structure */
	if ((eRetVal = snd_pcm_open (&hPlayback, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		return OMX_ErrorHardware;
	}

	gettimeofday (&tv1, NULL);
	LOG_DEBUG("Alsa open time: %d\n", (tv1.tv_sec-tv.tv_sec)*1000+(tv1.tv_usec-tv.tv_usec)/1000);

	if ((eRetVal = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		return OMX_ErrorHardware;
	}
	if ((eRetVal = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
		return OMX_ErrorHardware;
	}

	return ret;
}

OMX_ERRORTYPE AlsaRender::CloseDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (hPlayback)
    {
        if(hw_params) {
          snd_pcm_hw_params_free (hw_params);
          hw_params = NULL;
        }
        if(sw_params) {
          snd_pcm_sw_params_free (sw_params);
          sw_params = NULL;
        }
        if(hPlayback) {
          snd_pcm_close(hPlayback);
          hPlayback = NULL;
        }
    }

	return ret;
}
//#define LOG_DEBUG printf
OMX_ERRORTYPE AlsaRender::SetDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	snd_pcm_uframes_t period_size_min;
	snd_pcm_uframes_t period_size_max;
	snd_pcm_uframes_t buffer_size_min;
	snd_pcm_uframes_t buffer_size_max;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t period_size;
	snd_output_t *output = NULL;
	snd_pcm_uframes_t start_threshold, stop_threshold;
	snd_pcm_uframes_t xfer_align;
	OMX_S32 sleep_min = 0;
	OMX_U32 buffer_time = 0;                  /* ring buffer length in us */
	OMX_U32 period_time = 0;                  /* period time in us */
	OMX_U32 nperiods = 4;                     /* number of periods */
	OMX_BOOL bNeedChange = OMX_FALSE;
	OMX_S8 *pDevice;
	OMX_U32 nSamplingRate, nSamplingRateBak;
	OMX_S32 eRetVal;
	OMX_S32 err;
	OMX_U32 n;
  
	nSamplingRate = (OMX_U64)PcmMode.nSamplingRate * nClockScale / Q16_SHIFT;
	switch(nSamplingRate)
	{
		case 8000:
			pDevice = (OMX_S8 *)"dmix_8000";
			break;
		case 11025:
			pDevice = (OMX_S8 *)"dmix_11025";
			break;
		case 12000:
			pDevice = (OMX_S8 *)"dmix_12000";
			break;
		case 16000:
			pDevice = (OMX_S8 *)"dmix_16000";
			break;
		case 22050:
			pDevice = (OMX_S8 *)"dmix_22050";
			break;
		case 24000:
			pDevice = (OMX_S8 *)"dmix_24000";
			break;
		case 32000:
			pDevice = (OMX_S8 *)"dmix_32000";
			break;
		case 44100:
			pDevice = (OMX_S8 *)"dmix_44100";
			break;
		case 48000:
			pDevice = (OMX_S8 *)"dmix_48000";
			break;
		default:
			pDevice = (OMX_S8 *)"default";
			break;
	} 

#ifdef USE_DEFAULT_AUDIO_DEVICE
	pDevice = (OMX_S8 *)("default");
#endif

AGAIN:
	if (fsl_osal_strcmp((fsl_osal_char *)Device, (fsl_osal_char *)pDevice))
	{
		bNeedChange = OMX_TRUE;
	}
	fsl_osal_strcpy((fsl_osal_char *)Device, (fsl_osal_char *)pDevice);

	LOG_DEBUG("Try Open Alsa times.\n");
	if (hPlayback != NULL && bNeedChange)
	{
		LOG_DEBUG("Free ALSA.\n");
		if(hw_params) {
			snd_pcm_hw_params_free (hw_params);
			hw_params = NULL;
		}
		if(sw_params) {
			snd_pcm_sw_params_free (sw_params);
			sw_params = NULL;
		}
		if(hPlayback) {
			snd_pcm_close(hPlayback);
			hPlayback = NULL;
		}
	}

	if (hPlayback == NULL || bNeedChange)
	{
		/* Allocate the playback handle and the hardware parameter structure */
		LOG_DEBUG("Open %s\n", pDevice);
		struct timeval tv, tv1;
		gettimeofday (&tv, NULL);

		if ((eRetVal = snd_pcm_open (&hPlayback, (char *)pDevice, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
			goto ERRORS;
		}

		gettimeofday (&tv1, NULL);
		LOG_DEBUG("Alsa open time: %d\n", (tv1.tv_sec-tv.tv_sec)*1000+(tv1.tv_usec-tv.tv_usec)/1000);

		if ((eRetVal = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
			return OMX_ErrorHardware;
		}
		if ((eRetVal = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
			return OMX_ErrorHardware;
		}
	}

	if ((eRetVal = snd_pcm_hw_params_any (hPlayback, hw_params)) < 0) {
		return OMX_ErrorHardware;
	}

	unsigned int nValue;
	//if(snd_pcm_hw_params_get_channels(hw_params, &nValue)){
	//	goto ERRORS;
	//}
	//LOG_DEBUG("Channels:%d\n", nValue);

	if(snd_pcm_hw_params_get_channels_min(hw_params, &nValue)){
		goto ERRORS;
	}
	LOG_DEBUG("Channels Min:%d\n", nValue);

	if(snd_pcm_hw_params_get_channels_max(hw_params, &nValue)){
		goto ERRORS;
	}
	LOG_DEBUG("Channels Max:%d\n", nValue);

	LOG_DEBUG("Channels:%d\n", PcmMode.nChannels);
	if(snd_pcm_hw_params_set_channels(hPlayback, hw_params, PcmMode.nChannels)){
		goto ERRORS;
	}

	if(PcmMode.bInterleaved == OMX_TRUE){
		if ((eRetVal = snd_pcm_hw_params_set_access(hPlayback, \
						hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			return OMX_ErrorHardware;
		}
	}
	else{
		if ((eRetVal = snd_pcm_hw_params_set_access(hPlayback, \
						hw_params, SND_PCM_ACCESS_RW_NONINTERLEAVED)) < 0) {
			return OMX_ErrorHardware;
		}
	}
	
	int dir;
   	//if(snd_pcm_hw_params_get_channels(hw_params, &nValue)){
	//	goto ERRORS;
	//}
	//LOG_DEBUG("Channels:%d\n", nValue);

	if(snd_pcm_hw_params_get_rate_min(hw_params, &nValue, &dir)){
		goto ERRORS;
	}
	LOG_DEBUG("Rate Min:%d\n", nValue);

	if(snd_pcm_hw_params_get_rate_max(hw_params, &nValue, &dir)){
		goto ERRORS;
	}
	LOG_DEBUG("Rate Max:%d\n", nValue);

	LOG_DEBUG("nSamplingRate before %d\n", nSamplingRate);
	nSamplingRateBak = nSamplingRate;
	if ((eRetVal = snd_pcm_hw_params_set_rate_near(hPlayback, hw_params, (unsigned int *)(&nSamplingRateBak), 0)) < 0) {
		goto ERRORS;
	}
	LOG_DEBUG("Sample Rate: %d Sample Rate Near: %d\n", nSamplingRate, nSamplingRateBak);
	if (nSamplingRate != nSamplingRateBak)
	{
		goto ERRORS; 
	}
	nSamplingRate = nSamplingRateBak;
	nSampleSize = PcmMode.nChannels*(PcmMode.nBitPerSample>> 3);
	LOG_DEBUG("nSamplingRate after %d\n", nSamplingRateBak);
	nFadeInFadeOutProcessLen = nSamplingRate * FADEPROCESSTIME / 1000 * nSampleSize; 
	LOG_DEBUG("nFadeInFadeOutProcessLen = %d\n", nFadeInFadeOutProcessLen);
 
	switch(PcmMode.ePCMMode)
	{
		case OMX_AUDIO_PCMModeLinear:
			switch(PcmMode.nBitPerSample)
			{
				case 8:
					if ((eRetVal = snd_pcm_hw_params_set_format(hPlayback, \
									hw_params, SND_PCM_FORMAT_S8)) < 0) {
						goto ERRORS;
					}
					break;
				case 16:
					if ((eRetVal = snd_pcm_hw_params_set_format(hPlayback, \
									hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
						goto ERRORS;
					}
					break;
				case 24:
					if ((eRetVal = snd_pcm_hw_params_set_format(hPlayback, \
									hw_params, SND_PCM_FORMAT_S24_3LE)) < 0) {
						goto ERRORS;
					}
					break;
			}
			break;
		case OMX_AUDIO_PCMModeALaw:
			if ((eRetVal = snd_pcm_hw_params_set_format(hPlayback, \
							hw_params, SND_PCM_FORMAT_A_LAW)) < 0) {
				return OMX_ErrorHardware;
			}
			break;
		case OMX_AUDIO_PCMModeMULaw:
			if ((eRetVal = snd_pcm_hw_params_set_format(hPlayback, \
							hw_params, SND_PCM_FORMAT_MU_LAW)) < 0) {
				return OMX_ErrorHardware;
			}
			break;
		default:
			return OMX_ErrorBadParameter;
	}

	/* set the buffer time */
	err = snd_pcm_hw_params_get_buffer_size_min(hw_params, &buffer_size_min);
	err = snd_pcm_hw_params_get_buffer_size_max(hw_params, &buffer_size_max);
	err = snd_pcm_hw_params_get_period_size_min(hw_params, &period_size_min, NULL);
	err = snd_pcm_hw_params_get_period_size_max(hw_params, &period_size_max, NULL);
	LOG_DEBUG("Buffer size range from %lu to %lu\n",buffer_size_min, buffer_size_max);
	LOG_DEBUG("Period size range from %lu to %lu\n",period_size_min, period_size_max);
	if (period_time > 0) {
		LOG_DEBUG("Requested period time %u us\n", period_time);
		err = snd_pcm_hw_params_set_period_time_near(hPlayback, \
				hw_params, (unsigned int *)(&period_time), NULL);
		if (err < 0) {
			LOG_DEBUG("Unable to set period time %u us for playback: %s\n",
					period_time, snd_strerror(err));
			return OMX_ErrorHardware;
		}
	}
	if (buffer_time > 0) {
		LOG_DEBUG("Requested buffer time %u us\n", buffer_time);
		err = snd_pcm_hw_params_set_buffer_time_near(hPlayback, \
				hw_params, (unsigned int *)&buffer_time, NULL);
		if (err < 0) {
			LOG_DEBUG("Unable to set buffer time %u us for playback: %s\n",
					buffer_time, snd_strerror(err));
			return OMX_ErrorHardware;
		}
	}
	if (! buffer_time && ! period_time) {
		buffer_size = buffer_size_max;
		if (! period_time)
			buffer_size = (buffer_size / nperiods) * nperiods;
		LOG_DEBUG("Using max buffer size %lu\n", buffer_size);
		err = snd_pcm_hw_params_set_buffer_size_near(hPlayback, \
				hw_params, &buffer_size);
		if (err < 0) {
			LOG_DEBUG("Unable to set buffer size %lu for playback: %s\n",
					buffer_size, snd_strerror(err));
			return OMX_ErrorHardware;
		}
	}
	if (! buffer_time || ! period_time) {
		LOG_DEBUG("Periods = %u\n", nperiods);
		period_size = buffer_size / nperiods;
		err = snd_pcm_hw_params_set_period_size_near(hPlayback, \
				hw_params, &period_size, (int *)&dir);
		if (err < 0) {
			LOG_DEBUG("Unable to set nperiods %u for playback: %s\n",
					nperiods, snd_strerror(err));
			return OMX_ErrorHardware;
		}
	}
	snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
	snd_pcm_hw_params_get_period_size(hw_params, &period_size, NULL);
	LOG_DEBUG("was set period_size = %lu\n",period_size);
	LOG_DEBUG("was set buffer_size = %lu\n",buffer_size);
	if (2*period_size > buffer_size) {
		LOG_DEBUG("buffer to small, could not use\n");
		return OMX_ErrorHardware;
	}
	period_time = period_size;
 
	nPeriodSize = period_time;

	/** Configure and prepare the ALSA handle */
	if ((eRetVal = snd_pcm_hw_params (hPlayback, hw_params)) < 0) {
		LOG_DEBUG("snd_pcm_hw_params() return: %d\n", eRetVal);
		return OMX_ErrorHardware;
	}
	err = snd_output_stdio_attach(&output, stdout, 0);
	if (err < 0) {
		LOG_DEBUG("Output failed: %s\n", snd_strerror(err));
		return OMX_ErrorHardware;
	}
	//snd_pcm_dump(hPlayback, output);
	//output = NULL;

	snd_pcm_sw_params_current(hPlayback, sw_params);
	err = snd_pcm_sw_params_get_xfer_align(sw_params, &xfer_align);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}
	if (sleep_min)
		xfer_align = 1;
	err = snd_pcm_sw_params_set_sleep_min(hPlayback, sw_params,
			sleep_min);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}

	n = period_size;
	err = snd_pcm_sw_params_set_avail_min(hPlayback, sw_params, n);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}

	/* round up to closest transfer boundary */
	n = ((buffer_size-period_size) / xfer_align) * xfer_align;
	if (n < 1)
		n = 1;
	if (n > buffer_size)
		n = buffer_size;
	start_threshold = n;
	LOG_DEBUG("start_threshold = %d\n", start_threshold);
	err = snd_pcm_sw_params_set_start_threshold(hPlayback, \
			sw_params, start_threshold);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}
	stop_threshold = buffer_size;
	err = snd_pcm_sw_params_set_stop_threshold(hPlayback, \
			sw_params, stop_threshold);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}

	err = snd_pcm_sw_params_set_xfer_align(hPlayback, sw_params, xfer_align);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}

	if (snd_pcm_sw_params(hPlayback, sw_params) < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}

	err = snd_output_stdio_attach(&output, stdout, 0);
	if (err < 0) {
		LOG_DEBUG("Output failed: %s\n", snd_strerror(err));
		return OMX_ErrorHardware;
	}
	//snd_pcm_dump(hPlayback, output);

	if ((eRetVal = snd_pcm_prepare (hPlayback)) < 0) {
		return OMX_ErrorHardware;
	}

	return ret;

ERRORS:
	if (!fsl_osal_strcmp((fsl_osal_char *)("default"), (fsl_osal_char *)pDevice))
	{
		if (nClockScale != Q16_SHIFT)
		{
			printf("Alsa can't support the speed. Change to normal playback.\n");
			nClockScale = Q16_SHIFT;
			nSamplingRate = (OMX_U64)PcmMode.nSamplingRate * nClockScale / Q16_SHIFT;
		}
		else
		{
			return OMX_ErrorHardware;
		}
	}
	pDevice = (OMX_S8 *)("default");
	goto AGAIN;
}

OMX_ERRORTYPE AlsaRender::WriteDevice(OMX_U8 *pBuffer, OMX_U32 nActuralLen)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 SndRet;
	nActuralLen /= nSampleSize;
   
	while(nActuralLen)
	{
		SndRet = snd_pcm_writei(hPlayback, pBuffer, nActuralLen);
		LOG_LOG("SndRet = %d\n", SndRet);
		if (SndRet < 0)
		{
			LOG_WARNING("PCM write fail.\n");
			snd_pcm_prepare(hPlayback);
			SndRet = 0;
		}
		nActuralLen -= SndRet;
		pBuffer += SndRet * nSampleSize;
	}
       
	return ret;
}

OMX_ERRORTYPE AlsaRender::DrainDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	snd_pcm_drain(hPlayback);

	return ret;
}

OMX_ERRORTYPE AlsaRender::DeviceDelay(OMX_U32 *nDelayLen)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	snd_pcm_sframes_t Delay;

	snd_pcm_delay(hPlayback, &Delay);
	*nDelayLen = Delay * nSampleSize;

	return ret;
}

OMX_ERRORTYPE AlsaRender::ResetDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	snd_pcm_prepare(hPlayback);

	return ret;
}

/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE AlsaRenderInit(OMX_IN OMX_HANDLETYPE pHandle)
		{
			OMX_ERRORTYPE ret = OMX_ErrorNone;
			AlsaRender *obj = NULL;
			ComponentBase *base = NULL;

			obj = FSL_NEW(AlsaRender, ());
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
