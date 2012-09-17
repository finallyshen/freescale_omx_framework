/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <sys/time.h>
#include "AlsaSource.h"

#ifndef timersub
#define	timersub(a, b, result) \
do { \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
	(result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
	if ((result)->tv_usec < 0) { \
		--(result)->tv_sec; \
		(result)->tv_usec += 1000000; \
	} \
} while (0)
#endif

AlsaSource::AlsaSource()
{ 
	fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.audio_source.alsa.sw-based");
	ComponentVersion.s.nVersionMajor = 0x0;
	ComponentVersion.s.nVersionMinor = 0x1;
	ComponentVersion.s.nRevision = 0x0;
	ComponentVersion.s.nStep = 0x0;
	role_cnt = 1;
	role[0] = "audio_source.alsa";

	hCapture = 0;
    nSourceFrames = 0;
	nCurTS = 0;
    lock = NULL;
}

OMX_ERRORTYPE AlsaSource::InitSourceComponent()
{
    if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal)) {
        LOG_ERROR("Create mutex for alsa device failed.\n");
        return OMX_ErrorInsufficientResources;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AlsaSource::DeInitSourceComponent()
{
    if(lock)
        fsl_osal_mutex_destroy(lock);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE AlsaSource::OpenDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 eRetVal;
	char alsa_device[100] = "default";

	if ((eRetVal = snd_pcm_open(&hCapture, alsa_device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		LOG_ERROR("Unable to open %s\n", alsa_device);
		return OMX_ErrorHardware;
	}

	if ((eRetVal = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		return OMX_ErrorHardware;
	}
	if ((eRetVal = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
		return OMX_ErrorHardware;
	}

	ret = SetDevice();
	if (ret != OMX_ErrorNone)
		return ret;

	return ret;
}

OMX_ERRORTYPE AlsaSource::CloseDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (hCapture)
    {
        if(hw_params) {
          snd_pcm_hw_params_free (hw_params);
          hw_params = NULL;
        }
        if(sw_params) {
          snd_pcm_sw_params_free (sw_params);
          sw_params = NULL;
        }
        if(hCapture) {
          snd_pcm_close(hCapture);
          hCapture = NULL;
        }
    }

	return ret;
}

OMX_ERRORTYPE AlsaSource::SetDevice()
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
	OMX_U32 nSamplingRate;
	OMX_S32 eRetVal;
	OMX_S32 err;
	OMX_U32 n;
	unsigned int nValue;
	int dir;
  
	if ((eRetVal = snd_pcm_hw_params_any (hCapture, hw_params)) < 0) {
		return OMX_ErrorHardware;
	}

	if(snd_pcm_hw_params_get_channels_min(hw_params, &nValue)){
		goto ERRORS;
	}
	LOG_DEBUG("Channels Min:%d\n", nValue);

	if(snd_pcm_hw_params_get_channels_max(hw_params, &nValue)){
		goto ERRORS;
	}
	LOG_DEBUG("Channels Max:%d\n", nValue);

	printf("Channels:%d\n", PcmMode.nChannels);
	if(snd_pcm_hw_params_set_channels(hCapture, hw_params, PcmMode.nChannels)){
		goto ERRORS;
	}

	if(PcmMode.bInterleaved == OMX_TRUE){
		if ((eRetVal = snd_pcm_hw_params_set_access(hCapture, \
						hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			return OMX_ErrorHardware;
		}
	} else {
		if ((eRetVal = snd_pcm_hw_params_set_access(hCapture, \
						hw_params, SND_PCM_ACCESS_RW_NONINTERLEAVED)) < 0) {
			return OMX_ErrorHardware;
		}
	}
	
	if(snd_pcm_hw_params_get_rate_min(hw_params, &nValue, &dir)){
		goto ERRORS;
	}
	LOG_DEBUG("Rate Min:%d\n", nValue);

	if(snd_pcm_hw_params_get_rate_max(hw_params, &nValue, &dir)){
		goto ERRORS;
	}
	LOG_DEBUG("Rate Max:%d\n", nValue);

	nSamplingRate = PcmMode.nSamplingRate;;
	printf("nSamplingRate before %d\n", nSamplingRate);
	if ((eRetVal = snd_pcm_hw_params_set_rate_near(hCapture, hw_params, (unsigned int *)(&nSamplingRate), 0)) < 0) {
		goto ERRORS;
	}
	nSampleSize = PcmMode.nChannels*(PcmMode.nBitPerSample>> 3);
	LOG_DEBUG("nSamplingRate after %d\n", nSamplingRate);
	nFadeInFadeOutProcessLen = nSamplingRate * FADEPROCESSTIME / 1000 * nSampleSize; 
	LOG_DEBUG("nFadeInFadeOutProcessLen = %d\n", nFadeInFadeOutProcessLen);
 
	switch(PcmMode.ePCMMode)
	{
		case OMX_AUDIO_PCMModeLinear:
			switch(PcmMode.nBitPerSample)
			{
				case 8:
					if ((eRetVal = snd_pcm_hw_params_set_format(hCapture, \
									hw_params, SND_PCM_FORMAT_S8)) < 0) {
						goto ERRORS;
					}
					break;
				case 16:
					if ((eRetVal = snd_pcm_hw_params_set_format(hCapture, \
									hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
						goto ERRORS;
					}
					break;
				case 24:
					if ((eRetVal = snd_pcm_hw_params_set_format(hCapture, \
									hw_params, SND_PCM_FORMAT_S24_3LE)) < 0) {
						goto ERRORS;
					}
					break;
			}
			break;
		case OMX_AUDIO_PCMModeALaw:
			if ((eRetVal = snd_pcm_hw_params_set_format(hCapture, \
							hw_params, SND_PCM_FORMAT_A_LAW)) < 0) {
				return OMX_ErrorHardware;
			}
			break;
		case OMX_AUDIO_PCMModeMULaw:
			if ((eRetVal = snd_pcm_hw_params_set_format(hCapture, \
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
		err = snd_pcm_hw_params_set_period_time_near(hCapture, \
				hw_params, (unsigned int *)(&period_time), NULL);
		if (err < 0) {
			LOG_DEBUG("Unable to set period time %u us for playback: %s\n",
					period_time, snd_strerror(err));
			return OMX_ErrorHardware;
		}
	}
	if (buffer_time > 0) {
		LOG_DEBUG("Requested buffer time %u us\n", buffer_time);
		err = snd_pcm_hw_params_set_buffer_time_near(hCapture, \
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
		err = snd_pcm_hw_params_set_buffer_size_near(hCapture, \
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
		err = snd_pcm_hw_params_set_period_size_near(hCapture, \
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
	if ((eRetVal = snd_pcm_hw_params (hCapture, hw_params)) < 0) {
		LOG_DEBUG("snd_pcm_hw_params() return: %d\n", eRetVal);
		return OMX_ErrorHardware;
	}
	err = snd_output_stdio_attach(&output, stdout, 0);
	if (err < 0) {
		LOG_DEBUG("Output failed: %s\n", snd_strerror(err));
		return OMX_ErrorHardware;
	}
	//snd_pcm_dump(hCapture, output);
	//output = NULL;

	snd_pcm_sw_params_current(hCapture, sw_params);
	err = snd_pcm_sw_params_get_xfer_align(sw_params, &xfer_align);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}
	if (sleep_min)
		xfer_align = 1;
	err = snd_pcm_sw_params_set_sleep_min(hCapture, sw_params,
			sleep_min);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}

	n = period_size;
	err = snd_pcm_sw_params_set_avail_min(hCapture, sw_params, n);
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
	err = snd_pcm_sw_params_set_start_threshold(hCapture, \
			sw_params, start_threshold);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}
	stop_threshold = buffer_size;
	err = snd_pcm_sw_params_set_stop_threshold(hCapture, \
			sw_params, stop_threshold);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}

	err = snd_pcm_sw_params_set_xfer_align(hCapture, sw_params, xfer_align);
	if (err < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}

	if (snd_pcm_sw_params(hCapture, sw_params) < 0) {
		LOG_DEBUG("%s,%d Error.\n", __FUNCTION__,__LINE__);
	}

	err = snd_output_stdio_attach(&output, stdout, 0);
	if (err < 0) {
		LOG_DEBUG("Output failed: %s\n", snd_strerror(err));
		return OMX_ErrorHardware;
	}
	//snd_pcm_dump(hCapture, output);

	if ((eRetVal = snd_pcm_prepare (hCapture)) < 0) {
		return OMX_ErrorHardware;
	}

	return ret;

ERRORS:
	return ret;
}

OMX_ERRORTYPE AlsaSource::StartDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	return ret;
}

OMX_ERRORTYPE AlsaSource::StopDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	snd_pcm_drop(hCapture);

	return ret;
}

int AlsaSource::xrun()
{
	snd_pcm_status_t *status;
	int res;

	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(hCapture, status))<0) {
		fprintf(stderr, "Status error:\n", snd_strerror(res));
		return -1;
	}
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		struct timeval now, diff, tstamp;
		gettimeofday(&now, 0);
		snd_pcm_status_get_trigger_tstamp(status, &tstamp);
		timersub(&now, &tstamp, &diff);
		fprintf(stderr, "overrun!!! (at least %.3f ms long)\n", diff.tv_sec * 1000 + diff.tv_usec / 1000.0);
		if ((res = snd_pcm_prepare(hCapture))<0) {
			fprintf(stderr, "xrun prepare error:\n",snd_strerror(res));
			return -1;
		}
		return 1;		/* ok, data should be accepted again */
	}
}

int AlsaSource::suspend()
{
	int res;

	while ((res = snd_pcm_resume(hCapture)) == -EAGAIN)
		sleep(1);	/* wait until suspend flag is released */
	if (res < 0) {
		fprintf(stderr, "Failed. Restarting stream. "); 
		fflush(stderr);
		if ((res = snd_pcm_prepare(hCapture)) < 0) {
			fprintf(stderr, "suspend prepare error:\n",snd_strerror(res));
			return -1;
		}
	}
	return 1;
}

OMX_ERRORTYPE AlsaSource::GetOneFrameFromDevice()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U8 *pBuffer = pDeviceReadBuffer;
    OMX_U32 nReadSize = nPeriodSize;
	OMX_S32 snd_ret;
	OMX_U32 result = 0;

    if(hCapture <= 0)
        return OMX_ErrorNotReady;

    fsl_osal_mutex_lock(lock);

	while (nReadSize > 0) {
		snd_ret = snd_pcm_readi(hCapture, pBuffer, nReadSize);
		LOG_LOG("snd_ret: %d\n", snd_ret);
		if (snd_ret == -EAGAIN || (snd_ret >= 0 && snd_ret < nReadSize)) {
			snd_pcm_wait(hCapture, 1000);
		} else if (snd_ret == -EPIPE) {
			if (xrun() < 0) {
				fsl_osal_mutex_unlock(lock);
				return OMX_ErrorHardware;
			}
		} else if (snd_ret == -ESTRPIPE) {
			if (suspend() < 0) {
				fsl_osal_mutex_unlock(lock);
				return OMX_ErrorHardware;
			}
		} else if (snd_ret < 0) {
			printf("read error: %s", snd_strerror(snd_ret));
			fsl_osal_mutex_unlock(lock);
			return OMX_ErrorHardware;
		}
		if (snd_ret > 0) {
			result += snd_ret;
			nReadSize -= snd_ret;
			pBuffer += snd_ret * nSampleSize;
		}
	}

	nDeviceReadLen = nPeriodSize * nSampleSize;
 
	fsl_osal_mutex_unlock(lock);

	return ret;
}

OMX_ERRORTYPE AlsaSource::GetDeviceDelay(OMX_U32 *nDelayLen)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	snd_pcm_sframes_t Delay;

	snd_pcm_delay(hCapture, &Delay);
	*nDelayLen = Delay * nSampleSize;

	return ret;
}

OMX_S16 AlsaSource::getMaxAmplitude() {
    return 0;
}

/**< C style functions to expose entry point for the shared library */
	extern "C" {
		OMX_ERRORTYPE AlsaSourceInit(OMX_IN OMX_HANDLETYPE pHandle)
		{
			OMX_ERRORTYPE ret = OMX_ErrorNone;
			AlsaSource *obj = NULL;
			ComponentBase *base = NULL;

			obj = FSL_NEW(AlsaSource, ());
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
