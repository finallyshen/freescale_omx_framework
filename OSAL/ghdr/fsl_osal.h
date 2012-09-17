/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*!
 * @file fsl_osal.h
 *
 * @brief Defines the OS Abstraction layer APIs
 *
 * @ingroup osal
 */

#ifndef FSL_OSAL_H_
#define FSL_OSAL_H_
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** The OMX_OSAL_API is platform specific definitions used
 *  to declare OMX function prototypes.  They are modified to meet the
 *  requirements for a particular platform */
#ifdef __WINCE_DLL
#   ifdef __OMX_OSAL_EXPORTS
#       define OMX_OSAL_API __declspec(dllexport)
#   else
#       define OMX_OSAL_API __declspec(dllimport)
#   endif
#else
#   ifdef __OMX_OSAL_EXPORTS
#       define OMX_OSAL_API
#   else
#       define OMX_OSAL_API extern
#   endif
#endif


#include <fsl_osal_types.h>
#include <unistd.h>

#if defined ANDROID_BUILD || defined LINUX
    #include <pthread.h>

    #define  FSL_OSAL_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
    #define  FSL_OSAL_RECURSIVE_MUTEX_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER   
    #define  FSL_OSAL_ERRORCHECK_MUTEX_INITIALIZER PTHREAD_ERRORCHECK_MUTEX_INITIALIZER 

    typedef pthread_mutex_t fsl_osal_mutex_t ;
#endif


/////////////////////////////////////////////////////////////////////////////////////////////
///	Memory management APIS
/////////////////////////////////////////////////////////////////////////////////////////////

typedef enum efsl_osal_return_type
{
	E_FSL_OSAL_SUCCESS,		/*!< Successful operation */
	E_FSL_OSAL_FAILURE,		/*!< Erroneous Operation */
	E_FSL_OSAL_UNAVAILABLE, /*!< Requested resource not available */
	E_FSL_OSAL_BUSY,		/*!< Mutex could not be acquired because it was currently locked*/
	E_FSL_OSAL_INVALIDMODE,
	E_FSL_OSAL_INVALIDPARAM,/*!< One of the parameter is invalid */
	E_FSL_OSAL_INVALIDSTATE,/*!< The resource in invalid state.
								 Eg: The calling task did not currently own the mutex */
	E_FSL_OSAL_EOF, 		/*!< End of file reached */
	E_FSL_OSAL_NOTSUPPORTED,/*!< The specified feature is not supported */
	E_FSL_OSAL_UNKNOWN
} efsl_osal_return_type_t;

typedef fsl_osal_ptr fsl_osal_file;

/*! Specifies the reference position for fseek API.
 */
typedef enum efsl_osal_seek_pos
{
	E_FSL_OSAL_SEEK_SET, 	/*!< start of the file */
	E_FSL_OSAL_SEEK_CUR, 	/*!< current position  */
	E_FSL_OSAL_SEEK_END 	/*!< end-of-file */
}efsl_osal_seek_pos_t;

/*! Initialize the OSAL system. OSAL should be initilalized beore calling
 *	any of the osal APIs.
 *
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success otherwise E_FSL_OSAL_UNAVAILABLE
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_init(void);

/*! De-Initialize the OSAL system
 *
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success otherwise E_FSL_OSAL_UNAVAILABLE
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_deinit(void);


/*! Allocates the requested memory.
 *
 *	@param [In] size
 *	    memory size need to allocateed.
 *	@param [out] ptr
 *		pointer to allocated memory if successfully allocated otherwise NULL.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success otherwise E_FSL_OSAL_UNAVAILABLE
 */
OMX_OSAL_API fsl_osal_ptr fsl_osal_malloc_new(fsl_osal_u32 size);

/*! ReAllocates the requested memory.
 *
 *	@param [In] size
 *	    memory size need to allocateed.
 *	@param [In/out] ptr
 *		pointer to allocated memory if successfully allocated otherwise NULL.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success otherwise E_FSL_OSAL_UNAVAILABLE
 */
OMX_OSAL_API fsl_osal_ptr fsl_osal_realloc_new(fsl_osal_ptr ptr, fsl_osal_u32 size);	


/*! Releases the requested memory
 *
 *	@param [In] ptr
 *		memory to be released.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_dealloc(fsl_osal_ptr ptr);

/*!  The  fsl_osal_memset function fills the first size bytes of the memory area
 *	 pointed to by ptr with the constant byte ch.
 *
 *	@param [In] ptr
 *		pointer to memory.
 *	@param [In] ch
 *		The byte to be written.
 *	@param [In] size
 *		Size of memory to be updated.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 */
 OMX_OSAL_API efsl_osal_return_type_t fsl_osal_memset(fsl_osal_ptr ptr,
 										 fsl_osal_char ch,
 										 fsl_osal_u32 size);

/*!  The  fsl_osal_memcpy function copies 'size' bytes from memory area src to memory
 *   area dest. The memory area is not overlappable.
 *
 *	@param [In] dest
 *		destination location.
 *	@param [In] src
 *		source location.
 *	@param [In] size
 *		Size of memory to be copied.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 */
 OMX_OSAL_API efsl_osal_return_type_t fsl_osal_memcpy(fsl_osal_ptr dest,
 										 fsl_osal_ptr src,
 										 fsl_osal_u32 size);

/*!  The  fsl_osal_memmove function copies 'size' bytes from memory area src to memory
 *   area dest. The memory area is overlappable.
 *
 *	@param [In] dest
 *		destination location.
 *	@param [In] src
 *		source location.
 *	@param [In] size
 *		Size of memory to be copied.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 */
 OMX_OSAL_API efsl_osal_return_type_t fsl_osal_memmove(fsl_osal_ptr dest,
 										 fsl_osal_ptr src,
 										 fsl_osal_u32 size);

/*!  The  fsl_osal_memcmp function compare two memory.
 *
 *	@param [In] dest
 *		destination location.
 *	@param [In] src
 *		source location.
 *	@param [In] size
 *		Size of memory to be copied.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 */
 fsl_osal_s32 fsl_osal_memcmp(const fsl_osal_ptr ptr1,
 										 const fsl_osal_ptr ptr2,
 										 fsl_osal_u32 size);

/*! String copy.
 *
 *	@param [out] dest
 *		destination point for string copy.
  *	@param [In] src
 *		source point for string copy.
 *	@return fsl_osal_char
 *		return destination point.
 */

 OMX_OSAL_API fsl_osal_char *fsl_osal_strcpy(fsl_osal_char *dest, const fsl_osal_char *src);
 
/*! String copy.
 *
 *	@param [out] dest
 *		destination point for string copy.
 *	@param [In] src
 *		source point for string copy.
 *	@param [In] n
 *		Length.
 *	@return fsl_osal_char
 *		return destination point.
 */

  OMX_OSAL_API fsl_osal_char *fsl_osal_strncpy(fsl_osal_char *dest, const fsl_osal_char *src, fsl_osal_s32 n);

/*! Search sub string.
 *
 *	@param [In] src1
 *		string wants to be search.
 *	@param [In] src2
 *		the sub string.
 *	@return fsl_osal_char
 *		return sub string position .
 */

 fsl_osal_char *fsl_osal_strstr(const fsl_osal_char *src1, const fsl_osal_char *src2);
 
 /*! String segmentation.
 *
 *	@param [in] s
 *		string will be segmentation.
  *	@param [In] delim
 *		segmentation string.
 *	@return fsl_osal_char
 *		return segmentation point.
 */

  OMX_OSAL_API fsl_osal_char *fsl_osal_strtok(fsl_osal_char *s, const fsl_osal_char *delim);
  
 /*! Thread safe string segmentation.
 *
 *	@param [in] s
 *		string will be segmentation.
  *	@param [In] delim
 *		segmentation string.
 *	@return fsl_osal_char
 *		return segmentation point.
 */

  OMX_OSAL_API fsl_osal_char *fsl_osal_strtok_r(fsl_osal_char *s, const fsl_osal_char *delim, fsl_osal_char ** pLast);
   
 /*! String find.
 *
 *	@param [in] s
 *		string.
  *	@param [In] c
 *		char.
 *	@return fsl_osal_char
 *		return string point.
 */

 OMX_OSAL_API fsl_osal_char *fsl_osal_strrchr(const fsl_osal_char *s, fsl_osal_s32 c);
 
/*! String compare.
 *
 *	@param [In] src1
 *		the first string.
 *	@param [In] src2
 *		the second string.
 *	@return fsl_osal_s32
 *		return 0 if the string is same otherwise nonezero.
 */

 OMX_OSAL_API fsl_osal_s32 fsl_osal_strcmp(const fsl_osal_char *src1, const fsl_osal_char *src2);

/*! String compare.
 *
 *	@param [In] src1
 *		the first string.
 *	@param [In] src2
 *		the second string.
 *	@param [In] n
 *		string size to compare.
 *	@return fsl_osal_s32
 *		return 0 if the string is same otherwise nonezero.
 */

 fsl_osal_s32 fsl_osal_strncmp(const fsl_osal_char *src1, const fsl_osal_char *src2, fsl_osal_s32 n);

 fsl_osal_s32 fsl_osal_strncasecmp(const fsl_osal_char *src1, const fsl_osal_char *src2, fsl_osal_s32 n);

/*! String dump.
 *
 *	@param [In] src
 *		source point for string copy.
 *	@return dest
 *		destination point for string copy.
 *		return destination point.
 */

 fsl_osal_char *fsl_osal_strdup(const fsl_osal_char *src);

 /*! String length.
 *
 *	@param [In] src
 *		the input string.
 *	@return fsl_osal_u32
 *		The length of the input string, not include "\0".
 */

 OMX_OSAL_API fsl_osal_u32 fsl_osal_strlen(const fsl_osal_char *src);

/*! Change char to integrate.
 *
 *	@param [In] src
 *		the input string.
 *	@return fsl_osal_s32
 *		return the value.
 */

 OMX_OSAL_API fsl_osal_s32 fsl_osal_atoi(const fsl_osal_char *src);

/*! Get envirement parameter.
 *
 *	@param [In] name
 *		envirement name.
 *	@return fsl_osal_char
 *		return envirement string.
 */

 OMX_OSAL_API fsl_osal_char *fsl_osal_getenv_new(const fsl_osal_char *name);
 
/////////////////////////////////////////////////////////////////////////////////////////////
///	Task/Thread related APIS: synchronization, communication etc
////////////////////////////////////////////////////////////////////////////////////////////

/*!	OS specific synchronization object */
typedef fsl_osal_ptr fsl_osal_mutex;
typedef fsl_osal_ptr fsl_osal_sem;

typedef enum
{
    fsl_osal_mutex_normal=0,
    fsl_osal_mutex_recursive,
    fsl_osal_mutex_errorcheck,
    fsl_osal_mutex_default
}fsl_osal_mutex_type;

/*! Allocates and Initilizes the mutual exclusion object.
 *
 *	@param [out] sync_obj
 *		The mutex object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_UNAVAILABLE if mutex not available
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_mutex_init(fsl_osal_mutex *sync_obj, fsl_osal_mutex_type type);

/*! De-initializes and Releases the mutex object.
 *
 *	@param [in] sync_obj
 *		The mutex object
 *	@param [out] os_specific_error_code
 *		Updated with OS specific error code if the API fails. This is valid only if
 *		the return value is E_FSL_OSAL_SUCCESS.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_mutex_destroy(fsl_osal_mutex sync_obj);

/*! Acquires the lock.
 *
 *	@param [in] sync_obj
 *		The mutex object
 *	@param [out] os_specific_error_code
 *		Updated with OS specific error code if the API fails. This is valid
 *		only if the return value is E_FSL_OSAL_SUCCESS.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_UNAVAILABLE if mutex is locked the maximum number of times
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_mutex_lock(fsl_osal_mutex sync_obj);

/*! Tries to acquire the lock. This is not a blocking call. E_FSL_OSAL_SUCCESS
 *	indicates that the lock is acquired.
 *
 *	@param [in] sync_obj
 *		The mutex object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 *		E_FSL_OSAL_BUSY if mutex is already locked
 *
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_mutex_trylock(fsl_osal_mutex sync_obj);

/*! Releases the lock.
 *
 *	@param [in] sync_obj
 *		The mutex object
 *	@param [out] os_specific_error_code
 *		Updated with OS specific error code if the API fails. This is valid
 *		only if the return value is E_FSL_OSAL_SUCCESS.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDSTATE if the calling task did not currently own the mutex
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_mutex_unlock(fsl_osal_mutex sync_obj);


/*! Allocates and Initilizes the semaphore object.
 *
 *	@param [out] sem_obj
 *		The semaphore object
 *	@param [in] pshared
 *		This argument indicates whether semaphore is shared across
 *		processes.
 *	@param [in] value
 *		The value indicates the initial value of the
 *		count associated with the semaphore.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_UNAVAILABLE if semaphore not available
 *		E_FSL_OSAL_FAILURE if semaphore initialisation is unsuccessful
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_sem_init(fsl_osal_sem *sem_obj,
										  fsl_osal_s32 pshared,
										  fsl_osal_u32 value);

/*! This call suspends the calling thread until the
 *	semaphore pointed to  by sem_obj has non-zero count.
 *
 *	@param [in] sem_obj
 *		The semaphore object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_sem_wait(fsl_osal_sem sem_obj);

/*! The fsl_osal_sem_trywait call is a non-blocking variant of fsl_osal_sem_wait.
 * 	If the semaphore pointed to by sem_obj has non-zero count, the count is
 *	atomically decreased and fsl_osal_sem_trywait immediately returns 0.
 *	If the semaphore count is zero, fsl_osal_sem_trywait immediately returns with error.
 *
 *	@param [in] sem_obj
 *		The semaphore object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS
 *		E_FSL_OSAL_FAILURE
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_sem_trywait(fsl_osal_sem sem_obj);


/*! The fsl_osal_sem_post method atomically increases the count of the
 *	semaphore pointed to by sem_obj.
 *
 *	@param [in] sem_obj
 *		The semaphore object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS
 *		E_FSL_OSAL_INVALIDPARAM
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_sem_post(fsl_osal_sem sem_obj);


/*! The fsl_osal_sem_getvalue call stores  in  the  location
 *	pointed to by sval the current count of the semaphore pointed to by sem_obj.
 *
 *	@param [in] sem_obj
 *		The semaphore object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS
 *		E_FSL_OSAL_FAILURE
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_sem_getvalue(fsl_osal_sem sem_obj,
											  fsl_osal_s32 *sval);

/*! The fsl_osal_sem_destroy method destroys the semaphore object,
 *	pointed to by sem_obj.
 *
 *	@param [in] sem_obj
 *		The semaphore object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS
 *		E_FSL_OSAL_FAILURE
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_sem_destroy(fsl_osal_sem sem_obj);

#ifndef __WINCE
typedef fsl_osal_ptr (*thread_func)(fsl_osal_ptr);
#else
typedef fsl_osal_u32 (*thread_func)(fsl_osal_ptr);
#endif

/*! Creates a thread.
 *
 *	@param [out] thread
 *		Identifier of newly created thread.
 *	@param [in] start_routine
 *		Function to be run in the new thread.
 *	@param [in] arg
 *		Arguments passed to the start_routine function.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if thread creation is failure
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_thread_create(fsl_osal_ptr *thread,
												fsl_osal_ptr *attr,
												thread_func func,
												void *arg);

/*! De-initializes and Releases the mutex object.
 *
 *	@param [in] sync_obj
 *		The mutex object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_thread_destroy(fsl_osal_ptr thread);

/*! Get the current task/thread id.
 *  *
 *      @param [out] thread id
 *      @return efsl_osal_return_type_t
 *              E_FSL_OSAL_SUCCESS if success
 *              E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_thread_self(fsl_osal_u32 *id);

/////////////////////////////////////////////////////////////////////////////////////////////
///	Input/Output related APIs
/////////////////////////////////////////////////////////////////////////////////////////////
/*! Opens a file.
 *
 *	@param [in] path
 *		The location of file
 *	@param [in] mode
 *		Specifies whether read only, read write etc
 *	@param [out] file_handle
 *		Handle of the opened file
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 *		E_FSL_OSAL_INVALIDMODE if mode is invalid
 *		E_FSL_OSAL_UNAVAILABLE if the file is not found
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fopen(const fsl_osal_char *path,
									   const fsl_osal_char *mode,
									   fsl_osal_file *file_handle);

/*! Closes the given file.
 *
 *	@param [in] file_handle
 *		Handle of the opened file
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fclose(fsl_osal_file file_handle);


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
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 *		E_FSL_OSAL_EOF end of file reached
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fread(fsl_osal_ptr buffer,
									   fsl_osal_s32 size,
									   fsl_osal_file file_handle,
									   fsl_osal_s32 *actual_size);

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
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 *		E_FSL_OSAL_INVALIDMODE if mode is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fwrite(const fsl_osal_ptr buffer,
									    fsl_osal_s32 size,
									    fsl_osal_file file_handle,
									    fsl_osal_s32 *actual_size);


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
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fseek(fsl_osal_file file_handle,
									   fsl_osal_s64 offset,
									   efsl_osal_seek_pos_t whence);

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
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_ftell(fsl_osal_file file_handle,
									   fsl_osal_s64 *offset);

/*! The fsl_osal_fflush function forces a write of buffered data.
 *
 *	@param [in] file_handle
 *		The file handle .  .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fflush(fsl_osal_file file_handle);


/*! The fsl_osal_fdelete function removes the specified file
 *
 *	@param [in] file_name
 *		The file name .  .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fdelete(const char *file_name);


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
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fsize(fsl_osal_file file_handle, fsl_osal_s64 *file_size);


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
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fexist(const char *file_name, fsl_osal_s32 *result);


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
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_fspace(const char *path_name, fsl_osal_u64 *avail_space);


/*! The fsl_osal_mkdir API creates a directory named pathname
 *
 *	@param [in] pathname
 *		 directory name .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_NOTSUPPORTED if directory creation is not supported/permitted
 *		E_FSL_OSAL_UNAVAILABLE if resources requred for creation are not available
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_mkdir(const char *pathname);


/*! The fsl_osal_rmdir API removes a directory specified by pathname
 *
 *	@param [in] pathname
 *		 directory name .
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_NOTSUPPORTED if directory creation is not supported/permitted
 *		E_FSL_OSAL_UNAVAILABLE if resources requred for creation are not available
 */

OMX_OSAL_API efsl_osal_return_type_t fsl_osal_rmdir(const char *pathname);

/*! The fsl_osal_opendir API -  This API opens a directory and returns the directory handle. This handle should be used for other directory related APIs.
 *
 *	@param [in] pathname
 *		 directory name .
 *	@param [out] dir_handle
 *		 directory handle.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDMODE if directory open modes are invalid
 *		E_FSL_OSAL_UNAVAILABLE if resources requred for creation are not available
 *		E_FSL_OSAL_INVALIDSTATE if file system not initialized
 *		E_FSL_OSAL_INVALIDPARAM if parameters are invalid
 */

OMX_OSAL_API efsl_osal_return_type_t fsl_osal_opendir(const char *pathname,fsl_osal_ptr *dir_handle);


/*! The fsl_osal_closedir API -  This API closes the specified directory
 *
 *	@param [in] dir_handle
 *		 directory handle.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDSTATE if file system not initialized
 *		E_FSL_OSAL_INVALIDPARAM if parameters are invalid
 */


OMX_OSAL_API efsl_osal_return_type_t fsl_osal_closedir(fsl_osal_ptr dir_handle);

/*! The fsl_osal_readdir API -  This API reads the open directory and writes the list of subdirectories in an array
 *
 *	@param [in] dir_handle
 *		 directory handle.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDSTATE if file system not initialized
 *
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_readdir(fsl_osal_ptr dir_handle);

typedef struct {
    fsl_osal_u32 sec;
    fsl_osal_u32 usec;
}fsl_osal_timeval;

/*! Get system time.
 *
 *	@param [out] cond
 *		Identifier of newly created time.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if thread creation is failure
 */
efsl_osal_return_type_t fsl_osal_systime(fsl_osal_timeval *time);

/////////////////////////////////////////////////////////////////////////////////////////////
///	Sleep API
/////////////////////////////////////////////////////////////////////////////////////////////
/*! Sleep the current task/thread.
 *
 *	@param [in] delay
 *		Delay in milliseconds.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if parameter is invalid
 */
OMX_OSAL_API efsl_osal_return_type_t fsl_osal_sleep(fsl_osal_u32 delay);

/*! Creates a condition.
 *
 *	@param [out] cond
 *		Identifier of newly created condition.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if thread creation is failure
 */
efsl_osal_return_type_t fsl_osal_cond_create(fsl_osal_ptr *cond);

/*! Destroy a condition.
 *
 *	@param [out] cond
 *		Identifier of newly created condition.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if condition variable destroy is failure
 */
efsl_osal_return_type_t fsl_osal_cond_destroy(fsl_osal_ptr cond);

/*! Timed wait a condition.
 *
 *	@param [out] cond
 *		Identifier of newly created condition.
 *	@param [in] time
 *		Wait time in milliseconds.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if timed wait condition is failure
 */
efsl_osal_return_type_t fsl_osal_cond_timedwait(fsl_osal_ptr cond, fsl_osal_timeval *tv);

/*! Broadcase a condition.
 *
 *	@param [out] cond
 *		Identifier of newly created condition.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_FAILURE if broadcast condition is failure
 */
efsl_osal_return_type_t fsl_osal_cond_broadcast(fsl_osal_ptr cond);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif 
/* File EOF */



