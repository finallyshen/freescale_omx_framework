/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file RegistryAnalyser.h
 *  @brief Class definition of Regitsry file analyser
 *  @ingroup RegistryAnalyser
 */


#ifndef RegistryAnalyser_h
#define RegistryAnalyser_h

#include "fsl_osal.h"
#include "List.h"

#define ITEM_NAME_LEN 32
#define ITEM_VALUE_LEN 128
#define FILE_READ_SIZE (1024*4)

typedef enum {
    REG_SUCCESS,
    REG_FAILURE,
    REG_INSUFFICIENT_RESOURCES,
    REG_INVALID_REG
}REG_ERRORTYPE;

typedef struct _REG_ENTRY {
    fsl_osal_char name[ITEM_NAME_LEN];
    fsl_osal_char value[ITEM_VALUE_LEN];
}REG_ENTRY;

class RegistryAnalyser {
	public:
		RegistryAnalyser();
		REG_ERRORTYPE Open(fsl_osal_char *file_name);
		REG_ERRORTYPE Close();
		List<REG_ENTRY> *GetNextEntry();
		~RegistryAnalyser();
	private:
		FILE *pfile;
		List<REG_ENTRY> RegList;
		efsl_osal_bool FileReadEnd;
		fsl_osal_s32 BufferDataLen;
		fsl_osal_s32 UseOffset;
		fsl_osal_char readBuffer[FILE_READ_SIZE];
		REG_ERRORTYPE RemoveEntry(); 

};

#endif
/* File EOF */
