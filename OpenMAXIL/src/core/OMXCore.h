/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file OMXCore.h
 *  @brief Class definition of OpenMAX core
 *  @ingroup OMXCore
 */

#ifndef OMXCore_h
#define OMXCore_h

#include "List.h"
#include "OMX_Core.h"
#include "ShareLibarayMgr.h"
#include "OMX_Implement.h"

typedef struct _CONTENTPIPE_INFO {
    OMX_S8 ContentPipeName[CONTENTPIPE_NAME_LEN];
    OMX_S8 LibName[LIB_NAME_LEN];
    OMX_S8 EntryFunction[FUNCTION_NAME_LEN];
}CONTENTPIPE_INFO;

typedef struct _ROLE_INFO {
    OMX_S8 role[ROLE_NAME_LEN];
    OMX_S32 priority; /**< Component priority, [1, 5]. 3 is the normal priority, 
						1 is the lowest priority, 5 is the highest priority */ 
}ROLE_INFO;


typedef struct _COMPONENT_INFO {
    OMX_S8 ComponentName[COMPONENT_NAME_LEN];
    OMX_S8 LibName[LIB_NAME_LEN];
    OMX_S8 EntryFunction[FUNCTION_NAME_LEN];
    List<ROLE_INFO> RoleList;
}COMPONENT_INFO;


typedef struct _HANDLE_INFO {
    OMX_HANDLETYPE pHandle;
    OMX_PTR hlib;
    OMX_S8 ContentPipeName[CONTENTPIPE_NAME_LEN];
}HANDLE_INFO;

class OMXCore {
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
	List<CONTENTPIPE_INFO> ContentPipeList;
    List<COMPONENT_INFO> ComponentList;
    List<HANDLE_INFO> HandleList;
	List<HANDLE_INFO> ContentPipeHandleList;
    ShareLibarayMgr *LibMgr;
	fsl_osal_mutex lock;
	OMX_ERRORTYPE ComponentRegister();
	OMX_ERRORTYPE ContentPipeRegister();
	OMX_ERRORTYPE FreeAllComponent();
	OMX_ERRORTYPE FreeAllContentPipe();
	OMX_ERRORTYPE FreeCoreResource();
	COMPONENT_INFO *SearchComponent(OMX_STRING cComponentName);
	OMX_ERRORTYPE ConstructComponent(HANDLE_INFO *pHandleInfo, OMX_HANDLETYPE hHandle, \
			COMPONENT_INFO *pComponentInfoPtr);
	OMX_ERRORTYPE ConstructContentPipe(HANDLE_INFO *pHandleInfo, OMX_HANDLETYPE hHandle, \
			CONTENTPIPE_INFO *pContentPipeInfoPtr);

};

#endif
/* File EOF */
