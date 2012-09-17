/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file fsl_osal_linux_mutex.c
 *
 * @brief Defines the OS Abstraction layer APIs for mutex mechanisms in Linux
 *
 * @ingroup osal
 */

 #include <fsl_osal.h>
 #include <pthread.h>
 #include "Log.h"

/*! Allocates and Initilizes the mutual exclusion object.
 *
 *	@param [out] sync_obj
 *		The mutex object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_UNAVAILABLE if mutex not available
 */
efsl_osal_return_type_t fsl_osal_mutex_init(fsl_osal_mutex *sync_obj, fsl_osal_mutex_type type)
{
     pthread_mutexattr_t attr;
     
 	*sync_obj = (pthread_mutex_t *)fsl_osal_malloc_new(sizeof(pthread_mutex_t));
 	if(*sync_obj == NULL)
 	{
 		LOG_ERROR("\n malloc of mutex object failed.");
 		return E_FSL_OSAL_UNAVAILABLE;
 	}
    
    pthread_mutexattr_init(&attr);     
    switch(type)
    {
        case fsl_osal_mutex_normal:
            pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_NORMAL);
            break;
        case fsl_osal_mutex_recursive:
            pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE_NP);
            break;
        case fsl_osal_mutex_errorcheck:
            pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_ERRORCHECK_NP);
            break;
        default:
            pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_NORMAL);
            break;
    }
    
 	pthread_mutex_init((pthread_mutex_t *)(*sync_obj), &attr);

 	return E_FSL_OSAL_SUCCESS;
}

/*! De-initializes and Releases the mutex object.
 *
 *	@param [in] sync_obj
 *		The mutex object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 */
efsl_osal_return_type_t fsl_osal_mutex_destroy(fsl_osal_mutex sync_obj)
 {

 	if (pthread_mutex_destroy((pthread_mutex_t *)sync_obj) != 0)
 	{
 		LOG_ERROR("\n Error in destroying mutex.");
 		return E_FSL_OSAL_INVALIDPARAM ;
 	}

 	fsl_osal_dealloc(sync_obj);

 	return E_FSL_OSAL_SUCCESS;
 }

/*! Acquires the lock.
 *
 *	@param [in] sync_obj
 *		The mutex object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 */
efsl_osal_return_type_t fsl_osal_mutex_lock(fsl_osal_mutex sync_obj)
 {
 	if ( (pthread_mutex_lock((pthread_mutex_t *)sync_obj)) != 0)
 	{
 		LOG_ERROR("\n Error in locking the mutex");
 		return E_FSL_OSAL_INVALIDPARAM ;
 	}

 	return E_FSL_OSAL_SUCCESS;
 }

/*! Tries to acquire the lock. This is not a blocking call. E_FSL_OSAL_SUCCESS
 *	indicates that the lock is acquired.
 *
 *	@param [in] sync_obj
 *		The mutex object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 *		E_FSL_OSAL_BUSY if mutex is already locked
 *
 */
efsl_osal_return_type_t fsl_osal_mutex_trylock(fsl_osal_mutex sync_obj)
 {
 	if ( (pthread_mutex_trylock((pthread_mutex_t *)sync_obj)) != 0)
 	{
 		//LOG_ERROR("\n Error while trying to lock the mutex.");
 		return E_FSL_OSAL_BUSY ;
 	}
 	return E_FSL_OSAL_SUCCESS;
 }

/*! Releases the lock.
 *
 *	@param [in] sync_obj
 *		The mutex object
 *	@param [out] os_specific_error_code
 *		Updated with OS specific error code if the API fails. This is valid
 *		only if the return value is E_FSL_OSAL_SUCCESS.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 *		E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 */
efsl_osal_return_type_t fsl_osal_mutex_unlock(fsl_osal_mutex sync_obj)
 {
 	if ( (pthread_mutex_unlock((pthread_mutex_t *)sync_obj)) != 0)
 	{
 		LOG_ERROR("\n Error while trying to unlock the mutex.");
 		return E_FSL_OSAL_INVALIDPARAM ;
 	}
 	return E_FSL_OSAL_SUCCESS;
 }
