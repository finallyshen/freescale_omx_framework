/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file IOMXInterface.h
 *  @brief Abstract Class definition of OpenMAX Core
 *  @ingroup IOMXInterface
 */

#ifndef IOMXInterface_h
#define IOMXInterface_h

#include "OMX_Core.h"

class IOMXInterface {
public:
    virtual ~IOMXInterface() {};
    virtual OMX_ERRORTYPE OMX_Init() = 0;
    virtual OMX_ERRORTYPE OMX_Deinit() = 0;
    virtual OMX_ERRORTYPE OMX_ComponentNameEnum(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex) = 0;
    virtual OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE *pHandle, OMX_STRING cComponentName, OMX_PTR pAppData, OMX_CALLBACKTYPE *pCallBacks) = 0;
    virtual OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent) = 0;
    virtual OMX_ERRORTYPE OMX_GetComponentsOfRole(OMX_STRING role, OMX_U32 *pNumComps, OMX_U8 **compNames) = 0;
    virtual OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_STRING compName, OMX_U32 *pNumRoles, OMX_U8 **roles) = 0;
    virtual OMX_ERRORTYPE OMX_SetupTunnel(OMX_HANDLETYPE hOutput, OMX_U32 nPortOutput, 
                                          OMX_HANDLETYPE hInput, OMX_U32 nPortInput) = 0;
    virtual OMX_ERRORTYPE OMX_GetContentPipe(OMX_HANDLETYPE *hPipe, OMX_STRING szURI) = 0;
};


OMX_ERRORTYPE CreateOMXItf(OMX_STRING core_lib_name, IOMXInterface **pItf);

#endif
/* File EOF */
