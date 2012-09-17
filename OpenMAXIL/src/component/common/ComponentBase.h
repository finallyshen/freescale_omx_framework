/**
 *  Copyright (c) 2009-2010, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file ComponentBase.h
 *  @brief Base class definition of component
 *  @ingroup ComponentBase
 */

#ifndef ComponentBase_h
#define ComponentBase_h

#include "ComponentCommon.h"
#include "Port.h"
#include "Queue.h"
#include "State.h"
#include "List.h"

class ComponentBase {
    public:
        friend class State;
        friend class LoadedState;
        friend class IdleState;
        friend class ExecutingState;
        friend class PauseState;
        friend void *DoThread(void *arg);
        State *CurState; /**< stores the instance of current state */
        ComponentBase();
        virtual ~ComponentBase() {};
        OMX_ERRORTYPE ConstructComponent(OMX_HANDLETYPE pHandle);
        OMX_ERRORTYPE DestructComponent();
        OMX_ERRORTYPE SetCallbacks(OMX_CALLBACKTYPE* pCbs, OMX_PTR pAppData);
        OMX_ERRORTYPE ComponentRoleEnum(OMX_U8 *cRole, OMX_U32 nIndex);
        OMX_HANDLETYPE GetComponentHandle();
        OMX_ERRORTYPE GetState(OMX_STATETYPE* pState);
        OMX_ERRORTYPE SetState(OMX_STATETYPE eState);
        OMX_ERRORTYPE SendEvent(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
        OMX_ERRORTYPE EmptyDone(OMX_BUFFERHEADERTYPE* pBufferHdr);
        OMX_ERRORTYPE FillDone(OMX_BUFFERHEADERTYPE* pBufferHdr);
        OMX_ERRORTYPE PortNotify(OMX_U32 nPortIndex, PORT_NOTIFY_TYPE ePortNotify);
        OMX_ERRORTYPE MarkOtherPortsBuffer(OMX_MARKTYPE *pMarkData);
        OMX_BOOL bHasCmdToProcess();
        virtual OMX_ERRORTYPE DoAllocateBuffer(OMX_PTR *buffer, OMX_U32 nSize,OMX_U32 nPortIndex);
        virtual OMX_ERRORTYPE DoFreeBuffer(OMX_PTR buffer,OMX_U32 nPortIndex);
        virtual OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex);
        virtual OMX_ERRORTYPE ComponentReturnBuffer(OMX_U32 nPortIndex);
        virtual OMX_ERRORTYPE UseEGLImage(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, void* eglImage);
        OMX_U8 name[MAX_NAME_LEN];
    protected:

        OMX_VERSIONTYPE ComponentVersion;
        OMX_U32 role_cnt; /**< how many roles this component supported */
        OMX_STRING role[MAX_COMPONENT_ROLE]; /**< store all the roles this component supported */
        OMX_BOOL bInContext;
        OMX_U32 nPorts;
        Port *ports[MAX_PORT_NUM]; /**< store all the port instance this component have */
        OMX_PORT_PARAM_TYPE PortParam[TOTAL_PORT_PARM];
    private:
        OMX_VERSIONTYPE SpecVersion;
        OMX_COMPONENTTYPE *hComponent;
        OMX_CALLBACKTYPE *pCallbacks;
        fsl_osal_ptr pThreadId;
        fsl_osal_sem outContextSem;
        fsl_osal_mutex SemLock;
        OMX_S32 SemCnt;
        OMX_BOOL bStopThread;
        fsl_osal_mutex inContextMutex;
        fsl_osal_u32 InContextThreadId;
        List<MSG_TYPE> DelayedInContextMsgList;
        OMX_STATETYPE eCurState;
        State *states[TOTAL_STATE]; /**< stores all the instance of each state */
        Queue *pCmdQueue;
        PENDING_CMD pendingCmd; /**< stores the current processing command */
        virtual OMX_ERRORTYPE InitComponent() = 0;
        virtual OMX_ERRORTYPE DeInitComponent() = 0;
        virtual OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
        virtual OMX_ERRORTYPE GetExtensionIndex(OMX_STRING cParameterName, OMX_INDEXTYPE* pIndexType);
        virtual OMX_ERRORTYPE PortFormatChanged(OMX_U32 nPortIndex);
        virtual OMX_ERRORTYPE InstanceInit();
        virtual OMX_ERRORTYPE InstanceDeInit();
        virtual OMX_ERRORTYPE DoLoaded2Idle();
        virtual OMX_ERRORTYPE DoIdle2Loaded();
        virtual OMX_ERRORTYPE DoExec2Pause();
        virtual OMX_ERRORTYPE DoPause2Exec();
        OMX_ERRORTYPE ProcessInContext(MSG_TYPE msg);
        OMX_ERRORTYPE ProcessOutContext(MSG_TYPE msg);
        OMX_ERRORTYPE Up();
        OMX_ERRORTYPE Down();
        virtual OMX_ERRORTYPE ProcessClkBuffer();
        virtual OMX_ERRORTYPE ProcessDataBuffer() = 0;
        OMX_BOOL CheckAllPortsState(PORT_NOTIFY_TYPE ePortNotify);
};

#endif
/* File EOF */
