/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file fsl_osal_linux_mem.c
 *
 * @brief Defines the OS Abstraction layer APIs for memory allocation in Linux
 *
 * @ingroup osal
 */

 #include <string.h>
 #include <fsl_osal.h>
 #include <malloc.h>

/*! Allocates the requested memory.
 *
 *	@param [In] size
 *	    memory size need to allocateed.
 *	@param [out] ptr
 *		pointer to allocated memory if successfully allocated otherwise NULL.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success otherwise E_FSL_OSAL_UNAVAILABLE
 */

 fsl_osal_ptr fsl_osal_malloc_new(fsl_osal_u32 size)
 {
    fsl_osal_ptr ptr;
	ptr = (void *)malloc(size);
 	return ptr;
 }

/*! ReAllocates the requested memory.
 *
 *	@param [In] size
 *	    memory size need to allocateed.
 *	@param [In/out] ptr
 *		pointer to allocated memory if successfully allocated otherwise NULL.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success otherwise E_FSL_OSAL_UNAVAILABLE
 */
fsl_osal_ptr fsl_osal_realloc_new(fsl_osal_ptr ptr, fsl_osal_u32 size) 
{
    fsl_osal_ptr ptr2;
    ptr2 = (void *)realloc(ptr, size);

    return ptr2;
}

/*! Releases the requested memory
 *
 *	@param [In] ptr
 *		memory to be released.
 *	@return efsl_osal_return_type_t
 *		E_FSL_OSAL_SUCCESS if success
 */
 efsl_osal_return_type_t fsl_osal_dealloc(fsl_osal_ptr ptr)
{
    free(ptr);
	return E_FSL_OSAL_SUCCESS;
}

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
 efsl_osal_return_type_t fsl_osal_memset(fsl_osal_ptr ptr,
 										 fsl_osal_char ch,
 										 fsl_osal_u32 size)
{
	memset(ptr, ch, size);
	return E_FSL_OSAL_SUCCESS;
}

/*!  The  fsl_osal_memcpy function copies 'size' bytes from memory area src to memory
 *   area dest.
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
 efsl_osal_return_type_t fsl_osal_memcpy(fsl_osal_ptr dest,
 										 fsl_osal_ptr src,
 										 fsl_osal_u32 size)
{
	if( (dest == NULL) || (src == NULL) )
	{
		return E_FSL_OSAL_INVALIDPARAM;
	}
	memcpy(dest, src, size);
	return E_FSL_OSAL_SUCCESS;
}


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
 efsl_osal_return_type_t fsl_osal_memmove(fsl_osal_ptr dest,
 										 fsl_osal_ptr src,
 										 fsl_osal_u32 size)
{
	if( (dest == NULL) || (src == NULL) )
	{
		return E_FSL_OSAL_INVALIDPARAM;
	}
	memmove(dest, src, size);
	return E_FSL_OSAL_SUCCESS;
}

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
 										 fsl_osal_u32 size)
{
	if( (ptr1 == NULL) || (ptr2 == NULL) )
	{
		return E_FSL_OSAL_INVALIDPARAM;
	}
	return memcmp(ptr1, ptr2, size);
}


/* File EOF */

