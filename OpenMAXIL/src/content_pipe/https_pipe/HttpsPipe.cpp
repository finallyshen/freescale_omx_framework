/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "HttpsPipe.h"
#include "HttpsCache.h"
#include "Log.h"
#include "Mem.h"

#define HTTPS_PIPE_SUCESS 0
#define CACHE_SIZE 5 //5MB

typedef struct _HTTPS_PIPE
{
    HttpsCache *dataSource;
    CPstring szURI;
    CPint64 nLen;   /**< file length  */
    CPint64 nOffset;   /**< seek position */
    CPresult (*ClientCallback)(CP_EVENTTYPE eEvent, CPuint iParam);
}HTTPS_PIPE;

OMX_HTTPS_PIPE_API CPresult HttpsPipe_Open(CPhandle* hContent, CPstring szURI, CP_ACCESSTYPE eAccess)
{
    HTTPS_PIPE *hPipe = NULL;

    LOG_DEBUG("HTTPS open %s\n", szURI);

    if(hContent == NULL || szURI == NULL)
        return KD_EINVAL;

    if(eAccess != CP_AccessRead)
        return KD_EINVAL;


    /* allocate memory for component private structure */
    hPipe = FSL_NEW(HTTPS_PIPE, ());
    if(hPipe == NULL)
        return KD_EIO;

    fsl_osal_memset((fsl_osal_ptr)hPipe , 0, sizeof(HTTPS_PIPE));
    hPipe->szURI = szURI;
    hPipe->dataSource = FSL_NEW(HttpsCache, ());
    if(hPipe->dataSource == NULL) {
        LOG_ERROR("Failed to New dataSource.\n");
        return KD_EIO;
    }

    if(OMX_ErrorNone != hPipe->dataSource->Create(szURI, 2*1024, 512*CACHE_SIZE)) {
        LOG_ERROR("Failed to create HttpsCache for %s\n", szURI);
        return KD_EIO;
    }

    hPipe->nLen = hPipe->dataSource->GetContentLength();
    hPipe->nOffset = 0;

    LOG_DEBUG("HTTPS content: len: %lld\n", hPipe->nLen);

    *hContent = hPipe;

    return HTTPS_PIPE_SUCESS;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_Close(CPhandle hContent)
{
    HTTPS_PIPE *hPipe = (HTTPS_PIPE *)hContent;

    LOG_DEBUG("HTTPS close.\n");

    if(hContent == NULL)
        return KD_EINVAL;

    if(hPipe->dataSource != NULL) {
        hPipe->dataSource->Destroy();
        FSL_DELETE(hPipe->dataSource);
    }

    FSL_DELETE(hPipe);

    return HTTPS_PIPE_SUCESS;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_Create(CPhandle *hContent, CPstring szURI)
{
    return HttpsPipe_Open(hContent, szURI, CP_AccessRead);
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_CheckAvailableBytes(
        CPhandle hContent, CPuint nBytesRequested, CP_CHECKBYTESRESULTTYPE *eResult)
{
    HTTPS_PIPE *hPipe = (HTTPS_PIPE *)hContent;

    if(hContent == NULL || eResult == NULL)
        return KD_EINVAL;

    if(nBytesRequested == -1) {
        OMX_U32 nAvailabe = hPipe->dataSource->AvailableBytes(hPipe->nOffset);
        *eResult = (CP_CHECKBYTESRESULTTYPE) nAvailabe;
        return HTTPS_PIPE_SUCESS;
    }

    if(hPipe->nLen > 0 && hPipe->nOffset >= hPipe->nLen) {
        *eResult = CP_CheckBytesAtEndOfStream;
        LOG_DEBUG("Data is EOS at [%lld:%d]\n", hPipe->nOffset, nBytesRequested);
        return HTTPS_PIPE_SUCESS;
    }

    if(OMX_TRUE != hPipe->dataSource->AvailableAt(hPipe->nOffset, nBytesRequested)) {
        *eResult = CP_CheckBytesInsufficientBytes;
        LOG_DEBUG("Data is not enough at [%lld:%d]\n", hPipe->nOffset, nBytesRequested);
    }
    else {
        *eResult = CP_CheckBytesOk;
        LOG_DEBUG("Data is enough at [%lld:%d]\n", hPipe->nOffset, nBytesRequested);
    }

    return HTTPS_PIPE_SUCESS;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_SetPosition(
        CPhandle  hContent, CPint64 nOffset, CP_ORIGINTYPE eOrigin)
{
    HTTPS_PIPE *hPipe = (HTTPS_PIPE *)hContent;

    LOG_DEBUG("seek to: %lld, type: %d\n", nOffset, eOrigin);

    if(hContent == NULL)
        return KD_EINVAL;

    if(hPipe->nLen == 0)
        return HTTPS_PIPE_SUCESS;

    switch(eOrigin)
    {
        case CP_OriginBegin:
            hPipe->nOffset = nOffset;
            break;
        case CP_OriginCur:
            hPipe->nOffset += nOffset;
            break;
        case CP_OriginEnd:
            hPipe->nOffset = hPipe->nLen + nOffset;
            break;
        default:
            return KD_EINVAL;
    }

    return HTTPS_PIPE_SUCESS;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_GetPosition(CPhandle hContent, CPint64 *pPosition)
{
    HTTPS_PIPE *hPipe = (HTTPS_PIPE *)hContent;

    if(hContent == NULL || pPosition == NULL)
        return KD_EINVAL;

    *pPosition = hPipe->nOffset;

    return HTTPS_PIPE_SUCESS;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_Read(CPhandle hContent, CPbyte *pData, CPuint nSize)
{
    HTTPS_PIPE *hPipe = (HTTPS_PIPE *)hContent;
    CPint nActualRead = 0;

    if(hContent == NULL || pData == NULL || nSize == 0)
        return -KD_EINVAL;

    nActualRead = hPipe->dataSource->ReadAt(hPipe->nOffset, nSize, pData);
    LOG_DEBUG("Want read %d at offset %d, actual read %d\n", nSize, hPipe->nOffset, nActualRead);
    if(nActualRead <= 0)
        return -KD_EIO;

    hPipe->nOffset += nActualRead;

    return HTTPS_PIPE_SUCESS;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_ReadBuffer(CPhandle hContent, CPbyte **ppBuffer, CPuint *nSize, CPbool bForbidCopy)
{
    return KD_EINVAL;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_ReleaseReadBuffer(CPhandle hContent, CPbyte *pBuffer)
{
    return KD_EINVAL;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_Write(CPhandle hContent, CPbyte *data, CPuint nSize)
{
    return KD_EINVAL;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_GetWriteBuffer(CPhandle hContent, CPbyte **ppBuffer, CPuint nSize)
{
    return KD_EINVAL;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_WriteBuffer(CPhandle hContent, CPbyte *pBuffer, CPuint nFilledSize)
{
    return KD_EINVAL;
}

OMX_HTTPS_PIPE_API CPresult HttpsPipe_RegisterCallback(CPhandle hContent,
        CPresult (*ClientCallback)(CP_EVENTTYPE eEvent, CPuint iParam))
{
    HTTPS_PIPE *hPipe = NULL;

    if(hContent == NULL || ClientCallback == NULL)
        return KD_EINVAL;

    hPipe = (HTTPS_PIPE *)hContent;

    hPipe->ClientCallback = ClientCallback;

    return HTTPS_PIPE_SUCESS;
}

/*! This method is the entry point function to this content pipe implementation. This function
  is registered with core during content pipe registration (Will happen at OMX_Init).
  Later when OMX_GetContentPipe is called for this implementation this entry point function
  is called, and content pipe function pointers are assigned with the implementation function
  pointers.

  @param [InOut] pipe
  pipe is the handle to the content pipe that is passed.

  @return OMX_ERRORTYPE
  This will denote the success or failure of the method call.
  */
OMX_HTTPS_PIPE_API CPresult HttpsPipe_Init(CP_PIPETYPE *pipe)
{
    if(pipe == NULL)
        return KD_EINVAL;

    pipe->Open = HttpsPipe_Open;
    pipe->Close = HttpsPipe_Close;
    pipe->Create = HttpsPipe_Create;
    pipe->CheckAvailableBytes = HttpsPipe_CheckAvailableBytes;
    pipe->SetPosition = HttpsPipe_SetPosition;
    pipe->GetPosition = HttpsPipe_GetPosition;
    pipe->Read = HttpsPipe_Read;
    pipe->ReadBuffer = HttpsPipe_ReadBuffer;
    pipe->ReleaseReadBuffer = HttpsPipe_ReleaseReadBuffer;
    pipe->Write = HttpsPipe_Write;
    pipe->GetWriteBuffer = HttpsPipe_GetWriteBuffer;
    pipe->WriteBuffer = HttpsPipe_WriteBuffer;
    pipe->RegisterCallback = HttpsPipe_RegisterCallback;

    return HTTPS_PIPE_SUCESS;
}

/*EOF*/




