/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file ComponentCommon.h
 *  @brief Components common defines 
 *  @ingroup ComponentCommon
 */

#ifndef ComponentCommon_h
#define ComponentCommon_h

#include "OMX_Implement.h"
#include "PlatformResourceMgrItf.h"
#include "fsl_osal.h"
#include "Mem.h"
#include "Log.h"

#include "OMX_Core.h"
#include "OMX_Component.h"    

#define MAX_COMPONENT_ROLE 8
#define MAX_ROLE_NAME_LEN 128

#define TOTAL_STATE 6
#define InvalidStateIdx          0
#define LoadedStateIdx           1
#define IdleStateIdx             2
#define ExecutingStateIdx        3
#define PauseStateIdx            4
#define WaitForResourcesStateIdx 5

#define TOTAL_PORT_PARM          4
#define AudioPortParamIdx        0
#define VideoPortParamIdx        1
#define ImagePortParamIdx        2
#define OtherPortParamIdx        3


#define OMX_TIME_UpdateVideoLate ((OMX_TIME_UPDATETYPE)(OMX_TIME_UpdateVendorStartUnused + 1))
#define OMX_IndexConfigTimeVideoLate FSL_INDEXTYPE(256)  /* OMX_TIME_CONFIG_TIMEVIDEOLATE */

#define OMX_CHECK_STRUCT(ptr, type, err) \
    do { \
        if((ptr)->nSize != sizeof(type)) err = OMX_ErrorBadParameter; \
        if(((ptr)->nVersion.s.nVersionMajor != 0x1)|| \
                ((ptr)->nVersion.s.nVersionMinor != 0x1 &&\
				(ptr)->nVersion.s.nVersionMinor != 0x0 )) \
        err = OMX_ErrorVersionMismatch; \
    } while(0);

#define WAIT_CONDTION(cond, state, to) \
    do { \
        OMX_S32 cnt = to/10000 ; \
        while (cond != state) { \
            fsl_osal_sleep(10000); \
            cnt --; \
            if(cnt <= 0) { \
                LOG_ERROR("wait condition timeout.\n"); \
                return OMX_ErrorTimeout; \
            } \
        } \
    } while(0);

typedef enum{
    COMMAND = 1,
    BUFFER
}MSG_TYPE;

typedef enum {
    PORT_ON = 1,
    PORT_OFF,
    BUFFER_RETURNED
}PORT_NOTIFY_TYPE;

typedef struct {
    OMX_COMMANDTYPE Cmd;
    OMX_U32 nParam;
    OMX_PTR pCmdData;
}CMD_MSG;

typedef struct {
    OMX_COMMANDTYPE cmd;
    OMX_U32 nParam;
    OMX_PTR pCmdData;
    OMX_BOOL bProcessed;
}PENDING_CMD;

typedef struct {
    OMX_HANDLETYPE hTunneledComp;
    OMX_U32 nTunneledPort;
}TUNNEL_INFO;

typedef struct {
    OMX_U32 nSize; 
    OMX_VERSIONTYPE nVersion; 
    OMX_U32 nPortIndex;
    OMX_BOOL bLate;
}OMX_TIME_CONFIG_TIMEVIDEOLATE;

class ComponentBase;
class Port;
class State;

static OMX_U32 pxlfmt2bpp(OMX_COLOR_FORMATTYPE omx_pxlfmt)
{
	OMX_U32 bpp; // bit per pixel

	switch(omx_pxlfmt) {
	case OMX_COLOR_FormatMonochrome:
	  bpp = 1;
	  break;
	case OMX_COLOR_FormatL2:
	  bpp = 2;
	case OMX_COLOR_FormatL4:
	  bpp = 4;
	  break;
	case OMX_COLOR_FormatL8:
	case OMX_COLOR_Format8bitRGB332:
	case OMX_COLOR_FormatRawBayer8bit:
	case OMX_COLOR_FormatRawBayer8bitcompressed:
	  bpp = 8;
	  break;
	case OMX_COLOR_FormatRawBayer10bit:
	  bpp = 10;
	  break;
	case OMX_COLOR_FormatYUV411Planar:
	case OMX_COLOR_FormatYUV411PackedPlanar:
	case OMX_COLOR_Format12bitRGB444:
	case OMX_COLOR_FormatYUV420Planar:
	case OMX_COLOR_FormatYUV420PackedPlanar:
	case OMX_COLOR_FormatYUV420SemiPlanar:
	case OMX_COLOR_FormatYUV444Interleaved:
	  bpp = 12;
	  break;
	case OMX_COLOR_FormatL16:
	case OMX_COLOR_Format16bitARGB4444:
	case OMX_COLOR_Format16bitARGB1555:
	case OMX_COLOR_Format16bitRGB565:
	case OMX_COLOR_Format16bitBGR565:
	case OMX_COLOR_FormatYUV422Planar:
	case OMX_COLOR_FormatYUV422PackedPlanar:
	case OMX_COLOR_FormatYUV422SemiPlanar:
	case OMX_COLOR_FormatYCbYCr:
	case OMX_COLOR_FormatYCrYCb:
	case OMX_COLOR_FormatCbYCrY:
	case OMX_COLOR_FormatCrYCbY:
	  bpp = 16;
	  break;
	case OMX_COLOR_Format18bitRGB666:
	case OMX_COLOR_Format18bitARGB1665:
	  bpp = 18;
	  break;
	case OMX_COLOR_Format19bitARGB1666:
	  bpp = 19;
	  break;
	case OMX_COLOR_FormatL24:
	case OMX_COLOR_Format24bitRGB888:
	case OMX_COLOR_Format24bitBGR888:
	case OMX_COLOR_Format24bitARGB1887:
	  bpp = 24;
	  break;
	case OMX_COLOR_Format25bitARGB1888:
	  bpp = 25;
	  break;
	case OMX_COLOR_FormatL32:
	case OMX_COLOR_Format32bitBGRA8888:
	case OMX_COLOR_Format32bitARGB8888:
	  bpp = 32;
	  break;
	default:
	  bpp = 0;
	  break;
	}
	return bpp;
}

#endif
/* File EOF */
