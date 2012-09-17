/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file fsl_osal_linux_file.h
 *
 * @brief Defines the OS Abstraction layer APIs for file handling in Linux
 *
 * @ingroup osal
 */

#ifdef ANDROID_BUILD

//#define USE_LIBC_IF

 #include <string.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/stat.h>
 #include <sys/vfs.h>
#ifndef USE_LIBC_IF
 #include <fcntl.h>
#endif
 #include "Log.h"
 #include "fsl_osal.h"

//#define LOG_DEBUG printf
/*! Opens a file.
 *
 *	@param [in] path
 *		The location of file
 *	@param [in] mode
 *		Specifies whether read only, read write etc
 *	@param [out] file_handle
 *		Handle of the opened file
 *	@return efsl_osal_return_type
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 *		E_FSL_OSAL_INVALIDMODE if mode is invalid
 *		E_FSL_OSAL_UNAVAILABLE if the file is not found
 */
efsl_osal_return_type_t fsl_osal_fopen(const fsl_osal_char *path,
									 const fsl_osal_char *mode,
									 fsl_osal_file *file_handle)
 {
#ifdef USE_LIBC_IF
 	FILE *fp;
 	fp = fopen(path, mode);
 	if(fp == NULL)
 	{
 		//LOG_ERROR("\n Error in opening the file.");
 		return E_FSL_OSAL_UNAVAILABLE;
 	}
 	*file_handle = (void *)fp;
#else
        int fd;
        fd = open(path, O_LARGEFILE | O_RDONLY);
        if(fd < 0)
 	{
 		LOG_DEBUG("\n Error in opening the file.");
 		return E_FSL_OSAL_UNAVAILABLE;
 	}
        *file_handle = (void *)fd;
#endif
 	return E_FSL_OSAL_SUCCESS;
 }

/*! Closes the given file.
 *
 *	@param [in] file_handle
 *		Handle of the opened file
 *	@return efsl_osal_return_type
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 */
efsl_osal_return_type_t fsl_osal_fclose(fsl_osal_file file_handle)
 {
#ifdef USE_LIBC_IF
 	FILE *fp;
 	if(file_handle == NULL)
 	{
 		LOG_ERROR("\n NULL pointer.");
 		return E_FSL_OSAL_INVALIDPARAM;
 	}
 	fp = (FILE *)file_handle;
 	clearerr(fp);
 	if(fclose(fp) != 0)
 	{
		LOG_ERROR("\n Error in closing file.");
		return E_FSL_OSAL_UNKNOWN;
	}
#else
        int mFd = (int)file_handle;
        if (mFd >= 0) {
            close(mFd);
        }
#endif
 	return E_FSL_OSAL_SUCCESS;
 }

/*! Read from a file.
 *
 *	@param [out] buffer
 *		The read operation will fill the data in this buffer
 *	@param [in] size
 *		Size of data to read.
 *	@param [in] file_handle
 *		The file handle .
 *	@param [out] actual_size
 *		The actual size read.
 *	@return efsl_osal_return_type
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 *		E_FSL_OSAL_EOF end of file reached
 *		E_FSL_OSAL_FAILURE in case of error.
 */
efsl_osal_return_type_t fsl_osal_fread(fsl_osal_ptr buffer,
									 fsl_osal_s32 size,
									 fsl_osal_file file_handle,
									 fsl_osal_s32 *actual_size)
 {
#ifdef USE_LIBC_IF
 	FILE *fp;

 	if(file_handle == NULL)
 	{
 		LOG_ERROR("\n NULL pointer.");
 		return E_FSL_OSAL_INVALIDPARAM;
 	}
 	fp = (FILE *)file_handle;
 	clearerr(fp);
  	
 	*actual_size = fread(buffer,1,size,fp);

	if(feof(fp) != 0)
 	{
		LOG_DEBUG("\n End of file reached.");
		return E_FSL_OSAL_EOF;
	}
	
 	if(ferror(fp) != 0)
 	{
		LOG_ERROR("\n Error in reading from file.");
		return E_FSL_OSAL_INVALIDPARAM;
	}
#else
        int ret;
        int mFd = (int)file_handle;

 	if(mFd < 0 )
 	{
 		LOG_ERROR("\n Invalid fd.");
 		return E_FSL_OSAL_INVALIDPARAM;
 	}
        ret = ::read(mFd, (void *)buffer, size);
        if(ret < 0)
 	{
		LOG_ERROR("\n Error in reading from file.");
		return E_FSL_OSAL_FAILURE;
	}

        *actual_size = ret;
#endif
 	return E_FSL_OSAL_SUCCESS;
 }

/*! Writes the given buffer to the specified file.
 *
 *	@param [in] buffer
 *		Data to write
 *	@param [in] size
 *		Size of data to write.
 *	@param [in] file_handle
 *		The file handle .
 *	@param [out] actual_size
 *		The actual size writen.
 *	@return efsl_osal_return_type
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 *		E_FSL_OSAL_INVALIDMODE if mode is invalid
 *		E_FSL_OSAL_FAILURE in case of error.
 */
efsl_osal_return_type_t fsl_osal_fwrite(const fsl_osal_ptr buffer,
									  fsl_osal_s32 size,
									  fsl_osal_file file_handle,
									  fsl_osal_s32 *actual_size)
 {
#ifdef USE_LIBC_IF
 	FILE *fp;

 	if(file_handle == NULL)
 	{
 		LOG_ERROR("\n NULL pointer.");
 		return E_FSL_OSAL_INVALIDPARAM;
 	}
 	fp = (FILE*)file_handle;
 	clearerr(fp);
 	*actual_size = fwrite(buffer ,1, size, fp);
 	if(ferror(fp) != 0)
 	{
		LOG_ERROR("\n Error in closing file.");
		return E_FSL_OSAL_INVALIDPARAM;
	}
	fflush(fp); 
#else
        int ret;
        int mFd = (int)file_handle;
 	if(mFd < 0 )
 	{
 		LOG_ERROR("\n Invalid fd.");
 		return E_FSL_OSAL_INVALIDPARAM;
 	}
 	ret = ::write(mFd, buffer, size);
        if(ret < 0)
 	{
		LOG_ERROR("\n Error in reading from file.");
		return E_FSL_OSAL_FAILURE;
	}

        *actual_size = ret;
#endif

 	return E_FSL_OSAL_SUCCESS;
 }

/*! The fsl_osal_fseek function sets the file position indicator for the file
 *  referred by file_handle.  The new position is obtained by adding offset bytes
 *	to the position specified by whence.
 *
 *	@param [in] file_handle
 *		The file handle .
 *	@param [in] offset
 *		offset in number of bytes.
 *	@param [in] whence
 *		Reefence position.   .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid or if the file is not seakable.
 *		E_FSL_OSAL_FAILURE in case of error.
 */

efsl_osal_return_type_t fsl_osal_fseek(fsl_osal_file file_handle,
									   fsl_osal_s64 offset,
									   efsl_osal_seek_pos_t whence)
{ 
#ifdef USE_LIBC_IF
	if(file_handle == NULL)
	{
		LOG_ERROR(" Error.Passing NULL pointer.\n");
		return E_FSL_OSAL_INVALIDPARAM;
	}
	if (whence == E_FSL_OSAL_SEEK_SET)
	{
		if(fseeko((FILE *)file_handle, offset, SEEK_SET) != 0)
		{
			LOG_ERROR(" Error in fseek.\n");
			return E_FSL_OSAL_INVALIDPARAM;
		}
	}
	else if(whence == E_FSL_OSAL_SEEK_END)
	{
		if(fseeko((FILE *)file_handle, offset, SEEK_END) != 0)
		{
			LOG_ERROR(" Error in fseek.\n");
			return E_FSL_OSAL_INVALIDPARAM;
		}
	}
	else if(whence == E_FSL_OSAL_SEEK_CUR)
	{
		if(fseeko((FILE *)file_handle, offset, SEEK_CUR) != 0)
		{
			LOG_ERROR(" Error in fseek.\n");
			return E_FSL_OSAL_INVALIDPARAM;
		}
	}
	else
	{
		LOG_ERROR(" Error. Invalid seek type.");
		return E_FSL_OSAL_INVALIDPARAM;
	}
#else
        int mFd = (int)file_handle;
        if(mFd < 0)
	{
 		LOG_ERROR("\n Invalid fd.");
		return E_FSL_OSAL_INVALIDPARAM;
	}
	if (whence == E_FSL_OSAL_SEEK_SET)
	{
                off64_t result = lseek64(mFd, offset, SEEK_SET);
                if (result == -1) {
                    LOG_ERROR("seek to %lld failed", offset);
                    return E_FSL_OSAL_FAILURE;
                }
	}
	else if(whence == E_FSL_OSAL_SEEK_END)
	{
                off64_t result = lseek64(mFd, offset, SEEK_END);
                if (result == -1) {
                    LOG_ERROR("seek to %lld failed", offset);
                    return E_FSL_OSAL_FAILURE;
                }
	}
	else if(whence == E_FSL_OSAL_SEEK_CUR)
	{
                off64_t result = lseek64(mFd, offset, SEEK_CUR);
                if (result == -1) {
                    LOG_ERROR("seek to %lld failed", offset);
                    return E_FSL_OSAL_FAILURE;
                }
	}
	else
	{
		LOG_ERROR(" Error. Invalid seek type.");
		return E_FSL_OSAL_INVALIDPARAM;
	}
#endif

	return E_FSL_OSAL_SUCCESS;
}


/*! The fsl_osal_ftell function gets the current file position indicator for the file.
 *
 *	@param [in] file_handle
 *		The file handle
 *	@param [out] offset
 *		The current offset
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE in case of error.
 */
efsl_osal_return_type_t fsl_osal_ftell(fsl_osal_file file_handle,
									   fsl_osal_s64 *offset)
{
#ifdef USE_LIBC_IF
	fsl_osal_s32 current_offset = 0;
	current_offset = ftello((FILE *)file_handle);
	if(current_offset == -1)
		return E_FSL_OSAL_FAILURE;
	*offset = current_offset;
#else
        int mFd = (int)file_handle;
        if(mFd < 0)
	{
            LOG_ERROR("\n Invalid fd.");
            return E_FSL_OSAL_INVALIDPARAM;
	}

        *offset = lseek64(mFd, 0, SEEK_CUR);
        if (*offset == -1) {
            LOG_ERROR("ftell failed");
            return E_FSL_OSAL_FAILURE;
        }

#endif
	return E_FSL_OSAL_SUCCESS;
}


/*! The fsl_osal_fflush function forces a write of buffered data.
 *
 *	@param [in] file_handle
 *		The file handle .  .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
efsl_osal_return_type_t fsl_osal_fflush(fsl_osal_file file_handle)
{
#ifdef USE_LIBC_IF
	if(file_handle == NULL)
	{
		LOG_ERROR(" Error.Passing NULL pointer.\n");
		return E_FSL_OSAL_INVALIDPARAM;
	}

	if (fflush((FILE *)file_handle) != 0)
	{
		LOG_ERROR(" Error while flushing. \n");
		return E_FSL_OSAL_INVALIDPARAM;
	}
#else

        int mFd = (int)file_handle;
        if (mFd < 0) {
            LOG_ERROR("\n Invalid fd.");
            return E_FSL_OSAL_INVALIDPARAM;
        }
        fsync(mFd);
#endif
	return E_FSL_OSAL_SUCCESS;
}

/*! The fsl_osal_fsize function gets the size of the specified file
 *
 *	@param [in] file_handle
 *		The file handle .  .
 *	@param [out] actual_size
 *		The file size .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 *		E_FSL_OSAL_FAILURE if operation fails
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fsize(fsl_osal_file file_handle, fsl_osal_s64 *file_size)
{
        struct stat st;

#ifdef USE_LIBC_IF
	if (fstat(fileno((FILE *)file_handle), &st) != 0)
		return E_FSL_OSAL_FAILURE;
	
#else
        int mFd = (int)file_handle;
        if (mFd < 0) {
            LOG_ERROR("\n Invalid fd.");
            return E_FSL_OSAL_INVALIDPARAM;
        }

	if (fstat(mFd, &st) != 0)
            return E_FSL_OSAL_FAILURE;
#endif
	*file_size = st.st_size;
	return E_FSL_OSAL_SUCCESS;
}


/*! The fsl_osal_fexist function checks whether the specified file exists
 *
 *	@param [in] file_handle
 *		The file handle .  .
 *	@param [out] result
 *		The file exist or not .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fexist(const char *file_name, fsl_osal_s32 *result)
{
	struct stat st;
	
	*result = (stat(file_name,&st) == 0);

	return E_FSL_OSAL_SUCCESS;
}


/*! The fsl_osal_fsize function gets the size of the specified file
 *
 *	@param [in] file_handle
 *		The file handle .  .
 *	@param [out] actual_size
 *		The file size .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fspace(const char *path_name, fsl_osal_u64 *avail_space)
{
	struct statfs buf;
	
	if(statfs(path_name, &buf) < 0)
		return E_FSL_OSAL_FAILURE;
	
	*avail_space = (long long)buf.f_bsize * buf.f_bavail;
	
	return E_FSL_OSAL_SUCCESS;
}


/*! The fsl_osal_feof function  tests the end-of-file indicator
 *	for the stream pointed to by stream
 *
 *	@param [in] file_handle
 *		The file handle .
 *	@param [out] result
 *		The return value of feof call.
 *
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 *		E_FSL_OSAL_FAILURE if operation fails
 */
efsl_osal_return_type_t fsl_osal_feof(fsl_osal_file file_handle, fsl_osal_s32 *result)
{
#ifdef USE_LIBC_IF
	if(file_handle == NULL)
	{
		LOG_ERROR(" Error.Passing NULL pointer.\n");
		return E_FSL_OSAL_INVALIDPARAM;
	}

	*result = feof((FILE *)file_handle);
#else
        int ret;
        fsl_osal_s64 pos, size;
        int mFd = (int)file_handle;

        if (mFd < 0) {
            LOG_ERROR("\n Invalid fd.");
            return E_FSL_OSAL_INVALIDPARAM;
        }


        if(fsl_osal_fsize(file_handle, &size)< 0){
            return E_FSL_OSAL_FAILURE;
        }
        
        if(fsl_osal_ftell(file_handle, &pos)< 0){
            return E_FSL_OSAL_FAILURE;
        }
        
        if(pos >= size)
            *result = 1;
        else
            *result = 0;
#endif
	return E_FSL_OSAL_SUCCESS;
}

/*! The fsl_osal_mkdir API creates a directory named pathname
 *
 *	@param [in] pathname
 *		 directory name .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_NOTSUPPORTED if directory creation is not supported/permitted
 *		E_FSL_OSAL_UNAVAILABLE if resources requred for creation are not available
 */
efsl_osal_return_type_t fsl_osal_mkdir(const char *pathname)
{
	FILE *fp;
	char cmd[256];

	fp = fopen(pathname, "r");
	if (fp == 0)
	{
        strcpy(cmd, "mkdir -p ");
        strcat(cmd, pathname);
        return (efsl_osal_return_type_t)system(cmd);
	}
	else
		fclose(fp);
	return E_FSL_OSAL_SUCCESS;
}


efsl_osal_return_type_t fsl_osal_init()
{
	return E_FSL_OSAL_SUCCESS;
}

/*! De-Initialize the OSAL system
 *
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success otherwise E_FSL_OSAL_UNAVAILABLE
 */
efsl_osal_return_type_t fsl_osal_deinit()
{
	return E_FSL_OSAL_SUCCESS;
}

#else

#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

 #include <string.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/stat.h>
 #include <sys/vfs.h>
 #include "Log.h"
 #include "fsl_osal.h"

/*! Opens a file.
 *
 *	@param [in] path
 *		The location of file
 *	@param [in] mode
 *		Specifies whether read only, read write etc
 *	@param [out] file_handle
 *		Handle of the opened file
 *	@return efsl_osal_return_type
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 *		E_FSL_OSAL_INVALIDMODE if mode is invalid
 *		E_FSL_OSAL_UNAVAILABLE if the file is not found
 */
efsl_osal_return_type_t fsl_osal_fopen(const fsl_osal_char *path,
									 const fsl_osal_char *mode,
									 fsl_osal_file *file_handle)
 {
 	FILE *fp;
 	fp = fopen64(path, mode);
 	if(fp == NULL)
 	{
 		//LOG_ERROR("\n Error in opening the file %s.", path);
 		return E_FSL_OSAL_UNAVAILABLE;
 	}
 	*file_handle = (void *)fp;
 	return E_FSL_OSAL_SUCCESS;
 }

/*! Closes the given file.
 *
 *	@param [in] file_handle
 *		Handle of the opened file
 *	@return efsl_osal_return_type
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 */
efsl_osal_return_type_t fsl_osal_fclose(fsl_osal_file file_handle)
 {
 	FILE *fp;
 	if(file_handle == NULL)
 	{
 		LOG_ERROR("\n NULL pointer.");
 		return E_FSL_OSAL_INVALIDPARAM;
 	}
 	fp = (FILE *)file_handle;
 	clearerr(fp);
 	if(fclose(fp) != 0)
 	{
		LOG_ERROR("\n Error in closing file.");
		return E_FSL_OSAL_UNKNOWN;
	}

 	return E_FSL_OSAL_SUCCESS;
 }

/*! Read from a file.
 *
 *	@param [out] buffer
 *		The read operation will fill the data in this buffer
 *	@param [in] size
 *		Size of data to read.
 *	@param [in] file_handle
 *		The file handle .
 *	@param [out] actual_size
 *		The actual size read.
 *	@return efsl_osal_return_type
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 *		E_FSL_OSAL_EOF end of file reached
 */
efsl_osal_return_type_t fsl_osal_fread(fsl_osal_ptr buffer,
									 fsl_osal_s32 size,
									 fsl_osal_file file_handle,
									 fsl_osal_s32 *actual_size)
 {
 	FILE *fp;

 	if(file_handle == NULL)
 	{
 		LOG_ERROR("\n NULL pointer.");
 		return E_FSL_OSAL_INVALIDPARAM;
 	}
 	fp = (FILE *)file_handle;
 	clearerr(fp);
  	
 	*actual_size = fread(buffer,1,size,fp);

	if(feof(fp) != 0)
 	{
		LOG_DEBUG("\n End of file reached.");
		return E_FSL_OSAL_EOF;
	}
	
 	if(ferror(fp) != 0)
 	{
		LOG_ERROR("\n Error in reading from file.");
		return E_FSL_OSAL_INVALIDPARAM;
	}

 	return E_FSL_OSAL_SUCCESS;
 }

/*! Writes the given buffer to the specified file.
 *
 *	@param [in] buffer
 *		Data to write
 *	@param [in] size
 *		Size of data to write.
 *	@param [in] file_handle
 *		The file handle .
 *	@param [out] actual_size
 *		The actual size writen.
 *	@return efsl_osal_return_type
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 *		E_FSL_OSAL_INVALIDMODE if mode is invalid
 */
efsl_osal_return_type_t fsl_osal_fwrite(const fsl_osal_ptr buffer,
									  fsl_osal_s32 size,
									  fsl_osal_file file_handle,
									  fsl_osal_s32 *actual_size)
 {
 	FILE *fp;

 	if(file_handle == NULL)
 	{
 		LOG_ERROR("\n NULL pointer.");
 		return E_FSL_OSAL_INVALIDPARAM;
 	}
 	fp = (FILE*)file_handle;
 	clearerr(fp);
 	*actual_size = fwrite(buffer ,1, size, fp);
	LOG_DEBUG("actual_size: %d input size: %d\n", *actual_size, size);
 	if(ferror(fp) != 0)
 	{
		LOG_ERROR("\n Error in closing file.");
		return E_FSL_OSAL_INVALIDPARAM;
	}
	fflush(fp); 

 	return E_FSL_OSAL_SUCCESS;
 }

/*! The fsl_osal_fseek function sets the file position indicator for the file
 *  referred by file_handle.  The new position is obtained by adding offset bytes
 *	to the position specified by whence.
 *
 *	@param [in] file_handle
 *		The file handle .
 *	@param [in] offset
 *		offset in number of bytes.
 *	@param [in] whence
 *		Reefence position.   .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid or if the file is not seakable.
 */

efsl_osal_return_type_t fsl_osal_fseek(fsl_osal_file file_handle,
									   fsl_osal_s64 offset,
									   efsl_osal_seek_pos_t whence)
{ 
	if(file_handle == NULL)
	{
		LOG_ERROR(" Error.Passing NULL pointer.\n");
		return E_FSL_OSAL_INVALIDPARAM;
	}
	if (whence == E_FSL_OSAL_SEEK_SET)
	{
		if(fseeko64((FILE *)file_handle, offset, SEEK_SET) != 0)
		{
			LOG_ERROR(" Error in fseek.\n");
			return E_FSL_OSAL_INVALIDPARAM;
		}
	}
	else if(whence == E_FSL_OSAL_SEEK_END)
	{
		if(fseeko64((FILE *)file_handle, offset, SEEK_END) != 0)
		{
			LOG_ERROR(" Error in fseek.\n");
			return E_FSL_OSAL_INVALIDPARAM;
		}
	}
	else if(whence == E_FSL_OSAL_SEEK_CUR)
	{
		if(fseeko64((FILE *)file_handle, offset, SEEK_CUR) != 0)
		{
			LOG_ERROR(" Error in fseek.\n");
			return E_FSL_OSAL_INVALIDPARAM;
		}
	}
	else
	{
		LOG_ERROR(" Error. Invalid seek type.");
		return E_FSL_OSAL_INVALIDPARAM;
	}

	return E_FSL_OSAL_SUCCESS;
}


/*! The fsl_osal_ftell function gets the current file position indicator for the file.
 *
 *	@param [in] file_handle
 *		The file handle
 *	@param [out] offset
 *		The current offset
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE in case of error.
 */
efsl_osal_return_type_t fsl_osal_ftell(fsl_osal_file file_handle,
									   fsl_osal_s64 *offset)
{
	fsl_osal_s64 current_offset = 0;
	current_offset = ftello64((FILE *)file_handle);
	if(current_offset == -1)
		return E_FSL_OSAL_FAILURE;
	*offset = current_offset;
	return E_FSL_OSAL_SUCCESS;
}


/*! The fsl_osal_fflush function forces a write of buffered data.
 *
 *	@param [in] file_handle
 *		The file handle .  .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
efsl_osal_return_type_t fsl_osal_fflush(fsl_osal_file file_handle)
{
	if(file_handle == NULL)
	{
		LOG_ERROR(" Error.Passing NULL pointer.\n");
		return E_FSL_OSAL_INVALIDPARAM;
	}

	if (fflush((FILE *)file_handle) != 0)
	{
		LOG_ERROR(" Error while flushing. \n");
		return E_FSL_OSAL_INVALIDPARAM;
	}
	return E_FSL_OSAL_SUCCESS;
}

/*! The fsl_osal_fsize function gets the size of the specified file
 *
 *	@param [in] file_handle
 *		The file handle .  .
 *	@param [out] actual_size
 *		The file size .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fsize(fsl_osal_file file_handle, fsl_osal_s64 *file_size)
{
	struct stat64 st = {0};
	
	if (fstat64(fileno((FILE *)file_handle), &st) != 0)
		return E_FSL_OSAL_FAILURE;
	
	*file_size = st.st_size;
	return E_FSL_OSAL_SUCCESS;
}


/*! The fsl_osal_fexist function checks whether the specified file exists
 *
 *	@param [in] file_handle
 *		The file handle .  .
 *	@param [out] result
 *		The file exist or not .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fexist(const char *file_name, fsl_osal_s32 *result)
{
	struct stat64 st = {0};
	
	*result = (stat64(file_name,&st) == 0);

	return E_FSL_OSAL_SUCCESS;
}


/*! The fsl_osal_fsize function gets the size of the specified file
 *
 *	@param [in] file_handle
 *		The file handle .  .
 *	@param [out] actual_size
 *		The file size .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fspace(const char *path_name, fsl_osal_u64 *avail_space)
{
	struct statfs64 buf={0};
	
	if(statfs64(path_name, &buf) < 0)
		return E_FSL_OSAL_FAILURE;
	
	*avail_space = (long long)buf.f_bsize * buf.f_bavail;
	
	return E_FSL_OSAL_SUCCESS;
}


/*! The fsl_osal_feof function  tests the end-of-file indicator
 *	for the stream pointed to by stream
 *
 *	@param [in] file_handle
 *		The file handle .
 *	@param [out] result
 *		The return value of feof call.
 *
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
efsl_osal_return_type_t fsl_osal_feof(fsl_osal_file file_handle, fsl_osal_s32 *result)
{
	if(file_handle == NULL)
	{
		LOG_ERROR(" Error.Passing NULL pointer.\n");
		return E_FSL_OSAL_INVALIDPARAM;
	}

	*result = feof((FILE *)file_handle);
	return E_FSL_OSAL_SUCCESS;
}

/*! The fsl_osal_mkdir API creates a directory named pathname
 *
 *	@param [in] pathname
 *		 directory name .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_NOTSUPPORTED if directory creation is not supported/permitted
 *		E_FSL_OSAL_UNAVAILABLE if resources requred for creation are not available
 */
efsl_osal_return_type_t fsl_osal_mkdir(const char *pathname)
{
	FILE *fp;
	char cmd[256];

	fp = fopen(pathname, "r");
	if (fp == 0)
	{
        strcpy(cmd, "mkdir -p ");
        strcat(cmd, pathname);
        return (efsl_osal_return_type_t)system(cmd);
	}
	else
		fclose(fp);
	return E_FSL_OSAL_SUCCESS;
}


efsl_osal_return_type_t fsl_osal_init()
{
	return E_FSL_OSAL_SUCCESS;
}

/*! De-Initialize the OSAL system
 *
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success otherwise E_FSL_OSAL_UNAVAILABLE
 */
efsl_osal_return_type_t fsl_osal_deinit()
{
	return E_FSL_OSAL_SUCCESS;
}

#endif

