/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file ShareLibarayMgr.h
 *  @brief Class definition of shared library manager
 *  @ingroup ShareLibarayMgr
 */


#ifndef ShareLibarayMgr_h
#define ShareLibarayMgr_h

#include "fsl_osal.h"
#include "List.h"

#define LIB_NAME_LEN 512

typedef struct _LIB_INFO {
    fsl_osal_char lib_name[LIB_NAME_LEN];
    fsl_osal_ptr hlib;
    fsl_osal_s32 refCount;
}LIB_INFO;

class ShareLibarayMgr {
public:
    fsl_osal_ptr load(fsl_osal_char *lib_name);
    fsl_osal_s32 unload(fsl_osal_ptr hlib);
    fsl_osal_ptr getSymbol(fsl_osal_ptr hlib, fsl_osal_char *symbol);
private:
    List<LIB_INFO> lib_info;
};

#endif
/* File EOF */
