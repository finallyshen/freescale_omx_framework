/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#ifdef ANDROID_BUILD
#include <utils/String8.h>
#include "id3_parser/ID3.h"
using namespace android;
#endif
#include "AudioParserBase.h"
#include "ShareLibarayMgr.h"

OMX_ERRORTYPE AudioParserBase::InstanceInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    bHasAudioTrack = OMX_TRUE;

	nBeginPoint = 0;
	nReadPoint = 0;
	nBeginPointOffset = 0;
	bBeginPointFounded = OMX_FALSE;
	pThreadId = NULL;
	nSource2CurPos = 0;
	bStopVBRDurationThread = OMX_FALSE;
	bSegmentStart = OMX_TRUE;
	bSeekable = OMX_TRUE;
	bTOCSeek = OMX_FALSE;
	nOneSecondSample = 0;
	nSampleRate = 44100;
	bCBR = OMX_FALSE; 
	bVBRDurationReady = OMX_FALSE;
	fsl_osal_memset(SeekTable, 0, sizeof(OMX_U64 **) * MAXAUDIODURATIONHOUR);
#ifndef ANDROID_BUILD
	fsl_osal_memset(&info, 0, sizeof(meta_data_v1));
	fsl_osal_memset(&info_v2, 0, sizeof(meta_data_v2));
#endif
	sourceFileHandle = fileOps.Open((const uint8*)pMediaName,(const uint8*) "rb", this);
	if (sourceFileHandle == 0 )
	{
		LOG_ERROR("Can't open input file.\n");
		return OMX_ErrorInsufficientResources;
	}

	nFileSize = fileOps.Size(sourceFileHandle, this);
	nEndPoint = nFileSize;
        if(nEndPoint == 0 && isStreamingSource == OMX_TRUE)
            nEndPoint = -1L;

	ret = GetCoreParser();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Get core parser fail.\n");
		return ret;
	}

	struct timeval tv, tv1;
	gettimeofday (&tv, NULL);

	ret = AudioParserInstanceInit();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Audio parser instance init fail.\n");
		return ret;
	}

	SetMetadata((OMX_STRING)"mime", GetMimeFromComponentRole(sCompRole.cRole), 32);
	// The duration value is a string representing the duration in ms. 
	char tmp[32];
	sprintf(tmp, "%lld", (usDuration + 500) / 1000); 
	SetMetadata((OMX_STRING)"duration", tmp, 32);
	gettimeofday (&tv1, NULL);
	LOG_DEBUG("Audio Parser Time: %d\n", (tv1.tv_sec-tv.tv_sec)*1000+(tv1.tv_usec-tv.tv_usec)/1000);

    return ret;
}

void *ParserCalculateVBRDurationFunc(void *arg);

OMX_ERRORTYPE AudioParserBase::AudioParserInstanceInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 nAvrageBitRate;

	LOG_DEBUG("Audio file size: %lld\n", nFileSize);
	if (nFileSize == 0)
	{
		SetupPortMediaFormat();
		return ret;
	}

	/** Parser ID3 tag */
	ParserID3Tag();

	/** Parser file header */
	ret = AudioParserFileHeader();
	if (ret != OMX_ErrorNone)
	{
		if (ret == OMX_ErrorNoMore)
			return OMX_ErrorNone;
		else
			return ret;
	}

	/** Parser three segment to check if the file format is supported and get rough duration*/
	LOG_DEBUG("Begin Point: %lld\n", nBeginPoint);
	if (bGetMetadata == OMX_TRUE)
		ParserOneSegmentAudio();
	else
		ParserThreeSegmentAudio();
	LOG_DEBUG("bCBR after three segment = %d\n", bCBR);

	if (bBeginPointFounded == OMX_TRUE)
	{
		nBeginPoint += nBeginPointOffset;
	}
	LOG_DEBUG("Begin Point: %lld\n", nBeginPoint);
	nAvrageBitRate = GetAvrageBitRate();
	if (nAvrageBitRate == 0)
	{
		LOG_ERROR("Audio duration is 0.\n");
		return OMX_ErrorFormatNotDetected;
	}
	nAudioDuration = (OMX_U64)((nEndPoint - nBeginPoint) << 3) * OMX_TICKS_PER_SECOND / nAvrageBitRate;
	usDuration = nAudioDuration;
	LOG_DEBUG("Audio Duration: %lld\n", usDuration);

	if (bBeginPointFounded == OMX_FALSE && bGetMetadata != OMX_TRUE)
	{
		ParserFindBeginPoint();
	}

	LOG_DEBUG("Have Xing: %d total frame: %d sample rate: %d sample_per_fr: %d total bytes: %d TOC(1): %d\n", FrameInfo.FrameInfo.xing_exist, FrameInfo.FrameInfo.total_frame_num, FrameInfo.FrameInfo.sampling_rate, FrameInfo.FrameInfo.sample_per_fr, FrameInfo.FrameInfo.total_bytes, FrameInfo.FrameInfo.TOC[1]);
	if (FrameInfo.FrameInfo.sampling_rate && FrameInfo.FrameInfo.sample_per_fr && FrameInfo.FrameInfo.total_frame_num)
		nAudioDuration = (OMX_U64)FrameInfo.FrameInfo.total_frame_num * FrameInfo.FrameInfo.sample_per_fr * OMX_TICKS_PER_SECOND / FrameInfo.FrameInfo.sampling_rate;
	usDuration = nAudioDuration;
	LOG_DEBUG("Audio Duration: %lld\n", usDuration);

	if (bCBR == OMX_FALSE && FrameInfo.FrameInfo.total_bytes && FrameInfo.FrameInfo.TOC[1]) {
		bTOCSeek = OMX_TRUE;
		bVBRDurationReady = OMX_TRUE;
	} else if (bCBR == OMX_FALSE && isStreamingSource != OMX_TRUE && isLiveSource != OMX_TRUE && bGetMetadata != OMX_TRUE)
	{
		/** Calculate accurate duration and seek table in background thread */
		if(E_FSL_OSAL_SUCCESS != fsl_osal_thread_create(&pThreadId, NULL, \
					ParserCalculateVBRDurationFunc, this))
		{
			LOG_ERROR("Create audio parser calculate duration thread failed.\n");
			return OMX_ErrorInsufficientResources;
		}
	}

	fileOps.Seek(sourceFileHandle, nBeginPoint, SEEK_SET, this);
	nReadPoint = nBeginPoint;

	SetupPortMediaFormat();

	return ret;
}

OMX_ERRORTYPE AudioParserBase::AudioParserFileHeader()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	/** Parser file header */
	LOG_DEBUG("Begin Point: %lld\n", nBeginPoint);
	fileOps.Seek(sourceFileHandle, nBeginPoint, SEEK_SET, this);
	OMX_U8 *pTmpBuffer = (OMX_U8 *)FSL_MALLOC(AUDIO_PARSER_READ_SIZE);
	if (pTmpBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}

	OMX_U32 nReadLen = AUDIO_PARSER_READ_SIZE;
	OMX_U32 nActuralRead;

	if (nReadLen > nFileSize - nBeginPoint)
	{
		nReadLen = nFileSize - nBeginPoint;
	}

	nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);

	if (ParserFileHeader(&FileInfo, pTmpBuffer, nActuralRead) == AUDIO_PARSERRETURNFAIL)
		return OMX_ErrorUndefined;

	FSL_FREE(pTmpBuffer);

	if (FileInfo.bSeekable == E_FSL_OSAL_FALSE)
	{
		/** ADIF file will into this part code. */
		bSeekable = OMX_FALSE;
		LOG_DEBUG("BitRate: %d\n", FileInfo.nBitRate);
		if (FileInfo.nBitRate != 0)
		{
			nAudioDuration = (nFileSize<<3)/FileInfo.nBitRate*OMX_TICKS_PER_SECOND;
			usDuration = nAudioDuration;
		}
		goto Done;
	}

	if (FileInfo.bGotDuration == E_FSL_OSAL_TRUE && FileInfo.bIsCBR == E_FSL_OSAL_TRUE)
	{
		/** WAVE and FLAC file will into this part code. */
		bCBR = OMX_TRUE;
		nAudioDuration = FileInfo.nDuration;
		usDuration = nAudioDuration;
		nBeginPoint += FileInfo.nBeginPointOffset;
		if (FileInfo.ePCMMode != PCM_MODE_UNKNOW)
			nEndPoint = nBeginPoint + FileInfo.nBitStreamLen;
		goto Done;
	}

	if (FileInfo.bGotDuration == E_FSL_OSAL_TRUE && FileInfo.bIsCBR == E_FSL_OSAL_FALSE)
	{
		/** FLAC and MP3 with Xing header file will into this part code. */
		bCBR = OMX_FALSE;
		nAudioDuration = FileInfo.nDuration;
		usDuration = nAudioDuration;
		nBeginPoint += FileInfo.nBeginPointOffset;
		goto DoneDuration;
	}

	return ret;

DoneDuration:
	if (bCBR == OMX_FALSE)
	{
		fileOps.Seek(sourceFileHandle, nBeginPoint, SEEK_SET, this);
		/** Calculate accurate duration and seek table in background thread */
		if(E_FSL_OSAL_SUCCESS != fsl_osal_thread_create(&pThreadId, NULL, \
					ParserCalculateVBRDurationFunc, this))
		{
			LOG_ERROR("Create audio parser calculate duration thread failed.\n");
			return OMX_ErrorInsufficientResources;
		}
	}

Done:
	fileOps.Seek(sourceFileHandle, nBeginPoint, SEEK_SET, this);
	nReadPoint = nBeginPoint;

	SetupPortMediaFormat();

	return OMX_ErrorNoMore;
}

OMX_ERRORTYPE AudioParserBase::ParserID3Tag()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
#ifdef ANDROID_BUILD
	OMX_U8 *pTmpBuffer = (OMX_U8 *)FSL_MALLOC(AUDIO_PARSER_READ_SIZE);
	if (pTmpBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pTmpBuffer, 0, AUDIO_PARSER_READ_SIZE);

	OMX_U32 nReadLen = 10;
	OMX_U32 nActuralRead;

	if (nReadLen > nFileSize)
	{
		nReadLen = nFileSize;
	}

	nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);

	ID3 id3;
	bool isValid;
	if (!fsl_osal_memcmp((void *)("ID3"), pTmpBuffer, 3)) {
		// Skip the ID3v2 header.
		OMX_U32 len =
			((pTmpBuffer[6] & 0x7f) << 21)
			| ((pTmpBuffer[7] & 0x7f) << 14)
			| ((pTmpBuffer[8] & 0x7f) << 7)
			| (pTmpBuffer[9] & 0x7f);

		if (len > 3 * 1024 * 1024)
			len = 3 * 1024 * 1024;

		len += 10;

		nBeginPoint = len;

		LOG_DEBUG("Begin point: %lld\n", nBeginPoint);

		if (nBeginPoint > AUDIO_PARSER_READ_SIZE) 
		{
			FSL_FREE(pTmpBuffer);

			pTmpBuffer = (OMX_U8 *)FSL_MALLOC(nBeginPoint);
			if (pTmpBuffer == NULL)
			{
				LOG_ERROR("Can't get memory.\n");
				return OMX_ErrorInsufficientResources;
			}
			fsl_osal_memset(pTmpBuffer, 0, nBeginPoint);
		}

		fileOps.Seek(sourceFileHandle, 0, SEEK_SET, this); 
		nReadLen = nBeginPoint;

		if (nReadLen > nFileSize)
		{
			nReadLen = nFileSize;
		}

		nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);

		OMX_BOOL bGetV2 = OMX_FALSE;

		isValid = id3.parseV2((const char *)pTmpBuffer); 

	} else {
		isValid = false;
	}

	if (!isValid) {
		nReadLen = 128;
		fsl_osal_memset(pTmpBuffer, 0, AUDIO_PARSER_READ_SIZE);
		fileOps.Seek(sourceFileHandle, -128, SEEK_END, this); /*< ID3v1 */
		nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);

		LOG_DEBUG("Parser ID3 TAG V1.\n");
		isValid = id3.parseV1((const char *)pTmpBuffer); 

		if (!isValid) {
			LOG_DEBUG("No any metadata.\n");
			FSL_FREE(pTmpBuffer);
			return ret;
		}
	}

    struct Map {
        const char *key;
        const char *tag1;
        const char *tag2;
    };
    static const Map kMap[] = {
        { "album", "TALB", "TAL" },
        { "artist", "TPE1", "TP1" },
        { "albumartist", "TPE2", "TP2" },
        { "composer", "TCOM", "TCM" },
        { "genre", "TCON", "TCO" },
        { "title", "TIT2", "TT2" },
        { "year", "TYE", "TYER" },
        { "tracknumber", "TRK", "TRCK" },
        { "discnumber", "TPA", "TPOS" },
    };
	
    static const OMX_U32 kNumMapEntries = sizeof(kMap) / sizeof(kMap[0]);

    for (OMX_U32 i = 0; i < kNumMapEntries; ++i) {
        ID3::Iterator *it = FSL_NEW(ID3::Iterator, (id3, kMap[i].tag1));
        if (it->done()) {
			FSL_DELETE(it);
			it = FSL_NEW(ID3::Iterator, (id3, kMap[i].tag2));
        }

        if (it->done()) {
			FSL_DELETE(it);
			continue;
        }

        String8 s;
        it->getString(&s);
        FSL_DELETE(it);

		SetMetadata((char *)kMap[i].key, (char *)s.string(), s.length());
		LOG_DEBUG("Key: %s  metadate: %s\n", (char *)kMap[i].key, (char *)s.string());
    }

    size_t dataSize;
    String8 mime;
    const void *data = id3.getAlbumArt(&dataSize, &mime);

    if (data) {
		SetMetadata((OMX_STRING)"albumart", (char *)data, dataSize);
		LOG_DEBUG("albumart format: %s\n", mime.string());
    }

	FSL_FREE(pTmpBuffer);
#else
    ShareLibarayMgr *libMgr = NULL;
    OMX_PTR hLibId3 = NULL;
	int (*parse_id3_v2_size)(char *);
	id3_err (*parse_id3_v2)(meta_data_v2 *, unsigned char *);
	id3_err (*parse_id3_albumart)(meta_data_v2 *, unsigned char *);
	id3_err (*parse_id3_v1_metadata)(meta_data_v1 *,char *);

    libMgr = FSL_NEW(ShareLibarayMgr, ());
    if(libMgr == NULL)
        return OMX_ErrorInsufficientResources;

    hLibId3 = libMgr->load("lib_id3_parser_arm11_elinux.so");
    if(hLibId3 == NULL) {
        FSL_DELETE(libMgr);
        return OMX_ErrorUndefined;
    }

    parse_id3_v2_size = (int (*)(char *))libMgr->getSymbol(hLibId3, "id3_parser_get_id3_v2_size");
    parse_id3_v2 = (id3_err (*)(meta_data_v2 *, unsigned char *))libMgr->getSymbol(hLibId3, "id3_parser_parse_v2");
    parse_id3_albumart = (id3_err (*)(meta_data_v2 *, unsigned char *))libMgr->getSymbol(hLibId3, "id3_parser_parse_albumart_v2");
    parse_id3_v1_metadata = (id3_err (*)(meta_data_v1 *,char *))libMgr->getSymbol(hLibId3, "get_metadata_v1");
    if(parse_id3_v2_size == NULL
			|| parse_id3_v2 == NULL
			|| parse_id3_albumart == NULL
			|| parse_id3_v1_metadata == NULL) {
        libMgr->unload(hLibId3);
        FSL_DELETE(libMgr);
        return OMX_ErrorUndefined;
    }


	OMX_U8 *pTmpBuffer = (OMX_U8 *)FSL_MALLOC(AUDIO_PARSER_READ_SIZE);
	if (pTmpBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
        libMgr->unload(hLibId3);
        FSL_DELETE(libMgr);
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(pTmpBuffer, 0, AUDIO_PARSER_READ_SIZE);

	OMX_U32 nReadLen = AUDIO_PARSER_READ_SIZE;
	OMX_U32 nActuralRead;

	if (nReadLen > nFileSize)
	{
		nReadLen = nFileSize;
	}

	nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);

	nBeginPoint = parse_id3_v2_size((char *)pTmpBuffer);

	LOG_DEBUG("Begin point: %lld\n", nBeginPoint);
	id3_err retID3 = ID3_OK;

	if (nBeginPoint > AUDIO_PARSER_READ_SIZE) 
	{
		FSL_FREE(pTmpBuffer);

		pTmpBuffer = (OMX_U8 *)FSL_MALLOC(nBeginPoint);
		if (pTmpBuffer == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			libMgr->unload(hLibId3);
			FSL_DELETE(libMgr);
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pTmpBuffer, 0, nBeginPoint);
	}

	fileOps.Seek(sourceFileHandle, 0, SEEK_SET, this); 
	nReadLen = nBeginPoint;

	if (nReadLen > nFileSize)
	{
		nReadLen = nFileSize;
	}

	nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);

	OMX_BOOL bGetV2 = OMX_FALSE;
	retID3 = parse_id3_v2(&info_v2, pTmpBuffer);
	if (retID3 == ID3_OK)
	{
		if (fsl_osal_strlen(info_v2.song_title))
			SetMetadata("title", info_v2.song_title, MAX_V2_STRING);
		if (fsl_osal_strlen(info_v2.artist))
			SetMetadata("artist", info_v2.artist, MAX_V2_STRING);
		if (fsl_osal_strlen(info_v2.album))
			SetMetadata("album", info_v2.album, MAX_V2_STRING);
		if (fsl_osal_strlen(info_v2.year))
			SetMetadata("year", info_v2.year, MAX_V2_STRING);
		if (fsl_osal_strlen(info_v2.comment))
			SetMetadata("comment", info_v2.comment, MAX_V2_STRING);
		if (fsl_osal_strlen(info_v2.genre))
			SetMetadata("genre", info_v2.genre, MAX_V2_STRING);
		if (fsl_osal_strlen(info_v2.composer))
			SetMetadata("composer", info_v2.composer, MAX_V2_STRING);
		if (fsl_osal_strlen(info_v2.copyright))
			SetMetadata("copyright", info_v2.copyright, MAX_V2_STRING);
		bGetV2 = OMX_TRUE;
	}

	retID3 = parse_id3_albumart(&info_v2, pTmpBuffer);
	if (retID3 == ID3_OK && info_v2.albumartsize != 0)
	{
		SetMetadata("albumart", info_v2.albumart, info_v2.albumartsize);
		free(info_v2.albumart);
	}

	OMX_U32 nDataSize = nEndPoint - nBeginPoint;
	fsl_osal_memset(pTmpBuffer, 0, AUDIO_PARSER_READ_SIZE);
	if (nDataSize >= 128 && nDataSize <= 128 + 227)
	{
		nReadLen = 128;
		fileOps.Seek(sourceFileHandle, -128, SEEK_END, this); /*< ID3v1 */
		nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);
		retID3 = parse_id3_v1_metadata(&info, (char *)pTmpBuffer);
		if (retID3 == ID3_OK && bGetV2 == OMX_FALSE)
		{
			nEndPoint -= 128;
			if (fsl_osal_strlen(info.song_title))
				SetMetadata("title", info.song_title, MAX_V1_STRING);
			if (fsl_osal_strlen(info.artist))
				SetMetadata("artist", info.artist, MAX_V1_STRING);
			if (fsl_osal_strlen(info.album))
				SetMetadata("album", info.album, MAX_V1_STRING);
			if (fsl_osal_strlen(info.year))
				SetMetadata("year", info.year, MAX_V1_STRING);
			if (fsl_osal_strlen(info.comment))
				SetMetadata("comment", info.comment, MAX_V1_STRING);
			if (fsl_osal_strlen(info.genre_string))
				SetMetadata("genre", info.genre_string, MAX_V1_STRING);
		}
	}
	else if (nDataSize >= 128 + 227)
	{
		nReadLen = 128 + 227;
		fileOps.Seek(sourceFileHandle, -128-227, SEEK_END, this); /*< ID3v1 and ID3v1.1 */
		nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);
		retID3 = parse_id3_v1_metadata(&info, (char *)(pTmpBuffer + 227));
		if (retID3 == ID3_OK && bGetV2 == OMX_FALSE)
		{
			nEndPoint -= 128;
			if (fsl_osal_strlen(info.song_title))
				SetMetadata("title", info.song_title, MAX_V1_STRING);
			if (fsl_osal_strlen(info.artist))
				SetMetadata("artist", info.artist, MAX_V1_STRING);
			if (fsl_osal_strlen(info.album))
				SetMetadata("album", info.album, MAX_V1_STRING);
			if (fsl_osal_strlen(info.year))
				SetMetadata("year", info.year, MAX_V1_STRING);
			if (fsl_osal_strlen(info.comment))
				SetMetadata("comment", info.comment, MAX_V1_STRING);
			if (fsl_osal_strlen(info.genre_string))
				SetMetadata("genre", info.genre_string, MAX_V1_STRING);
		}
		retID3 = parse_id3_v1_metadata(&info, (char *)pTmpBuffer);
		if (retID3 == ID3_OK && bGetV2 == OMX_FALSE)
		{
			nEndPoint -= 227;
			if (fsl_osal_strlen(info.song_title))
				SetMetadata("title", info.song_title, MAX_V1_STRING);
			if (fsl_osal_strlen(info.artist))
				SetMetadata("artist", info.artist, MAX_V1_STRING);
			if (fsl_osal_strlen(info.album))
				SetMetadata("album", info.album, MAX_V1_STRING);
			if (fsl_osal_strlen(info.year))
				SetMetadata("year", info.year, MAX_V1_STRING);
			if (fsl_osal_strlen(info.comment))
				SetMetadata("comment", info.comment, MAX_V1_STRING);
			if (fsl_osal_strlen(info.genre_string))
				SetMetadata("genre", info.genre_string, MAX_V1_STRING);
		}
	}

	FSL_FREE(pTmpBuffer);
	libMgr->unload(hLibId3);
	FSL_DELETE(libMgr);

#endif
	return ret;
}

OMX_ERRORTYPE AudioParserBase::ParserOneSegmentAudio()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	fileOps.Seek(sourceFileHandle, nBeginPoint, SEEK_SET, this);
	bCBR = OMX_TRUE;
	FrameInfo.bIsCBR = E_FSL_OSAL_TRUE;

	OMX_U8 *pTmpBuffer = AudioParserGetBuffer(AUDIO_PARSER_READ_SIZE);
	if (pTmpBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}

	OMX_U32 nActuralRead, nSegmentCnt = 1;
	OMX_U32 nReadPointTmp = 0, nReadPointTmp2 = 0;
	OMX_U32 nReadLen = AUDIO_PARSER_READ_SIZE;
	OMX_U32 nDataSize = nEndPoint - nBeginPoint;

	OMX_S64 nSkip = (nDataSize - 3 * AUDIO_PARSER_SEGMENT_SIZE) / 2;
	if (nSkip < 0)
	{
		nSkip = 0;
	}

	if(-1 == fileOps.Seek(sourceFileHandle, nSkip, SEEK_CUR, this))
	{
		LOG_DEBUG("Audio file seek fail.\n");
	}

	for (OMX_U32 i = 0; ; i ++)
	{
		if (nReadPointTmp + nReadLen > nDataSize)
		{
			nReadLen = nDataSize - nReadPointTmp;
		}

		LOG_DEBUG("Audio parser read point: %lld\n", nBeginPoint + nReadPointTmp + nSkip);
		nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);

		nReadPointTmp += nActuralRead;
		nReadPointTmp2 += nActuralRead;

		LOG_LOG("nReadPointTmp = %d\n", nReadPointTmp);
		ParserAudioFrame(pTmpBuffer, nActuralRead, nSegmentCnt);

		if (nActuralRead < AUDIO_PARSER_READ_SIZE)
		{
			LOG_DEBUG("Audio file parser reach end.\n");
			break;
		}

		if (nReadPointTmp2 >= AUDIO_PARSER_SEGMENT_SIZE)
		{
			break;
			nReadPointTmp2 = 0;
			nSegmentCnt ++;
			nReadPointTmp += nSkip;
		}
	}

	if (FrameInfo.bIsCBR == E_FSL_OSAL_FALSE)
	{
		bCBR = OMX_FALSE;
	}

	AudioParserFreeBuffer(pTmpBuffer);

	return ret;
}

OMX_ERRORTYPE AudioParserBase::ParserThreeSegmentAudio()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	fileOps.Seek(sourceFileHandle, nBeginPoint, SEEK_SET, this);
	bCBR = OMX_TRUE;
	FrameInfo.bIsCBR = E_FSL_OSAL_TRUE;

	OMX_U8 *pTmpBuffer = AudioParserGetBuffer(AUDIO_PARSER_READ_SIZE);
	if (pTmpBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}

	OMX_U32 nActuralRead, nSegmentCnt = 0;
	OMX_U32 nReadPointTmp = 0, nReadPointTmp2 = 0;
	OMX_U32 nReadLen = AUDIO_PARSER_READ_SIZE;
	OMX_U32 nDataSize = nEndPoint - nBeginPoint;

	for (OMX_U32 i = 0; ; i ++)
	{
		if (nReadPointTmp + nReadLen > nDataSize)
		{
			nReadLen = nDataSize - nReadPointTmp;
		}

		LOG_LOG("Audio parser read point: %lld\n", nBeginPoint + nReadPointTmp);
		nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);

		nReadPointTmp += nActuralRead;
		nReadPointTmp2 += nActuralRead;

		LOG_LOG("nReadPointTmp = %d\n", nReadPointTmp);
		ParserAudioFrame(pTmpBuffer, nActuralRead, nSegmentCnt);

		if (nActuralRead < AUDIO_PARSER_READ_SIZE)
		{
			LOG_DEBUG("Audio file parser reach end.\n");
			break;
		}

		if (nReadPointTmp2 >= AUDIO_PARSER_SEGMENT_SIZE)
		{
			nReadPointTmp2 = 0;
			nSegmentCnt ++;
			OMX_S64 nSkip = (nDataSize - 3 * AUDIO_PARSER_SEGMENT_SIZE) / 2;
			if (nSkip < 0)
			{
				nSkip = 0;
			}

			if(-1 == fileOps.Seek(sourceFileHandle, nSkip, SEEK_CUR, this))
			{
				LOG_DEBUG("Audio file seek fail.\n");
				break;
			}
			nReadPointTmp += nSkip;
		}
	}

	if (FrameInfo.bIsCBR == E_FSL_OSAL_FALSE)
	{
		bCBR = OMX_FALSE;
	}

	AudioParserFreeBuffer(pTmpBuffer);

	return ret;
}

OMX_ERRORTYPE AudioParserBase::ParserFindBeginPoint()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	fileOps.Seek(sourceFileHandle, nBeginPoint + AUDIO_PARSER_SEGMENT_SIZE, SEEK_SET, this);
	OMX_U8 *pTmpBuffer = AudioParserGetBuffer(AUDIO_PARSER_READ_SIZE);
	if (pTmpBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}

	OMX_U32 nReadLen = AUDIO_PARSER_READ_SIZE;
	OMX_U32 nActuralRead, nSegmentCnt = PARSERAUDIO_BEGINPOINT;
	OMX_U32 nReadPointTmp = 0;
	OMX_U32 nDataSize = nEndPoint - nBeginPoint;

	for (OMX_U32 i = 0; ; i ++)
	{
		if (nReadPointTmp + nReadLen > nDataSize)
		{
			nReadLen = nDataSize - nReadPointTmp;
		}

		nActuralRead = fileOps.Read(sourceFileHandle, pTmpBuffer, nReadLen, this);

		nReadPointTmp += nActuralRead;

		ParserAudioFrame(pTmpBuffer, nActuralRead, nSegmentCnt);

		if (nActuralRead < AUDIO_PARSER_READ_SIZE)
		{
			LOG_DEBUG("Audio file parser reach end.\n");
			break;
		}

		if (bBeginPointFounded == OMX_TRUE)
		{
			LOG_DEBUG("Audio file begin point found.\n");
			break;
		}
	}

	AudioParserFreeBuffer(pTmpBuffer);

	nBeginPoint += AUDIO_PARSER_SEGMENT_SIZE;
	nBeginPoint += nBeginPointOffset;
	LOG_DEBUG("Begin Point: %lld\n", nBeginPoint);

	return ret;
}

void *ParserCalculateVBRDurationFunc(void *arg) 
{
	AudioParserBase *AudioParserBasePtr = (AudioParserBase *)arg;

	AudioParserBasePtr->ParserCalculateVBRDuration();

	return NULL;
}

OMX_ERRORTYPE AudioParserBase::ParserCalculateVBRDuration() 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	FslFileStream fileOps2;
	FslFileHandle sourceFileHandle2;

	OMX_U8 *pTmpBuffer = AudioParserGetBuffer(AUDIO_PARSER_READ_SIZE);
	if (pTmpBuffer == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}

	fsl_osal_memcpy(&fileOps2, &fileOps, sizeof(FslFileStream));

	sourceFileHandle2 = fileOps2.Open((const uint8*)pMediaName,(const uint8*) "rb", this);
	if (sourceFileHandle2 == 0 )
	{
		LOG_ERROR("Can't open input file.\n");
		return OMX_ErrorInsufficientResources;
	}

	LOG_DEBUG("Begin point: %d\n", nBeginPoint);
	fileOps2.Seek(sourceFileHandle2, nBeginPoint, SEEK_SET, this);

	SeekTable[0] = (OMX_U64 **)FSL_MALLOC(60 * sizeof(OMX_U64 *));
	if(SeekTable[0] == NULL)
	{
		LOG_ERROR("Failed in allocate memory for seek_index\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(SeekTable[0], 0, 60 * sizeof(OMX_U64 *));

	SeekTable[0][0] = (OMX_U64 *)FSL_MALLOC(60 * sizeof(OMX_U64));
	if(SeekTable[0][0] == NULL)
	{
		LOG_ERROR("Failed in allocate memory for seek_index\n");
		return OMX_ErrorInsufficientResources;
	}
	fsl_osal_memset(SeekTable[0][0], 0, 60 * sizeof(OMX_U64));

	SeekTable[0][0][0] = nBeginPoint;
	secctr = 1;
	minctr = 0;
	hourctr =0;

	OMX_U32 nReadLen = AUDIO_PARSER_READ_SIZE;
	OMX_U32 nActuralRead, nSegmentCnt = PARSERAUDIO_VBRDURATION;
	OMX_U32 nDataSize = nEndPoint - nBeginPoint;
	OMX_U32 nReadPointTmp = 0;
	for (OMX_U32 i = 0; ; i ++)
	{
		if (nReadPointTmp + nReadLen > nDataSize)
		{
			nReadLen = nDataSize - nReadPointTmp;
		}

		LOG_LOG("Read: %lld\n", nBeginPoint + nReadPointTmp);
		nSource2CurPos = fileOps.Tell(sourceFileHandle2, this);
		nActuralRead = fileOps2.Read(sourceFileHandle2, pTmpBuffer, nReadLen, this);

		nReadPointTmp += nActuralRead;

		ParserAudioFrame(pTmpBuffer, nActuralRead, nSegmentCnt);

		if (nActuralRead < AUDIO_PARSER_READ_SIZE || bStopVBRDurationThread == OMX_TRUE)
		{
			LOG_DEBUG("Audio file parser reach end.\n");
			break;
		}
	}

	if (fileOps2.Close(sourceFileHandle2, this) != 0)
	{
		LOG_ERROR("Can't close input file.\n");
		return OMX_ErrorUndefined;
	}

	AudioParserFreeBuffer(pTmpBuffer);

	nAudioDuration = ((hourctr * 60 + minctr) * 60 + (secctr-1)) * (OMX_U64) OMX_TICKS_PER_SECOND + \
					 (OMX_U64)nOneSecondSample * OMX_TICKS_PER_SECOND / nSampleRate;
	usDuration = nAudioDuration;
	LOG_DEBUG("Audio Duration: %lld\n", usDuration);

	bVBRDurationReady = OMX_TRUE;

	return ret;
}

OMX_ERRORTYPE AudioParserBase::ParserAudioFrame(OMX_U8 *pBuffer, OMX_U32 nActuralRead, OMX_U32 nSegmentCnt)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U8 *pBuffer2 = pBuffer;
	OMX_U32 nActuralRead2;

	pBuffer2 -= nOverlap;
	nActuralRead2 = nActuralRead + nOverlap;

	ParserAudioFrameOverlap(pBuffer2, nActuralRead2, nSegmentCnt);

	if (nOverlap < FileInfo.nFrameHeaderSize)
	{
		nOverlap = FileInfo.nFrameHeaderSize;
	}
	else if (nOverlap > AUDIO_PARSER_READ_SIZE>>1)
	{
		LOG_DEBUG("Over lap audio data wrong.\n");
		nOverlap = AUDIO_PARSER_READ_SIZE>>1;
	}

	LOG_LOG("Audio parser overlap data length: %d\n", nOverlap);
	fsl_osal_memcpy(pBuffer - nOverlap, pBuffer+nActuralRead-nOverlap, nOverlap);

	return ret;
}

OMX_ERRORTYPE AudioParserBase::ParserAudioFrameOverlap(OMX_U8 *pBuffer, OMX_U32 nActuralRead, OMX_U32 nSegmentCnt)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 offset = 0;

	while (offset < nActuralRead)
	{
		ParserFrame(&FrameInfo, pBuffer+offset, nActuralRead-offset);
		if (FrameInfo.bGotOneFrame == E_FSL_OSAL_FALSE)
		{
			nOverlap = nActuralRead - offset;
			LOG_DEBUG("nOverlap: %d\n", nOverlap);
			break;
		}

		OMX_U32 nFrameLen = FrameInfo.nFrameHeaderConsumed + FrameInfo.nFrameSize;
		LOG_LOG("nFrameLen: %d\n", nFrameLen);

		if (FrameInfo.nFrameCount == 1)
		{
			if (nSegmentCnt == PARSERAUDIO_HEAD || nSegmentCnt == PARSERAUDIO_BEGINPOINT)
			{
				nBeginPointOffset = offset + FrameInfo.nFrameHeaderConsumed - FileInfo.nFrameHeaderSize;
				bBeginPointFounded = OMX_TRUE;
			}
			if (nSegmentCnt == PARSERAUDIO_BEGINPOINT)
			{
				break;
			}
		}

		if (nSegmentCnt == PARSERAUDIO_VBRDURATION )
		{
			AudioParserBuildSeekTable(offset - nOverlap + FrameInfo.nFrameHeaderConsumed - \
					FileInfo.nFrameHeaderSize, FrameInfo.nSamplesPerFrame, FrameInfo.nSamplingRate);
		}
		offset += nFrameLen;

		nTotalBitRate += FrameInfo.nBitRate * 1000;
	}

	return ret;
}

OMX_U8 *AudioParserBase::AudioParserGetBuffer(OMX_U32 nBufferSize)
{
	LOG_DEBUG("Audio parser malloc tmp buffer size: %d\n", nBufferSize*2);
	pTmpBufferPtr = (OMX_U8 *)FSL_MALLOC(nBufferSize*2);
	if (pTmpBufferPtr == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return NULL;
	}

	nTotalBitRate = 0;
	FrameInfo.nFrameCount = 0;
	nOverlap = 0;

	return pTmpBufferPtr+nBufferSize;
}

OMX_ERRORTYPE AudioParserBase::AudioParserFreeBuffer(OMX_U8 *pBuffer)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	FSL_FREE(pTmpBufferPtr);

	return ret;
}

OMX_U32 AudioParserBase::GetAvrageBitRate()
{
	if (FrameInfo.nFrameCount != 0)
	{
		nAvrageBitRate = nTotalBitRate / FrameInfo.nFrameCount;
		LOG_DEBUG("nAvrageBitRate: %d\n", nAvrageBitRate);
	}
	else 
	{
		nAvrageBitRate = 0;
	}

	return nAvrageBitRate;
}

OMX_ERRORTYPE AudioParserBase::AudioParserBuildSeekTable(OMX_S32 nOffset, OMX_U32 nSamples, OMX_U32 nSamplingRate)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	//static int iCnt = 0;
	//iCnt ++;
	//printf("iCnt = %d\n", iCnt);
	//printf("nSamplingRate = %d\n", nSamplingRate);

	nOneSecondSample += nSamples;
	if (nSamplingRate)
		nSampleRate = nSamplingRate;
	LOG_LOG("nOneSecondSample: %d\t nSamples: %d\n", nOneSecondSample, nSamples);
	LOG_LOG("Frame bound: %lld\n", nSource2CurPos + nOffset);

	if (nOneSecondSample >= nSamplingRate)
	{
		SeekTable[hourctr][minctr][secctr] = nSource2CurPos + nOffset;
		LOG_LOG("Seek Tabel: %lld\n", nSource2CurPos + nOffset);
		nOneSecondSample -= nSamplingRate;

		secctr ++;
		if(secctr == 60)
		{
			minctr ++;
			secctr = 0;
			if(minctr == 60)
			{
				hourctr ++;
				minctr = 0;
				if(hourctr >= MAXAUDIODURATIONHOUR)
				{
					LOG_ERROR("Audio duration is biger than 1024 hour, can't build seek table.\n");
					return ret;
				}

				SeekTable[hourctr] = (OMX_U64 **)FSL_MALLOC(60 * sizeof(OMX_U64 *));
				if(SeekTable[hourctr] == NULL)
				{
					LOG_ERROR("Failed in allocate memory for seek table\n");
					return OMX_ErrorInsufficientResources;
				}
				fsl_osal_memset(SeekTable[hourctr], 0, 60 * sizeof(OMX_U64 *));
			}

			SeekTable[hourctr][minctr] = (OMX_U64 *)FSL_MALLOC(60 * sizeof(OMX_U64));
			if(SeekTable[hourctr][minctr] == NULL)
			{
				LOG_ERROR("Failed in allocate memory for seek table\n");
				return OMX_ErrorInsufficientResources;
			}
			fsl_osal_memset(SeekTable[hourctr][minctr], 0, 60 * sizeof(OMX_U64));
		}
	}

	return ret;
}

OMX_ERRORTYPE AudioParserBase::SetupPortMediaFormat()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

	if (ports[AUDIO_OUTPUT_PORT] == NULL)
		return ret;

	OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	sPortDef.nPortIndex = AUDIO_OUTPUT_PORT;
	ports[AUDIO_OUTPUT_PORT]->GetPortDefinition(&sPortDef);

    OMX_AUDIO_CODINGTYPE CodingType = OMX_AUDIO_CodingUnused;

	CodingType = GetAudioCodingType();

	sPortDef.format.audio.eEncoding = CodingType;
	ports[AUDIO_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
        nNumAvailAudioStream = 1;
	SendEvent(OMX_EventPortFormatDetected, AUDIO_OUTPUT_PORT, 0, NULL);
	SendEvent(OMX_EventPortSettingsChanged, AUDIO_OUTPUT_PORT, 0, NULL);

	sPortDef.nPortIndex = VIDEO_OUTPUT_PORT;
	ports[VIDEO_OUTPUT_PORT]->GetPortDefinition(&sPortDef);
        sPortDef.format.video.eCompressionFormat = OMX_VIDEO_CodingMax;
	ports[VIDEO_OUTPUT_PORT]->SetPortDefinition(&sPortDef);
        nNumAvailVideoStream = 0;
	SendEvent(OMX_EventPortFormatDetected, VIDEO_OUTPUT_PORT, 0, NULL);

	return ret;
}

OMX_ERRORTYPE AudioParserBase::InstanceDeInit()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	if(pThreadId != NULL) {
		bStopVBRDurationThread = OMX_TRUE;
		fsl_osal_thread_destroy(pThreadId);
	}

	ret = AudioParserInstanceDeInit();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Audio parser instance de-init fail.\n");
		return ret;
	}

	if (fileOps.Close(sourceFileHandle, this) != 0)
	{
		LOG_ERROR("Can't close input file.\n");
		return OMX_ErrorUndefined;
	}
	sourceFileHandle = NULL;

    return ret;
}

OMX_ERRORTYPE AudioParserBase::AudioParserInstanceDeInit()
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	// Free seek table.
	OMX_U32 nSec, nMin, nHour;

	for (nHour=0; nHour<MAXAUDIODURATIONHOUR; nHour++)
	{
		if(SeekTable[nHour] != NULL)
		{
			for(nMin=0; nMin<60; nMin++)
				if(SeekTable[nHour][nMin] != NULL)
					FSL_FREE(SeekTable[nHour][nMin]);
			FSL_FREE(SeekTable[nHour]);
		}
		else
			break;
	}

	return ret;
}

OMX_ERRORTYPE AudioParserBase::AudioParserSetCodecConfig(OMX_BUFFERHEADERTYPE *pOutBuffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return ret;
}
 
OMX_ERRORTYPE AudioParserBase::GetNextSample(Port *pPort, OMX_BUFFERHEADERTYPE *pOutBuffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 nReadLen = pOutBuffer->nAllocLen;
	OMX_U32 nActuralRead;

	if (bNeedSendCodecConfig == OMX_TRUE)
	{
		bNeedSendCodecConfig = OMX_FALSE;
		AudioParserSetCodecConfig(pOutBuffer);
	}
	else
	{
		if (nReadPoint + nReadLen > nEndPoint)
		{
			nReadLen = nEndPoint - nReadPoint;
			pOutBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
			LOG_DEBUG("Audio parser send EOS.\n");
		}

		nActuralRead = fileOps.Read(sourceFileHandle, pOutBuffer->pBuffer, nReadLen, this);

		if (nActuralRead == 0)
			pOutBuffer->nFlags |= OMX_BUFFERFLAG_EOS;

		pOutBuffer->nFilledLen = nActuralRead;
		LOG_DEBUG("Audio parser send length = %d\n", nActuralRead);
		pOutBuffer->nOffset = 0;
		if (bSegmentStart == OMX_TRUE)
		{
			pOutBuffer->nFlags |= OMX_BUFFERFLAG_STARTTIME;
			bSegmentStart = OMX_FALSE;
			LOG_DEBUG("Audio parser ts = %lld\n", sAudioSeekPos);
			LOG_DEBUG("Audio begin data = %p\n", *((OMX_U32 *)pOutBuffer->pBuffer));
		}
		nReadPoint += nActuralRead;
	}
	pOutBuffer->nTimeStamp = sAudioSeekPos;

	return ret;
}

OMX_S64 AudioParserBase::nGetTimeStamp(OMX_S64 *pSeekPoint)
{
	return sAudioSeekPos;
}

OMX_U64 AudioParserBase::FrameBound(OMX_U64 nSkip)
{
	return nSkip;
}

OMX_ERRORTYPE AudioParserBase::DoSeek(OMX_U32 nPortIndex, OMX_U32 nSeekFlag)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

	if (nPortIndex != AUDIO_OUTPUT_PORT)
	{
		sActualVideoSeekPos = sAudioSeekPos;
		return ret;
	}

	LOG_DEBUG("Seek Position: %lld\t Audio Duration: %lld\n", sAudioSeekPos, nAudioDuration);
	if (sAudioSeekPos > nAudioDuration)
	{
		LOG_WARNING("Seek time is bigger than audio duration.\n");
		return ret;
	}

	LOG_DEBUG("bCBR = %d\t bVBRDurationReady = %d\n", bCBR, bVBRDurationReady);
	OMX_S64 nSeekPoint;
	if (bCBR == OMX_TRUE || bVBRDurationReady == OMX_FALSE)
	{
		OMX_U64 nSkip = (nEndPoint - nBeginPoint) * sAudioSeekPos / nAudioDuration;
		nSeekPoint = nBeginPoint + FrameBound(nSkip);
		LOG_DEBUG("Stream Len = %lld\n", (nEndPoint - nBeginPoint));
		nGetTimeStamp(&nSeekPoint);
		LOG_DEBUG("After adjust Seek Position: %lld\t Audio Duration: %lld\n", sAudioSeekPos, nAudioDuration);
	} else if (bTOCSeek == OMX_TRUE) {
		float percent = (float)sAudioSeekPos * 100 / nAudioDuration;
		float fx;
		if( percent <= 0.0f ) {
			fx = 0.0f;
		} else if( percent >= 100.0f ) {
			fx = 256.0f;
		} else {
			OMX_S32 a = (OMX_S32)percent;
			float fa, fb;
			if ( a == 0 ) {
				fa = 0.0f;
			} else {
				fa = (float)FrameInfo.FrameInfo.TOC[a];
			}
			if ( a < 99 ) {
				fb = (float)FrameInfo.FrameInfo.TOC[a+1];
			} else {
				fb = 256.0f;
			}
			fx = fa + (fb-fa)*(percent-a);
		}
		nSeekPoint = nBeginPoint + (OMX_S32)((1.0f/256.0f)*fx*FrameInfo.FrameInfo.total_bytes);
		LOG_DEBUG("Seek Point = %lld\n", nSeekPoint);
	} else {
		OMX_U32 nSec = (sAudioSeekPos / OMX_TICKS_PER_SECOND) % 60;
		OMX_U32 nMin = (sAudioSeekPos / (OMX_TICKS_PER_SECOND * 60)) % 60;
		OMX_U32 nHour = sAudioSeekPos / (OMX_TICKS_PER_SECOND * 60) / 60;

		nSeekPoint = SeekTable[nHour][nMin][nSec];
		LOG_DEBUG("Seek Point = %lld\n", nSeekPoint);
	}

	fileOps.Seek(sourceFileHandle, nSeekPoint, SEEK_SET, this);
	nReadPoint = nSeekPoint;
	bSegmentStart = OMX_TRUE;
	sActualAudioSeekPos = sAudioSeekPos;
	sVideoSeekPos = sAudioSeekPos;
	sActualVideoSeekPos = sAudioSeekPos;

	return OMX_ErrorNone;
}


/* File EOF */
