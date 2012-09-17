/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*!
 * @file fsl_osal_types.h
 *
 * @brief Defines data types
 *
 * @ingroup osal
 */

#ifndef FSL_OSAL_TYPES_H_
#define FSL_OSAL_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! fsl_osal_u8 8 bit unsigned data type */
typedef unsigned char fsl_osal_u8;

/*! fsl_osal_s8 8 bit signed data type */
typedef signed char fsl_osal_s8;

/*! fsl_osal_u16 is 16 bit unsigned data type */
typedef unsigned short fsl_osal_u16;

/*! fsl_osal_s16 is 16 bit signed data type*/
typedef signed short fsl_osal_s16;

/*! fsl_osal_u32 is 32 bit unsigned data type */
typedef unsigned long fsl_osal_u32;

/*! fsl_osal_s32 is 32 bit signed data type */
typedef signed long fsl_osal_s32;

/*! fsl_osal_u64 is 64 bit unsigned data type */
typedef unsigned long long fsl_osal_u64;

/*! fsl_osal_s64 is 64 bit signed data type */
typedef signed long long fsl_osal_s64;

/*! fsl_osal_char is an 8 bit data type */
typedef char fsl_osal_char;

/*! fsl_osal_float is an 32 bit data type */
typedef float fsl_osal_float;

/*! fsl generic pointer */
typedef void* fsl_osal_ptr;

/*! fsl_osal_void  */
typedef void fsl_osal_void;

/*!	boolean type*/
typedef enum efsl_osal_bool
{
	E_FSL_OSAL_FALSE = 0,
	E_FSL_OSAL_TRUE
}efsl_osal_bool;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
