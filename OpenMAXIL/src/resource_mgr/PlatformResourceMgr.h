/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file PlatformResourceMgr.h
 *  @brief Interface definition of PlatformResourceMgr
 *  @ingroup State
 */

#ifndef PlatformResourceMgr_h
#define PlatformResourceMgr_h

#include "OMX_Core.h"
#include "fsl_osal.h"
#include "List.h"

typedef struct {
    OMX_PTR pVirtualAddr;
    OMX_PTR pPhyiscAddr;
}PLATFORM_DATA;

class PlatformResourceMgr {
    public:
        OMX_ERRORTYPE Init();
        OMX_ERRORTYPE DeInit();
        OMX_ERRORTYPE AddHwBuffer(OMX_PTR pPhyiscAddr, OMX_PTR pVirtualAddr);
        OMX_ERRORTYPE RemoveHwBuffer(OMX_PTR pVirtualAddr);
        OMX_PTR GetHwBuffer(OMX_PTR pVirtualAddr);
    private:
	List<PLATFORM_DATA> *PlatformDataList;
        fsl_osal_mutex lock;
        OMX_PTR SearchData(OMX_PTR pVirtualAddr);
};

#endif
