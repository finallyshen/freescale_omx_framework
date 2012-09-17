/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file OMXInterface.h
 *  @brief Implement Class definition of OpenMAX Core
 *  @ingroup OMXInterface
 */

#ifndef OMXInterface_h
#define OMXInterface_h


#include "IOMXInterface.h"
#include "ShareLibarayMgr.h"

class OMXInterface : public IOMXInterface {
public:
    OMX_ERRORTYPE Construct(OMX_STRING lib_name);
    OMX_ERRORTYPE DeConstruct();
    OMX_ERRORTYPE OMX_Init();
    OMX_ERRORTYPE OMX_Deinit();
    OMX_ERRORTYPE OMX_ComponentNameEnum(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex);
    OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE *pHandle, OMX_STRING cComponentName,
                                OMX_PTR pAppData, OMX_CALLBACKTYPE *pCallBacks);
    OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent);
    OMX_ERRORTYPE OMX_GetComponentsOfRole(OMX_STRING role, OMX_U32 *pNumComps, OMX_U8 **compNames);
    OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_STRING compName, OMX_U32 *pNumRoles, OMX_U8 **roles);
    OMX_ERRORTYPE OMX_SetupTunnel(OMX_HANDLETYPE hOutput, OMX_U32 nPortOutput, 
                                  OMX_HANDLETYPE hInput, OMX_U32 nPortInput);
    OMX_ERRORTYPE OMX_GetContentPipe(OMX_HANDLETYPE *hPipe, OMX_STRING szURI);
private:
    OMX_PTR hCoreLib;
    ShareLibarayMgr *lib_mgr;
    OMX_ERRORTYPE (*pOMX_Init)();
    OMX_ERRORTYPE (*pOMX_Deinit)();
    OMX_ERRORTYPE (*pOMX_ComponentNameEnum)(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex);
    OMX_ERRORTYPE (*pOMX_GetHandle)(OMX_HANDLETYPE *pHandle, OMX_STRING cComponentName,
                                    OMX_PTR pAppData, OMX_CALLBACKTYPE *pCallBacks);
    OMX_ERRORTYPE (*pOMX_FreeHandle)(OMX_HANDLETYPE hComponent);
    OMX_ERRORTYPE (*pOMX_GetComponentsOfRole)(OMX_STRING role, OMX_U32 *pNumComps, OMX_U8 **compNames);
    OMX_ERRORTYPE (*pOMX_GetRolesOfComponent)(OMX_STRING compName, OMX_U32 *pNumRoles, OMX_U8 **roles);
    OMX_ERRORTYPE (*pOMX_SetupTunnel)(OMX_HANDLETYPE hOutput, OMX_U32 nPortOutput, 
                                      OMX_HANDLETYPE hInput, OMX_U32 nPortInput);
    OMX_ERRORTYPE (*pOMX_GetContentPipe)(OMX_HANDLETYPE *hPipe, OMX_STRING szURI);
};

#endif
