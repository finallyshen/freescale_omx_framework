/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file fsl_osal_linux_sem.c
 *
 * @brief Defines the OS Abstraction layer APIs for semaphore mechanisms in Linux
 *
 * @ingroup osal
 */

 #include <fsl_osal.h>
 #include <semaphore.h>
 #include "Log.h"

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
efsl_osal_return_type_t fsl_osal_sem_init(fsl_osal_sem *sem_obj,
										  fsl_osal_s32 pshared,
										  fsl_osal_u32 value)
{
 	*sem_obj = (sem_t *)fsl_osal_malloc_new(sizeof(sem_t));
 	if(*sem_obj == NULL)
 	{
 		LOG_ERROR("\n Creation of semaphore failed.");
 		return E_FSL_OSAL_UNAVAILABLE;
 	}
 	if(sem_init((sem_t *)(*sem_obj), pshared, value) != 0)
 		return E_FSL_OSAL_FAILURE;

 	return E_FSL_OSAL_SUCCESS;
}

/*! The fsl_osal_sem_destroy method destroys the semaphore object,
 *	pointed to by sem_obj.
 *
 *	@param [in] sem_obj
 *		The semaphore object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS
 *		E_FSL_OSAL_INVALIDPARAM
 */

efsl_osal_return_type_t fsl_osal_sem_destroy(fsl_osal_sem sem_obj)
{
#ifdef ANDROID_BUILD
        /* workaround for android semaphore usage */
        fsl_osal_sem_post(sem_obj);
#endif

 	if (sem_destroy((sem_t *)sem_obj) != 0)
 	{
 		LOG_ERROR("\n Error in destroying semaphore.");
 		return E_FSL_OSAL_INVALIDPARAM ;
 	}

 	fsl_osal_dealloc(sem_obj);
 	return E_FSL_OSAL_SUCCESS;
}

/*! This call suspends the calling thread until the
 *	semaphore pointed to  by sem_obj has non-zero count.
 *
 *	@param [in] sem_obj
 *		The semaphore object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS
 */
efsl_osal_return_type_t fsl_osal_sem_wait(fsl_osal_sem sem_obj)
{
 	sem_wait((sem_t *)sem_obj);
 	return E_FSL_OSAL_SUCCESS;
}

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
efsl_osal_return_type_t fsl_osal_sem_trywait(fsl_osal_sem sem_obj)
{
 	if ( (sem_trywait((sem_t *)sem_obj)) != 0)
 		return E_FSL_OSAL_FAILURE ;

 	return E_FSL_OSAL_SUCCESS;
}

/*! The fsl_osal_sem_post method atomically increases the count of the
 *	semaphore pointed to by sem_obj.
 *
 *	@param [in] sem_obj
 *		The semaphore object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS
 *		E_FSL_OSAL_INVALIDPARAM
 */
efsl_osal_return_type_t fsl_osal_sem_post(fsl_osal_sem sem_obj)
 {
 	if ( (sem_post((sem_t *)sem_obj)) != 0)
 	{
 		LOG_ERROR("\n Error while signalling to the semaphore.");
 		return E_FSL_OSAL_INVALIDPARAM ;
 	}
 	return E_FSL_OSAL_SUCCESS;
 }

/*! The fsl_osal_sem_getvalue call stores  in  the  location
 *	pointed to by sval the current count of the semaphore pointed to by sem_obj.
 *
 *	@param [in] sem_obj
 *		The semaphore object
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS
 *		E_FSL_OSAL_FAILURE
 */
efsl_osal_return_type_t fsl_osal_sem_getvalue(fsl_osal_sem sem_obj,
											  fsl_osal_s32 *sval)
{
 	*sval = sem_getvalue((sem_t *)sem_obj, (int *)sval);
 	return E_FSL_OSAL_SUCCESS;
}




