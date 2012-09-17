/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*!
 * @file OpenMAXIL/src/content_pipe/LocalFilePipe.c
 *
 * @brief Defines content pipe methods for a local file system.
 *
 *	The ContentPipe source contains the methods specific
 *	to the content pipe implementation for a local file sysytem.
 *
 * @ingroup OMX_Content_pipe
 */

#include <errno.h>
#include "LocalFilePipe.h"
#include "fsl_osal.h"
#include "Mem.h"
#include "Log.h"

#define LOCAL_FILE_PIPE_SUCESS 0
#define PATH_NAME_LENGTH 1032

typedef struct _LOCAL_FILE_PIPE
{
	CPint64 nLen;                                 /**< file length  */
	CPint64 nPos;                                 /**< seek position */
	fsl_osal_file pFile;
	CP_ACCESSTYPE eAccess;
	CPbyte path_name[PATH_NAME_LENGTH];
	CPresult (*ClientCallback)(CP_EVENTTYPE eEvent, CPuint iParam);
}LOCAL_FILE_PIPE;



/*! This method is used to Open a local file for reading or writing.

    @param [In] hContent
 				Handle of the content pipe.

 	@param [In] szURI
 				Filename

 	@param [In] eAccess
 				Access type

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_Open(CPhandle* hContent, CPstring szURI, CP_ACCESSTYPE eAccess)
{
	LOCAL_FILE_PIPE *hPipe = NULL;
	fsl_osal_file pFile;
	CPbyte mode[4] = {0};

	if(hContent == NULL || szURI == NULL)
		return KD_EINVAL;
	if(fsl_osal_strlen(szURI) > PATH_NAME_LENGTH)
		return KD_EINVAL;

	switch(eAccess)
	{
		case CP_AccessRead:
			fsl_osal_strcpy((OMX_STRING)mode, "rb");
			break;
		case CP_AccessWrite:
			fsl_osal_strcpy((OMX_STRING)mode, "wb");
			break;
		case CP_AccessReadWrite:
			fsl_osal_strcpy((OMX_STRING)mode, "rb+");
			break;
		default:
			return KD_EINVAL;
	}

    if (fsl_osal_fopen((OMX_STRING)szURI,
    					(OMX_STRING)mode,
    					&pFile) != E_FSL_OSAL_SUCCESS)
    {
		switch(errno)
		{
			case EACCES:
				return KD_EACCES;
			case EBADF:
				return KD_EBADF;
			case EHOSTUNREACH:
				return KD_EHOSTUNREACH;
			case EINVAL:
				return KD_EINVAL;
			case EIO:
				return KD_EIO;
			case EISDIR:
				return KD_EISDIR;
			case EMFILE:
				return KD_EMFILE;
			case ENOENT:
				return KD_ENOENT;
			default:
				return KD_EIO;
		}
	}

    /* allocate memory for component private structure */
    hPipe = (LOCAL_FILE_PIPE *)FSL_MALLOC(sizeof(LOCAL_FILE_PIPE));
    if(hPipe == NULL)
	{
		fsl_osal_fclose(pFile);
		return KD_EIO;
	}

    fsl_osal_memset((fsl_osal_ptr)hPipe , 0, sizeof(LOCAL_FILE_PIPE));

	hPipe->eAccess = eAccess;
	hPipe->pFile = pFile;

	if(eAccess!=CP_AccessWrite)
	{
		if (fsl_osal_fsize(pFile, &hPipe->nLen) != E_FSL_OSAL_SUCCESS)
		{
			fsl_osal_fclose(pFile);
			FSL_FREE(hPipe);
			return KD_EIO;
		}
	}
	if(eAccess==CP_AccessReadWrite)
		hPipe->nPos = hPipe->nLen;

	if(eAccess!=CP_AccessRead)
	{
		CPbyte *pSeperator = fsl_osal_strrchr(szURI, '/');
		if (pSeperator != NULL)
		{
			fsl_osal_strncpy(hPipe->path_name, szURI, PATH_NAME_LENGTH);
			if(pSeperator == szURI)
				pSeperator ++;
			hPipe->path_name[pSeperator-szURI] = '\0';
		}
		else
			fsl_osal_strcpy(hPipe->path_name, ".");
	}

	*hContent = hPipe;

	return LOCAL_FILE_PIPE_SUCESS;
}


/*! This method is used to Close a local file .

    @param [In] hContent
 				Handle of the content pipe.

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_Close(CPhandle hContent)
{
	LOCAL_FILE_PIPE *hPipe = (LOCAL_FILE_PIPE *)hContent;

	if(hContent == NULL)
		return KD_EINVAL;
	
    if(fsl_osal_fclose(hPipe->pFile) != E_FSL_OSAL_SUCCESS)
        return KD_EIO;

	FSL_FREE(hPipe);
	
	return LOCAL_FILE_PIPE_SUCESS;
}


/*! This method is used to create a local content source and open it for writing.

    @param [In] hContent
 				Handle of the content pipe.

 	@param [In] szURI
 				Directory name

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_Create(CPhandle *hContent, CPstring szURI)
{
	LOCAL_FILE_PIPE *hPipe = NULL;
	fsl_osal_file pFile = NULL;
	OMX_S32 exist = 0;

	if(hContent == NULL || szURI == NULL)
		return KD_EINVAL;
	if(fsl_osal_strlen(szURI) > PATH_NAME_LENGTH)
		return KD_EINVAL;

	if(fsl_osal_fexist(szURI, &exist) != E_FSL_OSAL_SUCCESS)
		return KD_EIO;
	if(exist == 1)
		return KD_EEXIST;

    if (fsl_osal_fopen((OMX_STRING)szURI,
    					"wb",
    					&pFile) != E_FSL_OSAL_SUCCESS)
    {
		switch(errno)
		{
			case EACCES:
				return KD_EACCES;
			case EBADF:
				return KD_EBADF;
			case EHOSTUNREACH:
				return KD_EHOSTUNREACH;
			case EINVAL:
				return KD_EINVAL;
			case EIO:
				return KD_EIO;
			case EISDIR:
				return KD_EISDIR;
			case EMFILE:
				return KD_EMFILE;
			case ENOENT:
				return KD_ENOENT;
			default:
				return KD_EIO;
		}
	}

    /* allocate memory for component private structure */
    hPipe = (LOCAL_FILE_PIPE *)FSL_MALLOC(sizeof(LOCAL_FILE_PIPE));
    if(hPipe == NULL)
	{
		fsl_osal_fclose(pFile);
		return KD_EIO;
	}

    fsl_osal_memset((OMX_PTR)hPipe , 0, sizeof(LOCAL_FILE_PIPE));

	hPipe->eAccess = CP_AccessWrite;
	hPipe->pFile = pFile;

	{
		CPbyte *pSeperator = fsl_osal_strrchr(szURI, '/');
		if (pSeperator != NULL)
		{
			fsl_osal_strncpy(hPipe->path_name, szURI, PATH_NAME_LENGTH);
			hPipe->path_name[pSeperator-szURI] = '\0';
		}
		else
			fsl_osal_strcpy(hPipe->path_name, ".");
	}

	*hContent = hPipe;

	return LOCAL_FILE_PIPE_SUCESS;
}


/*! This method is used to Check the that specified number of bytes are available for reading or writing

    @param [In] hContent
 				Handle of the content pipe.

 	@param [In] nBytesRequested
 				number of bytes requested

 	@param [In] *eResult
 				Result

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_CheckAvailableBytes(CPhandle hContent, CPuint nBytesRequested, CP_CHECKBYTESRESULTTYPE *eResult)
{
	LOCAL_FILE_PIPE *hPipe = (LOCAL_FILE_PIPE *)hContent;

	if(hContent == NULL || eResult == NULL)
		return KD_EINVAL;

	if(hPipe->pFile == NULL)
		return KD_EINVAL;

	if(hPipe->eAccess == CP_AccessRead)
	{
		CPuint diff;

		if(hPipe->nPos > hPipe->nLen)
			return KD_EIO;

		diff = hPipe->nLen - hPipe->nPos;

		if( nBytesRequested < diff)
			*eResult = CP_CheckBytesOk;
		else if(nBytesRequested > diff)
            *eResult = CP_CheckBytesInsufficientBytes;
		else
			*eResult = CP_CheckBytesAtEndOfStream;
	}
	else
	{
		OMX_U64 total;
		if(fsl_osal_fspace(hPipe->path_name, &total) != E_FSL_OSAL_SUCCESS)
			return KD_EIO;

		if(total > nBytesRequested)
			*eResult =  CP_CheckBytesOk;
		else
			*eResult =  CP_CheckBytesInsufficientBytes;
	}

   	return LOCAL_FILE_PIPE_SUCESS;
}



/*! This method is used to Seek to certain position in the content relative to the specified origin.

   	@param [In] hContent
 				Handle of the content pipe.

 	@param [In] nOffset
 				offset

 	@param [In] eOrigin
 				Orgin

   	@return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_SetPosition(CPhandle  hContent, CPint64 nOffset, CP_ORIGINTYPE eOrigin)
{
	LOCAL_FILE_PIPE *hPipe = (LOCAL_FILE_PIPE *)hContent;
	efsl_osal_seek_pos_t whence;
	OMX_S64 nPos = nOffset;

	if(hContent == NULL)
		return KD_EINVAL;
	
	switch(eOrigin)
	{
		case CP_OriginBegin:
			whence = E_FSL_OSAL_SEEK_SET;
			break;
		case CP_OriginCur:
			whence = E_FSL_OSAL_SEEK_CUR;
			nPos += hPipe->nPos;
			break;
		case CP_OriginEnd:
			whence = E_FSL_OSAL_SEEK_END;
			nPos += hPipe->nLen;
			break;
		default:
			return KD_EINVAL;
	}

	if( nPos > hPipe->nLen || nPos < 0)
		return KD_EINVAL;

	fsl_osal_s64 nOffset64 = nOffset;
    if (fsl_osal_fseek(hPipe->pFile, nOffset64, whence) != E_FSL_OSAL_SUCCESS)
        return KD_EIO;     

	hPipe->nPos = nPos;

   	return LOCAL_FILE_PIPE_SUCESS;
}


/*! This method is used to Seek to Retrieve the current position relative to the start of the content

    @param [In] hContent
 				Handle of the content pipe.

 	@param [In] nOffset
 				offset

 	@param [Out] *pPosition
 				position

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_GetPosition(CPhandle hContent, CPint64 *pPosition)
{
	LOCAL_FILE_PIPE *hPipe = (LOCAL_FILE_PIPE *)hContent;

	if(hContent == NULL || pPosition == NULL)
		return KD_EINVAL;
	
	if(hPipe->nPos < 0 || hPipe->nPos > hPipe->nLen)
		return KD_EIO;

	*pPosition = hPipe->nPos;

   	return LOCAL_FILE_PIPE_SUCESS;
}


/*! This method is used to Retrieve data of the specified size from the local file

    @param [In] hContent
 				Handle of the content pipe.

 	@param [Out] *pData
 				data

 	@param [In] nSize
 				size of data to be read

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_Read(CPhandle hContent, CPbyte *pData, CPuint nSize)
{
	LOCAL_FILE_PIPE *hPipe = (LOCAL_FILE_PIPE *)hContent;
	OMX_S32 nActualRead = 0;
	efsl_osal_return_type_t status;

	if(hContent == NULL || pData == NULL || nSize == 0)
		return -KD_EINVAL;
	if(hPipe->eAccess != CP_AccessRead && hPipe->eAccess != CP_AccessReadWrite)
		return -KD_EINVAL;

	status = fsl_osal_fread((OMX_PTR)pData,
					nSize,
					hPipe->pFile,
					&nActualRead);
	
	hPipe->nPos += nActualRead;

	if(status != E_FSL_OSAL_SUCCESS && status != E_FSL_OSAL_EOF)
		return -KD_EIO;
	
   	return LOCAL_FILE_PIPE_SUCESS;
}



/*! This method is used to Retrieve a buffer allocated by the pipe that contains the requested number of bytes.

    @param [In] hContent
 				Handle of the content pipe.

 	@param [Out] **ppBuffer
 				Pointer to receive pipe supplied data buffer.

 	@param [In/Out] *nSize
 				Prior to call: number of bytes to read. After call: number of bytes actually read.

 	@param [In] bForbidCopy
 				If set the pipe shall never perform a copy opting instead to provide less bytes than in requested.

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_ReadBuffer(CPhandle hContent, CPbyte **ppBuffer, CPuint *nSize, CPbool bForbidCopy)
{
	LOCAL_FILE_PIPE *hPipe = (LOCAL_FILE_PIPE *)hContent;
	efsl_osal_return_type_t status;

	if(hContent == NULL || ppBuffer == NULL || nSize == NULL)
		return KD_EINVAL;
	if(*nSize == 0 || (hPipe->eAccess != CP_AccessRead && hPipe->eAccess != CP_AccessReadWrite))
		return KD_EINVAL;
		
    /* allocate memory for component private structure */
    *ppBuffer = (CPbyte *)FSL_MALLOC(*nSize);
    if(*ppBuffer == NULL)
        return KD_EIO;

    fsl_osal_memset((OMX_PTR)*ppBuffer, 0, *nSize);

	status = fsl_osal_fread((OMX_PTR)*ppBuffer,
					*nSize,
					hPipe->pFile,
					(fsl_osal_s32 *)nSize);

	hPipe->nPos += *nSize;

	if(status != E_FSL_OSAL_SUCCESS && status != E_FSL_OSAL_EOF)
		return KD_EIO;

   	return LOCAL_FILE_PIPE_SUCESS;
}


/*! This method is used to Release a buffer obtained by ReadBuffer back to the pipe.

    @param [In] hContent
 				Handle of the content pipe.

 	@param [Out] *pBuffer
 				pointer to buffer

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_ReleaseReadBuffer(CPhandle hContent, CPbyte *pBuffer)
{
	LOCAL_FILE_PIPE *hPipe = (LOCAL_FILE_PIPE *)hContent;

	if(hContent == NULL || pBuffer == NULL)
		return KD_EINVAL;
	if(hPipe->eAccess != CP_AccessRead && hPipe->eAccess != CP_AccessReadWrite)
		return KD_EINVAL;
	
	FSL_FREE(pBuffer);
	
   	return LOCAL_FILE_PIPE_SUCESS;
}


/*! This method is used to Write data of the specified size to the content pipe.

    @param [In] hContent
 				Handle of the content pipe.

 	@param [In] *data
 				pointer to data

 	@param [In] nSize
 			size of the data.

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_Write(CPhandle hContent, CPbyte *data, CPuint nSize)
{
	LOCAL_FILE_PIPE *hPipe = (LOCAL_FILE_PIPE *)hContent;
	OMX_S32 nActualWritten = 0;

	if(hContent == NULL || data == NULL || nSize == 0)
		return -KD_EINVAL;
	if(hPipe->eAccess != CP_AccessWrite && hPipe->eAccess != CP_AccessReadWrite)
		return -KD_EINVAL;
	
	if(fsl_osal_fwrite((OMX_PTR)data,
					nSize,
					hPipe->pFile,
					&nActualWritten) != E_FSL_OSAL_SUCCESS)
	{
		hPipe->nPos += nActualWritten;
		if(hPipe->nPos > hPipe->nLen)
			hPipe->nLen = hPipe->nPos;
		if(errno == EACCES)
				return -KD_EACCES;
		else
				return -KD_EIO;
	}

	hPipe->nPos += nActualWritten;
	if(hPipe->nPos > hPipe->nLen)
		hPipe->nLen = hPipe->nPos;
	
	return LOCAL_FILE_PIPE_SUCESS;
}



/*! This method is used to Retrieve a buffer allocated by the pipe used to write data to the content.

    @param [In] hContent
 				Handle of the content pipe.

 	@param [Out] **ppBuffer
 				buffer

 	@param [In] nSize
 				size of the block actually read.

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_GetWriteBuffer(CPhandle hContent, CPbyte **ppBuffer, CPuint nSize)
{
	LOCAL_FILE_PIPE *hPipe = (LOCAL_FILE_PIPE *)hContent;

	if(hContent == NULL || ppBuffer == NULL || nSize == 0)
		return KD_EINVAL;
	if(hPipe->eAccess != CP_AccessWrite && hPipe->eAccess != CP_AccessReadWrite)
		return KD_EINVAL;

    /* allocate memory for component private structure */
    *ppBuffer = (CPbyte *)FSL_MALLOC(nSize);
    if(*ppBuffer == NULL)
        return KD_EIO;
    fsl_osal_memset((OMX_PTR)*ppBuffer , 0, nSize);

   	return LOCAL_FILE_PIPE_SUCESS;
}


/*! This method is used to Deliver a buffer obtained via GetWriteBuffer to the pipe.

    @param [In] hContent
 				Handle of the content pipe.

 	@param [In] *ppBuffer
 				buffer

 	@param [In] nSize
 				size of the block actually read.

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_WriteBuffer(CPhandle hContent, CPbyte *pBuffer, CPuint nFilledSize)
{
	LOCAL_FILE_PIPE *hPipe = (LOCAL_FILE_PIPE *)hContent;
	CPuint nActualWritten = 0;

	if(hContent == NULL || pBuffer == NULL || nFilledSize == 0)
		return KD_EINVAL;
	if(hPipe->eAccess != CP_AccessWrite && hPipe->eAccess != CP_AccessReadWrite)
		return KD_EINVAL;

	if(fsl_osal_fwrite((OMX_PTR)pBuffer,
					nFilledSize,
					hPipe->pFile,
					(fsl_osal_s32 *)&nActualWritten) != E_FSL_OSAL_SUCCESS)
	{
		hPipe->nPos += nActualWritten;
		if(hPipe->nPos > hPipe->nLen)
			hPipe->nLen = hPipe->nPos;
		return KD_EIO;
	}
	
	hPipe->nPos += nActualWritten;
	if(hPipe->nPos > hPipe->nLen)
		hPipe->nLen = hPipe->nPos;

   	return OMX_ErrorNone;
}


/*! This method is used to Register a per-handle client callback with the content pipe.

    @param [In] hContent
 				Handle of the content pipe.

 	@param [In] *ClientCallback
 				function pointer to Callback function

    @return CPresult
 				This will denote the success or failure of the  call.
 */
OMX_LFILE_PIPE_API CPresult LocalFilePipe_RegisterCallback(CPhandle hContent,
							CPresult (*ClientCallback)(CP_EVENTTYPE eEvent, CPuint iParam))
{
	LOCAL_FILE_PIPE *hPipe = NULL;

	if(hContent == NULL || ClientCallback == NULL)
		return KD_EINVAL;
	hPipe = (LOCAL_FILE_PIPE *)hContent;

	hPipe->ClientCallback = ClientCallback;

   	return LOCAL_FILE_PIPE_SUCESS;
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
OMX_LFILE_PIPE_API CPresult LocalFilePipe_Init(CP_PIPETYPE *pipe)
{
	if(pipe == NULL)
		return KD_EINVAL;
	
	pipe->Open 					= LocalFilePipe_Open;
	pipe->Close 				= LocalFilePipe_Close;
	pipe->Create 				= LocalFilePipe_Create;
	pipe->CheckAvailableBytes 	= LocalFilePipe_CheckAvailableBytes;
	pipe->SetPosition			= LocalFilePipe_SetPosition;
	pipe->GetPosition 			= LocalFilePipe_GetPosition;
	pipe->Read 					= LocalFilePipe_Read;
	pipe->ReadBuffer 			= LocalFilePipe_ReadBuffer;
	pipe->ReleaseReadBuffer 	= LocalFilePipe_ReleaseReadBuffer;
	pipe->Write 				= LocalFilePipe_Write;
	pipe->GetWriteBuffer 		= LocalFilePipe_GetWriteBuffer;
	pipe->WriteBuffer 			= LocalFilePipe_WriteBuffer;
	pipe->RegisterCallback		= LocalFilePipe_RegisterCallback;

    return LOCAL_FILE_PIPE_SUCESS;
}

/*EOF*/




