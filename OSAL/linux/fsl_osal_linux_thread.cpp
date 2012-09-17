
/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file fsl_osal_linux_thread.c
 *
 * @brief Defines the OS Abstraction layer APIs for thread functions in Linux
 *
 * @ingroup osal
 */

 #include <unistd.h> 
 #include <fsl_osal.h>
 #include <pthread.h>

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
efsl_osal_return_type_t fsl_osal_thread_create(fsl_osal_ptr *thread, fsl_osal_ptr *ATTR, thread_func func, void *arg)
 {
	/* Create a new thread. The new thread will run the function start_routine. */

    pthread_create((pthread_t *)thread, (const pthread_attr_t *)ATTR, func, arg);
    
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
efsl_osal_return_type_t fsl_osal_thread_destroy(fsl_osal_ptr thread)
 {

	pthread_join((pthread_t)thread, NULL);
 	return E_FSL_OSAL_SUCCESS;
 }

/*! Get the current task/thread id.
 *  *
 *      @param [out] thread id
 *      @return efsl_osal_return_type_t
 *              E_FSL_OSAL_SUCCESS if success
 *              E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 */
efsl_osal_return_type_t fsl_osal_thread_self(fsl_osal_u32 *id)
{
    *id = pthread_self();
    return E_FSL_OSAL_SUCCESS;
}

/*! Sleep the current task/thread.
 *  *
 *      @param [in] delay
 *              delay in milliseconds.
 *      @return efsl_osal_return_type_t
 *              E_FSL_OSAL_SUCCESS if success
 *              E_FSL_OSAL_INVALIDPARAM if paramter is invalid
 */
efsl_osal_return_type_t fsl_osal_sleep(fsl_osal_u32 delay)
{
    usleep(delay);
    return E_FSL_OSAL_SUCCESS;
}

