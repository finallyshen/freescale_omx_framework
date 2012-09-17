/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor, Inc. 
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file OpenMAXIL/src/core_mgr/OMXCoreMgr.cpp
 *  @brief Open MAX IL core manager implement.
 *  @ingroup core
 */

#include "Mem.h"
#include "Log.h"
#include "RegistryAnalyser.h"
#include "OMXCoreMgr.h"
#include "OMX_Component.h"
#include "OMX_Implement.h"
#include "OMX_ContentPipe.h"

#define OMX_CORE_DBG
#ifdef OMX_CORE_DBG
#define OMX_CORE_DBG_LEVEL	LOG_LEVEL_APIINFO
#define OMX_CORE_API_LOG(...)   {if((nLogLevel >=OMX_CORE_DBG_LEVEL)){printf("OMXCORE: "); printf(__VA_ARGS__);}}
#else
#define OMX_CORE_API_LOG(...)
#endif

static fsl_osal_mutex_t omx_mutex = FSL_OSAL_MUTEX_INITIALIZER;

OMX_ERRORTYPE OMXCoreMgr::OMX_Init() 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	lock = NULL;
	if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal)) {
		LOG_ERROR("Create mutex for camera device failed.\n");
		return OMX_ErrorInsufficientResources;
	}

	/** Get core information from core configre file. */
	ret = CoreRegister();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Core register fail.\n");
		return ret;
	}

	/** Load cores and get core interface functions. */
	for (OMX_U32 i = 0; ; i ++)
	{
		CORE_INFO *pCoreInfo = CoreInfoList.GetNode(i);
		if (pCoreInfo == NULL)
		{
			break;
		}

		ret = CreateOMXItf((OMX_STRING)pCoreInfo->LibName, &(pCoreInfo->Interface));
		if (ret != OMX_ErrorNone)
		{
			LOG_WARNING("Create interface fail.\n");
			CoreInfoList.Remove(pCoreInfo);
			FSL_DELETE(pCoreInfo);
			pCoreInfo = NULL;
			i --;
			continue;
		}

		/** Cores initialization */
		ret = pCoreInfo->Interface->OMX_Init();
		if (ret != OMX_ErrorNone)
		{
			LOG_ERROR("OMX initialize fail.\n");
			return ret;
		}
		
		OMX_U32 j;
		/** Get component count of the core */
		for (j = 0; ; j ++)
		{
			OMX_S8 CompName[COMPONENT_NAME_LEN] = {0};
			ret = pCoreInfo->Interface->OMX_ComponentNameEnum((OMX_STRING)CompName, COMPONENT_NAME_LEN, j);
			if (ret != OMX_ErrorNone)
			{
				break;
			}
		}
		pCoreInfo->ComponentCnt = j;

	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXCoreMgr::CoreRegister()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	REG_ERRORTYPE ret_reg;
	OMX_STRING CoreRegisterFile;
	OMX_U32 EntryIndex;
	List<REG_ENTRY> *RegEntry;
	RegistryAnalyser RegistryAnalyser;

	CoreRegisterFile = fsl_osal_getenv_new("CORE_REGISTER_FILE");
	if (CoreRegisterFile == NULL)
	{
		LOG_WARNING("Can't get core register file.\n");
		CoreRegisterFile = (OMX_STRING)"/etc/omx_registry/core_register";
	}

	ret_reg = RegistryAnalyser.Open(CoreRegisterFile);
	if (ret_reg != REG_SUCCESS)
	{
		LOG_ERROR("Open core register file fail.\n");
		return OMX_ErrorUndefined;
	}

	do
	{
		RegEntry = RegistryAnalyser.GetNextEntry();
		if (RegEntry->GetNodeCnt() == 0)
		{
			LOG_DEBUG("Read register finished.\n");
			break;
		}

		CORE_INFO *pCoreInfo = FSL_NEW(CORE_INFO, ());
		if (pCoreInfo == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}
		fsl_osal_memset(pCoreInfo, 0, sizeof(CORE_INFO));

		pCoreInfo->priority = 3;

		for (EntryIndex=0; ;EntryIndex++)
		{
			REG_ENTRY *pRegEntryItem = RegEntry->GetNode(EntryIndex);
			if (pRegEntryItem == NULL)
			{
				break;
			}
			else
			{
				if (!fsl_osal_strcmp(pRegEntryItem->name, "core_name"))
				{
					fsl_osal_strcpy((fsl_osal_char *)pCoreInfo->CoreName, \
							pRegEntryItem->value);
				}
				else if (!fsl_osal_strcmp(pRegEntryItem->name, "core_library_path"))
				{
					fsl_osal_strcpy((fsl_osal_char *)pCoreInfo->LibName, \
							pRegEntryItem->value);
				}
				else if (!fsl_osal_strcmp(pRegEntryItem->name, "core_priority"))
				{
						pCoreInfo->priority = fsl_osal_atoi(pRegEntryItem->value);
				}
				else
				{
					LOG_WARNING("Unknow register entry.\n");
				}
			}
		}

		CoreInfoList.Add(pCoreInfo, pCoreInfo->priority);

	}while (1);

	ret_reg = RegistryAnalyser.Close();
	if (ret_reg != REG_SUCCESS)
	{
		LOG_ERROR("Registry analyser close fail.\n");
		return OMX_ErrorUndefined;
	}

	return ret;
}

OMX_ERRORTYPE OMXCoreMgr::OMX_Deinit() 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	/** Free all core */
	ret = FreeAllCore();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Free all core fail.\n");
		return ret;
	}

	/** Release core manager resouce */
	ret = FreeCoreMgrResource();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Free core manager resource fail.\n");
		return ret;
	}

	if(lock)
		fsl_osal_mutex_destroy(lock);

	return ret;
}

OMX_ERRORTYPE OMXCoreMgr::FreeAllCore() 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	for (OMX_S32 i = CoreInfoList.GetNodeCnt() - 1; i >= 0; i --)
	{
		CORE_INFO *pCoreInfo = CoreInfoList.GetNode(i);
		if (pCoreInfo == NULL)
		{
			LOG_ERROR("Free core list error.\n");
			return OMX_ErrorUndefined;
		}

		/** Cores de-initialization */
		ret = pCoreInfo->Interface->OMX_Deinit();
		if (ret != OMX_ErrorNone)
		{
			LOG_ERROR("OMX initialize fail.\n");
			return ret;
		}

		CoreInfoList.Remove(pCoreInfo);
		FSL_DELETE(pCoreInfo);
		pCoreInfo = NULL;
	}

	return ret;
}

OMX_ERRORTYPE OMXCoreMgr::FreeCoreMgrResource() 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	/** Free handle list. */
	fsl_osal_mutex_lock(lock);
	for (OMX_S32 i = HandleList.GetNodeCnt() - 1; i >= 0 ; i --)
	{
		HANDLE_INFO *pHandleInfo = HandleList.GetNode(i);
		if (pHandleInfo == NULL)
		{
			LOG_ERROR("Free handle list error.\n");
			fsl_osal_mutex_unlock(lock);
			return OMX_ErrorUndefined;
		}

		HandleList.Remove(pHandleInfo);
		FSL_DELETE(pHandleInfo);
		pHandleInfo = NULL;
	}

	fsl_osal_mutex_unlock(lock);
	return ret;
}

OMX_ERRORTYPE OMXCoreMgr::OMX_ComponentNameEnum(
        OMX_STRING cComponentName, 
        OMX_U32 nNameLength, 
        OMX_U32 nIndex) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	for (OMX_U32 i = 0; ; i ++)
	{
		CORE_INFO *pCoreInfo = CoreInfoList.GetNode(i);
		if (pCoreInfo == NULL)
		{
			break;
		}

		if (nIndex <= pCoreInfo->ComponentCnt - 1)
		{
			pCoreInfo->Interface->OMX_ComponentNameEnum(cComponentName, nNameLength, nIndex);
			if (ret != OMX_ErrorNone)
			{
				LOG_ERROR("OMX component name enum fail.\n");
				return ret;
			}

			return OMX_ErrorNone;
		}

		nIndex -= pCoreInfo->ComponentCnt;
	}

	return OMX_ErrorNoMore;
}

OMX_ERRORTYPE OMXCoreMgr::OMX_GetHandle(
        OMX_HANDLETYPE *pHandle, 
        OMX_STRING cComponentName,
        OMX_PTR pAppData, 
        OMX_CALLBACKTYPE *pCallBacks) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	for (OMX_U32 i = 0; ; i ++)
	{
		CORE_INFO *pCoreInfo = CoreInfoList.GetNode(i);
		if (pCoreInfo == NULL)
		{
			break;
		}

		ret = pCoreInfo->Interface->OMX_GetHandle(
				pHandle, 
				cComponentName,
				pAppData, 
				pCallBacks);
		if (ret == OMX_ErrorNone)
		{
			HANDLE_INFO *pHandleInfo = FSL_NEW(HANDLE_INFO, ());
			if (pHandleInfo == NULL)
			{
				LOG_ERROR("Can't get memory.\n");
				return OMX_ErrorInsufficientResources;
			}
			
			pHandleInfo->pHandle = *pHandle;
			pHandleInfo->CoreIndex = i;

			fsl_osal_mutex_lock(lock);
			HandleList.Add(pHandleInfo);
			fsl_osal_mutex_unlock(lock);

			return ret;
		}
	}

	LOG_ERROR("OMX get handle fail.\n");
	return ret;
}

OMX_ERRORTYPE OMXCoreMgr::OMX_FreeHandle(
        OMX_HANDLETYPE hComponent) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

    fsl_osal_mutex_lock(lock);
	for (OMX_U32 i = 0; ; i ++)
	{
		HANDLE_INFO *pHandleInfo = HandleList.GetNode(i);
		if (pHandleInfo == NULL)
		{
			break;
		}

		if (pHandleInfo->pHandle == hComponent)
		{
			CORE_INFO *pCoreInfo = CoreInfoList.GetNode(pHandleInfo->CoreIndex);
			if (pCoreInfo == NULL)
			{
				LOG_ERROR("Core index error.\n");
				fsl_osal_mutex_unlock(lock);
				return OMX_ErrorUndefined;
			}

			ret = pCoreInfo->Interface->OMX_FreeHandle(hComponent);
			if (ret == OMX_ErrorNone)
			{
				HandleList.Remove(pHandleInfo);
				FSL_DELETE(pHandleInfo);
				pHandleInfo = NULL;

				fsl_osal_mutex_unlock(lock);
				return ret;
			}
		}
	}

    fsl_osal_mutex_unlock(lock);
	LOG_ERROR("OMX free handle fail.\n");
	return ret;
}

OMX_ERRORTYPE OMXCoreMgr::OMX_GetComponentsOfRole(
        OMX_STRING role, 
        OMX_U32 *pNumComps, 
        OMX_U8 **compNames) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 nCompNum = *pNumComps, nCompTotalNum = 0;

	for (OMX_U32 i = 0; ; i ++)
	{
		CORE_INFO *pCoreInfo = CoreInfoList.GetNode(i);
		if (pCoreInfo == NULL)
		{
			break;
		}

		ret = pCoreInfo->Interface->OMX_GetComponentsOfRole(
				role, 
				&nCompNum, 
				compNames);
		if (ret != OMX_ErrorNone)
		{
			return ret;
		}
		nCompTotalNum += nCompNum;
		if (compNames != NULL)
		{
			if (nCompTotalNum >= *pNumComps)
			{
				break;
			}
			else
			{
				compNames = (OMX_U8 **)((OMX_U8 *)compNames + nCompNum * COMPONENT_NAME_LEN);
				nCompNum = *pNumComps - nCompTotalNum;
			}
		}
	}

	*pNumComps = nCompTotalNum;

	return ret;
}

OMX_ERRORTYPE OMXCoreMgr::OMX_GetRolesOfComponent(
        OMX_STRING compName, 
        OMX_U32 *pNumRoles, 
        OMX_U8 **roles) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 nRoleNum = *pNumRoles, nRoleTotalNum = 0;

	for (OMX_U32 i = 0; ; i ++)
	{
		CORE_INFO *pCoreInfo = CoreInfoList.GetNode(i);
		if (pCoreInfo == NULL)
		{
			break;
		}

		ret = pCoreInfo->Interface->OMX_GetRolesOfComponent(
				compName, 
				&nRoleNum, 
				roles);
		if (ret == OMX_ErrorNone)
		{
			return ret;
		}
		nRoleTotalNum += nRoleNum;
		if (roles != NULL)
		{
			if (nRoleTotalNum >= *pNumRoles)
			{
				break;
			}
			else
			{
				roles = (OMX_U8 **)((OMX_U8 *)roles + nRoleNum * COMPONENT_NAME_LEN);
				nRoleNum = *pNumRoles - nRoleTotalNum;
			}
		}
	}

	if (roles == NULL)
	{
		*pNumRoles = nRoleTotalNum;
	}

	return ret;
}

OMX_ERRORTYPE OMXCoreMgr::OMX_SetupTunnel(
        OMX_HANDLETYPE hOutput, 
        OMX_U32 nPortOutput, 
        OMX_HANDLETYPE hInput, 
        OMX_U32 nPortInput) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	for (OMX_U32 i = 0; ; i ++)
	{
		HANDLE_INFO *pHandleInfo = HandleList.GetNode(i);
		if (pHandleInfo == NULL)
		{
			break;
		}

		if (pHandleInfo->pHandle == hOutput || pHandleInfo->pHandle == hInput)
		{
			CORE_INFO *pCoreInfo = CoreInfoList.GetNode(pHandleInfo->CoreIndex);
			if (pCoreInfo == NULL)
			{
				LOG_ERROR("Core index error.\n");
				return OMX_ErrorUndefined;
			}

			ret = pCoreInfo->Interface->OMX_SetupTunnel(
					hOutput, 
					nPortOutput, 
					hInput, 
					nPortInput);
			if (ret == OMX_ErrorNone)
			{
				return ret;
			}
		}
	}

	LOG_ERROR("OMX setup tunnel fail.\n");
	return ret;
}

OMX_ERRORTYPE OMXCoreMgr::OMX_GetContentPipe(
        OMX_HANDLETYPE *hPipe, 
        OMX_STRING szURI) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	for (OMX_U32 i = 0; ; i ++)
	{
		CORE_INFO *pCoreInfo = CoreInfoList.GetNode(i);
		if (pCoreInfo == NULL)
		{
			break;
		}

		ret = pCoreInfo->Interface->OMX_GetContentPipe(
				hPipe, 
				szURI);
		if (ret == OMX_ErrorNone)
		{
			return ret;
		}
	}

	LOG_ERROR("OMX get content pipe: %s fail.\n", szURI);
	return ret;
}

/**< C style functions to expose entry point for the shared library */
extern "C" 
{
    /**< OMX core manager handle definition, global variable */
    OMXCoreMgr *gCoreMgrHandle = NULL;
    static OMX_S32 refCnt = 0;

    OMX_ERRORTYPE OMX_Init() 
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;
        OMX_CORE_API_LOG("OMX_Init: \r\n");
        fsl_osal_mutex_lock(&omx_mutex);

        if(NULL == gCoreMgrHandle) {
#ifdef OMX_MEM_CHECK
            init_memmanager(NULL);
#endif
            gCoreMgrHandle = FSL_NEW(OMXCoreMgr, ());
            if(NULL == gCoreMgrHandle) {
                fsl_osal_mutex_unlock(&omx_mutex);
                return OMX_ErrorInsufficientResources;
            }

            LogInit(-1, NULL);

            LOG_DEBUG("OMXCoreMgr is Created %p.\n", gCoreMgrHandle);

            ret = gCoreMgrHandle->OMX_Init();
        }

        refCnt ++;

        fsl_osal_mutex_unlock(&omx_mutex);

        return ret;
    }

    OMX_ERRORTYPE OMX_Deinit() 
    {
        OMX_CORE_API_LOG("OMX_Deinit: \r\n");
        fsl_osal_mutex_lock(&omx_mutex);

        refCnt --;
        if(refCnt > 0) {
            fsl_osal_mutex_unlock(&omx_mutex);
            return OMX_ErrorNone;
        }

        if(NULL != gCoreMgrHandle) {
            gCoreMgrHandle->OMX_Deinit();

            LOG_DEBUG("OMXCoreMgr is deleted %p.\n", gCoreMgrHandle);

            FSL_DELETE(gCoreMgrHandle);
            gCoreMgrHandle = NULL;
        }

	LogDeInit();

#ifdef OMX_MEM_CHECK
        deinit_memmanager();
#endif

        fsl_osal_mutex_unlock(&omx_mutex);

        return OMX_ErrorNone;
    }

    OMX_ERRORTYPE OMX_ComponentNameEnum(
            OMX_STRING cComponentName, 
            OMX_U32 nNameLength, 
            OMX_U32 nIndex) 
    {
        if(NULL == gCoreMgrHandle)
            return OMX_ErrorNotReady;
        OMX_CORE_API_LOG("OMX_ComponentNameEnum: Index: 0x%X \r\n",nIndex);
        return gCoreMgrHandle->OMX_ComponentNameEnum(cComponentName, nNameLength, nIndex);
    }

    OMX_ERRORTYPE OMX_GetHandle(
            OMX_HANDLETYPE *pHandle, 
            OMX_STRING cComponentName,
            OMX_PTR pAppData, 
            OMX_CALLBACKTYPE *pCallBacks) 
    {
        OMX_ERRORTYPE ret;
        if(NULL == gCoreMgrHandle)
            return OMX_ErrorNotReady;
        ret=gCoreMgrHandle->OMX_GetHandle(pHandle, cComponentName, pAppData, pCallBacks);
        OMX_CORE_API_LOG("OMX_GetHandle: ComponentName: %s, Handle: 0x%X \r\n",cComponentName,*pHandle);
        return ret;
    }

    OMX_ERRORTYPE OMX_FreeHandle(
            OMX_HANDLETYPE hComponent) 
    {
        if(NULL == gCoreMgrHandle)
            return OMX_ErrorNotReady;
        OMX_CORE_API_LOG("OMX_FreeHandle: Handle: 0x%X \r\n",hComponent);
        return gCoreMgrHandle->OMX_FreeHandle(hComponent);
    }

    OMX_ERRORTYPE OMX_GetComponentsOfRole(
            OMX_STRING role, 
            OMX_U32 *pNumComps, 
            OMX_U8 **compNames) 
    {
        if(NULL == gCoreMgrHandle)
            return OMX_ErrorNotReady;
        OMX_CORE_API_LOG("OMX_GetComponentsOfRole: Role: %s \r\n",role);
        return gCoreMgrHandle->OMX_GetComponentsOfRole(role, pNumComps, compNames);
    }

    OMX_ERRORTYPE OMX_GetRolesOfComponent(
            OMX_STRING compName, 
            OMX_U32 *pNumRoles, 
            OMX_U8 **roles) 
    {
        if(NULL == gCoreMgrHandle)
            return OMX_ErrorNotReady;
        OMX_CORE_API_LOG("OMX_GetRolesOfComponent: ComponentName: %s \r\n",compName);
        return gCoreMgrHandle->OMX_GetRolesOfComponent(compName, pNumRoles, roles);
    }

    OMX_ERRORTYPE OMX_SetupTunnel(
            OMX_HANDLETYPE hOutput, 
            OMX_U32 nPortOutput, 
            OMX_HANDLETYPE hInput, 
            OMX_U32 nPortInput) 
    {
        if(NULL == gCoreMgrHandle)
            return OMX_ErrorNotReady;
        OMX_CORE_API_LOG("OMX_SetupTunnel: OutHandle: 0x%X, OutPort: 0x%X, InHandle: 0x%X, InPort: 0x%X \r\n",hOutput,nPortOutput,hInput,nPortInput);
        return gCoreMgrHandle->OMX_SetupTunnel(hOutput, nPortOutput, hInput, nPortInput);
    }

    OMX_ERRORTYPE OMX_GetContentPipe(
            OMX_HANDLETYPE *hPipe, 
            OMX_STRING szURI) 
    {
        if(NULL == gCoreMgrHandle)
            return OMX_ErrorNotReady;
        OMX_CORE_API_LOG("OMX_GetContentPipe: URI: %s \r\n",szURI);
        return gCoreMgrHandle->OMX_GetContentPipe(hPipe, szURI);
    }
}

/* File EOF */

