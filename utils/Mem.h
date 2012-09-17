/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Mem.h
 *  @brief Memory utils
 *  @ingroup Utils
 */
#ifndef Mem_h
#define Mem_h


#include "fsl_osal.h"

#ifdef OMX_MEM_CHECK

#define FSL_NEW(type, params) new(__FILE__, __LINE__) type params
#define FSL_DELETE(ptr) { if(ptr) {delete(ptr); ptr=NULL;}}
#define FSL_DELETE_THIS(ptr) { if(ptr) {delete(ptr);}}
#define FSL_MALLOC(size) dbg_malloc(size, __FILE__, __LINE__)
#define FSL_REALLOC(ptr, size) dbg_realloc(ptr, size, __FILE__, __LINE__)
#define FSL_FREE(ptr) { if(ptr) {dbg_free(ptr); ptr=NULL;}}

#ifdef __cplusplus
extern "C" {
#endif
fsl_osal_void init_memmanager(fsl_osal_char *shortname);
fsl_osal_void deinit_memmanager();
fsl_osal_void clear_memmanager();
fsl_osal_ptr dbg_malloc(fsl_osal_u32 size, fsl_osal_char * desc,fsl_osal_s32 line);
fsl_osal_ptr dbg_realloc(fsl_osal_ptr ptr, fsl_osal_u32 size, fsl_osal_char * desc,fsl_osal_s32 line);
fsl_osal_void dbg_free(fsl_osal_ptr mem);
#ifdef __cplusplus
}

void * operator new(unsigned int size, char* file, int line);
void operator delete(fsl_osal_ptr ptr, char* file, int line);
void * operator new(unsigned int size);
void operator delete(fsl_osal_ptr ptr);
#endif

#else

#define FSL_NEW(type, params) new type params
#define FSL_DELETE(ptr) { if(ptr) {delete(ptr); ptr=NULL;}}
#define FSL_DELETE_THIS(ptr) { if(ptr) {delete(ptr);}}
#define FSL_MALLOC(size) fsl_osal_malloc_new(size)
#define FSL_REALLOC(ptr, size) fsl_osal_realloc_new(ptr, size)
#define FSL_FREE(ptr) { if(ptr) {fsl_osal_dealloc(ptr); ptr=NULL;}}

#endif

#endif
/* File EOF */
