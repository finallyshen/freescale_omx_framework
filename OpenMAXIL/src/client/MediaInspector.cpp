/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "OMX_Core.h"
#include "MediaInspector.h"
#include "Mem.h"
#include "Log.h"
#include "ShareLibarayMgr.h"
#include "common/AudioCoreParser.h"
#include "GMComponent.h"

#define DETECT_BUFFER_SIZE 64
#define MKV_BUFFER_SIZE 50
#define MP3_BUFFER_SIZE 4096
#define AAC_BUFFER_SIZE 4096

#define MKV_RECTYPE "matroska"
#define MKV_WEBM_RECTYPE "webm"
#define MKV_EBML_ID_HEADER 0xA3DF451A

static const CPbyte ASF_GUID[16] = {
    0x30,0x26,0xb2,0x75,0x8e,0x66,0xcf,0x11,0xa6,0xd9,0x0,0xaa,0x0,0x62,0xce,0x6c
};

typedef struct {
    CP_PIPETYPE *hPipe;
    CPhandle hContent;
    MediaType type;
    OMX_STRING sContentURI;
    OMX_BOOL streaming;
}CONTENTDATA;


typedef AUDIO_PARSERRETURNTYPE (*PARSE_HEADER)(AUDIO_FILE_INFO *pFileInfo, uint8 *pBuffer, uint32 nBufferLen);
typedef AUDIO_PARSERRETURNTYPE (*PARSE_FRAME)(AUDIO_FRAME_INFO *pFrameInfo, uint8 *pBuffer, uint32 nBufferLen);

OMX_ERRORTYPE ReadContent(CONTENTDATA *pData, CPbyte *buffer, OMX_S32 buffer_size)
{
    if(pData->streaming == OMX_TRUE) {
        CPresult nContentPipeResult = 0;
        CP_CHECKBYTESRESULTTYPE eReady = CP_CheckBytesOk; 
        while(1) {
            nContentPipeResult = pData->hPipe->CheckAvailableBytes(pData->hContent, buffer_size, &eReady);
            if (nContentPipeResult == 0 && eReady != CP_CheckBytesInsufficientBytes)
                break;
            fsl_osal_sleep(5000);
        }
    }

    pData->hPipe->Read(pData->hContent, buffer, buffer_size);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE DetectAudioType(
        CONTENTDATA *pData,
        OMX_S32 buffer_size,
        OMX_STRING parse_header_name,
        OMX_STRING parse_frame_name,
        OMX_STRING lib_name)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    ShareLibarayMgr *libMgr = NULL;
    OMX_PTR hLib = NULL;
    PARSE_HEADER parse_header = NULL;
    PARSE_FRAME parse_frame = NULL;
    CPbyte *buffer = NULL;
    CPint64 pos = 0, posEnd = 0, nSeekPos = 0;
    OMX_S32 Id3Size = 0;

    libMgr = FSL_NEW(ShareLibarayMgr, ());
    if(libMgr == NULL)
        return OMX_ErrorInsufficientResources;

    hLib = libMgr->load(lib_name);
    if(hLib == NULL) {
        FSL_DELETE(libMgr);
        return OMX_ErrorUndefined;
    }

    parse_header = (PARSE_HEADER)libMgr->getSymbol(hLib, parse_header_name);
    parse_frame = (PARSE_FRAME)libMgr->getSymbol(hLib, parse_frame_name);
    if(parse_header == NULL || parse_frame == NULL) {
        libMgr->unload(hLib);
        FSL_DELETE(libMgr);
        return OMX_ErrorUndefined;
    }

    buffer = (CPbyte*)FSL_MALLOC(buffer_size * sizeof(CPbyte));
    if(buffer == NULL) {
        libMgr->unload(hLib);
        FSL_DELETE(libMgr);
        return OMX_ErrorInsufficientResources;
    }
    fsl_osal_memset(buffer, 0, buffer_size*sizeof(CPbyte));

    if(0 != pData->hPipe->Open(&(pData->hContent), pData->sContentURI, CP_AccessRead)) {
        LOG_ERROR("Can't open content: %s\n", (char*)(pData->sContentURI));
        libMgr->unload(hLib);
        FSL_DELETE(libMgr);
        FSL_FREE(buffer);
        return OMX_ErrorUndefined;
    }

    if(0 != pData->hPipe->GetPosition(pData->hContent, &pos)) {
        libMgr->unload(hLib);
        FSL_DELETE(libMgr);
        FSL_FREE(buffer);
        pData->hPipe->Close(pData->hContent);
        return OMX_ErrorUndefined;
    }

    ReadContent(pData, buffer, buffer_size);

	if (!fsl_osal_memcmp((void *)("ID3"), buffer, 3)) {
		// Skip the ID3v2 header.
		size_t len =
			((buffer[6] & 0x7f) << 21)
			| ((buffer[7] & 0x7f) << 14)
			| ((buffer[8] & 0x7f) << 7)
			| (buffer[9] & 0x7f);

		if (len > 3 * 1024 * 1024)
			len = 3 * 1024 * 1024;

		len += 10;

		Id3Size = len;
	}

	LOG_DEBUG("Id3Size: %d\n", Id3Size);
	pData->hPipe->SetPosition(pData->hContent, 0, CP_OriginEnd);
	pData->hPipe->GetPosition(pData->hContent, &posEnd);
	LOG_DEBUG("File size: %lld\n", posEnd);
	nSeekPos = Id3Size + ((posEnd - pos - Id3Size) >> 1);
	LOG_DEBUG("Read from: %lld\n", nSeekPos);
	pData->hPipe->SetPosition(pData->hContent, nSeekPos, CP_OriginBegin);
	fsl_osal_memset(buffer, 0, buffer_size*sizeof(CPbyte));
	ReadContent(pData, buffer, buffer_size);

	AUDIO_FRAME_INFO FrameInfo;
    fsl_osal_memset(&FrameInfo, 0, sizeof(AUDIO_FRAME_INFO));
    ret = OMX_ErrorUndefined;

    LOG_DEBUG("Audio frame detecting try at begining.\n");
    parse_frame(&FrameInfo, (uint8*)buffer, buffer_size);
    if(FrameInfo.bGotOneFrame) {
        LOG_DEBUG("Audio Frame detected.\n");
        ret = OMX_ErrorNone;
    }

    pData->hPipe->Close(pData->hContent);
    FSL_FREE(buffer);
    libMgr->unload(hLib);
    FSL_DELETE(libMgr);

    return ret;
}

OMX_ERRORTYPE TryLoadComponent(
        CONTENTDATA *pData, 
        OMX_STRING role)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_S32 len = fsl_osal_strlen(pData->sContentURI) + 1;
    OMX_PTR ptr = FSL_MALLOC(sizeof(OMX_PARAM_CONTENTURITYPE) + len);
    if(ptr == NULL)
        return OMX_ErrorInsufficientResources;

    OMX_PARAM_CONTENTURITYPE *Content = (OMX_PARAM_CONTENTURITYPE*)ptr;
    if(fsl_osal_strncmp(pData->sContentURI, "file://", 7) == 0)
        fsl_osal_strcpy((fsl_osal_char*)(&Content->contentURI), pData->sContentURI + 7);
    else
        fsl_osal_strcpy((fsl_osal_char*)(&Content->contentURI), pData->sContentURI);

    GMComponent *pComponent = FSL_NEW(GMComponent, ());
    if(pComponent == NULL)
        goto err;

    ret = pComponent->Load(role);
    if(ret != OMX_ErrorNone) {
        goto err;
    }

    ret = OMX_SetParameter(pComponent->hComponent, OMX_IndexParamContentURI, Content);
    if(ret != OMX_ErrorNone)
        goto err;

    OMX_PARAM_CONTENTPIPETYPE sPipe;
    OMX_INIT_STRUCT(&sPipe, OMX_PARAM_CONTENTPIPETYPE);
    sPipe.hPipe = pData->hPipe;
    ret = OMX_SetParameter(pComponent->hComponent, OMX_IndexParamCustomContentPipe, &sPipe);
    if(ret != OMX_ErrorNone)
        goto err;

    ret = pComponent->StateTransUpWard(OMX_StateIdle);
    if(ret != OMX_ErrorNone)
        goto err;

    ret = pComponent->StateTransUpWard(OMX_StatePause);
    if(ret != OMX_ErrorNone) {
        pComponent->StateTransDownWard(OMX_StateLoaded);
        goto err;
    }

    pComponent->StateTransDownWard(OMX_StateIdle);
    pComponent->StateTransDownWard(OMX_StateLoaded);

err:
    if(pComponent->hComponent) {
        pComponent->UnLoad();
        FSL_DELETE(pComponent);
    }

    if(Content)
        FSL_FREE(Content);

    return ret;
}

OMX_ERRORTYPE TryFlacType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("TryFlacType\n");

    pData->type = TYPE_NONE;
    if (buffer[0] == 'f' && buffer[1] == 'L' && buffer[2] == 'a' && buffer[3] == 'C') {
        pData->type = TYPE_FLAC;
        LOG_DEBUG("Content type is flac.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryMp3Type(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_STRING lib = (OMX_STRING )"lib_mp3_parser_v2_arm11_elinux.so";
    OMX_STRING parse_header = (OMX_STRING) "Mp3ParserFileHeader";
    OMX_STRING parse_frame = (OMX_STRING) "Mp3ParserFrame";

    LOG_DEBUG("TryMp3Type\n");

    pData->type = TYPE_NONE;
    if(OMX_ErrorNone == DetectAudioType(pData, MP3_BUFFER_SIZE, parse_header, parse_frame, lib)) {
        pData->type = TYPE_MP3;
        LOG_DEBUG("Content type is mp3.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryAacType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_STRING lib = (OMX_STRING)"lib_aac_parser_arm11_elinux.so";
    OMX_STRING parse_header = (OMX_STRING)"AacParserFileHeader";
    OMX_STRING parse_frame = (OMX_STRING)"AacParserFrame";

    LOG_DEBUG("TryAacType\n");

    pData->type = TYPE_NONE;
    if(OMX_ErrorNone == DetectAudioType(pData, AAC_BUFFER_SIZE, parse_header, parse_frame, lib)) {
        pData->type = TYPE_AAC;
        LOG_DEBUG("Content type is aac adts.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryAACADIFType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("TryAACADIFType\n");

    pData->type = TYPE_NONE;
    if (buffer[0] == 'A' && buffer[1] == 'D' && buffer[2] == 'I' && buffer[3] == 'F') {
        pData->type = TYPE_AAC;
        LOG_DEBUG("Content type is aac adif.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryAc3Type(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_STRING lib = (OMX_STRING)"lib_ac3_parser_arm11_elinux.so";
    OMX_STRING parse_header = (OMX_STRING)"Ac3ParserFileHeader";
    OMX_STRING parse_frame = (OMX_STRING)"Ac3ParserFrame";

    LOG_DEBUG("TryAc3Type\n");

    pData->type = TYPE_NONE;
    if(0) {//OMX_ErrorNone == DetectAudioType(pData, AAC_BUFFER_SIZE, parse_header, parse_frame, lib)) {
        pData->type = TYPE_AC3;
        LOG_DEBUG("Content type is ac3.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}


OMX_ERRORTYPE TryAviType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("TryAviType\n");

    pData->type = TYPE_NONE;
    if (buffer[0] == 'R' && buffer[1] == 'I' && buffer[2] == 'F' && buffer[3] == 'F' &&
            buffer[8] == 'A' && buffer[9] == 'V' && buffer[10] == 'I' && buffer[11] == ' ' ) {
        pData->type = TYPE_AVI;
        LOG_DEBUG("Content type is avi.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryMp4Type(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("TryMp4Type\n");

    pData->type = TYPE_NONE;
    if ((buffer[4] == 'f' && buffer[5] == 't' && buffer[6] == 'y' && buffer[7] == 'p')
            || (buffer[4] == 'm' && buffer[5] == 'o' && buffer[6] == 'o' && buffer[7] == 'v')
            || (buffer[4] == 's' && buffer[5] == 'k' && buffer[6] == 'i' && buffer[7] == 'p')
            || (buffer[4] == 'm' && buffer[5] == 'd' && buffer[6] == 'a' && buffer[7] == 't')) {
        pData->type = TYPE_MP4;
        LOG_DEBUG("Content type is mp4.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryRmvbType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("TryRmvbType\n");

    pData->type = TYPE_NONE;
    if (buffer[0] == '.' && buffer[1] == 'R' && buffer[2] == 'M' && buffer[3] == 'F') {
        pData->type = TYPE_RMVB;
        LOG_DEBUG("Content type is rmvb.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryMkvType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 temp = 0;
    OMX_BOOL is_matroska = OMX_FALSE;
    OMX_BOOL has_matroska = OMX_FALSE;

    LOG_DEBUG("TryMkvType\n");

    pData->type = TYPE_NONE;

    temp = *(OMX_S32*)buffer;
    if(temp == MKV_EBML_ID_HEADER)
        is_matroska = OMX_TRUE;

    OMX_S32 i;
    for (i = 0; i <= MKV_BUFFER_SIZE - (OMX_S32)(sizeof(MKV_RECTYPE)-1); i++) {
        if (!fsl_osal_memcmp((const fsl_osal_ptr)(buffer + i), (const fsl_osal_ptr)(MKV_RECTYPE), sizeof(MKV_RECTYPE)-1)) {
            has_matroska = OMX_TRUE;
            break;
        }
    }

#ifdef MX6X
    if(is_matroska == OMX_TRUE && has_matroska == OMX_FALSE)
    {
        //search syntax for ".webm"
        for (i = 0; i <= MKV_BUFFER_SIZE - (OMX_S32)(sizeof(MKV_WEBM_RECTYPE)-1); i++) {	
            if (!fsl_osal_memcmp((const fsl_osal_ptr)(buffer + i), (const fsl_osal_ptr)(MKV_WEBM_RECTYPE), sizeof(MKV_WEBM_RECTYPE)-1)) {
                has_matroska = OMX_TRUE;
                break;
            }
        }
    }	
#endif

    if(is_matroska == OMX_TRUE && has_matroska == OMX_TRUE) {
        pData->type = TYPE_MKV;
        LOG_DEBUG("Content type is mkv.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryAsfType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("TryAsfType\n");

    pData->type = TYPE_NONE;
    if (!fsl_osal_memcmp((const fsl_osal_ptr)buffer, (const fsl_osal_ptr) ASF_GUID, 16)) {
        pData->type = TYPE_ASF;
        LOG_DEBUG("Content type is asf.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryWavType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("TryWavType\n");

    pData->type = TYPE_NONE;
    if (buffer[0] == 'R' && buffer[1] == 'I' && buffer[2] == 'F' && buffer[3] == 'F' &&
            buffer[8] == 'W' && buffer[9] == 'A' && buffer[10] == 'V' && buffer[11] == 'E' ) {
        pData->type = TYPE_WAV;
        LOG_DEBUG("Content type is wav.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryHttpliveType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("TryHttpliveType\n");

    pData->type = TYPE_NONE;

    if (!fsl_osal_strncmp(buffer, "#EXTM3U", 7)) {
        pData->type = TYPE_HTTPLIVE;
        LOG_DEBUG("Content type is Httplive.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryFlvType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("TryFlvType\n");

    pData->type = TYPE_NONE;
    if (buffer[0] == 'F' && buffer[1] == 'L' && buffer[2] == 'V'){
        pData->type = TYPE_FLV;
        LOG_DEBUG("Content type is flv.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryMpg2type(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    pData->type = TYPE_NONE;
    if(OMX_ErrorNone == TryLoadComponent(pData, (OMX_STRING)"parser.mpg2")) {
        pData->type = TYPE_MPG2;
        LOG_DEBUG("Content type is mpg2.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}

OMX_ERRORTYPE TryOggType(CONTENTDATA *pData, CPbyte *buffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOG_DEBUG("TryOggType\n");

    pData->type = TYPE_NONE;
    if (buffer[0] == 'O' && buffer[1] == 'g' && buffer[2] == 'g' && buffer[3] == 'S' ) {
        pData->type = TYPE_OGG;
        LOG_DEBUG("Content type is ogg.\n");
    }
    else
        ret = OMX_ErrorUndefined;

    return ret;
}


OMX_ERRORTYPE GetContentPipe(CONTENTDATA *pData)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    CP_PIPETYPE *Pipe = NULL;

    if(fsl_osal_strstr((fsl_osal_char*)pData->sContentURI, "sharedfd://"))
        ret = OMX_GetContentPipe((void**)&Pipe, (OMX_STRING)"SHAREDFD_PIPE");
    else if(fsl_osal_strstr((fsl_osal_char*)pData->sContentURI, "http://")) {
        pData->streaming = OMX_TRUE;
        ret = OMX_GetContentPipe((void**)&Pipe, (OMX_STRING)"HTTPS_PIPE");
    }
    else
        ret = OMX_GetContentPipe((void**)&Pipe, (OMX_STRING)"LOCAL_FILE_PIPE_NEW");

    if(Pipe == NULL)
        return OMX_ErrorContentPipeCreationFailed;

    pData->hPipe = Pipe;

    return ret;
}

typedef OMX_ERRORTYPE (*TRYTYPEFUNC)(CONTENTDATA *pData, CPbyte *buffer);

static TRYTYPEFUNC TryFunc[] = {
    TryAviType,
    TryMp4Type,
    TryMkvType,
    TryAsfType,
    TryFlvType,
    TryRmvbType,
    TryAACADIFType,
    TryOggType,
    TryFlacType,
    TryWavType,
    TryHttpliveType,
    TryMpg2type,
    TryMp3Type,
    TryAacType,
    TryAc3Type
};

MediaType GetContentType(OMX_STRING contentURI)
{
    CONTENTDATA sData;
    CPresult ret = 0;
    int fd, mMediaType = 0;
    int64_t offset, len;

    if(fsl_osal_strstr((fsl_osal_char*)(contentURI), "sharedfd://")) {
        sscanf(contentURI, "sharedfd://%d:%lld:%lld:%d", &fd, &offset, &len, &mMediaType);
        if (mMediaType)
            return (MediaType)mMediaType;
    }

    OMX_Init();

    fsl_osal_memset(&sData, 0, sizeof(CONTENTDATA));
    sData.type = TYPE_NONE;
    sData.sContentURI = contentURI;
    if(OMX_ErrorNone != GetContentPipe(&sData))
        return TYPE_NONE;

    ret = sData.hPipe->Open(&sData.hContent, contentURI, CP_AccessRead);
    if(ret != 0) {
        LOG_ERROR("Can't open content: %s\n", contentURI);
        return TYPE_NONE;
    }

    CPbyte buffer[DETECT_BUFFER_SIZE];
    fsl_osal_memset(buffer, 0, DETECT_BUFFER_SIZE);
    ReadContent(&sData, buffer, DETECT_BUFFER_SIZE);

    sData.hPipe->Close(sData.hContent);

    for(OMX_U32 i=0; i<sizeof(TryFunc)/sizeof(TRYTYPEFUNC); i++) {
        if(OMX_ErrorNone == (*TryFunc[i])(&sData, buffer))
            break;
    }

    OMX_Deinit();

    return sData.type;
}

typedef struct {
    const char *key;
    TRYTYPEFUNC TryFunc;
}TYPECONFIRM;

static TYPECONFIRM TypeConfirm[] = {
    {"parser.avi", TryAviType},
    {"parser.mp4", TryMp4Type},
    {"parser.mkv", TryMkvType},
    {"parser.asf", TryAsfType},
    {"parser.flv", TryFlvType},
    {"parser.rmvb", TryRmvbType},
    {"parser.aac", TryAACADIFType},
    {"parser.ogg", TryOggType},
    {"parser.flac", TryFlacType},
    {"parser.wav", TryWavType},
    {"parser.httplive", TryHttpliveType},
    {"parser.mpg2", TryMpg2type},
    {"parser.mp3", TryMp3Type},
    {"parser.aac", TryAacType},
    {"parser.ac3",TryAc3Type}
};

OMX_STRING MediaTypeConformByContent(OMX_STRING contentURI, OMX_STRING role)
{
    MediaType type = TYPE_NONE;
    CONTENTDATA sData;
    CPresult ret = 0;
    OMX_STRING confirmed_role = NULL;

    OMX_Init();

    fsl_osal_memset(&sData, 0, sizeof(CONTENTDATA));
    sData.sContentURI = contentURI;
    if(OMX_ErrorNone != GetContentPipe(&sData))
        return NULL;

    ret = sData.hPipe->Open(&sData.hContent, contentURI, CP_AccessRead);
    if(ret != 0) {
        LOG_ERROR("Can't open content: %s\n", contentURI);
        return NULL;
    }

    CPbyte buffer[DETECT_BUFFER_SIZE];
    fsl_osal_memset(buffer, 0, DETECT_BUFFER_SIZE);
    ReadContent(&sData, buffer, DETECT_BUFFER_SIZE);

    sData.hPipe->Close(sData.hContent);

    for(OMX_U32 i=0; i<sizeof(TypeConfirm)/sizeof(TYPECONFIRM); i++) {
        const char *key = TypeConfirm[i].key;
        TRYTYPEFUNC func = TypeConfirm[i].TryFunc;
        if(fsl_osal_strcmp(role, key) == 0) {
            if(OMX_ErrorNone == (*func)(&sData, buffer)) {
                confirmed_role = role;
				break;
			}
        }
    }

    OMX_Deinit();

    return confirmed_role;
}


/* File EOF */
