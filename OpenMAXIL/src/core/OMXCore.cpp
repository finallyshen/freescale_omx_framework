/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file OpenMAXIL/src/core/OMXCore.cpp
 *  @brief Open MAX IL core implement.
 *  @ingroup core
 */

#include "Mem.h"
#include "Log.h"
#include "RegistryAnalyser.h"
#include "OMXCore.h"
#include "OMX_Component.h"
#include "OMX_Implement.h"
#include "OMX_ContentPipe.h"
#include "PlatformResourceMgrItf.h"

OMX_ERRORTYPE OMXCore::OMX_Init() 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	lock = NULL;
	if(E_FSL_OSAL_SUCCESS != fsl_osal_mutex_init(&lock, fsl_osal_mutex_normal)) {
		LOG_ERROR("Create mutex for camera device failed.\n");
		return OMX_ErrorInsufficientResources;
	}

	LibMgr = FSL_NEW(ShareLibarayMgr, ());
        if(LibMgr == NULL)
            return OMX_ErrorInsufficientResources;

	/** Get component info of the core */
	ret = ComponentRegister();
	if (ret != OMX_ErrorNone)
	{
		return ret;
	}

	/** Get content pipe info of the core */
	ret = ContentPipeRegister();
	if (ret != OMX_ErrorNone)
	{
		return ret;
	}

	return ret;
}

OMX_ERRORTYPE OMXCore::ComponentRegister()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	REG_ERRORTYPE ret_reg;
	OMX_STRING ComponentRegisterFile;
	OMX_U32 EntryIndex;
	List<REG_ENTRY> *RegEntry;
	RegistryAnalyser RegistryAnalyser;

	ComponentRegisterFile = fsl_osal_getenv_new("COMPONENT_REGISTER_FILE");
	if (ComponentRegisterFile == NULL)
	{
		LOG_WARNING("Can't get component register file.\n");
		ComponentRegisterFile = (OMX_STRING)"/etc/omx_registry/component_register";
	}

	ret_reg = RegistryAnalyser.Open(ComponentRegisterFile);
	if (ret_reg != REG_SUCCESS)
	{
		LOG_ERROR("Open component register file fail.\n");
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

		COMPONENT_INFO *pComponentInfo = FSL_NEW(COMPONENT_INFO, ());
		if (pComponentInfo == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}

		fsl_osal_memset(pComponentInfo, 0, sizeof(COMPONENT_INFO));

		for (EntryIndex=0; ;EntryIndex++)
		{
			REG_ENTRY *pRegEntryItem = RegEntry->GetNode(EntryIndex);
			if (pRegEntryItem == NULL)
			{
				break;
			}
			else
			{
				if (!fsl_osal_strcmp(pRegEntryItem->name, "component_name"))
				{
					fsl_osal_strcpy((fsl_osal_char *)pComponentInfo->ComponentName, \
							pRegEntryItem->value);
				}
				else if (!fsl_osal_strcmp(pRegEntryItem->name, "component_role"))
				{
					ROLE_INFO *pRoleInfo = FSL_NEW(ROLE_INFO, ());
					fsl_osal_memset(pRoleInfo, 0, sizeof(ROLE_INFO));
					fsl_osal_strcpy((fsl_osal_char *)pRoleInfo->role, pRegEntryItem->value);
					pRoleInfo->priority = 3;
					EntryIndex ++;
					REG_ENTRY *pRegEntryItem = RegEntry->GetNode(EntryIndex);
					if (pRegEntryItem == NULL)
					{
						break;
					}

					if (!fsl_osal_strcmp(pRegEntryItem->name, "role_priority"))
					{
						pRoleInfo->priority = fsl_osal_atoi(pRegEntryItem->value);
					}
					else
					{
						EntryIndex --;
					}

					pComponentInfo->RoleList.Add(pRoleInfo);
				}
				else if (!fsl_osal_strcmp(pRegEntryItem->name, "library_path"))
				{
					fsl_osal_strcpy((fsl_osal_char *)pComponentInfo->LibName, \
							pRegEntryItem->value);
				}
				else if (!fsl_osal_strcmp(pRegEntryItem->name, "component_entry_function"))
				{
					fsl_osal_strcpy((fsl_osal_char *)pComponentInfo->EntryFunction, \
							pRegEntryItem->value);
				}
				else
				{
					LOG_WARNING("Unknow register entry.\n");
				}
			}
		}

		ComponentList.Add(pComponentInfo);

	}while (1);

	ret_reg = RegistryAnalyser.Close();
	if (ret_reg != REG_SUCCESS)
	{
		LOG_ERROR("Registry analyser close fail.\n");
		return OMX_ErrorUndefined;
	}

	return ret;
}

OMX_ERRORTYPE OMXCore::ContentPipeRegister()
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	REG_ERRORTYPE ret_reg;
	OMX_STRING ContentPipeRegisterFile;
	OMX_U32 EntryIndex;
	List<REG_ENTRY> *RegEntry;
	RegistryAnalyser RegistryAnalyser;

	ContentPipeRegisterFile = fsl_osal_getenv_new("CONTENTPIPE_REGISTER_FILE");
	if (ContentPipeRegisterFile == NULL)
	{
		LOG_WARNING("Can't get content pipe register file.\n");
		ContentPipeRegisterFile = (OMX_STRING)"/etc/omx_registry/contentpipe_register";
	}

	ret_reg = RegistryAnalyser.Open(ContentPipeRegisterFile);
	if (ret_reg != REG_SUCCESS)
	{
		LOG_ERROR("Open contentpipe register file fail.\n");
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

		CONTENTPIPE_INFO *pContentPipeInfo = FSL_NEW(CONTENTPIPE_INFO, ());
		if (pContentPipeInfo == NULL)
		{
			LOG_ERROR("Can't get memory.\n");
			return OMX_ErrorInsufficientResources;
		}

		fsl_osal_memset(pContentPipeInfo, 0, sizeof(CONTENTPIPE_INFO));

		for (EntryIndex=0; ;EntryIndex++)
		{
			REG_ENTRY *pRegEntryItem = RegEntry->GetNode(EntryIndex);
			if (pRegEntryItem == NULL)
			{
				break;
			}
			else
			{
				if (!fsl_osal_strcmp(pRegEntryItem->name, "content_pipe_name"))
				{
					fsl_osal_strcpy((fsl_osal_char *)pContentPipeInfo->ContentPipeName, \
							pRegEntryItem->value);
				}
				else if (!fsl_osal_strcmp(pRegEntryItem->name, "content_pipe_library_path"))
				{
					fsl_osal_strcpy((fsl_osal_char *)pContentPipeInfo->LibName, \
							pRegEntryItem->value);
				}
				else if (!fsl_osal_strcmp(pRegEntryItem->name, "content_pipe_entry_function"))
				{
					fsl_osal_strcpy((fsl_osal_char *)pContentPipeInfo->EntryFunction, \
							pRegEntryItem->value);
				}
				else
				{
					LOG_ERROR("Unknow register entry.\n");
				}
			}
		}

		ContentPipeList.Add(pContentPipeInfo);

	}while (1);

	ret_reg = RegistryAnalyser.Close();
	if (ret_reg != REG_SUCCESS)
	{
		LOG_ERROR("Registry analyser close fail.\n");
		return OMX_ErrorUndefined;
	}

	return ret;
}

OMX_ERRORTYPE OMXCore::OMX_Deinit() 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	/** Free all component */
	ret = FreeAllComponent();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Free all component fail.\n");
		return ret;
	}

	/** Free all content pipe */
	ret = FreeAllContentPipe();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Free all content pipe fail.\n");
		return ret;
	}

	/** Release core resouce */
	ret = FreeCoreResource();
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Free core resource fail.\n");
		return ret;
	}

	if(lock)
		fsl_osal_mutex_destroy(lock);

	return ret;
}

OMX_ERRORTYPE OMXCore::FreeAllComponent() 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	HANDLE_INFO *HandleListPtr;
	OMX_S32 RefCnt, i;
	OMX_U32 ComponentCnt;

    fsl_osal_mutex_lock(lock);
	ComponentCnt = HandleList.GetNodeCnt();
	if (ComponentCnt == 0)
	{
		fsl_osal_mutex_unlock(lock);
		return ret;
	}

	for (i=ComponentCnt-1; i>=0; i--)
	{
		HandleListPtr = HandleList.GetNode(i);
		if (HandleListPtr == NULL)
		{
			LOG_ERROR("Handle item is NULL.\n");
			fsl_osal_mutex_unlock(lock);
			return OMX_ErrorUndefined;
		}

		ret = OMX_FreeHandle(HandleListPtr->pHandle);
		if (ret != OMX_ErrorNone)
		{
			LOG_ERROR("Free component fail.\n");
			fsl_osal_mutex_unlock(lock);
			return OMX_ErrorUndefined;
		}
	}

    fsl_osal_mutex_unlock(lock);

	return ret;
}

OMX_ERRORTYPE OMXCore::FreeAllContentPipe() 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	HANDLE_INFO *ContentPipeHandleListPtr;
	OMX_S32 RefCnt, i;
	OMX_U32 ContentPipeCnt;

	fsl_osal_mutex_lock(lock);
	ContentPipeCnt = ContentPipeHandleList.GetNodeCnt();
	if (ContentPipeCnt == 0)
	{
		fsl_osal_mutex_unlock(lock);
		return ret;
	}

	for (i=ContentPipeCnt-1; i>=0; i--)
	{
		ContentPipeHandleListPtr = ContentPipeHandleList.GetNode(i);
		if (ContentPipeHandleListPtr == NULL)
		{
			LOG_ERROR("Content pipe handle item is NULL.\n");
			fsl_osal_mutex_unlock(lock);
			return OMX_ErrorUndefined;
		}

		CP_PIPETYPE *pPipe = (CP_PIPETYPE *)ContentPipeHandleListPtr->pHandle;
		FSL_DELETE(pPipe);
		ContentPipeHandleListPtr->pHandle = NULL;
		RefCnt = LibMgr->unload(ContentPipeHandleListPtr->hlib);
		if (RefCnt != 0)
		{
			LOG_ERROR("Library refer count isn't to 0.\n");
			fsl_osal_mutex_unlock(lock);
			return OMX_ErrorUndefined;
		}
		ContentPipeHandleList.Remove(ContentPipeHandleListPtr);
		FSL_DELETE(ContentPipeHandleListPtr);
		ContentPipeHandleListPtr = NULL;
	}

	fsl_osal_mutex_unlock(lock);
	return ret;
}

OMX_ERRORTYPE OMXCore::FreeCoreResource() 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;

	/** Free component information list */
	OMX_S32 i;
	COMPONENT_INFO *pComponentInfoPtr;

	for (i = ComponentList.GetNodeCnt() - 1; i >= 0; i--)
	{
		pComponentInfoPtr = ComponentList.GetNode(i);
		if (pComponentInfoPtr == NULL)
		{
			break;
		}

		OMX_S32 j;
		ROLE_INFO *pRoleInfo;
		for (j = pComponentInfoPtr->RoleList.GetNodeCnt() - 1; j >= 0; j --)
		{
			pRoleInfo = pComponentInfoPtr->RoleList.GetNode(j);
			if (pRoleInfo == NULL)
			{
				break;
			}

			pComponentInfoPtr->RoleList.Remove(pRoleInfo);
			FSL_DELETE(pRoleInfo);
			pRoleInfo = NULL;
		}

		ComponentList.Remove(pComponentInfoPtr);
		FSL_DELETE(pComponentInfoPtr);
		pComponentInfoPtr = NULL;
	}

	CONTENTPIPE_INFO *pContentPipeInfo;
	/** Free Content pipe information list */
	for (i = ContentPipeList.GetNodeCnt() - 1; i >= 0; i --)
	{
		pContentPipeInfo = ContentPipeList.GetNode(i);
		if (pContentPipeInfo == NULL)
		{
			break;
		}

		ContentPipeList.Remove(pContentPipeInfo);
		FSL_DELETE(pContentPipeInfo);
		pContentPipeInfo = NULL;
	}


	/** Free library manager resource */
	FSL_DELETE(LibMgr);
	LibMgr = NULL;

	return ret;
}

OMX_ERRORTYPE OMXCore::OMX_ComponentNameEnum(
        OMX_STRING cComponentName, 
        OMX_U32 nNameLength, 
        OMX_U32 nIndex) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 nComponentCnt;
	COMPONENT_INFO *pComponentInfoPtr;

	nComponentCnt = ComponentList.GetNodeCnt();
	if (nIndex > nComponentCnt-1)
	{
		LOG_DEBUG("Component index is out of range.\n");
		return OMX_ErrorNoMore;
	}

	pComponentInfoPtr = ComponentList.GetNode(nIndex);
	if (nNameLength >= fsl_osal_strlen((fsl_osal_char *)pComponentInfoPtr->ComponentName)+1)
	{
		fsl_osal_strcpy((fsl_osal_char *)cComponentName, (fsl_osal_char *)pComponentInfoPtr->ComponentName);
	}
	else
	{
		LOG_ERROR("Component name buffer is too small.\n");
		return OMX_ErrorInsufficientResources;
	}

	return ret;
}

OMX_ERRORTYPE OMXCore::OMX_GetHandle(
        OMX_HANDLETYPE *pHandle, 
        OMX_STRING cComponentName,
        OMX_PTR pAppData, 
        OMX_CALLBACKTYPE *pCallBacks) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	COMPONENT_INFO *pComponentInfoPtr;
	HANDLE_INFO *pHandleInfo = FSL_NEW(HANDLE_INFO, ());
	OMX_COMPONENTTYPE *pComp = FSL_NEW(OMX_COMPONENTTYPE, ());
	OMX_HANDLETYPE hHandle = (OMX_HANDLETYPE)pComp;
	
	fsl_osal_memset(pHandleInfo, 0, sizeof(HANDLE_INFO));
	OMX_INIT_STRUCT(pComp, OMX_COMPONENTTYPE);

	/** Search component in component list. */
	pComponentInfoPtr = SearchComponent(cComponentName);
	if (pComponentInfoPtr == NULL)
	{
		FSL_DELETE(pHandleInfo);
		pHandleInfo = NULL;
		FSL_DELETE(pComp);
		pComp = NULL;
		LOG_ERROR("Can't find component based on component name: %s\n", cComponentName);
		return OMX_ErrorComponentNotFound;
	}

	/** Load library and call entry function */
	ret = ConstructComponent(pHandleInfo, hHandle, pComponentInfoPtr);
	if (ret != OMX_ErrorNone)
	{
		FSL_DELETE(pHandleInfo);
		pHandleInfo = NULL;
		FSL_DELETE(pComp);
		pComp = NULL;
		LOG_ERROR("Load and call entry function fail.\n");
		return ret;
	}

	/** Set callback function */
	ret = pComp->SetCallbacks(hHandle, pCallBacks, pAppData);
	if (ret != OMX_ErrorNone)
	{
		FSL_DELETE(pHandleInfo);
		pHandleInfo = NULL;
		FSL_DELETE(pComp);
		pComp = NULL;
		LOG_ERROR("Component set callbacks fail.\n");
		return ret;
	}

	/** Record handle info to handle list */
    fsl_osal_mutex_lock(lock);
	HandleList.Add(pHandleInfo);
    fsl_osal_mutex_unlock(lock);
	*pHandle = hHandle;
	
	return ret;
}

COMPONENT_INFO *OMXCore::SearchComponent(
        OMX_STRING cComponentName) 
{
	OMX_U32 i;
	COMPONENT_INFO *pComponentInfoPtr;

	for (i =  0; ; i++)
	{
		pComponentInfoPtr = ComponentList.GetNode(i);
		if (pComponentInfoPtr == NULL)
		{
			break;
		}
		if (!fsl_osal_strcmp((fsl_osal_char *)cComponentName, (fsl_osal_char *) \
					pComponentInfoPtr->ComponentName))
		{
			return pComponentInfoPtr;
		}
	}

	return NULL;
}

OMX_ERRORTYPE OMXCore::ConstructComponent(
		HANDLE_INFO *pHandleInfo,
		OMX_HANDLETYPE hHandle,
		COMPONENT_INFO *pComponentInfoPtr)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_ERRORTYPE (*ComponentEntry)(OMX_HANDLETYPE);

    fsl_osal_mutex_lock(lock);
	pHandleInfo->hlib = LibMgr->load((fsl_osal_char *)pComponentInfoPtr->LibName);
    fsl_osal_mutex_unlock(lock);
	if (pHandleInfo->hlib == NULL)
	{
		LOG_ERROR("Load library fail. library name: %s\n", pComponentInfoPtr->LibName);
		return OMX_ErrorComponentNotFound;
	}

	ComponentEntry = (OMX_ERRORTYPE (*)(void*))LibMgr->getSymbol(pHandleInfo->hlib, \
			(fsl_osal_char *)pComponentInfoPtr->EntryFunction);
	if (ComponentEntry == NULL)
	{
		LOG_ERROR("Can't get component entry function.\n");
		return OMX_ErrorUndefined;
	}

	ret = ComponentEntry(hHandle);
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Can't initialize component.\n");
		return ret;
	}
	pHandleInfo->pHandle = hHandle;

	return ret;
}

OMX_ERRORTYPE OMXCore::OMX_FreeHandle(
        OMX_HANDLETYPE hComponent) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pComponent = NULL;
	HANDLE_INFO *pHandleInfo;
	OMX_S32 i;

	if (hComponent == NULL)
	{
		LOG_ERROR("NULL point.\n");
		return OMX_ErrorBadParameter;
	}

    fsl_osal_mutex_lock(lock);
	for (i = HandleList.GetNodeCnt() - 1; i >= 0; i --)
	{
		pHandleInfo = HandleList.GetNode(i);
		if (pHandleInfo == NULL)
		{
			LOG_ERROR("Free handle list error.\n");
			fsl_osal_mutex_unlock(lock);
			return OMX_ErrorUndefined;
		}

		if (pHandleInfo->pHandle == hComponent)
		{
			pComponent = (OMX_COMPONENTTYPE *)hComponent;

			/** Call component de-initialize */
			ret = pComponent->ComponentDeInit(hComponent);
			if (ret != OMX_ErrorNone)
			{
				LOG_ERROR("Can't de-initialize component.\n");
				fsl_osal_mutex_unlock(lock);
				return ret;
			}

			/** Free handle memory and close library and release handle list resouce */
			FSL_DELETE(pComponent);
			pComponent = NULL;
			LibMgr->unload(pHandleInfo->hlib);
			HandleList.Remove(pHandleInfo);
			FSL_DELETE(pHandleInfo);
			pHandleInfo = NULL;

			fsl_osal_mutex_unlock(lock);
			return OMX_ErrorNone;
		}
	}

	LOG_ERROR("Can't find the component in component list.\n");
    fsl_osal_mutex_unlock(lock);
	return OMX_ErrorBadParameter;
}

OMX_ERRORTYPE OMXCore::OMX_GetComponentsOfRole(
        OMX_STRING role, 
        OMX_U32 *pNumComps, 
        OMX_U8 **compNames) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 i, j, k, nComponentCnt;
	COMPONENT_INFO *pComponentInfo;
	ROLE_INFO *pRoleInfo;
	OMX_S32 RolePriority[MAX_COMPONENTNUM_WITH_SAME_ROLE] = { 0 };
	List<COMPONENT_INFO> ComponentInfoOfRoleList;

	k = 0;
	for (i = 0; ; i ++)
	{
		pComponentInfo = ComponentList.GetNode(i);
		if (pComponentInfo == NULL)
		{
			break;
		}

		for (j = 0; ; j ++)
		{
			pRoleInfo = pComponentInfo->RoleList.GetNode(j);
			if (pRoleInfo == NULL)
			{
				break;
			}

			if (!fsl_osal_strcmp((fsl_osal_char *)pRoleInfo->role, (fsl_osal_char *)role))
			{
				ComponentInfoOfRoleList.Add(pComponentInfo);
				RolePriority[k] = pRoleInfo->priority;
				k ++;
				if (k >= MAX_COMPONENTNUM_WITH_SAME_ROLE)
				{
					LOG_WARNING("Too many component with the role.");
					goto SEARCH_BREAK;
				}
			}
		}
	}
SEARCH_BREAK:

	/** If compNames is NULL, return with the numbers which support the role */
	nComponentCnt = k;
	if (compNames == NULL)
	{
		*pNumComps = nComponentCnt;
		return ret;
	}

	if (nComponentCnt == 0)
	{
		*pNumComps = 0;
		LOG_DEBUG("Haven't found component with role: %s\n.", role);
		return ret;
	}

	if (*pNumComps > nComponentCnt)
	{
		*pNumComps = nComponentCnt;
	}

	for (i = 0; i < *pNumComps; i ++)
	{
		k = 0;
		for (j = 1; j < nComponentCnt; j ++)
		{
			if (RolePriority[j] > RolePriority[k])
			{
				k = j;
			}
		}
		RolePriority[k] = 0;

		pComponentInfo = ComponentInfoOfRoleList.GetNode(k);
		if (pComponentInfo == NULL)
		{
			LOG_ERROR("NULL point.\n");
			return OMX_ErrorUndefined;
		}

		fsl_osal_strcpy(((fsl_osal_char *)compNames[i]), (fsl_osal_char *)pComponentInfo->ComponentName);
	}

	return ret;
}

OMX_ERRORTYPE OMXCore::OMX_GetRolesOfComponent(
        OMX_STRING compName, 
        OMX_U32 *pNumRoles, 
        OMX_U8 **roles) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_U32 i, nRoleCnt;
	COMPONENT_INFO *pComponentInfo = NULL;
	ROLE_INFO *pRoleInfo;

	for (i = 0; ; i ++)
	{
		pComponentInfo = ComponentList.GetNode(i);
		if (pComponentInfo == NULL)
		{
			break;
		}

		if (!fsl_osal_strcmp((fsl_osal_char *)pComponentInfo->ComponentName, (fsl_osal_char *)compName))
		{
			break;
		}
	}

	if (pComponentInfo == NULL)
	{
		*pNumRoles = 0;
		LOG_DEBUG("Haven't found component: %s\n.", compName);
		return OMX_ErrorComponentNotFound;
	}

	/** If roles is NULL, return with the numbers of roles of the component */
	nRoleCnt = pComponentInfo->RoleList.GetNodeCnt();
	if (roles == NULL)
	{
		*pNumRoles = nRoleCnt;
		return ret;
	}

	if (nRoleCnt == 0)
	{
		*pNumRoles = 0;
		LOG_DEBUG("Haven't found role for the component: %s\n.", compName);
		return ret;
	}

	if (*pNumRoles > nRoleCnt)
	{
		*pNumRoles = nRoleCnt;
	}

	for (i = 0; i < *pNumRoles; i ++)
	{
		pRoleInfo = pComponentInfo->RoleList.GetNode(i);
		if (pRoleInfo == NULL)
		{
			break;
		}

		fsl_osal_strcpy(((fsl_osal_char *)roles + i*COMPONENT_NAME_LEN), (fsl_osal_char *)pRoleInfo->role);
	}

	return ret;
}

OMX_ERRORTYPE OMXCore::OMX_SetupTunnel(
        OMX_HANDLETYPE hOutput, 
        OMX_U32 nPortOutput, 
        OMX_HANDLETYPE hInput, 
        OMX_U32 nPortInput) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_TUNNELSETUPTYPE TunnelSetup;
	OMX_COMPONENTTYPE *pInComp, *pOutComp;

	if (hOutput == NULL)
	{
		return OMX_ErrorInvalidComponent;
	}

	fsl_osal_memset(&TunnelSetup, 0, sizeof(OMX_TUNNELSETUPTYPE));
	pInComp = (OMX_COMPONENTTYPE *)hInput;
	pOutComp = (OMX_COMPONENTTYPE *)hOutput;

	TunnelSetup.nTunnelFlags = OMX_PORTTUNNELFLAG_READONLY;

	ret = pOutComp->ComponentTunnelRequest(hOutput, nPortOutput, hInput, nPortInput, &TunnelSetup);
	if (ret != OMX_ErrorNone)
	{
		LOG_DEBUG("Component tunnel request fail.");
		return ret;
	}

        if(hInput == NULL)
            return ret;

	ret = pInComp->ComponentTunnelRequest(hInput, nPortInput, hOutput, nPortOutput, &TunnelSetup);
	if (ret != OMX_ErrorNone)
	{
		LOG_DEBUG("Component tunnel request fail.");
		ret = pOutComp->ComponentTunnelRequest(hOutput, nPortOutput, NULL, 0, NULL);
		if (ret != OMX_ErrorNone)
		{
			LOG_DEBUG("Component tunnel request fail.");
			return ret;
		}
	}

	return ret;
}

OMX_ERRORTYPE OMXCore::OMX_GetContentPipe(
        OMX_HANDLETYPE *hPipe, 
        OMX_STRING szURI) 
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_S32 i;
	CONTENTPIPE_INFO *pContentPipeInfo;
	HANDLE_INFO *ContentPipeHandleListPtr;
	CP_PIPETYPE *pPipe = FSL_NEW(CP_PIPETYPE, ());
	HANDLE_INFO	*pContentPipeHandleInfo = FSL_NEW(HANDLE_INFO, ());

	/* Check if the content pipe already loaded */
	for (i=0; ; i++)
	{
		fsl_osal_mutex_lock(lock);
		ContentPipeHandleListPtr = ContentPipeHandleList.GetNode(i);
		fsl_osal_mutex_unlock(lock);
		if (ContentPipeHandleListPtr == NULL)
		{
			break;
		}

		if (!fsl_osal_strcmp((fsl_osal_char *)szURI, (fsl_osal_char *)ContentPipeHandleListPtr->ContentPipeName))
		{
			FSL_DELETE(pPipe);
			pPipe = NULL;
			FSL_DELETE(pContentPipeHandleInfo);
			pContentPipeHandleInfo = NULL;
			LOG_DEBUG("Content pipe: %s already loaded.\n", szURI);
			*hPipe = ContentPipeHandleListPtr->pHandle;
			return ret;
		}
	}

	fsl_osal_memset(pPipe, 0, sizeof(CP_PIPETYPE));

	for (i = 0; ; i ++)
	{
		pContentPipeInfo = ContentPipeList.GetNode(i);
		if (pContentPipeInfo == NULL)
		{
			break;
		}

		if (!fsl_osal_strcmp((fsl_osal_char *)szURI, (fsl_osal_char *)pContentPipeInfo->ContentPipeName))
		{
			break;
		}
	}

	if (pContentPipeInfo == NULL)
	{
		FSL_DELETE(pPipe);
		pPipe = NULL;
		FSL_DELETE(pContentPipeHandleInfo);
		pContentPipeHandleInfo = NULL;
		LOG_ERROR("Content pipe can't find.\n");
		return OMX_ErrorContentPipeCreationFailed;
	}

	ret = ConstructContentPipe(pContentPipeHandleInfo, pPipe, pContentPipeInfo);
	if (ret != OMX_ErrorNone)
	{
		FSL_DELETE(pPipe);
		pPipe = NULL;
		FSL_DELETE(pContentPipeHandleInfo);
		pContentPipeHandleInfo = NULL;
		LOG_ERROR("Create content pipe fail.\n");
		return ret;
	}

	fsl_osal_strcpy((fsl_osal_char *)pContentPipeHandleInfo->ContentPipeName, (fsl_osal_char *)szURI);
    fsl_osal_mutex_lock(lock);
	ContentPipeHandleList.Add(pContentPipeHandleInfo);
    fsl_osal_mutex_unlock(lock);
	*hPipe = (OMX_HANDLETYPE)pPipe;

	return ret;
}

OMX_ERRORTYPE OMXCore::ConstructContentPipe(
		HANDLE_INFO *pHandleInfo,
		OMX_HANDLETYPE hHandle,
		CONTENTPIPE_INFO *pContentPipeInfoPtr)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	OMX_ERRORTYPE (*ContentPipeEntry)(OMX_HANDLETYPE);

	pHandleInfo->hlib = LibMgr->load((fsl_osal_char *)pContentPipeInfoPtr->LibName);
	if (pHandleInfo->hlib == NULL)
	{
		LOG_ERROR("Load library fail. library name: %s\n", pContentPipeInfoPtr->LibName);
		return OMX_ErrorUndefined;
	}

	ContentPipeEntry = (OMX_ERRORTYPE (*)(OMX_HANDLETYPE))LibMgr->getSymbol(pHandleInfo->hlib, (fsl_osal_char *)pContentPipeInfoPtr->EntryFunction);
	if (ContentPipeEntry == NULL)
	{
		LOG_ERROR("Can't get content pipe entry function.\n");
		return OMX_ErrorUndefined;
	}

	ret = ContentPipeEntry(hHandle);
	if (ret != OMX_ErrorNone)
	{
		LOG_ERROR("Can't initialize component.\n");
		return ret;
	}
	pHandleInfo->pHandle = hHandle;

	return ret;
}

/**< C style functions to expose entry point for the shared library */
extern "C" 
{
    /**< OMX core handle definition, global variable */
    OMXCore *gCoreHandle = NULL;
    static OMX_S32 refCnt = 0;

    OMX_ERRORTYPE OMX_Init() 
    {
        OMX_ERRORTYPE ret = OMX_ErrorNone;

        if(NULL == gCoreHandle) {
            gCoreHandle = FSL_NEW(OMXCore, ());
            if(NULL == gCoreHandle)
                return OMX_ErrorInsufficientResources;
            ret = gCoreHandle->OMX_Init();
            if(ret != OMX_ErrorNone) {
                FSL_DELETE(gCoreHandle);
                return ret;
            }

            CreatePlatformResMgr();

            LOG_DEBUG("OMXCore is Created.\n");
        }

        refCnt ++;

        return ret;
    }

    OMX_ERRORTYPE OMX_Deinit() 
    {
        refCnt --;
        if(refCnt > 0)
            return OMX_ErrorNone;

        if(NULL != gCoreHandle) {
            gCoreHandle->OMX_Deinit();
            FSL_DELETE(gCoreHandle);
            gCoreHandle = NULL;
        }

        DestroyPlatformResMgr();

        LOG_DEBUG("OMXCore is Destroyed.\n");

        return OMX_ErrorNone;
    }

    OMX_ERRORTYPE OMX_ComponentNameEnum(
            OMX_STRING cComponentName, 
            OMX_U32 nNameLength, 
            OMX_U32 nIndex) 
    {
        if(NULL == gCoreHandle)
            return OMX_ErrorNotReady;

        return gCoreHandle->OMX_ComponentNameEnum(cComponentName, nNameLength, nIndex);
    }

    OMX_ERRORTYPE OMX_GetHandle(
            OMX_HANDLETYPE *pHandle, 
            OMX_STRING cComponentName,
            OMX_PTR pAppData, 
            OMX_CALLBACKTYPE *pCallBacks) 
    {
        if(NULL == gCoreHandle)
            return OMX_ErrorNotReady;

        return gCoreHandle->OMX_GetHandle(pHandle, cComponentName, pAppData, pCallBacks);
    }

    OMX_ERRORTYPE OMX_FreeHandle(
            OMX_HANDLETYPE hComponent) 
    {
        if(NULL == gCoreHandle)
            return OMX_ErrorNotReady;

        return gCoreHandle->OMX_FreeHandle(hComponent);
    }

    OMX_ERRORTYPE OMX_GetComponentsOfRole(
            OMX_STRING role, 
            OMX_U32 *pNumComps, 
            OMX_U8 **compNames) 
    {
        if(NULL == gCoreHandle)
            return OMX_ErrorNotReady;

        return gCoreHandle->OMX_GetComponentsOfRole(role, pNumComps, compNames);
    }

    OMX_ERRORTYPE OMX_GetRolesOfComponent(
            OMX_STRING compName, 
            OMX_U32 *pNumRoles, 
            OMX_U8 **roles) 
    {
        if(NULL == gCoreHandle)
            return OMX_ErrorNotReady;

        return gCoreHandle->OMX_GetRolesOfComponent(compName, pNumRoles, roles);
    }

    OMX_ERRORTYPE OMX_SetupTunnel(
            OMX_HANDLETYPE hOutput, 
            OMX_U32 nPortOutput, 
            OMX_HANDLETYPE hInput, 
            OMX_U32 nPortInput) 
    {
        if(NULL == gCoreHandle)
            return OMX_ErrorNotReady;

        return gCoreHandle->OMX_SetupTunnel(hOutput, nPortOutput, hInput, nPortInput);
    }

    OMX_ERRORTYPE OMX_GetContentPipe(
            OMX_HANDLETYPE *hPipe, 
            OMX_STRING szURI) 
    {
        if(NULL == gCoreHandle)
            return OMX_ErrorNotReady;

        return gCoreHandle->OMX_GetContentPipe(hPipe, szURI);
    }
}

/* File EOF */
