/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "Mem.h"
#include "Log.h"
#include "OMXInterface.h"

OMX_ERRORTYPE OMXInterface::Construct(
        OMX_STRING lib_name) 
{
	lib_mgr = FSL_NEW(ShareLibarayMgr, ());
	if (lib_mgr == NULL)
	{
		LOG_ERROR("Can't get memory.\n");
		return OMX_ErrorInsufficientResources;
	}

    hCoreLib = lib_mgr->load(lib_name);
    if(NULL == hCoreLib)
	{
		LOG_ERROR("core library load fail.\n");
        goto err1;
	}

    pOMX_Init = (OMX_ERRORTYPE (*)())lib_mgr->getSymbol(hCoreLib, (fsl_osal_char*)"OMX_Init");
    pOMX_Deinit = (OMX_ERRORTYPE (*)())lib_mgr->getSymbol(hCoreLib, (fsl_osal_char*)"OMX_Deinit");
    pOMX_ComponentNameEnum = (OMX_ERRORTYPE (*)(OMX_STRING, OMX_U32, OMX_U32))lib_mgr->getSymbol(hCoreLib, (fsl_osal_char*)"OMX_ComponentNameEnum");
    pOMX_GetHandle = (OMX_ERRORTYPE (*)(OMX_HANDLETYPE *, OMX_STRING, OMX_PTR,\
				OMX_CALLBACKTYPE *))lib_mgr->getSymbol(hCoreLib, (fsl_osal_char*)"OMX_GetHandle");
    pOMX_FreeHandle = (OMX_ERRORTYPE (*)(OMX_HANDLETYPE))lib_mgr->getSymbol(hCoreLib, (fsl_osal_char*)"OMX_FreeHandle");
    pOMX_GetComponentsOfRole = (OMX_ERRORTYPE (*)(OMX_STRING, OMX_U32 *, OMX_U8 **))lib_mgr->getSymbol(hCoreLib, (fsl_osal_char*)"OMX_GetComponentsOfRole");
    pOMX_GetRolesOfComponent = (OMX_ERRORTYPE (*)(OMX_STRING, OMX_U32 *, OMX_U8 **))lib_mgr->getSymbol(hCoreLib, (fsl_osal_char*)"OMX_GetRolesOfComponent");
    pOMX_SetupTunnel = (OMX_ERRORTYPE (*)(OMX_HANDLETYPE, OMX_U32, OMX_HANDLETYPE,\
				OMX_U32))lib_mgr->getSymbol(hCoreLib, (fsl_osal_char*)"OMX_SetupTunnel");
    pOMX_GetContentPipe = (OMX_ERRORTYPE (*)(OMX_HANDLETYPE *, OMX_STRING))lib_mgr->getSymbol(hCoreLib, (fsl_osal_char*)"OMX_GetContentPipe");

    if(NULL == pOMX_Init
            || NULL == pOMX_Deinit
            || NULL == pOMX_ComponentNameEnum
            || NULL == pOMX_GetHandle
            || NULL == pOMX_FreeHandle
            || NULL == pOMX_GetComponentsOfRole
            || NULL == pOMX_GetRolesOfComponent
            || NULL == pOMX_SetupTunnel
            || NULL == pOMX_GetContentPipe)
	{
		LOG_ERROR("Get core symbol error.\n");
        goto err2;
	}

    return OMX_ErrorNone;

err2:
    lib_mgr->unload(hCoreLib);
err1:
    FSL_DELETE(lib_mgr);
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE OMXInterface::DeConstruct() 
{
    lib_mgr->unload(hCoreLib);
    FSL_DELETE(lib_mgr);
	FSL_DELETE_THIS(this);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE OMXInterface::OMX_Init() 
{
    return (*pOMX_Init)();
}

OMX_ERRORTYPE OMXInterface::OMX_Deinit() 
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = (*pOMX_Deinit)();
    if(ret != OMX_ErrorNone) {
		LOG_ERROR("OMX Deinit fail.\n");
        return ret;
    }

    ret = DeConstruct();
    if(ret != OMX_ErrorNone) {
		LOG_ERROR("Core interface de-construct fail.\n");
        return ret;
    }

    return ret;
}

OMX_ERRORTYPE OMXInterface::OMX_ComponentNameEnum(
        OMX_STRING cComponentName, 
        OMX_U32 nNameLength, 
        OMX_U32 nIndex) 
{
    return (*pOMX_ComponentNameEnum)(cComponentName, nNameLength, nIndex); 
}

OMX_ERRORTYPE OMXInterface::OMX_GetHandle(
        OMX_HANDLETYPE *pHandle, 
        OMX_STRING cComponentName,
        OMX_PTR pAppData, 
        OMX_CALLBACKTYPE *pCallBacks) 
{
    return (*pOMX_GetHandle)(pHandle, cComponentName, pAppData, pCallBacks);
}

OMX_ERRORTYPE OMXInterface::OMX_FreeHandle(
        OMX_HANDLETYPE hComponent) 
{
    return (*pOMX_FreeHandle)(hComponent);
}

OMX_ERRORTYPE OMXInterface::OMX_GetComponentsOfRole(
        OMX_STRING role, 
        OMX_U32 *pNumComps, 
        OMX_U8 **compNames) 
{
    return (*pOMX_GetComponentsOfRole)(role, pNumComps, compNames);
}

OMX_ERRORTYPE OMXInterface::OMX_GetRolesOfComponent(
        OMX_STRING compName, 
        OMX_U32 *pNumRoles, 
        OMX_U8 **roles) 
{
    return (*pOMX_GetRolesOfComponent)(compName, pNumRoles, roles);
}

OMX_ERRORTYPE OMXInterface::OMX_SetupTunnel(
        OMX_HANDLETYPE hOutput, 
        OMX_U32 nPortOutput, 
        OMX_HANDLETYPE hInput, 
        OMX_U32 nPortInput) 
{
    return (*pOMX_SetupTunnel)(hOutput, nPortOutput, hInput, nPortInput);
}

OMX_ERRORTYPE OMXInterface::OMX_GetContentPipe(
        OMX_HANDLETYPE *hPipe, 
        OMX_STRING szURI) 
{
    return (*pOMX_GetContentPipe)(hPipe, szURI);
}

OMX_ERRORTYPE CreateOMXItf(OMX_STRING core_lib_name, IOMXInterface **pItf)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMXInterface *pOmxItf = NULL;

    *pItf = NULL;
    pOmxItf = FSL_NEW(OMXInterface, ());
    if(NULL == pOmxItf)
	{
		LOG_ERROR("Can't get memory.\n");
        return OMX_ErrorInsufficientResources;
	}

    ret = pOmxItf->Construct(core_lib_name);
    if(ret != OMX_ErrorNone) {
		LOG_ERROR("Core interface construct fail.\n");
        FSL_DELETE(pOmxItf);
        return ret;
    }

    *pItf = (IOMXInterface*)pOmxItf;

    return ret;
}

/* File EOF */
