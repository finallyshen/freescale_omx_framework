/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file Clock.h
 *  @brief Class definition of Clock Component
 *  @ingroup Clock
 */

#ifndef Clock_h
#define Clock_h

#include "ComponentBase.h"

#define PORT_NUM 8

class Clock : public ComponentBase {
    public:
        Clock();
    private:
        fsl_osal_timeval WallTimeBase;
        OMX_TICKS MediaTimeBase;
        OMX_TIME_REFCLOCKTYPE RefClock;
        OMX_S32 Scale;
        OMX_TIME_CLOCKSTATE CurState;
        OMX_TIME_CONFIG_CLOCKSTATETYPE sState;
        OMX_U32 StartTimeWaitMask;
        OMX_U64 SegmentStartTime;
        fsl_osal_mutex lock;
        fsl_osal_ptr Cond;
        OMX_BOOL bPaused;
        OMX_ERRORTYPE InitComponent();
        OMX_ERRORTYPE DeInitComponent();
        OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE ProcessDataBuffer();
        OMX_ERRORTYPE ProcessClkBuffer();
        OMX_ERRORTYPE DoExec2Pause();
        OMX_ERRORTYPE DoPause2Exec();
        OMX_ERRORTYPE SetTimeScale(OMX_TIME_CONFIG_SCALETYPE *pScale);
        OMX_ERRORTYPE SetState(OMX_TIME_CONFIG_CLOCKSTATETYPE *pState);
        OMX_ERRORTYPE SetRefClock(OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE *pRefClock);
        OMX_ERRORTYPE SetRefTime(OMX_TIME_CONFIG_TIMESTAMPTYPE *pTs, OMX_TIME_REFCLOCKTYPE eClock);
        OMX_ERRORTYPE MediaTimeRequest(OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE *pRequst);
        OMX_ERRORTYPE SetStartTime(OMX_TIME_CONFIG_TIMESTAMPTYPE *pStartTime);
        OMX_ERRORTYPE SetVideoLate(OMX_TIME_CONFIG_TIMEVIDEOLATE *pVideoLate);
        OMX_ERRORTYPE MediaTimeUpdate(OMX_TIME_MEDIATIMETYPE *pUpdate, OMX_U32 nPortIndex);
        OMX_ERRORTYPE CurMediaAndWallTime(OMX_TICKS *pMediaTime, fsl_osal_timeval *pWallTime);
};

#endif
/* File EOF */
