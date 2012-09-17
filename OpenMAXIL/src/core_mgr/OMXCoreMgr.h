/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file OMXCoreMgr.h
 *  @brief Class definition of OpenMAX core manager
 *  @ingroup OMXCoreMgr
 */

#ifndef OMXCoreMgr_h
#define OMXCoreMgr_h

#include "List.h"
#include "ShareLibarayMgr.h"
#include "IOMXInterface.h"
#include "OMX_Implement.h"

typedef struct _CORE_INFO {
    OMX_S8 CoreName[CORE_NAME_LEN];
    OMX_S8 LibName[LIB_NAME_LEN];
    OMX_S32 priority; /**< Core priority, [1, 5]. 3 is the normal priority, 
						1 is the lowest priority, 5 is the highest priority */ 
	IOMXInterface *Interface;
	OMX_U32 ComponentCnt;
}CORE_INFO;

typedef struct _HANDLE_INFO {
    OMX_HANDLETYPE pHandle;
    OMX_U32 CoreIndex;
}HANDLE_INFO;

class OMXCoreMgr {
public:
    OMX_ERRORTYPE OMX_Init();
    OMX_ERRORTYPE OMX_Deinit();
    OMX_ERRORTYPE OMX_ComponentNameEnum(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex);
    OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE *pHandle, OMX_STRING cComponentName,
                                OMX_PTR pAppData, OMX_CALLBACKTYPE *pCallBacks);
    OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent);
    OMX_ERRORTYPE OMX_GetComponentsOfRole(OMX_STRING role, OMX_U32 *pNumComps, OMX_U8 **compNames);
    OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_STRING compName, OMX_U32 *pNumRoles, OMX_U8 **roles);
    OMX_ERRORTYPE OMX_SetupTunnel(OMX_HANDLETYPE hOutput, OMX_U32 nPortOutput, OMX_HANDLETYPE hInput, OMX_U32 nPortInput);
    OMX_ERRORTYPE OMX_GetContentPipe(OMX_HANDLETYPE *hPipe, OMX_STRING szURI);
private:
    List<CORE_INFO> CoreInfoList;
    List<HANDLE_INFO> HandleList;
	fsl_osal_mutex lock;
	OMX_ERRORTYPE CoreRegister();
	OMX_ERRORTYPE FreeAllCore();
	OMX_ERRORTYPE FreeCoreMgrResource();
};

#endif
/* File EOF */
