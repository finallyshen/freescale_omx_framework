/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <stdio.h>
#include <dlfcn.h>
#include "Mem.h"
#include "Log.h"
#include "ShareLibarayMgr.h"

fsl_osal_ptr ShareLibarayMgr::load(fsl_osal_char *lib_name) 
{
	LIB_INFO *pLibInfo;

	if (lib_name == NULL)
	{
		LOG_ERROR("Library name is NULL.\n");
		return NULL;
	}

	/** Check if the library already loaded */
	for (fsl_osal_s32 i = 0; ; i ++)
	{
		pLibInfo = lib_info.GetNode(i);
		if(pLibInfo == NULL)
		{
			break;
		}

		if (!fsl_osal_strcmp(pLibInfo->lib_name, lib_name))
		{
			pLibInfo->refCount ++;
			LOG_DEBUG("The reference times of the lib: %s is: %d\n", pLibInfo->lib_name, \
					pLibInfo->refCount);
			return pLibInfo->hlib;
		}
	}

    const fsl_osal_char *cError;

	fsl_osal_ptr pLibHandle = dlopen(lib_name, RTLD_NOW);
	if (pLibHandle == NULL)
	{
		LOG_WARNING("Can't open library: %s\n", lib_name);
		printf("Can't open library: %s\n", lib_name);
        cError = dlerror();
        LOG_WARNING("%s\n", cError);
        printf("%s\n", cError);
		return NULL;
	}

	pLibInfo = FSL_NEW(LIB_INFO, ());
	if (pLibInfo == NULL)
	{
		LOG_ERROR("New LIB_INFO fail.\n");
		return NULL;
	}
	
	fsl_osal_strcpy(pLibInfo->lib_name, lib_name);
	pLibInfo->hlib = pLibHandle;
	pLibInfo->refCount = 1;

	lib_info.Add(pLibInfo);

	return pLibHandle;
}

fsl_osal_s32 ShareLibarayMgr::unload(fsl_osal_ptr hlib) 
{
	LIB_INFO *pLibInfo;

	if (hlib == NULL)
	{
		LOG_ERROR("Library handle is NULL.\n");
		return 0;
	}

	/** Check if the library already loaded */
	for (fsl_osal_s32 i = 0; ; i ++)
	{
		pLibInfo = lib_info.GetNode(i);
		if(pLibInfo == NULL)
		{
			break;
		}

		if (pLibInfo->hlib == hlib)
		{
			pLibInfo->refCount --;
			LOG_DEBUG("The reference times of the library: %s is: %d\n", pLibInfo->lib_name, \
					pLibInfo->refCount);
			if (pLibInfo->refCount == 0)
			{
				LOG_DEBUG("Unload the librayr: %s\n", pLibInfo->lib_name);
				dlclose(pLibInfo->hlib);
				lib_info.Remove(pLibInfo);
				FSL_DELETE(pLibInfo);
				return 0;
			}
			return pLibInfo->refCount;
		}
	}
	
	return 0;
}

fsl_osal_ptr ShareLibarayMgr::getSymbol(fsl_osal_ptr hlib, fsl_osal_char *symbol) 
{
	LIB_INFO *pLibInfo;

	if (hlib == NULL || symbol == NULL)
	{
		LOG_ERROR("Library handle or symbol is NULL.\n");
		return NULL;
	}

	/** Check if the library already loaded */
	for (fsl_osal_s32 i = 0; ; i ++)
	{
		pLibInfo = lib_info.GetNode(i);
		if(pLibInfo == NULL)
		{
			break;
		}

		if (pLibInfo->hlib == hlib)
		{
			fsl_osal_ptr funcSymbol = dlsym(pLibInfo->hlib, symbol);
			if (funcSymbol != NULL)
			{
				return funcSymbol;
			}
			else
			{
				const fsl_osal_char *cError;
				cError = dlerror();
				LOG_DEBUG("%s\n", cError);
				break;
			}
		}
	}

	LOG_ERROR("Can't get symbol: %s in library: %s\n", symbol, pLibInfo->lib_name);
	return NULL;
}

/* File EOF */
