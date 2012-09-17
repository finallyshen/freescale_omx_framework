/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>
#include "FslParser.h"

#define PLATFORM_ARM11_ELINUX
#define BAILWITHERROR(v) \
	{ \
		err = (v); \
		goto bail; \
	}


#define MAX_USER_DATA_STRING_LENGTH 512
#define MAX_INTERLEAVE_TIME (60 * OMX_TICKS_PER_SECOND)
#define MAX_INTERLEAVE_SIZE (20 * 1024 * 1024)

static void unicode2AsciiString(OMX_S8 *s_ascii,OMX_U16 *s_wchar,OMX_S32 length)
{
	OMX_U16 temp;
	OMX_S32 count = 0;

	while(count < length)
	{
		temp = s_wchar[count];
		s_ascii[count] = (OMX_S8)temp;
		count++;
	}
	s_ascii[count] = '\0';

}

static uint8* appLocalRequestBuffer(   uint32 streamNum,
                                       uint32 *size,
                                       void ** bufContext,
                                       void * parserContext)
{
	FslParser *h;
	uint8 * buffer;
	if (!size || !bufContext || !parserContext)
		return NULL;

	h = (FslParser *)parserContext;

	buffer = h->GetEmptyBufFromList(streamNum, size, bufContext);

	//printf("track %d RequestBuffer %p\n", streamNum, buffer);

	return buffer;
}


static void appLocalReleaseBuffer(uint32 streamNum, uint8 * pBuffer, void * bufContext, void * parserContext)
{
	FslParser *h;

	//printf("track %d ReleaseBuffer %p\n", streamNum, pBuffer);

	if (!pBuffer || !bufContext || !parserContext)
		return;
	h = (FslParser *)parserContext;

	h->ReturnBufToFreeList(streamNum, bufContext);

	return;
}

static OMX_STRING GetFormatFromDecoderType(uint32 decoderType, uint32 decoderSubtype)
{
	struct TypeToFormat
	{
		OMX_U32 type;
		OMX_U32 subType;
		const char *video_format;
	};
	TypeToFormat kTypeToFormat[] =
	{
		{ VIDEO_MPEG2, 0, "X-Video-MPEG2" },
		{ VIDEO_MPEG4, 0, "X-Video-MPEG4" },
		{ VIDEO_MS_MPEG4, 0, "X-Video-MS-MPEG4" },
		{ VIDEO_H263, 0, "X-Video-H263" },
		{ VIDEO_H264, 0, "X-Video-H264" },
		{ VIDEO_MJPG, 0, "X-Video-MJPG" },
		{ VIDEO_DIVX, 0, "X-Video-DIVX" },
		{ VIDEO_XVID, 0, "X-Video-XVID" },
		{ VIDEO_WMV, VIDEO_WMV7, "X-Video-WMV7" },
		{ VIDEO_WMV, VIDEO_WMV8, "X-Video-WMV8" },
		{ VIDEO_WMV, VIDEO_WMV9, "X-Video-WMV9" },
		{ VIDEO_WMV, VIDEO_WMV9A, "X-Video-WMV9" },
		{ VIDEO_WMV, VIDEO_WVC1, "X-Video-WMV9" },
		{ VIDEO_SORENSON_H263, 0, "X-Video-SORENSON-H263" },
		{ VIDEO_FLV_SCREEN, 0, "X-Video-FLV-SCREEN" },
		{ VIDEO_ON2_VP, 0, "X-Video-ON2_VP" },
		{ VIDEO_REAL, 0, "X-Video-REAL" },
		{ VIDEO_JPEG, 0, "X-Video-JPEG" },
		{ VIDEO_SORENSON, 0, "X-Video-SORENSON" }
	};

	for (OMX_U32 i = 0; i < sizeof(kTypeToFormat) / sizeof(kTypeToFormat[0]); ++i)
	{
		if (decoderType ==  kTypeToFormat[i].type)
		{
			if (decoderType == VIDEO_WMV && decoderSubtype != kTypeToFormat[i].subType)
				continue;
			return (OMX_STRING)kTypeToFormat[i].video_format;
		}
	}

	return NULL;
}

FslParser::FslParser()
{
	trackNum = 0;
	sParserMutex = NULL;
	nVideoDataSize = 0;
	nAudioDataSize = 0;
	startTime = 0;
	endTime = 0;
	sampleFlag = 0;
	nextSampleSize = 0;

	IParser = NULL;
	hParserDll = NULL;

	fsl_osal_memset(appConext, 0, 16);


	outputBufferOps.RequestBuffer = appLocalRequestBuffer;
	outputBufferOps.ReleaseBuffer = appLocalReleaseBuffer;


	parserHandle = NULL;
	LibMgr = NULL;
	fsl_osal_memset(tracks, 0, sizeof(Track)* MAX_TRACK_COUNT);

	fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.parser.fsl.sw-based");
	role_cnt = 1;
	role[0] = (OMX_STRING)"component.parser";
}



int32 FslParser::ExportIndexTable(FslParserHandle  parserHandle)
{
	int32 err =(int32)PARSER_SUCCESS;
	FILE *fp_index = NULL;
	uint8 * buffer = NULL;
	uint32 track_index_size;
	uint32 size_written;

	fp_index = fopen(index_file_name, (const char *)"wb");
	if(NULL == fp_index)
	{
		LOG_ERROR("Fail to open index file to export index: %s\n", index_file_name);
		err = (int32)PARSER_FILE_OPEN_ERROR;
		goto bail;
	}


	err =  IParser->exportIndex( parserHandle,NULL, &track_index_size);
	size_written = fwrite(&track_index_size, sizeof(uint32), 1, fp_index);
	if(0 == size_written)
	{
		err = (int32)PARSER_WRITE_ERROR;
		goto bail;
	}

	if (0 != track_index_size)
	{
		buffer= (uint8 *) FSL_MALLOC(track_index_size);
		if(NULL == buffer)
		{
			err = (int32)PARSER_INSUFFICIENT_MEMORY;
			goto bail;
		}

		err =  IParser->exportIndex( parserHandle, buffer, &track_index_size);
		if(PARSER_SUCCESS != err)
		{
			goto bail;
		}

		size_written = fwrite(buffer, track_index_size, 1, fp_index);
		if(0 == size_written)
		{
			err = (int32)PARSER_WRITE_ERROR;
			goto bail;
		}
		if (buffer)
		{
			FSL_FREE(buffer);
			buffer = NULL;
		}
	}

bail:
	if(buffer)
	{
		FSL_FREE(buffer);
		buffer = NULL;
	}

	if(fp_index)
	{
		fclose(fp_index);
		fp_index = NULL;
	}

	return err;
}

int32 FslParser::ImportIndexTable(FslParserHandle  parserHandle)
{
	int32 err =(int32)PARSER_SUCCESS;
	FILE *fp_index = NULL;
	uint8 * buffer = NULL;
	uint32 buffer_size = 0;

	LOG_DEBUG("%s,%d.\n",__FUNCTION__,__LINE__);
	fp_index = fopen(index_file_name, (const char *)"rb");
	if(NULL == fp_index)
	{
		LOG_ERROR("fail to open index data file %s.\n",index_file_name);
		err = (int32)PARSER_FILE_OPEN_ERROR;
		goto bail;
	}

	if(0 == fread(&buffer_size, sizeof(uint32), 1, fp_index))
	{
		err = (int32)PARSER_READ_ERROR;
		goto bail;
	}

	if (0 != buffer_size)
	{
		buffer= (uint8 *) FSL_MALLOC(buffer_size);
		if(NULL == buffer)
		{
			err = (int32)PARSER_INSUFFICIENT_MEMORY;
			goto bail;
		}

		if(0 == fread(buffer, buffer_size, 1, fp_index))
		{
			err = (int32)PARSER_READ_ERROR;
			goto bail;
		}

		err =  IParser->importIndex( parserHandle,buffer, buffer_size);
		if(PARSER_SUCCESS != err)
		{
			goto bail;
		}
		if(buffer)
		{
			FSL_FREE(buffer);
			buffer = NULL;
		}

	}

bail:
	if(buffer)
	{
		FSL_FREE(buffer);
		buffer = NULL;
	}

	if(fp_index)
	{
		fclose(fp_index);
		fp_index = NULL;
	}

	return err;
}


void FslParser::SetVideoCodecType(
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef,
    uint32 video_num)
{
	Track *track = &tracks[video_num];
	uint32 decoderType = track->decoderType;
	uint32 decoderSubtype = track->decoderSubtype;

	switch (decoderType)
	{
	case VIDEO_MPEG4:
		pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
		LOG_DEBUG ("MPEG-4 video\n");
		break;
	case VIDEO_MPEG2:
		pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG2;
		LOG_DEBUG ("MPEG-2 video\n");
		break;

	case VIDEO_H263:
		pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
		LOG_DEBUG("H.263 video\n");
		break;

	case VIDEO_H264:
		LOG_DEBUG("H.264 video\n");
		pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
		break;

	case VIDEO_MJPG:
		LOG_DEBUG("Motion JPEG video\n");
		if(VIDEO_MJPEG_2000==decoderSubtype || VIDEO_MJPEG_FORMAT_B==decoderSubtype)
		{
			LOG_ERROR("Not supported subtype of MJPEG: %d\n", decoderSubtype);
		}
		else
			pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingMJPEG;
		break;

	case VIDEO_XVID:
		LOG_DEBUG("XVID video\n");
		pPortDef->format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingXVID;
		break;

	case VIDEO_DIVX:
		switch (decoderSubtype)
		{
		case VIDEO_DIVX3:
			LOG_DEBUG ("DIVX video version 3\n");
			pPortDef->format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingDIV3;
			break;

		case VIDEO_DIVX4:
			LOG_DEBUG ("DIVX video version 4\n");
			pPortDef->format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingDIV4;
			break;

		case VIDEO_DIVX5_6:
			LOG_DEBUG ("DIVX video version 5/6\n");
			pPortDef->format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingDIVX;
			break;
		}
		break;


	case VIDEO_MS_MPEG4:
		switch(decoderSubtype)
		{
		case VIDEO_MS_MPEG4_V2:
			LOG_WARNING ("FIXME:MS MPEG-4 video V2,OpenMax type not set\n");
			break;

		case VIDEO_MS_MPEG4_V3:
			LOG_WARNING ("FIXME:MS MPEG-4 video V3,OpenMax type not set\n");
			break;
		}
		break;

	case VIDEO_WMV:
		pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
		switch(decoderSubtype)
		{
		case VIDEO_WMV7:
			LOG_DEBUG("WMV7 video\n");
			track->video_type.wmv_type.eFormat =  OMX_VIDEO_WMVFormat7;
			break;

		case VIDEO_WMV8:
			LOG_DEBUG("WMV8 video\n");
			track->video_type.wmv_type.eFormat =  OMX_VIDEO_WMVFormat8;
			break;

		case VIDEO_WMV9:
			LOG_DEBUG("WMV9 video\n");
			track->video_type.wmv_type.eFormat =  OMX_VIDEO_WMVFormat9;
			pPortDef->format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingWMV9;
			break;

		case VIDEO_WMV9A:
			track->video_type.wmv_type.eFormat =  (OMX_VIDEO_WMVFORMATTYPE )OMX_VIDEO_WMVFormat9a;
			pPortDef->format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingWMV9;
			break;
		case VIDEO_WVC1:
			track->video_type.wmv_type.eFormat =  (OMX_VIDEO_WMVFORMATTYPE )OMX_VIDEO_WMVFormatWVC1;
			pPortDef->format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingWMV9;
			LOG_DEBUG("VC-1 video\n");
			break;
		}
		break;
	case VIDEO_REAL:
		pPortDef->format.video.eCompressionFormat = OMX_VIDEO_CodingRV;
		switch ( decoderSubtype )
		{
		case REAL_VIDEO_RV10 :
			LOG_DEBUG("REAL VIDEO RV10\n");
			break;
		case REAL_VIDEO_RV20 :
			track->video_type.rv_param.eFormat =  OMX_VIDEO_RVFormatG2;
			LOG_DEBUG("REAL VIDEO RV20\n");
			break;
		case REAL_VIDEO_RV30 :
			track->video_type.rv_param.eFormat =  OMX_VIDEO_RVFormat8;
			LOG_DEBUG("REAL VIDEO RV30\n");
			break;
		case REAL_VIDEO_RV40 :
			track->video_type.rv_param.eFormat =  OMX_VIDEO_RVFormat9;
			LOG_DEBUG("REAL VIDEO RV40\n");
			break;
		default :
			break;
		}
		break;
	case VIDEO_SORENSON:
		//printf("%s,%d, VIDEO_SORENSON.\n",__FUNCTION__,__LINE__);
		switch ( decoderSubtype )
		{
		case VIDEO_SORENSON_SPARK:
			LOG_DEBUG("%s,%d, VIDEO_SORENSON_SPARK.\n",__FUNCTION__,__LINE__);
			//printf("%s,%d, VIDEO_SORENSON_SPARK.\n",__FUNCTION__,__LINE__);
			pPortDef->format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_SORENSON263;
			break;
		case VIDEO_SVQ1 :
			LOG_DEBUG("%s,%d, VIDEO_SORENSON_SVQ1.\n",__FUNCTION__,__LINE__);
			break;
		case VIDEO_SVQ3 :
			LOG_DEBUG("%s,%d, VIDEO_SORENSON_SVQ3.\n",__FUNCTION__,__LINE__);
			break;
		default :
			break;
		}
		break;
	case VIDEO_SORENSON_H263:
		//printf("%s,%d, VIDEO_SORENSON.\n",__FUNCTION__,__LINE__);
		pPortDef->format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_SORENSON263;
		break;
	case VIDEO_ON2_VP:
		switch (decoderSubtype)
		{
		case VIDEO_VP8:
			LOG_DEBUG("%s,%d, VIDEO_ON2_VP, subtype: VP8 .\n",__FUNCTION__,__LINE__);
			pPortDef->format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_VP8;
			break;
		default:
			LOG_DEBUG("%s,%d, VIDEO_ON2_VP, subtype: %d is not supported .\n",__FUNCTION__,__LINE__,decoderSubtype);
			break;
		}
		break;
	default:
		LOG_DEBUG("Unknown video codec type\n ");
	}

	sVideoPortFormat.eCompressionFormat = pPortDef->format.video.eCompressionFormat;
	if (pPortDef->format.video.eCompressionFormat != OMX_VIDEO_CodingAutoDetect)
		bHasVideoTrack = OMX_TRUE;

	return;
}
void FslParser::SetAudioCodecType(
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef,
    uint32 audio_num)
{

	Track * track = &tracks[audio_num];
	uint32 decoderType = track->decoderType;
	uint32 decoderSubtype = track->decoderSubtype;

	switch (decoderType)
	{
	case AUDIO_MP3:
		LOG_DEBUG ("MP3 audio\n");
		pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingMP3;
		break;

	case AUDIO_VORBIS:
		LOG_DEBUG ("VORBIS audio\n");
		pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingVORBIS;
		OMX_INIT_STRUCT(&track->audio_type.vorbis_type, OMX_AUDIO_PARAM_VORBISTYPE);
		track->audio_type.vorbis_type.nPortIndex = AUDIO_OUTPUT_PORT;
		track->audio_type.vorbis_type.nChannels = track->numChannels;
		track->audio_type.vorbis_type.nSampleRate = track->sampleRate;
		break;

	case AUDIO_AAC:
		LOG_DEBUG ("AAC audio\n");
		pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingAAC;
		OMX_INIT_STRUCT(&track->audio_type.aac_type, OMX_AUDIO_PARAM_AACPROFILETYPE);
		track->audio_type.aac_type.nPortIndex = AUDIO_OUTPUT_PORT;
		track->audio_type.aac_type.nChannels = track->numChannels;
		track->audio_type.aac_type.nSampleRate = track->sampleRate;
		track->audio_type.aac_type.eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;

		break;

	case AUDIO_MPEG2_AAC:
		pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingAAC;
		LOG_DEBUG ("MPEG-2 AAC audio\n");
		break;

	case AUDIO_AC3:
		pPortDef->format.audio.eEncoding = (OMX_AUDIO_CODINGTYPE )OMX_AUDIO_CodingAC3;
		OMX_INIT_STRUCT(&track->audio_type.ac3_type, OMX_AUDIO_PARAM_AC3TYPE);
		track->audio_type.ac3_type.nPortIndex = AUDIO_OUTPUT_PORT;
		track->audio_type.ac3_type.nBitRate = track->bitRate;
		LOG_DEBUG("AC3 bit rate: %d\n", track->audio_type.ac3_type.nBitRate);
		LOG_DEBUG("AC3 audio\n");
		break;

	case AUDIO_WMA:
		pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingWMA;
		OMX_INIT_STRUCT(&track->audio_type.wma_type, OMX_AUDIO_PARAM_WMATYPE);
		OMX_INIT_STRUCT(&track->wma_type_ext, OMX_AUDIO_PARAM_WMATYPE_EXT);
		track->audio_type.wma_type.nPortIndex = AUDIO_OUTPUT_PORT;
		track->wma_type_ext.nPortIndex = AUDIO_OUTPUT_PORT;
		track->audio_type.wma_type.nChannels = track->numChannels;
		track->audio_type.wma_type.nBitRate = track->bitRate;
		track->audio_type.wma_type.nSamplingRate = track->sampleRate;
		track->audio_type.wma_type.nBlockAlign = track->audioBlockAlign;
		track->audio_type.wma_type.nSuperBlockAlign = track->audioBlockAlign;
		track->wma_type_ext.nBitsPerSample = track->bitsPerSample;

		switch (decoderSubtype)
		{
		case AUDIO_WMA1:
			LOG_DEBUG("WMA1 audio\n");
			track->audio_type.wma_type.eFormat = OMX_AUDIO_WMAFormat7;
			break;
		case AUDIO_WMA2:
			LOG_DEBUG("WMA2 audio\n");
			track->audio_type.wma_type.eFormat = OMX_AUDIO_WMAFormat8;
			break;
		case AUDIO_WMA3:
			LOG_DEBUG("WMA3 audio\n");
			track->audio_type.wma_type.eFormat = OMX_AUDIO_WMAFormat9;
			break;
		}

		break;
	case AUDIO_AMR:
		pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingAMR;
		OMX_INIT_STRUCT(&track->audio_type.amr_type, OMX_AUDIO_PARAM_AMRTYPE);
		track->audio_type.amr_type.nPortIndex = AUDIO_OUTPUT_PORT;
		track->audio_type.amr_type.nChannels = track->numChannels;
		track->audio_type.amr_type.nBitRate = track->bitRate;
		track->audio_type.amr_type.eAMRDTXMode = OMX_AUDIO_AMRDTXModeOff;

		switch (decoderSubtype)
		{
		case AUDIO_AMR_NB:
			track->audio_type.amr_type.eAMRBandMode = OMX_AUDIO_AMRBandModeNB0;
			track->audio_type.amr_type.eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
			LOG_DEBUG("AMR-NB\n");
			break;

		case AUDIO_AMR_WB:
			track->audio_type.amr_type.eAMRBandMode = OMX_AUDIO_AMRBandModeWB0;
			track->audio_type.amr_type.eAMRFrameFormat = OMX_AUDIO_AMRFrameFormatFSF;
			LOG_DEBUG("AMR-WB\n");
			break;

		case AUDIO_AMR_WB_PLUS:
			LOG_DEBUG("AMR-WB+\n");
			break;
		}
		break;

	case AUDIO_PCM:
		pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingPCM;
		OMX_INIT_STRUCT(&track->audio_type.pcm_type, OMX_AUDIO_PARAM_PCMMODETYPE);
		track->audio_type.pcm_type.nPortIndex = AUDIO_OUTPUT_PORT;
		track->audio_type.pcm_type.nChannels = track->numChannels;
		track->audio_type.pcm_type.nSamplingRate = track->sampleRate;
		track->audio_type.pcm_type.bInterleaved = OMX_TRUE;
		track->audio_type.pcm_type.nBitPerSample = track->bitsPerSample;

		switch (decoderSubtype)
		{
		case AUDIO_PCM_U8:
			track->audio_type.pcm_type.eEndian = OMX_EndianLittle;
			track->audio_type.pcm_type.nBitPerSample = 8;
			LOG_DEBUG("PCM, 8 bits per sample\n");
			break;

		case AUDIO_PCM_S16LE:
			track->audio_type.pcm_type.eEndian = OMX_EndianLittle;
			track->audio_type.pcm_type.nBitPerSample = 16;
			LOG_DEBUG("PCM, little-endian, 16 bits per sample\n");
			break;

		case AUDIO_PCM_S24LE:
			track->audio_type.pcm_type.eEndian = OMX_EndianLittle;
			track->audio_type.pcm_type.nBitPerSample = 24;
			LOG_DEBUG("PCM, little-endian, 24 bits per sample\n");
			break;

		case AUDIO_PCM_S32LE:
			track->audio_type.pcm_type.eEndian = OMX_EndianLittle;
			track->audio_type.pcm_type.nBitPerSample = 32;
			LOG_DEBUG("PCM, little-endian, 32 bits per sample\n");
			break;

		case AUDIO_PCM_S16BE:
			track->audio_type.pcm_type.eEndian = OMX_EndianBig;
			track->audio_type.pcm_type.nBitPerSample = 16;
			LOG_DEBUG("PCM, big-endian, 16 bits per sample\n");
			break;

		case AUDIO_PCM_S24BE:
			track->audio_type.pcm_type.eEndian = OMX_EndianBig;
			track->audio_type.pcm_type.nBitPerSample = 24;
			LOG_DEBUG("PCM, big-endian, 24 bits per sample\n");
			break;

		case AUDIO_PCM_S32BE:
			track->audio_type.pcm_type.eEndian = OMX_EndianBig;
			track->audio_type.pcm_type.nBitPerSample = 32;
			LOG_DEBUG("PCM, big-endian, 32 bits per sample\n");
			break;

		}
		break;
	case AUDIO_REAL:

		switch ( decoderSubtype )
		{
		case REAL_AUDIO_SIPR :
			pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingRA;
			OMX_INIT_STRUCT(&track->audio_type.ra_param,  OMX_AUDIO_PARAM_RATYPE);
			track->audio_type.ra_param.nPortIndex = AUDIO_OUTPUT_PORT;
			track->audio_type.ra_param.nChannels = track->numChannels;
			track->audio_type.ra_param.nSamplingRate = track->sampleRate;
			track->audio_type.ra_param.nBitsPerFrame = track->bits_per_frame;
#if 0
			track->audio_type.ra_param.nSamplePerFrame =      flavor_para[flavor_index].nSamples;
			track->audio_type.ra_param.nNumRegions =          flavor_para[flavor_index].nRegions;
			track->audio_type.ra_param.nCouplingQuantBits =   flavor_para[flavor_index].cplQbits;
			track->audio_type.ra_param.nCouplingStartRegion = flavor_para[flavor_index].cplStart;
#endif
			track->audio_type.ra_param.eFormat = OMX_AUDIO_RA8;

			LOG_DEBUG("REAL AUDIO SIPR.\n");
			break;
		case REAL_AUDIO_COOK :
			LOG_DEBUG("REAL AUDIO COOK.\n");
			pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingRA;
			OMX_INIT_STRUCT(&track->audio_type.ra_param,  OMX_AUDIO_PARAM_RATYPE);
			track->audio_type.ra_param.nPortIndex = AUDIO_OUTPUT_PORT;
			track->audio_type.ra_param.nChannels = track->numChannels;
			track->audio_type.ra_param.nSamplingRate = track->sampleRate;
			track->audio_type.ra_param.nBitsPerFrame = track->bits_per_frame;
#if 0
			track->audio_type.ra_param.nSamplePerFrame =      flavor_para[flavor_index].nSamples;
			track->audio_type.ra_param.nNumRegions =          flavor_para[flavor_index].nRegions;
			track->audio_type.ra_param.nCouplingQuantBits =   flavor_para[flavor_index].cplQbits;
			track->audio_type.ra_param.nCouplingStartRegion = flavor_para[flavor_index].cplStart;
#endif
			track->audio_type.ra_param.eFormat = OMX_AUDIO_RA8;
			break;
		case REAL_AUDIO_ATRC:
			LOG_DEBUG("REAL AUDIO ATRC.\n");
			pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingRA;
			OMX_INIT_STRUCT(&track->audio_type.ra_param,  OMX_AUDIO_PARAM_RATYPE);
			track->audio_type.ra_param.nPortIndex = AUDIO_OUTPUT_PORT;
			track->audio_type.ra_param.nChannels = track->numChannels;
			track->audio_type.ra_param.nSamplingRate = track->sampleRate;
			track->audio_type.ra_param.nBitsPerFrame = track->bits_per_frame;
#if 0
			track->audio_type.ra_param.nSamplePerFrame =      flavor_para[flavor_index].nSamples;
			track->audio_type.ra_param.nNumRegions =          flavor_para[flavor_index].nRegions;
			track->audio_type.ra_param.nCouplingQuantBits =   flavor_para[flavor_index].cplQbits;
			track->audio_type.ra_param.nCouplingStartRegion = flavor_para[flavor_index].cplStart;
#endif
			track->audio_type.ra_param.eFormat = OMX_AUDIO_RA10_VOICE;

			break;
		case REAL_AUDIO_RAAC:
			LOG_DEBUG("REAL AUDIO RAAC.\n");
			pPortDef->format.audio.eEncoding = OMX_AUDIO_CodingAAC;
			OMX_INIT_STRUCT(&track->audio_type.aac_type, OMX_AUDIO_PARAM_AACPROFILETYPE);
			track->audio_type.aac_type.nPortIndex = AUDIO_OUTPUT_PORT;
			track->audio_type.aac_type.nChannels = track->numChannels;
			track->audio_type.aac_type.nSampleRate = track->sampleRate;
			track->audio_type.aac_type.eAACStreamFormat = OMX_AUDIO_AACStreamFormatRAW;
			break;
		default :
			break;
		}
		break;
	default:
		LOG_DEBUG("Unknown audio codec type\n ");
	}
	sAudioPortFormat.eEncoding = pPortDef->format.audio.eEncoding;
	if (pPortDef->format.audio.eEncoding != OMX_AUDIO_CodingAutoDetect)
		bHasAudioTrack = OMX_TRUE;

	return;
}

#if 0
static void displayLanguage(LanguageCode lanCode)
{
	switch (lanCode)
	{
	case NONE_LANGUAGE:
		LOG_DEBUG("Lanuage is ignored\n");
		break;

	case ARABIC:
		LOG_DEBUG ("ARABIC\n");
		break;

	case BULGARIAN:
		LOG_DEBUG ("BULGARIAN\n");
		break;

	case CATALAN:
		LOG_DEBUG("CATALAN\n");
		break;

	case CHINESE:
		LOG_DEBUG ("CHINESE\n");
		break;

	case CZECH:
		LOG_DEBUG ("CZECH\n");
		break;

	case DANISH:
		LOG_DEBUG("DANISH\n");
		break;

	case GERMAN:
		LOG_DEBUG ("GERMAN\n");
		break;

	case GREEK:
		LOG_DEBUG ("GREEK\n");
		break;

	case ENGLISH:
		LOG_DEBUG("ENGLISH\n");
		break;

	case SPANISH:
		LOG_DEBUG ("SPANISH\n");
		break;

	case FINNISH:
		LOG_DEBUG ("FINNISH\n");
		break;

	case FRENCH:
		LOG_DEBUG("FRENCH\n");
		break;

	case HEBREW:
		LOG_DEBUG ("HEBREW\n");
		break;

	case HUNGARIAN:
		LOG_DEBUG ("HUNGARIAN\n");
		break;

	case ICELANDIC:
		LOG_DEBUG("ICELANDIC\n");
		break;

	case ITALIAN:
		LOG_DEBUG ("ITALIAN\n");
		break;

	case JAPANESE:
		LOG_DEBUG ("JAPANESE\n");
		break;

	case KOREAN:
		LOG_DEBUG("KOREAN\n");
		break;

	case DUTCH:
		LOG_DEBUG ("DUTCH\n");
		break;

	case NORWEGIAN:
		LOG_DEBUG ("NORWEGIAN\n");
		break;

	case POLISH:
		LOG_DEBUG("POLISH\n");
		break;

	case PORTUGUESE:
		LOG_DEBUG ("PORTUGUESE\n");
		break;

	case RHAETO_ROMANIC:
		LOG_DEBUG ("RHAETO_ROMANIC\n");
		break;

	case ROMANIAN:
		LOG_DEBUG("ROMANIAN\n");
		break;

	case RUSSIAN:
		LOG_DEBUG ("RUSSIAN\n");
		break;

	case SERBO_CROATIAN:
		LOG_DEBUG ("SERBO_CROATIAN\n");
		break;

	case SLOVAK:
		LOG_DEBUG("SOLVAK\n");
		break;

	case ALBANIAN:
		LOG_DEBUG ("ALBANIAN\n");
		break;

	case SWEDISH:
		LOG_DEBUG ("SWEDISH\n");
		break;

	case THAI:
		LOG_DEBUG("THAI\n");
		break;

	case TURKISH:
		LOG_DEBUG ("TURKISH\n");
		break;

	case URDU:
		LOG_DEBUG ("URDU\n");
		break;

	case BAHASA:
		LOG_DEBUG("BAHASA\n");
		break;

	default:
		LOG_DEBUG("Unknown lanuage\n ");
	}
	return;
}
#endif
#define CLEAR_AND_RETURN_ERROR() \
{\
    InstanceDeInit(); \
    ComponentBase::SendEvent(OMX_EventError,OMX_ErrorStreamCorrupt,VIDEO_OUTPUT_PORT,NULL); \
    return OMX_ErrorStreamCorrupt;\
}

OMX_ERRORTYPE FslParser::InitCoreParser()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	int32 err = (int32)PARSER_SUCCESS;
	LOG_DEBUG("%s \n", IParser->getVersionInfo());

	err = IParser->createParser((bool)isLiveSource,
	                            &fileOps,
	                            &memOps,
	                            &outputBufferOps,
	                            (void *)this,
	                            &parserHandle);
	if(PARSER_SUCCESS !=  err)
	{
		LOG_ERROR("fail to create the parser: %d\n", err);
		CLEAR_AND_RETURN_ERROR();

	}

	// ENGR00137229 MPEG2Dec: Take long time to load 1 MPEG2 clip
	// fixing for this issue can only work when parser is in file mode (check max interleave length of A/V data)
	OMX_BOOL isMpg2Parser = (fsl_osal_strncmp((char *)sCompRole.cRole, "parser.mpg2", strlen("parser.mpg2")) == 0) ? OMX_TRUE : OMX_FALSE;

	if(isStreamingSource == OMX_TRUE || isLiveSource == OMX_TRUE || isMpg2Parser == OMX_TRUE)
		read_mode = PARSER_READ_MODE_FILE_BASED;
	else
		read_mode = PARSER_READ_MODE_TRACK_BASED;

	err = IParser->setReadMode(parserHandle, read_mode);
	if(PARSER_SUCCESS != err)
	{
		LOG_WARNING("fail to set read mode to track mode\n");
		read_mode = PARSER_READ_MODE_FILE_BASED;
		err = IParser->setReadMode(parserHandle, read_mode);
		if(PARSER_SUCCESS != err)
		{
			LOG_ERROR("fail to set read mode to file mode\n");
			CLEAR_AND_RETURN_ERROR();
		}
	}
	if ((NULL == IParser->getNextSample && PARSER_READ_MODE_TRACK_BASED == read_mode)
	        || (NULL == IParser->getFileNextSample && PARSER_READ_MODE_FILE_BASED == read_mode))
		CLEAR_AND_RETURN_ERROR();


	if (pMediaName == NULL)
		CLEAR_AND_RETURN_ERROR();

	Parser::MakeIndexDumpFileName((char *)pMediaName);

	err = IParser->getNumTracks(parserHandle, &numTracks);
	if(PARSER_SUCCESS !=  err)
		CLEAR_AND_RETURN_ERROR();

	LOG_DEBUG("Number of tracks: %d\n", numTracks);
	if(numTracks > MAX_TRACK_COUNT)
		numTracks = MAX_TRACK_COUNT;

	if(IParser->initializeIndex)
	{
		FILE *tmp = fopen(index_file_name,"rb") ;
		bool is_export_index_table = FALSE;
		if (!tmp)
			is_export_index_table = TRUE;
		else
			fclose(tmp);

		if(is_export_index_table)
		{
			err = IParser->initializeIndex(parserHandle);
			if(PARSER_SUCCESS !=  err)
			{
				LOG_ERROR("fail to initialize index. Index may be missing or corrupted. Err code: %d\n", err);
			}

			if(PARSER_SUCCESS ==  err)
			{
				err = ExportIndexTable(parserHandle);
				if(PARSER_SUCCESS !=  err)
				{
					LOG_WARNING("fail to export index\n");
				}
			}
		}
		else
		{
			err = ImportIndexTable(parserHandle);
			if(PARSER_SUCCESS !=  err)
			{
				LOG_ERROR("fail to Import index\n");
				err = IParser->initializeIndex(parserHandle);
				if(PARSER_SUCCESS !=  err)
				{
					LOG_ERROR("fail to initialize index. Index may be missing or corrupted. Err code: %d\n", err);
				}

				if(PARSER_SUCCESS ==  err)
				{
					err = ExportIndexTable(parserHandle);
					if(PARSER_SUCCESS !=  err)
					{
						LOG_WARNING("fail to export index\n");
					}
				}

			}
		}
	}
	err = IParser->isSeekable(parserHandle,(bool *)&bSeekable);
	if(PARSER_SUCCESS !=  err)
		CLEAR_AND_RETURN_ERROR();
	//printf("%s, is seekable ? %d\n",__FUNCTION__,bSeekable);

	err = IParser->getMovieDuration(parserHandle, (uint64 *)&usDuration);
	if(PARSER_SUCCESS !=  err)
		CLEAR_AND_RETURN_ERROR();

	char *role = NULL;
	role = (char *)sCompRole.cRole;
	struct KeyMap
	{
		char *tag;
		UserDataID key;
	};
	const KeyMap kKeyMap[] =
	{
		{ (char *)"title", USER_DATA_TITLE },
		{ (char *)"language", USER_DATA_LANGUAGE },
		{ (char *)"genre", USER_DATA_GENRE },
		{ (char *)"artist", USER_DATA_ARTIST },
		{ (char *)"copyritht", USER_DATA_COPYRIGHT },
		{ (char *)"comments", USER_DATA_COMMENTS },
		{ (char *)"year", USER_DATA_CREATION_DATE },
		{ (char *)"rating", USER_DATA_RATING },
		{ (char *)"album", USER_DATA_ALBUM },
		{ (char *)"composer", USER_DATA_COMPOSER },
		{ (char *)"writer", USER_DATA_SONGWRITER },
		{ (char *)"tracknumber", USER_DATA_TRACKNUMBER },
		{ (char *)"location", USER_DATA_LOCATION},
	};

	OMX_U32 kNumMapEntries = sizeof(kKeyMap) / sizeof(kKeyMap[0]);

	if (IParser->getMetaData)
	{
		uint8 *metaData = NULL;
		uint32 metaDataSize = 0;
		UserDataFormat userDataFormat;

		for (OMX_U32 i = 0; i < kNumMapEntries; ++i)
		{
			userDataFormat = USER_DATA_FORMAT_UTF8;
			IParser->getMetaData(parserHandle, kKeyMap[i].key, &userDataFormat, &metaData, \
			                     &metaDataSize);

			LOG_DEBUG("FslParser Key: %s\t Value: %s\n", kKeyMap[i].tag, (OMX_STRING)metaData);
			if(metaData && metaDataSize && USER_DATA_FORMAT_UTF8 == userDataFormat)
			{
				if(metaDataSize > MAX_USER_DATA_STRING_LENGTH)
					metaDataSize = MAX_USER_DATA_STRING_LENGTH;

				SetMetadata(kKeyMap[i].tag, (OMX_STRING)metaData, (OMX_U32)metaDataSize);
			}

		}

		userDataFormat = USER_DATA_FORMAT_JPEG;
		IParser->getMetaData(parserHandle, USER_DATA_ARTWORK, &userDataFormat, &metaData, \
		                     &metaDataSize);

		if(metaData && metaDataSize && USER_DATA_FORMAT_JPEG == userDataFormat)
		{
			SetMetadata((OMX_STRING)"albumart", (OMX_STRING)metaData, (OMX_U32)metaDataSize);
		}
	}
	else if (IParser->getUserData)
	{
		uint8 asciiString[MAX_USER_DATA_STRING_LENGTH + 1];

		for (OMX_U32 i = 0; i < kNumMapEntries; ++i)
		{
			uint16 *userData = NULL;
			uint32 userDataSize = 0;
			IParser->getUserData(parserHandle, kKeyMap[i].key, &userData, &userDataSize);
			if(userData && userDataSize)
			{
				if(userDataSize > MAX_USER_DATA_STRING_LENGTH)
					userDataSize = MAX_USER_DATA_STRING_LENGTH;

				unicode2AsciiString((OMX_S8*)asciiString, userData, userDataSize);

				//SetMetadata(kKeyMap[i].tag, (OMX_STRING)asciiString, (OMX_U32)userDataSize);
			}

		}
	}

	return ret;
}


int32 FslParser::CreateParserInterface()
{
	int32 err = PARSER_SUCCESS;
	char *role = NULL;
	char * source_url = (char *)pMediaName;
	char parserLibName[255];
	char platform[50] = {0};
	char version[10]= {0};
	char fileNameExtension[10] = {0};

	tFslParserQueryInterface  myQueryInterface;


	role = (char *)sCompRole.cRole;

	if(!strncmp(role, "parser.rmvb", strlen("parser.rmvb")))
		strcpy(version, ".3.0");
	if(!strncmp(role, "parser.avi", strlen("parser.avi")))
		strcpy(version, ".3.0");
	if(!strncmp(role, "parser.mkv", strlen("parser.mkv")))
		strcpy(version, ".3.0");
	if(!strncmp(role, "parser.mp4", strlen("parser.mp4")))
		strcpy(version, ".3.0");
	if(!strncmp(role, "parser.flv", strlen("parser.flv")))
		strcpy(version, ".3.0");
	if(!strncmp(role, "parser.asf", strlen("parser.asf")))
		strcpy(version, ".3.0");
	if(!strncmp(role, "parser.mpg2", strlen("parser.mpg2")))
		strcpy(version, ".3.0");
	if(!strncmp(role, "parser.ogg", strlen("parser.ogg")))
		strcpy(version, ".3.0");


	LOG_DEBUG("Platform:\t");

#ifdef PLATFORM_ARM11_ELINUX
	LOG_DEBUG("ARM11 eLinux\n");
	strcpy(platform, "arm11_elinux");
	strcpy(fileNameExtension, ".so");

#elif defined PLATFORM_ARM9_ELINUX
	LOG_DEBUG("ARM9 eLinux\n");
	strcpy(platform, "arm9_elinux");
	strcpy(fileNameExtension, ".so");

#elif defined PLATFORM_X86_UNIX
	LOG_DEBUG("X86 Linux/UNIX\n");
	strcpy(platform, "x86_elinux");
	strcpy(fileNameExtension, ".so");

#elif defined PLATFORM_ARM11_WINCE
	LOG_DEBUG("ARM11 WINCE\n");
	strcpy(platform, "arm11_wince");
	strcpy(fileNameExtension, ".dll");

#else
	LOG_DEBUG("Unknown\n");
	err = -1;
	return err;
#endif

	strcpy(parserLibName, "lib_");

	if (!strncmp(role, "parser.mp4", strlen("parser.mp4")))
		strcat(parserLibName, "mp4");

	else if(!strncmp(role, "parser.mpg2", strlen("parser.mpg2")))
		strcat(parserLibName, "mpg2");

	else if(!strncmp(role, "parser.rmvb", strlen("parser.rmvb")))
		strcat(parserLibName, "rm");

	else if(!strncmp(role, "parser.asf", strlen("parser.asf")))
		strcat(parserLibName, "asf");

	else if(!strncmp(role, "parser.avi", strlen("parser.avi")))
		strcat(parserLibName, "avi");

	else if(!strncmp(role, "parser.mkv", strlen("parser.mkv")))
		strcat(parserLibName, "mkv");

	else if(!strncmp(role, "parser.flv", strlen("parser.flv")))
		strcat(parserLibName, "flv");

	else if(!strncmp(role, "parser.ogg", strlen("parser.ogg")))
		strcat(parserLibName, "ogg");

	else
	{
		LOG_ERROR("Err: unknown media format!\n");
		BAILWITHERROR(PARSER_ERR_INVALID_MEDIA)
	}

	strcat(parserLibName, "_parser_");
	strcat(parserLibName, platform);
	strcat(parserLibName, version);
	strcat(parserLibName, fileNameExtension);

	LOG_DEBUG("Parser library name: %s\n", parserLibName);

	hParserDll = LibMgr->load((fsl_osal_char *)parserLibName);
	if(NULL == hParserDll)
	{
		LOG_ERROR("Fail to open parser DLL for %s\n", role);
		BAILWITHERROR(PARSER_FILE_OPEN_ERROR)

	}

	/* query interface */
	IParser = (FslParserInterface *)FSL_MALLOC(sizeof(FslParserInterface));
	if(!IParser)
		BAILWITHERROR(PARSER_INSUFFICIENT_MEMORY)
		fsl_osal_memset(IParser,0 ,sizeof(FslParserInterface));

	myQueryInterface = (tFslParserQueryInterface)LibMgr->getSymbol(hParserDll, (fsl_osal_char *)"FslParserQueryInterface");
	if(NULL == myQueryInterface)
	{
		LOG_ERROR("Fail to query parser interface\n");
		BAILWITHERROR(PARSER_ERR_INVALID_API)
	}

	/* create & delete */
	err = myQueryInterface(PARSER_API_GET_VERSION_INFO, (void **)&IParser->getVersionInfo);
	if(err) goto bail;
	if(!IParser->getVersionInfo)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		err = myQueryInterface(PARSER_API_CREATE_PARSER, (void **)&IParser->createParser);
	if(err) goto bail;
	if(!IParser->createParser)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		err = myQueryInterface(PARSER_API_DELETE_PARSER, (void **)&IParser->deleteParser);
	if(err) goto bail;
	if(!IParser->deleteParser)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		/* index export/import */
		err = myQueryInterface(PARSER_API_INITIALIZE_INDEX, (void **)&IParser->initializeIndex);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_EXPORT_INDEX, (void **)&IParser->exportIndex);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_IMPORT_INDEX, (void **)&IParser->importIndex);
	if(err) goto bail;
	if(IParser->initializeIndex || IParser->exportIndex || IParser->importIndex)
	{
		if((NULL == IParser->initializeIndex)
		        ||(NULL == IParser->exportIndex)
		        || (NULL == IParser->importIndex))
		{
			LOG_ERROR("Invalid API for index initialize/export/import. Must implement ALL or NONE of the three API.\n");
			BAILWITHERROR(PARSER_ERR_INVALID_API)
		}
	}

	/* movie properties */
	err = myQueryInterface(PARSER_API_IS_MOVIE_SEEKABLE, (void **)&IParser->isSeekable);
	if(err) goto bail;
	if(!IParser->isSeekable)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		err = myQueryInterface(PARSER_API_GET_MOVIE_DURATION, (void **)&IParser->getMovieDuration);
	if(err) goto bail;
	if(!IParser->getMovieDuration)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		err = myQueryInterface(PARSER_API_GET_USER_DATA, (void **)&IParser->getUserData);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_META_DATA, (void **)&IParser->getMetaData);
	if(err) goto bail;

	err = myQueryInterface(PARSER_API_GET_NUM_TRACKS, (void **)&IParser->getNumTracks);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_NUM_PROGRAMS, (void **)&IParser->getNumPrograms);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_PROGRAM_TRACKS, (void **)&IParser->getProgramTracks);
	if(err) goto bail;

	if((!IParser->getNumTracks && !IParser->getNumPrograms)
	        ||(IParser->getNumPrograms && !IParser->getProgramTracks))
	{
		LOG_ERROR("Invalid API to get tracks or programs.\n");
		BAILWITHERROR(PARSER_ERR_INVALID_API)
	}

	/* generic track properties */
	err = myQueryInterface(PARSER_API_GET_TRACK_TYPE, (void **)&IParser->getTrackType);
	if(err) goto bail;
	if(!IParser->getTrackType)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		err = myQueryInterface(PARSER_API_GET_TRACK_DURATION, (void **)&IParser->getTrackDuration);
	if(err) goto bail;
	if(!IParser->getTrackDuration)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		err = myQueryInterface(PARSER_API_GET_LANGUAGE, (void **)&IParser->getLanguage);
	if(err) goto bail;

	err = myQueryInterface(PARSER_API_GET_BITRATE, (void **)&IParser->getBitRate);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_DECODER_SPECIFIC_INFO, (void **)&IParser->getDecoderSpecificInfo);
	if(err) goto bail;

	/* video properties (not required for audio-only media */
	err = myQueryInterface(PARSER_API_GET_VIDEO_FRAME_WIDTH, (void **)&IParser->getVideoFrameWidth);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_VIDEO_FRAME_HEIGHT, (void **)&IParser->getVideoFrameHeight);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_VIDEO_FRAME_RATE, (void **)&IParser->getVideoFrameRate);
	if(err) goto bail;

	/* audio properties */
	err = myQueryInterface(PARSER_API_GET_AUDIO_NUM_CHANNELS, (void **)&IParser->getAudioNumChannels);
	if(err) goto bail;
	if(!IParser->getAudioNumChannels)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		err = myQueryInterface(PARSER_API_GET_AUDIO_SAMPLE_RATE, (void **)&IParser->getAudioSampleRate);
	if(err) goto bail;
	if(!IParser->getAudioSampleRate)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		err = myQueryInterface(PARSER_API_GET_AUDIO_BITS_PER_SAMPLE, (void **)&IParser->getAudioBitsPerSample);
	if(err) goto bail;

	err = myQueryInterface(PARSER_API_GET_AUDIO_BLOCK_ALIGN, (void **)&IParser->getAudioBlockAlign);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_AUDIO_CHANNEL_MASK, (void **)&IParser->getAudioChannelMask);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_AUDIO_BITS_PER_FRAME, (void **)&IParser->getAudioBitsPerFrame);
	if(err) goto bail;

	/* text/subtitle properties */
	err = myQueryInterface(PARSER_API_GET_TEXT_TRACK_WIDTH, (void **)&IParser->getTextTrackWidth);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_TEXT_TRACK_HEIGHT, (void **)&IParser->getTextTrackHeight);
	if(err) goto bail;

	/* sample reading, seek & trick mode */
	err = myQueryInterface(PARSER_API_GET_READ_MODE, (void **)&IParser->getReadMode);
	if(err) goto bail;
	if(!IParser->getReadMode)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		err = myQueryInterface(PARSER_API_SET_READ_MODE, (void **)&IParser->setReadMode);
	if(err) goto bail;
	if(!IParser->setReadMode)
		BAILWITHERROR(PARSER_ERR_INVALID_API)

		err = myQueryInterface(PARSER_API_ENABLE_TRACK, (void **)&IParser->enableTrack);
	if(err) goto bail;

	err = myQueryInterface(PARSER_API_GET_NEXT_SAMPLE, (void **)&IParser->getNextSample);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_NEXT_SYNC_SAMPLE, (void **)&IParser->getNextSyncSample);
	if(err) goto bail;

	err = myQueryInterface(PARSER_API_GET_FILE_NEXT_SAMPLE, (void **)&IParser->getFileNextSample);
	if(err) goto bail;
	err = myQueryInterface(PARSER_API_GET_FILE_NEXT_SYNC_SAMPLE, (void **)&IParser->getFileNextSyncSample);
	if(err) goto bail;

	if(!IParser->getNextSample && !IParser->getFileNextSample)
	{
		LOG_ERROR("ERR: Support neither track-based nor file-based reading mode\n");
		BAILWITHERROR(PARSER_ERR_INVALID_API)
	}

	if(IParser->getFileNextSample && !IParser->enableTrack)
	{
		LOG_ERROR("ERR: For file-based reading mode, need implement API to enable a track!\n");
		BAILWITHERROR(PARSER_ERR_INVALID_API)
	}

	err = myQueryInterface(PARSER_API_SEEK, (void **)&IParser->seek);
	if(err) goto bail;
	if(!IParser->seek)
		BAILWITHERROR(PARSER_ERR_INVALID_API)


bail:

		if(PARSER_SUCCESS != err)
		{
			if(IParser)
			{
				LOG_DEBUG("free IParser %p \n", IParser);
				FSL_FREE(IParser);
				IParser = NULL;
			}

			if(hParserDll)
			{
				LOG_DEBUG("close parser DLL %p \n", hParserDll);
				LibMgr->unload(hParserDll);
				hParserDll = NULL;
			}
		}

	return err;
}

OMX_ERRORTYPE FslParser::InstanceInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	int32 err = PARSER_SUCCESS;

	fsl_osal_mutex_init(&sParserMutex,fsl_osal_mutex_normal);

	LibMgr = FSL_NEW(ShareLibarayMgr, ());
	if(LibMgr == NULL)
		return OMX_ErrorInsufficientResources;

	err = CreateParserInterface();
	if (PARSER_SUCCESS != err)
	{
		LOG_ERROR("Error: Fail to get the parser interface, err %d\n", err);
		CLEAR_AND_RETURN_ERROR();
	}

	ret = InitCoreParser();
	if (ret != OMX_ErrorNone)
		return ret;

	ret = SetupPortMediaFormat();
	if (ret != OMX_ErrorNone)
		return ret;

	SetMetadata((OMX_STRING)"mime", GetMimeFromComponentRole(sCompRole.cRole), 32);

	return ret;
}


OMX_ERRORTYPE FslParser::InstanceDeInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 i;
	Track *track;
	MediaBuf *p;


	if(parserHandle)
	{
		IParser->deleteParser(parserHandle);
		parserHandle = NULL;
	}

	/*some buffer still hold by core parser.*/
	for (i = 0; i < MAX_TRACK_COUNT; i++)
	{
		track= &tracks[i];
		p = track->FreeListHead;
		while (p)
		{
			track->FreeListHead = track->FreeListHead->next;
			if (p->buf.pBuffer)
				FSL_FREE(p->buf.pBuffer);
			FSL_FREE(p);
			p = track->FreeListHead;
		}

		p = track->UsedListHead;
		while (p)
		{
			track->UsedListHead = track->UsedListHead->next;
			if (p->buf.pBuffer)
				FSL_FREE(p->buf.pBuffer);
			FSL_FREE(p);
			p = track->UsedListHead;
		}
	}

	if(IParser)
	{
		LOG_DEBUG("free IParser %p \n", IParser);
		FSL_FREE(IParser);
		IParser = NULL;
	}

	if(hParserDll)
	{
		LOG_DEBUG("close parser DLL %p \n",hParserDll);
		LibMgr->unload(hParserDll);
		hParserDll = NULL;
	}
	FSL_DELETE(LibMgr);
	LibMgr = NULL;

	if(sParserMutex)
	{
		fsl_osal_mutex_destroy(sParserMutex);
		sParserMutex = NULL;
	}

	return ret;
}


OMX_ERRORTYPE FslParser::GetNextSampleFileMode(MediaBuf **pBuf, uint32 *data_size, uint32 track_num)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	Track *track = &tracks[track_num];
	Track *trackAnother = NULL;
	OMX_TICKS tsStart = -1, tsCurr = -1;
	uint32 interleave_size = 0;
	uint32 request_data_size = *data_size;

	if(track_num == nActiveAudioNum)
		trackAnother = &tracks[nActiveVideoNum];
	else if(track_num == nActiveVideoNum)
		trackAnother = &tracks[nActiveAudioNum];

	//printf("GetNextSampleFileMode track %d\n", track_num);

	if(trackAnother && trackAnother->UsedListTail)
		tsStart = trackAnother->UsedListTail->buf.nTimeStamp;

	while (!track->UsedListHead)
	{
		//printf("%s,%d track %d.\n",__FUNCTION__,__LINE__,track_num);
		ret = GetOneSample(pBuf,data_size,track_num,FALSE,0);
		if (ret != OMX_ErrorNone)
			return ret;

		if(track->UsedListHead)
			break;

		if(trackAnother && trackAnother->UsedListTail)
		{
			interleave_size += trackAnother->UsedListTail->buf.nFilledLen;
			if(tsStart == -1 ) // this is the first sample added to trackAnother list
				tsStart = trackAnother->UsedListTail->buf.nTimeStamp;
			else  // check difference between tsStart and tsCurr
			{
				tsCurr = trackAnother->UsedListTail->buf.nTimeStamp;
				if(tsCurr - tsStart > MAX_INTERLEAVE_TIME && interleave_size > MAX_INTERLEAVE_SIZE)
				{
					MediaBuf * pTemp = NULL;
					uint32 tmp = 16;
					LOG_ERROR("read data for track %d time out, set EOS, interleave time %lld ms , interleave size %d Bytes\n", track_num, (tsCurr - tsStart)/1000, interleave_size);
					GetEmptyBufFromList((uint32)track_num,&tmp, (void**)&pTemp);
					if(pTemp != NULL)
					{
						pTemp->buf.nFlags |= OMX_BUFFERFLAG_EOS;
						pTemp->buf.nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
						AddBufferToReadyList(track_num, (void*)pTemp);
					}
					ret = OMX_ErrorNoMore;
				}
			}
		}

		*data_size = request_data_size;

	}
	*pBuf = track->UsedListHead;

	//printf("GetNextSampleFileMode %p, len %d\n", (*pBuf)->buf.pBuffer, (*pBuf)->buf.nFilledLen);

	return ret;
}

OMX_ERRORTYPE FslParser::GetNextSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	uint32 codec_init_data_len = 0;
	Track *track = NULL;
	uint32 track_num = 0;
	uint32 partial_offset = 0;
	uint32 data_size;
	MediaBuf *pBuf = NULL;

	if (pOutBuffer->nOutputPortIndex == VIDEO_OUTPUT_PORT)
	{
		track = &tracks[nActiveVideoNum];
		track_num = nActiveVideoNum;
	}
	else if (pOutBuffer->nOutputPortIndex == AUDIO_OUTPUT_PORT)
	{
		track = &tracks[nActiveAudioNum];
		track_num = nActiveAudioNum;
	}

	if(track==NULL)
		return OMX_ErrorUndefined;

	if (bHasAudioTrack)
		tracks[nActiveAudioNum].buffer_size = (int32)nAudioDataSize;
	if (bHasVideoTrack)
		tracks[nActiveVideoNum].buffer_size = (int32)nVideoDataSize;

	if (track->decoderSpecificInfoSize != 0 && !track->bCodecInitDataSent)
	{
		if (track->decoderSpecificInfoSize - track->decoderSpecificInfoOffset \
		        > pOutBuffer->nAllocLen)
		{
			codec_init_data_len = pOutBuffer->nAllocLen;
			fsl_osal_memcpy(pOutBuffer->pBuffer,track->decoderSpecificInfo + \
			                track->decoderSpecificInfoOffset, codec_init_data_len);
			track->decoderSpecificInfoOffset += codec_init_data_len;
			pOutBuffer->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
		}
		else
		{
			codec_init_data_len = track->decoderSpecificInfoSize - \
			                      track->decoderSpecificInfoOffset;
			fsl_osal_memcpy(pOutBuffer->pBuffer,track->decoderSpecificInfo + \
			                track->decoderSpecificInfoOffset, codec_init_data_len);
			track->bCodecInitDataSent = OMX_TRUE;
			pOutBuffer->nFlags |= OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;
		}
		track->buffer_size -= codec_init_data_len;
		/*FIXME:VPU need codec init data combined with first frame data*/
//        if (pOutBuffer->nOutputPortIndex == AUDIO_OUTPUT_PORT)
		{
			/*send the codec init data separately*/
			pOutBuffer->nFilledLen = codec_init_data_len;
			pOutBuffer->nTimeStamp = 0;
			LOG_DEBUG("Send codec init data: %d\n", pOutBuffer->nFilledLen);
			return ret;
		}
	}

	pOutBuffer->nFilledLen = codec_init_data_len;
	do
	{
		data_size = track->buffer_size;

		if (PARSER_READ_MODE_TRACK_BASED == read_mode)
		{
			ret = GetOneSample(&pBuf,&data_size,track_num,FALSE, 0);
			if (ret != OMX_ErrorNone)
			{
				if (pBuf)
					pOutBuffer->nFlags = pBuf->buf.nFlags;
				return ret;
			}
		}
		else if (PARSER_READ_MODE_FILE_BASED == read_mode)
		{
			ret = GetNextSampleFileMode(&pBuf,&data_size,track_num);
			if (ret != OMX_ErrorNone)
			{
				if (pBuf)
					pOutBuffer->nFlags = pBuf->buf.nFlags;

				return ret;
			}
		}

		if (pBuf)
		{

			if(codec_init_data_len + partial_offset + pBuf->buf.nFilledLen > pOutBuffer->nAllocLen)
			{
				LOG_ERROR("CoreParser read overflow: offset %d, data len %d, buffer len %d\n",
				          codec_init_data_len + partial_offset, pBuf->buf.nFilledLen, pOutBuffer->nAllocLen);
				fsl_osal_memcpy((uint8 *)pOutBuffer->pBuffer + codec_init_data_len + partial_offset,
				                pBuf->buf.pBuffer,
				                pOutBuffer->nAllocLen - (codec_init_data_len + partial_offset));
			}
			else
				fsl_osal_memcpy((uint8 *)pOutBuffer->pBuffer + codec_init_data_len + partial_offset,
				                pBuf->buf.pBuffer,
				                pBuf->buf.nFilledLen);

			pOutBuffer->nFlags = pBuf->buf.nFlags;
			if (!track->is_not_first_partial_buffer)
			{
				pOutBuffer->nTimeStamp = pBuf->buf.nTimeStamp;
				track->nPartialFrameTimeStamp = pOutBuffer->nTimeStamp;
				track->is_not_first_partial_buffer = TRUE;
			}
			else
				pOutBuffer->nTimeStamp = track->nPartialFrameTimeStamp;

			if (pOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)
				track->is_not_first_partial_buffer = FALSE;

			pOutBuffer->nFilledLen += pBuf->buf.nFilledLen;
			partial_offset += pBuf->buf.nFilledLen;
			track->buffer_size -= pBuf->buf.nFilledLen;

			ReturnBufToTrack(track_num,(void *)pBuf);
		}
	}
	while(!pBuf || (pBuf && !(pOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) && (track->buffer_size > 1024)));


	return ret;
}

OMX_ERRORTYPE FslParser::GetNextSyncSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer)
{
	return GetSyncSample(pPort,pOutBuffer,FLAG_FORWARD);
}

OMX_ERRORTYPE FslParser::GetOneSample(MediaBuf **pBuf, uint32 *data_size, uint32 track_num,bool is_sync, uint32 direction)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	int32 err = (int32)PARSER_SUCCESS;
	void * buffer_context = NULL;
	OMX_TICKS ts;
	uint64 endTime;
	uint8 *tmp;
	uint32 sampleFlag;
	uint32 track_num_got = track_num;

	fsl_osal_mutex_lock(sParserMutex);
	if (read_mode == PARSER_READ_MODE_TRACK_BASED)
	{
		if (is_sync)
		{
			err = IParser->getNextSyncSample(parserHandle,
			                                 direction,
			                                 (uint32)track_num,
			                                 &tmp,
			                                 &buffer_context,
			                                 data_size,
			                                 (uint64 *)&ts,
			                                 &endTime,
			                                 (uint32 *)&sampleFlag);
		}
		else
		{
			err = IParser->getNextSample(parserHandle,
			                             (uint32)track_num,
			                             &tmp,
			                             &buffer_context,
			                             data_size,
			                             (uint64 *)&ts,
			                             &endTime,
			                             (uint32 *)&sampleFlag);
		}
	}
	else
	{
		if (is_sync)
		{
			err = IParser->getFileNextSyncSample(parserHandle,
			                                     direction,
			                                     &track_num_got,
			                                     &tmp,
			                                     &buffer_context,
			                                     data_size,
			                                     (uint64 *)&ts,
			                                     &endTime,
			                                     (uint32 *)&sampleFlag);
		}
		else
		{
			err = IParser->getFileNextSample(parserHandle,
			                                 &track_num_got,
			                                 &tmp,
			                                 &buffer_context,
			                                 data_size,
			                                 (uint64 *)&ts,
			                                 &endTime,
			                                 (uint32 *)&sampleFlag);
		}

	}
	fsl_osal_mutex_unlock(sParserMutex);


	*pBuf = (MediaBuf *)buffer_context;
	if (!buffer_context || PARSER_SUCCESS != err)
	{
		uint32 tmp = 16;
		GetEmptyBufFromList((uint32)track_num_got,&tmp, (void **)pBuf);
		if (!*pBuf)
			return OMX_ErrorInsufficientResources;

	}

	if(PARSER_SUCCESS != err)
	{
		/*some clips will not send correct track number when reach EOS of clip*/
		if (read_mode == PARSER_READ_MODE_FILE_BASED)
			track_num_got = track_num;

		(*pBuf)->buf.nFlags |= OMX_BUFFERFLAG_EOS;
		(*pBuf)->buf.nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
		*data_size = 0;
		AddBufferToReadyList(track_num_got, (void*)(*pBuf));

		return OMX_ErrorNoMore;
	}

	/*MPEG2 parser will return zero length buffer, with valid flag that can't be ignored.*/
	(*pBuf)->buf.nTimeStamp = ts;
	(*pBuf)->buf.nFilledLen = (OMX_U32)*data_size;

	if (!(sampleFlag &FLAG_SAMPLE_NOT_FINISHED))
		(*pBuf)->buf.nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
	if (sampleFlag & FLAG_SYNC_SAMPLE)
		(*pBuf)->buf.nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

	AddBufferToReadyList(track_num_got, (void*)(*pBuf));

	//printf("Read one video frame to buffer %p:%d:%x\n", *pBuf, (*pBuf)->buf.nFilledLen, (*pBuf)->buf.nFlags);

	return ret;

}

OMX_ERRORTYPE FslParser::GetSyncSampleFileMode(MediaBuf **pBuf, uint32 *data_size, uint32 track_num,uint32 direction)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	Track *track = &tracks[track_num];
	while (!track->UsedListHead)
	{
		ret = GetOneSample(pBuf,data_size,track_num,TRUE,direction);
		if (ret != OMX_ErrorNone)
			return ret;
	}
	*pBuf = track->UsedListHead;

	return ret;
}

OMX_ERRORTYPE FslParser::GetSyncSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer,uint32 direction)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	uint32 codec_init_data_len = 0;
	uint32 partial_offset = 0;
	uint32 data_size;
	Track *track;
	MediaBuf *pBuf = NULL;

	if (tracks[nActiveVideoNum].decoderSpecificInfoSize != 0 && !tracks[nActiveVideoNum].bCodecInitDataSent)
	{
		if (tracks[nActiveVideoNum].decoderSpecificInfoSize - \
		        tracks[nActiveVideoNum].decoderSpecificInfoOffset \
		        > pOutBuffer->nAllocLen)
		{
			codec_init_data_len = pOutBuffer->nAllocLen;
			fsl_osal_memcpy(pOutBuffer->pBuffer,tracks[nActiveVideoNum].decoderSpecificInfo \
			                + tracks[nActiveVideoNum].decoderSpecificInfoOffset, codec_init_data_len);
			tracks[nActiveVideoNum].decoderSpecificInfoOffset += codec_init_data_len;
			pOutBuffer->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
		}
		else
		{
			codec_init_data_len = tracks[nActiveVideoNum].decoderSpecificInfoSize \
			                      - tracks[nActiveVideoNum].decoderSpecificInfoOffset;
			fsl_osal_memcpy(pOutBuffer->pBuffer,tracks[nActiveVideoNum].decoderSpecificInfo \
			                + tracks[nActiveVideoNum].decoderSpecificInfoOffset, codec_init_data_len);
			tracks[nActiveVideoNum].bCodecInitDataSent = OMX_TRUE;
			pOutBuffer->nFlags |= OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME;
		}

		/*send the codec init data separately*/
		pOutBuffer->nFilledLen = codec_init_data_len;
		pOutBuffer->nTimeStamp = 0;
		LOG_DEBUG("Send codec init data: %d\n", pOutBuffer->nFilledLen);
		return ret;
	}

	track = &tracks[nActiveVideoNum];

	if (bHasAudioTrack)
		tracks[nActiveAudioNum].buffer_size = (int32)nAudioDataSize;
	if (bHasVideoTrack)
		tracks[nActiveVideoNum].buffer_size = (int32)nVideoDataSize -codec_init_data_len;

	pOutBuffer->nFilledLen = codec_init_data_len;

	do
	{
		data_size = track->buffer_size;
		if (PARSER_READ_MODE_TRACK_BASED == read_mode)
		{
			ret = GetOneSample(&pBuf,&data_size,nActiveVideoNum,TRUE,direction);
			if (ret != OMX_ErrorNone)
			{
				if (pBuf)
					pOutBuffer->nFlags = pBuf->buf.nFlags;
				return ret;
			}
		}
		else if (PARSER_READ_MODE_FILE_BASED == read_mode)
		{
			ret = GetSyncSampleFileMode(&pBuf,&data_size,nActiveVideoNum,direction);
			if (ret != OMX_ErrorNone)
			{
				if (pBuf)
					pOutBuffer->nFlags = pBuf->buf.nFlags;
				return ret;
			}
		}
		if (pBuf)
		{
			fsl_osal_memcpy((uint8 *)pOutBuffer->pBuffer + codec_init_data_len + partial_offset,
			                pBuf->buf.pBuffer,
			                pBuf->buf.nFilledLen);

			pOutBuffer->nFlags = pBuf->buf.nFlags;
			if (!track->is_not_first_partial_buffer)
			{
				pOutBuffer->nTimeStamp = pBuf->buf.nTimeStamp;
				track->nPartialFrameTimeStamp = pOutBuffer->nTimeStamp;
				track->is_not_first_partial_buffer = TRUE;
			}
			else
				pOutBuffer->nTimeStamp = track->nPartialFrameTimeStamp;

			if (pOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME)
				track->is_not_first_partial_buffer = FALSE;

			pOutBuffer->nFilledLen += pBuf->buf.nFilledLen;
			partial_offset += pBuf->buf.nFilledLen;
			track->buffer_size -= pBuf->buf.nFilledLen;

			ReturnBufToTrack(nActiveVideoNum,(void *)pBuf);
		}
	}
	while(pBuf && !(pOutBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) && (track->buffer_size > 1024));

	LOG_DEBUG("%s,%d get a video framefilled len %d,ts %lld, is key frame %d!.\n",
	          __FUNCTION__,__LINE__, pOutBuffer->nFilledLen,pOutBuffer->nTimeStamp,sampleFlag&FLAG_SYNC_SAMPLE);

	return ret;

}


OMX_ERRORTYPE FslParser::GetPrevSyncSample(Port *pPort,OMX_BUFFERHEADERTYPE *pOutBuffer)
{
	return GetSyncSample(pPort,pOutBuffer,FLAG_BACKWARD);
}

OMX_ERRORTYPE FslParser::DoSeek(OMX_U32 nPortIndex,OMX_U32 nSeekFlag)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	int32 err = (int32)PARSER_SUCCESS;
	Track *track;
	MediaBuf *p;
	uint32 track_number;

	if (bHasAudioTrack)
		tracks[nActiveAudioNum].buffer_size = (int32)nAudioDataSize;
	if (bHasVideoTrack)
		tracks[nActiveVideoNum].buffer_size = (int32)nVideoDataSize;

	fsl_osal_mutex_lock(sParserMutex);

	if (nPortIndex == AUDIO_OUTPUT_PORT)
	{
		OMX_BOOL isAsf = strncmp((char *)sCompRole.cRole, "parser.asf", strlen("parser.asf")) ? OMX_FALSE : OMX_TRUE;
		sActualAudioSeekPos = sAudioSeekPos;
		track_number = nActiveAudioNum;
		if(isAsf != OMX_TRUE || (isAsf == OMX_TRUE && bHasVideoTrack != OMX_TRUE))
			err = IParser->seek(parserHandle, nActiveAudioNum, (uint64 *)&sActualAudioSeekPos, nSeekFlag);
		LOG_DEBUG("%s,%d seek audio to %lld,seek flag %d.\n",__FUNCTION__,__LINE__,sActualAudioSeekPos,nSeekFlag);
	}
	else
	{
		sActualVideoSeekPos = sVideoSeekPos;
		track_number = nActiveVideoNum;
		err = IParser->seek(parserHandle, nActiveVideoNum, (uint64 *)&sActualVideoSeekPos, nSeekFlag);
		LOG_DEBUG("%s,%d seek video to %lld,seek flag %d.\n",__FUNCTION__,__LINE__,sActualVideoSeekPos,nSeekFlag);
	}
	fsl_osal_mutex_unlock(sParserMutex);
	track= &tracks[track_number];

	track->is_not_first_partial_buffer = FALSE;

	p = track->UsedListHead;
	while (p)
	{
		ReturnBufToTrack(track_number,(void *)p);
		p = track->UsedListHead;
	}


	if (PARSER_SUCCESS != err)
	{
		if (PARSER_EOS == err)
		{
			LOG_DEBUG("Seek meet track %d EOS.\n", track_number);
			return OMX_ErrorNoMore;
		}
		else
		{
			LOG_ERROR("%s,%d Error!.\n",__FUNCTION__,__LINE__);
			ComponentBase::SendEvent(OMX_EventError,OMX_ErrorStreamCorrupt,nPortIndex,NULL);
			return OMX_ErrorStreamCorrupt;
		}
	}


	return ret;
}

OMX_ERRORTYPE FslParser::CheckAllPortDectedFinished()
{
	OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

	OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	sPortDef.nPortIndex = AUDIO_OUTPUT_PORT;
	ports[AUDIO_OUTPUT_PORT]->GetPortDefinition(&sPortDef);
	if (sPortDef.format.audio.eEncoding == OMX_AUDIO_CodingAutoDetect)
	{
		sPortDef.format.audio.eEncoding = OMX_AUDIO_CodingMax;
		ports[AUDIO_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
		LOG_DEBUG("%s,%d,Send port setting changed event.\n",__FUNCTION__,__LINE__);
		ComponentBase::SendEvent(OMX_EventPortFormatDetected,  AUDIO_OUTPUT_PORT, 0, NULL);
		ComponentBase::SendEvent(OMX_EventPortSettingsChanged, AUDIO_OUTPUT_PORT, 0, NULL);
	}

	sPortDef.nPortIndex = VIDEO_OUTPUT_PORT;
	ports[VIDEO_OUTPUT_PORT]->GetPortDefinition(&sPortDef);
	if (sPortDef.format.video.eCompressionFormat == OMX_VIDEO_CodingAutoDetect)
	{
		sPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingMax;
		ports[VIDEO_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
		LOG_DEBUG("%s,%d,Send port setting changed event.\n",__FUNCTION__,__LINE__);
		ComponentBase::SendEvent(OMX_EventPortFormatDetected,  VIDEO_OUTPUT_PORT, 0, NULL);
		ComponentBase::SendEvent(OMX_EventPortSettingsChanged, VIDEO_OUTPUT_PORT, 0, NULL);
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE FslParser::SetupPortMediaFormat()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	int32 err = (int32)PARSER_SUCCESS;
	char tmp[32];
	OMX_U32 i;
	OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

	MediaType trackType;
	uint32 decoderType;
	uint32 decoderSubtype;
	uint32 max_sample_rate = 0,max_resolution = 0;
	uint32 rate, scale;
	OMX_BOOL bHasVideo = OMX_FALSE,bHasAudio = OMX_FALSE;
	uint64 sSeekPosTmp = 0;
	OMX_S64 DurationTmp = 0;

	for(i = 0; i< numTracks; i++)
	{

		err = IParser->getBitRate(parserHandle, i, &tracks[i].bitRate);
		if(PARSER_SUCCESS !=  err)
			CLEAR_AND_RETURN_ERROR();
		LOG_DEBUG("\t bits per sec: %d\n", tracks[i].bitRate);


		err = IParser->getTrackDuration(parserHandle, i,&tracks[i].usDuration);
		if(PARSER_SUCCESS !=  err)
			CLEAR_AND_RETURN_ERROR();
		LOG_DEBUG ("\t ");
		LOG_DEBUG("duration in us: %lld\t",tracks[i].usDuration);


		if(IParser->getDecoderSpecificInfo)
		{
			err = IParser->getDecoderSpecificInfo(parserHandle, i, &tracks[i].decoderSpecificInfo, &tracks[i].decoderSpecificInfoSize);
			if(PARSER_SUCCESS !=  err)
				CLEAR_AND_RETURN_ERROR();
			LOG_DEBUG("\t ");
			LOG_DEBUG("codec specific info size : %d, data buffer %p\n", tracks[i].decoderSpecificInfoSize, tracks[i].decoderSpecificInfo);
		}


		err = IParser->getTrackType(  parserHandle,
		                              i,
		                              (uint32 *)&trackType,
		                              &decoderType,
		                              &decoderSubtype);
		if(PARSER_SUCCESS !=  err)
			CLEAR_AND_RETURN_ERROR();

		tracks[i].mediaType = trackType;
		tracks[i].decoderType = decoderType;
		tracks[i].decoderSubtype = decoderSubtype;

		switch(trackType)
		{
		case MEDIA_VIDEO:
			err = IParser->getVideoFrameWidth(parserHandle, i, &tracks[i].width);
			if(PARSER_SUCCESS !=  err)
				CLEAR_AND_RETURN_ERROR();
			err = IParser->getVideoFrameHeight(parserHandle, i, &tracks[i].height);
			if(PARSER_SUCCESS !=  err)
				CLEAR_AND_RETURN_ERROR();
			rate = 0;
			err = IParser->getVideoFrameRate( parserHandle,i,&rate,&scale);
			if(PARSER_SUCCESS !=  err)
				CLEAR_AND_RETURN_ERROR();
			LOG_DEBUG("\t width: %d, height: %d\n", tracks[i].width, tracks[i].height);
			if(VIDEO_H264 == decoderType)
				tracks[i].isH264Video = TRUE;


			nVideoTrackNum[nNumAvailVideoStream] = i;
			if (max_resolution <= tracks[i].width*tracks[i].height)
			{
				nActiveVideoNum = i;
				max_resolution = tracks[i].width*tracks[i].height;
				nVideoDuration = tracks[i].usDuration;
			}
			nNumAvailVideoStream++;
			bHasVideo = OMX_TRUE;

			break;

		case MEDIA_AUDIO:
			err = IParser->getAudioNumChannels(parserHandle, i, &tracks[i].numChannels);
			if(PARSER_SUCCESS !=  err)
				CLEAR_AND_RETURN_ERROR();
			LOG_DEBUG("\t channels: %d\n", tracks[i].numChannels);

			err = IParser->getAudioSampleRate(parserHandle, i, &tracks[i].sampleRate);
			if(PARSER_SUCCESS !=  err)
				CLEAR_AND_RETURN_ERROR();
			LOG_DEBUG("\t sample rate: %d HZ\n", tracks[i].sampleRate);

			if(IParser->getAudioBitsPerSample)
			{
				err = IParser->getAudioBitsPerSample(parserHandle, i, &tracks[i].bitsPerSample);
				if(PARSER_SUCCESS != err) goto bail;
			}
			if(IParser->getAudioBlockAlign)
			{
				err = IParser->getAudioBlockAlign(parserHandle, i, &tracks[i].audioBlockAlign);
				if(PARSER_SUCCESS != err) goto bail;
				LOG_DEBUG("\t BlockAlign: %d\n", tracks[i].audioBlockAlign);
			}
			if(IParser->getAudioBitsPerFrame)
			{
				err = IParser->getAudioBitsPerFrame(parserHandle, i, &tracks[i].bits_per_frame);
				if(PARSER_SUCCESS !=  err) CLEAR_AND_RETURN_ERROR();
			}
			if(IParser->getAudioChannelMask)
			{
				err = IParser->getAudioChannelMask(parserHandle, i, &tracks[i].audioChannelMask);
				if(PARSER_SUCCESS != err) goto bail;
			}

			if((AUDIO_AAC == decoderType)||(AUDIO_MPEG2_AAC == decoderType))
				tracks[i].isAAC = TRUE;
			if((AUDIO_AAC == decoderType) && (AUDIO_ER_BSAC == decoderSubtype))
			{
				tracks[i].isBSAC = TRUE;
				if (0 == tracks[i].firstBSACTrackNum )
					tracks[i].firstBSACTrackNum = i;
			}
			if (AUDIO_REAL == decoderType)
			{
				/*let the RA decoder to decipher the audio codec data to abtain the init info*/
				/*FIXME:don't send RealAudio AAC codec init data downstream.*/
				if (tracks[i].decoderSubtype == REAL_AUDIO_RAAC)
					tracks[i].decoderSpecificInfoSize = 0;

			}


			nAudioTrackNum[nNumAvailAudioStream] = i;
			if (max_sample_rate <= tracks[i].sampleRate)
			{
				nActiveAudioNum = i;
				max_sample_rate = tracks[i].sampleRate;
				nAudioDuration = tracks[i].usDuration;
			}
			nNumAvailAudioStream++;
			bHasAudio = OMX_TRUE;
			break;

		case MEDIA_TEXT:
			LOG_DEBUG("Text(subtitle)\n");
			break;


		case MEDIA_MIDI:
			LOG_DEBUG("MIDI\n");
			break;

		default:
			LOG_DEBUG("Unknown media type\n");

		}

		fsl_osal_mutex_lock(sParserMutex);
		sSeekPosTmp = 0;
		err = IParser->seek(parserHandle, i, &sSeekPosTmp, SEEK_FLAG_NO_LATER);
		if(PARSER_SUCCESS !=  err)
			CLEAR_AND_RETURN_ERROR();
		fsl_osal_mutex_unlock(sParserMutex);
	}

	if (bHasVideo)
	{
		OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
		sPortDef.nPortIndex = VIDEO_OUTPUT_PORT;
		ports[VIDEO_OUTPUT_PORT]->GetPortDefinition(&sPortDef);
		nVideoDataSize = sPortDef.nBufferSize;
		sPortDef.format.video.nFrameWidth  = tracks[nActiveVideoNum].width;
		sPortDef.format.video.nFrameHeight = tracks[nActiveVideoNum].height;
		if(rate !=0)
			sPortDef.format.video.xFramerate = (OMX_U64)rate * Q16_SHIFT /scale ;
		SetVideoCodecType(&sPortDef,nActiveVideoNum);
		ports[VIDEO_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
		LOG_DEBUG("%s,%d,Send port setting changed event.\n",__FUNCTION__,__LINE__);
		ComponentBase::SendEvent(OMX_EventPortFormatDetected,  VIDEO_OUTPUT_PORT, 0, NULL);
		ComponentBase::SendEvent(OMX_EventPortSettingsChanged, VIDEO_OUTPUT_PORT, 0, NULL);
	}

	if (bHasAudio)
	{
		OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
		sPortDef.nPortIndex = AUDIO_OUTPUT_PORT;
		ports[AUDIO_OUTPUT_PORT]->GetPortDefinition(&sPortDef);
		nAudioDataSize = sPortDef.nBufferSize;
		SetAudioCodecType(&sPortDef,nActiveAudioNum);
		ports[AUDIO_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
		LOG_DEBUG("%s,%d,Send port setting changed event.\n",__FUNCTION__,__LINE__);
		ComponentBase::SendEvent(OMX_EventPortFormatDetected, AUDIO_OUTPUT_PORT, 0, NULL);
		ComponentBase::SendEvent(OMX_EventPortSettingsChanged, AUDIO_OUTPUT_PORT, 0, NULL);
	}

	if (bHasVideo)
	{
		sprintf(tmp, "%d", (int)tracks[nActiveVideoNum].width);
		SetMetadata((OMX_STRING)"width", tmp, 32);
		sprintf(tmp, "%d", (int)tracks[nActiveVideoNum].height);
		SetMetadata((OMX_STRING)"height", tmp, 32);
		if (rate && scale)
		{
			sprintf(tmp, "%d", (int)(rate/scale));
			SetMetadata((OMX_STRING)"frame_rate", tmp, 32);
		}
		SetMetadata((OMX_STRING)"video_format", GetFormatFromDecoderType(tracks[nActiveVideoNum].decoderType, tracks[nActiveVideoNum].decoderSubtype), 32);
	}

	DurationTmp = usDuration;
	if (nVideoDuration > DurationTmp)
		DurationTmp = nVideoDuration;
	if (nAudioDuration > DurationTmp)
		DurationTmp = nAudioDuration;
	// The duration value is a string representing the duration in ms.
	sprintf(tmp, "%lld", (DurationTmp + 500) / 1000);
	SetMetadata((OMX_STRING)"duration", tmp, 32);

	//	{ "tracknum", USER_DATA_TRACKNUM },
	//	{ "albumart", USER_DATA_ALBUMART },

	if (OMX_FALSE == bHasVideoTrack && OMX_FALSE == bHasAudioTrack)
		ComponentBase::SendEvent(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);

	if (OMX_FALSE == bHasVideoTrack || OMX_FALSE == bHasAudioTrack)
		CheckAllPortDectedFinished();
bail:
	return ret;
}


OMX_ERRORTYPE FslParser::GetParameter(
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR pComponentParameterStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	switch (nParamIndex)
	{
	case OMX_IndexParamAudioPcm:
		fsl_osal_memcpy((void *)pComponentParameterStructure,(void *)&tracks[nActiveAudioNum].audio_type.pcm_type, sizeof (OMX_AUDIO_PARAM_PCMMODETYPE));
		break;
	case OMX_IndexParamAudioAac:
		fsl_osal_memcpy((void *)pComponentParameterStructure,(void *)&tracks[nActiveAudioNum].audio_type.aac_type, sizeof (OMX_AUDIO_PARAM_AACPROFILETYPE));
		break;
	case OMX_IndexParamAudioAc3:
		fsl_osal_memcpy((void *)pComponentParameterStructure,(void *)&tracks[nActiveAudioNum].audio_type.ac3_type, sizeof (OMX_AUDIO_PARAM_AC3TYPE));
		break;
	case OMX_IndexParamAudioVorbis:
		fsl_osal_memcpy((void *)pComponentParameterStructure,(void *)&tracks[nActiveAudioNum].audio_type.vorbis_type, sizeof (OMX_AUDIO_PARAM_VORBISTYPE));
		break;
	case OMX_IndexParamAudioAmr:
		fsl_osal_memcpy((void *)pComponentParameterStructure,(void *)&tracks[nActiveAudioNum].audio_type.amr_type, sizeof (OMX_AUDIO_PARAM_AMRTYPE));
		break;
	case OMX_IndexParamVideoWmv:
		fsl_osal_memcpy((void *)pComponentParameterStructure,(void *)&tracks[nActiveVideoNum].video_type.wmv_type, sizeof (OMX_VIDEO_PARAM_WMVTYPE));
		break;
	case OMX_IndexParamVideoRv:
		fsl_osal_memcpy((void *)pComponentParameterStructure,(void *)&tracks[nActiveVideoNum].video_type.rv_param, sizeof (OMX_VIDEO_PARAM_RVTYPE));
		break;
	case OMX_IndexParamAudioWma:
		fsl_osal_memcpy((void *)pComponentParameterStructure,(void *)&tracks[nActiveAudioNum].audio_type.wma_type, sizeof (OMX_AUDIO_PARAM_WMATYPE));
		break;
	case OMX_IndexParamAudioWmaExt:
		fsl_osal_memcpy((void *)pComponentParameterStructure,(void *)&tracks[nActiveAudioNum].wma_type_ext, sizeof (OMX_AUDIO_PARAM_WMATYPE_EXT));
		break;
	case OMX_IndexParamAudioRa:
		fsl_osal_memcpy((void *)pComponentParameterStructure,(void *)&tracks[nActiveAudioNum].audio_type.ra_param, sizeof (OMX_AUDIO_PARAM_RATYPE));
		break;
	default:
		ret = Parser::GetParameter(nParamIndex, pComponentParameterStructure);
		break;
	}

	return ret;
}

void FslParser::ActiveTrack(uint32 track_number)
{
	Track *track;
	uint64 sSeekPosTmp = 0;
	int32 err;
	track = &tracks[track_number];
	track->bCodecInitDataSent = OMX_FALSE;
	track->decoderSpecificInfoOffset = 0;
	LOG_DEBUG("%s,%d,enable track %d.\n",__FUNCTION__,__LINE__,track_number);
	IParser->enableTrack(parserHandle,track_number, TRUE);

	fsl_osal_mutex_lock(sParserMutex);
	err = IParser->seek(parserHandle, track_number, &sSeekPosTmp, SEEK_FLAG_NO_LATER);
	if(PARSER_SUCCESS !=  err)
		LOG_ERROR("seek error!\n");
	fsl_osal_mutex_unlock(sParserMutex);

}
void FslParser::DisableTrack(uint32 track_number)
{
	LOG_DEBUG("%s,%d,enable track %d.\n",__FUNCTION__,__LINE__,track_number);
	IParser->enableTrack(parserHandle,track_number, FALSE);
}

uint8 * FslParser::GetEmptyBufFromList(uint32 track_num,uint32 *size, void ** buf_context)
{
	Track *track = &tracks[track_num];
	MediaBuf *tmp = track->FreeListHead;
	MediaBuf *p;

	LOG_DEBUG("track->buffer_size: %d\n", track->buffer_size);
	if (*size > (uint32)track->buffer_size)
		*size = track->buffer_size;
	if (tmp)
	{
		/*remove from free list*/
		track->FreeListHead = track->FreeListHead->next;
		if (NULL == track->FreeListHead)
			track->FreeListTail = NULL;
		else
			track->FreeListHead->prev = NULL;

		if (tmp->buf.nAllocLen < *size)
		{
			tmp->buf.pBuffer = (uint8 *)FSL_REALLOC(tmp->buf.pBuffer,*size);
			if (!tmp->buf.pBuffer)
				return NULL;

			tmp->buf.nAllocLen = *size;
		}

		//printf("track %d get empty buffer %p from free list, free list buffer %d.\n", track_num, tmp->buf.pBuffer,--track->total_freed);
	}
	else
	{
		tmp = (MediaBuf *)FSL_MALLOC(sizeof(MediaBuf));
		if (!tmp)
			return NULL;
		fsl_osal_memset(tmp, 0, sizeof(MediaBuf));
		tmp->buf.pBuffer = (uint8 *)FSL_MALLOC(*size);
		if (!tmp->buf.pBuffer)
			return NULL;
		tmp->buf.nAllocLen = *size;

		//printf("track %d get empty buffer %p by allocate new, total allocated %d\n", track_num, tmp->buf.pBuffer, ++track->total_allocated);
	}

	//printf("track %d, request buffer %p.\n",track_num, tmp->buf.pBuffer);

	*buf_context = tmp;

	return tmp->buf.pBuffer;
}

OMX_ERRORTYPE FslParser::ReturnBufToTrack(uint32 track_num,void *buf_context)
{
	if(buf_context == NULL)
		return OMX_ErrorBadParameter;

	RemoveBufferFromReadyList(track_num, buf_context);
	ReturnBufToFreeList(track_num, buf_context);

	return OMX_ErrorNone;
}

OMX_ERRORTYPE FslParser::AddBufferToReadyList(uint32 track_num, void * buf_context)
{
	Track *track = &tracks[track_num];
	MediaBuf *p = track->UsedListTail;
	MediaBuf *tmp = (MediaBuf*)buf_context;

	//printf("track %d, add buffer %p to ready list, cnt: %d.\n",track_num, tmp->buf.pBuffer, ++track->total_used);

	if (!p)
	{
		track->UsedListTail = track->UsedListHead = tmp;
		tmp->prev = NULL;
		tmp->next = NULL;
	}
	else
	{
		track->UsedListTail = tmp;
		p->next = tmp;
		tmp->prev = p;
		tmp->next = NULL;
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE FslParser::RemoveBufferFromReadyList(uint32 track_num, void * buf_context)
{
	Track *track = &tracks[track_num];
	MediaBuf *tmp = (MediaBuf *)buf_context;
	MediaBuf *p;

	//printf("track %d, remove buffer %p from ready list, cnt: %d.\n",track_num, tmp->buf.pBuffer, --track->total_used);

	/*remove from used list*/
	if (tmp->prev)
	{
		tmp->prev->next = tmp->next;
		if (tmp->next)
			tmp->next->prev = tmp->prev;
		else
		{
			track->UsedListTail = tmp->prev;
			track->UsedListTail->next = NULL;
		}
	}
	else
	{
		track->UsedListHead = tmp->next;
		if (track->UsedListHead)
			track->UsedListHead->prev = NULL;
		else
			track->UsedListTail = NULL;
	}

	return OMX_ErrorNone;
}

void FslParser::ReturnBufToFreeList(uint32 track_num, void * buf_context)
{
	Track *track = &tracks[track_num];
	MediaBuf *tmp = (MediaBuf *)buf_context;
	MediaBuf *p;

	tmp->buf.nFilledLen = 0;
	tmp->buf.nTimeStamp = 0;
	tmp->buf.nFlags = 0;
	tmp->prev = tmp->next = NULL;
	p = track->FreeListTail;
	if (!p)
	{
		track->FreeListTail = track->FreeListHead = tmp;
	}
	else
	{
		track->FreeListTail = tmp;
		p->next = tmp;
		tmp->prev = p;
	}
	//printf("track %d, return buffer %p to list, free list buffer %d.\n",track_num, tmp->buf.pBuffer, ++track->total_freed);
}

/*FIXME:Port Enable has no virtual func left for dirived class,thus can't enable a track after disable it.*/
#if 0
OMX_ERRORTYPE FslParser::ComponentReturnBuffer(OMX_U32 nPortIndex)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;

	if (nPortIndex == VIDEO_OUTPUT_PORT)
		IParser->enableTrack(parserHandle,nActiveVideoNum, FALSE);
	else if (nPortIndex == AUDIO_OUTPUT_PORT)
		IParser->enableTrack(parserHandle,nActiveAudioNum, FALSE);

	return err;
}
#endif


/**< C style functions to expose entry point for the shared library */
extern "C" {
	OMX_ERRORTYPE FslParserInit(OMX_IN OMX_HANDLETYPE pHandle)
	{
		OMX_ERRORTYPE ret = OMX_ErrorNone;
		FslParser *obj = NULL;
		ComponentBase *base = NULL;

		obj = FSL_NEW(FslParser, ());
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
