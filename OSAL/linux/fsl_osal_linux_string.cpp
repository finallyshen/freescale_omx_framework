/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/*
 * @file fsl_osal_linux_string.cpp
 *
 * @brief Defines the OS Abstraction layer APIs for string in Linux
 *
 * @ingroup osal
 */

 #include <string.h>
 #include <stdlib.h>
 #include <fsl_osal.h>
 #include <malloc.h>

/*! String copy.
 *
 *	@param [out] dest
 *		destination point for string copy.
  *	@param [In] src
 *		source point for string copy.
 *	@return fsl_osal_char
 *		return destination point.
 */

 fsl_osal_char *fsl_osal_strcpy(fsl_osal_char *dest, const fsl_osal_char *src)
 {
	fsl_osal_char *ret;
	ret = strcpy(dest, src);
 	return ret;
 }

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

 fsl_osal_char *fsl_osal_strncpy(fsl_osal_char *dest, const fsl_osal_char *src, fsl_osal_s32 n)
 {
	fsl_osal_char *ret;
	ret = strncpy(dest, src, n);
 	return ret;
 }

/*! Search sub string.
 *
 *	@param [In] src1
 *		string wants to be search.
 *	@param [In] src2
 *		the sub string.
 *	@return fsl_osal_char
 *		return sub string position .
 */

 fsl_osal_char *fsl_osal_strstr(const fsl_osal_char *src1, const fsl_osal_char *src2)
 {
	fsl_osal_char *ret;
	ret = (fsl_osal_char *)strstr(src1, src2);
 	return ret;
 }


/*! String segmentation.
 *
 *	@param [in] s
 *		string will be segmentation.
  *	@param [In] delim
 *		segmentation string.
 *	@return fsl_osal_char
 *		return segmentation point.
 */

 fsl_osal_char *fsl_osal_strtok(fsl_osal_char *s, const fsl_osal_char *delim)
 {
	fsl_osal_char *ret;
	ret = strtok(s, delim);
 	return ret;
 }

/*! Thread safe string segmentation.
 *
 *	@param [in] s
 *		string will be segmentation.
  *	@param [In] delim
 *		segmentation string.
 *	@return fsl_osal_char
 *		return segmentation point.
 */

 fsl_osal_char *fsl_osal_strtok_r(fsl_osal_char *s, const fsl_osal_char *delim, fsl_osal_char **pLast)
 {
	fsl_osal_char *ret;
	ret = strtok_r(s, delim, pLast);
 	return ret;
 }

/*! String find.
 *
 *	@param [in] s
 *		string.
  *	@param [In] c
 *		char.
 *	@return fsl_osal_char
 *		return string point.
 */

 fsl_osal_char *fsl_osal_strrchr(const fsl_osal_char *s, fsl_osal_s32 c)
 {
	fsl_osal_char *ret;
	ret = (fsl_osal_char *)strrchr(s, c);
 	return ret;
 }


/*! String length.
 *
 *	@param [In] src
 *		the input string.
 *	@return fsl_osal_u32
 *		The length of the input string, not include "\0".
 */

 fsl_osal_u32 fsl_osal_strlen(const fsl_osal_char *src)
 {
	fsl_osal_u32 ret;
	ret = strlen(src);
 	return ret;
 }

/*! String compare.
 *
 *	@param [In] src1
 *		the first string.
 *	@param [In] src2
 *		the second string.
 *	@return fsl_osal_s32
 *		return 0 if the string is same otherwise nonezero.
 */

 fsl_osal_s32 fsl_osal_strcmp(const fsl_osal_char *src1, const fsl_osal_char *src2)
 {
	fsl_osal_s32 ret;
	ret = strcmp(src1, src2);
 	return ret;
 }

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

 fsl_osal_s32 fsl_osal_strncmp(const fsl_osal_char *src1, const fsl_osal_char *src2, fsl_osal_s32 n)
 {
	fsl_osal_s32 ret;
	ret = strncmp(src1, src2, n);
 	return ret;
 }


 fsl_osal_s32 fsl_osal_strncasecmp(const fsl_osal_char *src1, const fsl_osal_char *src2, fsl_osal_s32 n)
 {
	fsl_osal_s32 ret;
	ret = strncasecmp(src1, src2, n);
 	return ret;
 }

/*! String dump.
 *
 *	@param [In] src
 *		source point for string copy.
 *	@return dest
 *		destination point for string copy.
 *		return destination point.
 */

 fsl_osal_char *fsl_osal_strdup(const fsl_osal_char *src)
 {
	return strdup(src);
 }

/*! Change char to integrate.
 *
 *	@param [In] src
 *		the input string.
 *	@return fsl_osal_s32
 *		return the value.
 */

 fsl_osal_s32 fsl_osal_atoi(const fsl_osal_char *src)
 {
	fsl_osal_s32 ret;
	ret = atoi(src);
 	return ret;
 }


/*! Get envirement parameter.
 *
 *	@param [In] name
 *		envirement name.
 *	@return fsl_osal_char
 *		return envirement string.
 */

 fsl_osal_char *fsl_osal_getenv_new(const fsl_osal_char *name)
 {
	fsl_osal_char *pEnvValue;
	pEnvValue = getenv(name);
 	return pEnvValue;
 }


