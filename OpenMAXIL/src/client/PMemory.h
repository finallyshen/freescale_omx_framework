/**
 *  Copyright (c) 2010,2012 Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */


#ifndef PMemory_h
#define PMemory_h

#include "OMX_Core.h"

typedef struct {
    OMX_PTR pVirtualAddr;
    OMX_PTR pPhysicAddr;
}PMEMADDR;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

OMX_PTR CreatePMeoryContext();
OMX_ERRORTYPE DestroyPMeoryContext(OMX_PTR Context);
PMEMADDR GetPMBuffer(OMX_PTR Context, OMX_U32 nSize, OMX_U32 nNum);
OMX_ERRORTYPE FreePMBuffer(OMX_PTR Context, OMX_PTR pBuffer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
/* File EOF */
