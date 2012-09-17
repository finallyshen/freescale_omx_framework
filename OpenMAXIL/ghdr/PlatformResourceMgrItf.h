/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file PlatformResourceMgrItf.h
 *  @brief Interface definition of PlatformResourceMgr
 *  @ingroup State
 */

#ifndef PlatformResourceMgrItf_h
#define PlatformResourceMgrItf_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

OMX_ERRORTYPE CreatePlatformResMgr();
OMX_ERRORTYPE DestroyPlatformResMgr();
OMX_ERRORTYPE AddHwBuffer(OMX_PTR pPhyiscAddr, OMX_PTR pVirtualAddr);
OMX_ERRORTYPE RemoveHwBuffer(OMX_PTR pVirtualAddr);
OMX_ERRORTYPE GetHwBuffer(OMX_PTR pVirtualAddr, OMX_PTR *ppPhyiscAddr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
