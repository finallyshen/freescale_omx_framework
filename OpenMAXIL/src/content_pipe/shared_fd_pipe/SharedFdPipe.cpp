/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <errno.h>
#include <string.h>
#include "SharedFdPipe.h"
#include "fsl_osal.h"
#include "Mem.h"
#include "Log.h"

#define SFD_PIPE_SUCESS 0

typedef struct _SHARED_FD_PIPE
{
    CPint fd;
    CPint64 nLen;                        /**< file length  */
    CPint64 nBeginPoint;                 /**< file begin point  */
    CPint64 nPos;                        /**< seek position */
	CP_ACCESSTYPE eAccess;
    CPresult (*ClientCallback)(CP_EVENTTYPE eEvent, CPuint iParam);
}SHARED_FD_PIPE;

OMX_SFD_PIPE_API CPresult SharedFdPipe_Open(CPhandle* hContent, CPstring szURI, CP_ACCESSTYPE eAccess)
{
    SHARED_FD_PIPE *hPipe = NULL;
    CPint fd = 0;
    CPint64 len = 0, offset = 0;

    LOG_DEBUG("SFD open %s\n", szURI);

    if(hContent == NULL || szURI == NULL)
        return KD_EINVAL;

    if(sscanf(szURI, "sharedfd://%d:%lld:%lld", (int*)&fd, &offset, &len) != 3)
        return KD_EINVAL;

    /* allocate memory for component private structure */
    hPipe = FSL_NEW(SHARED_FD_PIPE, ());
    if(hPipe == NULL)
        return KD_EIO;

    fsl_osal_memset((fsl_osal_ptr)hPipe , 0, sizeof(SHARED_FD_PIPE));
	hPipe->eAccess = eAccess;
    hPipe->fd = fd;
    hPipe->nLen = len;
    hPipe->nPos = 0;
    hPipe->nBeginPoint = offset;
    hPipe->nPos = lseek64(hPipe->fd, offset, SEEK_SET);
    if(hPipe->nPos < 0) {
        LOG_ERROR("seek to pos: %lld failed, errno: %s\n", offset, strerror(errno));
        return KD_EIO;
    }
	if(eAccess==CP_AccessReadWrite)
		hPipe->nPos = hPipe->nLen;

    LOG_DEBUG("SFD fd:%d, len: %lld, offset: %lld\n", fd, len, offset);

    *hContent = hPipe;

    return SFD_PIPE_SUCESS;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_Close(CPhandle hContent)
{
    SHARED_FD_PIPE *hPipe = (SHARED_FD_PIPE *)hContent;

    LOG_DEBUG("SFD close.\n");

    if(hContent == NULL)
        return KD_EINVAL;

    FSL_DELETE(hPipe);

    return SFD_PIPE_SUCESS;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_Create(CPhandle *hContent, CPstring szURI)
{
    return SharedFdPipe_Open(hContent, szURI, CP_AccessRead);
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_CheckAvailableBytes(
        CPhandle hContent, CPuint nBytesRequested, CP_CHECKBYTESRESULTTYPE *eResult)
{
    SHARED_FD_PIPE *hPipe = (SHARED_FD_PIPE *)hContent;

    if(hContent == NULL || eResult == NULL)
        return KD_EINVAL;

    LOG_DEBUG("%s: nLen: %lld, nPos: %lld, Reqest: %d\n", __FUNCTION__, hPipe->nLen, hPipe->nPos, nBytesRequested);

	if(hPipe->eAccess == CP_AccessRead)
	{
		if(nBytesRequested <= hPipe->nLen + hPipe->nBeginPoint - hPipe->nPos)
			*eResult =  CP_CheckBytesOk;
		else
			*eResult = CP_CheckBytesInsufficientBytes;
	}
	else
	{
		*eResult =  CP_CheckBytesOk;
		return SFD_PIPE_SUCESS;
	}

    return SFD_PIPE_SUCESS;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_SetPosition(
        CPhandle  hContent, CPint64 nOffset, CP_ORIGINTYPE eOrigin)
{
    SHARED_FD_PIPE *hPipe = (SHARED_FD_PIPE *)hContent;
    OMX_S64 nPos = nOffset;
    int whence;

    LOG_DEBUG("seek to: %lld, type: %d\n", nOffset, eOrigin);

    if(hContent == NULL)
        return KD_EINVAL;

    switch(eOrigin)
    {
        case CP_OriginBegin:
            whence = SEEK_SET;
			nOffset += hPipe->nBeginPoint;
			nPos += hPipe->nBeginPoint;
            break;
        case CP_OriginCur:
            whence = SEEK_CUR;
            nPos += hPipe->nPos;
            break;
        case CP_OriginEnd:
            //whence = SEEK_END; 
            nPos += hPipe->nBeginPoint + hPipe->nLen;
			whence = SEEK_SET;
			nOffset += hPipe->nBeginPoint + hPipe->nLen;
            break;
        default:
            return KD_EINVAL;
    }

    if( nPos > hPipe->nBeginPoint + hPipe->nLen || nPos < hPipe->nBeginPoint) {
		if (hPipe->eAccess == CP_AccessWrite \
				&& nPos > hPipe->nBeginPoint + hPipe->nLen) {
			OMX_S32 ret = 0;
			ret = ftruncate(hPipe->fd, nPos);
			if (ret !=0)
				return KD_ENOSPC;
			return SFD_PIPE_SUCESS;

		} else
			return KD_EINVAL;
	}

    hPipe->nPos = lseek64(hPipe->fd, nOffset, whence);
    LOG_DEBUG("%s: seek to %lld\n", __FUNCTION__, hPipe->nPos);

    return SFD_PIPE_SUCESS;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_GetPosition(CPhandle hContent, CPint64 *pPosition)
{
    SHARED_FD_PIPE *hPipe = (SHARED_FD_PIPE *)hContent;

    if(hContent == NULL || pPosition == NULL)
        return KD_EINVAL;

    if(hPipe->nPos < hPipe->nBeginPoint || hPipe->nPos > hPipe->nBeginPoint + hPipe->nLen)
        return KD_EIO;

    LOG_DEBUG("%s: pos %lld\n", __FUNCTION__, hPipe->nPos);

    *pPosition = hPipe->nPos - hPipe->nBeginPoint;

    return SFD_PIPE_SUCESS;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_Read(CPhandle hContent, CPbyte *pData, CPuint nSize)
{
    SHARED_FD_PIPE *hPipe = (SHARED_FD_PIPE *)hContent;
    OMX_S32 nActualRead = 0;
    efsl_osal_return_type_t status;

    if(hContent == NULL || pData == NULL || nSize == 0)
        return -KD_EINVAL;

    if(hPipe->nPos < hPipe->nBeginPoint || hPipe->nPos > hPipe->nBeginPoint + hPipe->nLen)
        return -KD_EIO;

    hPipe->nPos = lseek64(hPipe->fd, hPipe->nPos, SEEK_SET);
	if (hPipe->nPos + nSize > hPipe->nBeginPoint + hPipe->nLen)
		nSize = hPipe->nBeginPoint + hPipe->nLen - hPipe->nPos;
    nActualRead = read(hPipe->fd, pData, nSize);

    LOG_DEBUG("read at pos: %lld, want: %d, actual: %d\n", hPipe->nPos, nSize, nActualRead);

    hPipe->nPos += nActualRead;

    return SFD_PIPE_SUCESS;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_ReadBuffer(CPhandle hContent, CPbyte **ppBuffer, CPuint *nSize, CPbool bForbidCopy)
{
    return KD_EINVAL;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_ReleaseReadBuffer(CPhandle hContent, CPbyte *pBuffer)
{
    return KD_EINVAL;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_Write(CPhandle hContent, CPbyte *data, CPuint nSize)
{
	SHARED_FD_PIPE *hPipe = (SHARED_FD_PIPE *)hContent;
	OMX_S32 nActualWritten = 0;

	if(hContent == NULL || data == NULL || nSize == 0)
		return -KD_EINVAL;
	if(hPipe->eAccess != CP_AccessWrite && hPipe->eAccess != CP_AccessReadWrite)
		return -KD_EINVAL;

	hPipe->nPos = lseek64(hPipe->fd, hPipe->nPos, SEEK_SET);
	nActualWritten = write(hPipe->fd, data, nSize);

    LOG_DEBUG("write at pos: %lld, want: %d, actual: %d\n", hPipe->nPos, nSize, nActualWritten);
	hPipe->nPos += nActualWritten;
	if(hPipe->nPos > hPipe->nLen)
		hPipe->nLen = hPipe->nPos;
	
    return SFD_PIPE_SUCESS;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_GetWriteBuffer(CPhandle hContent, CPbyte **ppBuffer, CPuint nSize)
{
    return KD_EINVAL;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_WriteBuffer(CPhandle hContent, CPbyte *pBuffer, CPuint nFilledSize)
{
    return KD_EINVAL;
}

OMX_SFD_PIPE_API CPresult SharedFdPipe_RegisterCallback(CPhandle hContent,
        CPresult (*ClientCallback)(CP_EVENTTYPE eEvent, CPuint iParam))
{
    SHARED_FD_PIPE *hPipe = NULL;

    if(hContent == NULL || ClientCallback == NULL)
        return KD_EINVAL;

    hPipe = (SHARED_FD_PIPE *)hContent;

    hPipe->ClientCallback = ClientCallback;

    return SFD_PIPE_SUCESS;
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
OMX_SFD_PIPE_API CPresult SharedFdPipe_Init(CP_PIPETYPE *pipe)
{
    if(pipe == NULL)
        return KD_EINVAL;

    pipe->Open = SharedFdPipe_Open;
    pipe->Close = SharedFdPipe_Close;
    pipe->Create = SharedFdPipe_Create;
    pipe->CheckAvailableBytes = SharedFdPipe_CheckAvailableBytes;
    pipe->SetPosition = SharedFdPipe_SetPosition;
    pipe->GetPosition = SharedFdPipe_GetPosition;
    pipe->Read = SharedFdPipe_Read;
    pipe->ReadBuffer = SharedFdPipe_ReadBuffer;
    pipe->ReleaseReadBuffer = SharedFdPipe_ReleaseReadBuffer;
    pipe->Write = SharedFdPipe_Write;
    pipe->GetWriteBuffer = SharedFdPipe_GetWriteBuffer;
    pipe->WriteBuffer = SharedFdPipe_WriteBuffer;
    pipe->RegisterCallback = SharedFdPipe_RegisterCallback;

    return SFD_PIPE_SUCESS;
}

/*EOF*/




