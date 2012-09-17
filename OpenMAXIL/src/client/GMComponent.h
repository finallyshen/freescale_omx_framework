/**
 *  Copyright (c) 2009-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file GMComponent.h
 *  @brief Class definition of graph manager component
 *  @ingroup GraphManager
 */

#ifndef GMComponent_h
#define GMComponent_h

#include <unistd.h>
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Implement.h"
#include "fsl_osal.h"
#include "List.h"
#include "Mem.h"
#include "Log.h"

//#define LOG_DEBUG printf

class GMComponent;

#define TOTAL_PORT_PARA 4
#define AUDIO_PORT_PARA 0
#define VIDEO_PORT_PARA 1
#define IMAGE_PORT_PARA 2
#define OTHER_PORT_PARA 3

typedef enum {
    GM_SELF_ALLOC,
    GM_PEER_ALLOC,
    GM_CLIENT_ALLOC,
    GM_PORT_SHARE,
    GM_USE_EGLIMAGE,
    GM_PEER_BUFFER
}GM_BUFFERALLOCATER;

typedef enum {
    GM_VIRTUAL,
    GM_PHYSIC,
    GM_EGLIMAGE
}GMBUFFERTYPE;

typedef struct {
    GMBUFFERTYPE type;
    OMX_U32 nSize;
    OMX_U32 nNum;
}GMBUFFERINFO;

typedef OMX_ERRORTYPE (*SYSCALLBACKTYPE)(GMComponent *pComponent, OMX_PTR pAppData, OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
typedef OMX_ERRORTYPE (*SYSBUFFERCB)(GMComponent *pComponent, OMX_BUFFERHEADERTYPE *pBufferHdr, OMX_U32 nPortIndex, OMX_PTR pAppData);
typedef OMX_ERRORTYPE (*GETBUFFERCB)(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_PTR *pBuffer, GMBUFFERINFO *pInfo, OMX_PTR pAppData);
typedef OMX_ERRORTYPE (*FREEBUFFERCB)(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex, OMX_PTR pBuffer, GMBUFFERINFO *pInfo, OMX_PTR pAppData);

typedef struct {
    GETBUFFERCB GetBuffer;
    FREEBUFFERCB FreeBuffer;
}GM_BUFFERCB;

typedef struct {
    GMComponent *pComponent;
    OMX_U32 nPortIndex;
}GM_PEERINFO;

typedef struct {
    OMX_DIRTYPE eDir;
    OMX_PORTDOMAINTYPE eDomain;
    OMX_BOOL bEnabled;
    OMX_U32 nBuffers;
    OMX_U32 nBufferSize;
    OMX_BUFFERHEADERTYPE *pBufferHdr[MAX_PORT_BUFFER];
    GM_BUFFERALLOCATER Allocater;
    GM_BUFFERALLOCATER Allocated;
    GM_PEERINFO peer;
    OMX_BOOL bOutputed;
    OMX_BOOL bSettingChanged;
    OMX_BOOL bPortResourceChanged;
    OMX_BOOL bHoldBuffers;
    OMX_BOOL bFreeBuffers;
	OMX_U32 nBufferHolded;
    fsl_osal_mutex HoldBufferListLock;
    List<OMX_BUFFERHEADERTYPE> HoldBufferList;
}GM_PORTINFO;

typedef struct {
    OMX_COMMANDTYPE Cmd;
    OMX_U32 nParam1;
    OMX_PTR pCmdData;
}GM_CMDINFO;


class GMComponent {
    public:
        OMX_U8 name[COMPONENT_NAME_LEN];
        OMX_HANDLETYPE hComponent;
        OMX_U32 nPorts;
        OMX_PORT_PARAM_TYPE PortPara[TOTAL_PORT_PARA];

        OMX_ERRORTYPE Load(OMX_STRING role);
        OMX_ERRORTYPE UnLoad();
        OMX_ERRORTYPE SetPortBufferAllocateType(OMX_U32 nPortIndex, GM_BUFFERALLOCATER eAllocater);
        OMX_ERRORTYPE Link(OMX_U32 nPortIndex, GMComponent *pPeer, OMX_U32 nPeerPortIndex);
        OMX_ERRORTYPE UnLink(OMX_U32 nPortIndex);
        OMX_ERRORTYPE StateTransUpWard(OMX_STATETYPE eState);
        OMX_ERRORTYPE StateTransDownWard(OMX_STATETYPE eState);
        OMX_ERRORTYPE StartProcessNoWait(OMX_U32 nPortIndex = OMX_ALL);
        OMX_ERRORTYPE StartProcess(OMX_U32 nPortIndex = OMX_ALL);
		OMX_BOOL IsPortEnabled(OMX_U32 nPortIndex);
		OMX_ERRORTYPE HoldBufferCheck(OMX_U32 nPortIndex);
		OMX_ERRORTYPE WaitAndFreePortBuffer(OMX_U32 nPortIndex);
		OMX_ERRORTYPE PortDisable(OMX_U32 nPortIndex);
		OMX_ERRORTYPE PortEnable(OMX_U32 nPortIndex);
		OMX_ERRORTYPE PortFlush(OMX_U32 nPortIndex);
		OMX_ERRORTYPE PortMarkBuffer(OMX_U32 nPortIndex, OMX_PTR pMarkData);
        OMX_ERRORTYPE TunneledPortDisable(OMX_U32 nPortIndex);
        OMX_ERRORTYPE WaitTunneledPortDisable(OMX_U32 nPortIndex);
        OMX_ERRORTYPE TunneledPortEnable(OMX_U32 nPortIndex);
        OMX_ERRORTYPE WaitTunneledPortEnable(OMX_U32 nPortIndex);
        OMX_ERRORTYPE PassOutBufferToPeer(OMX_U32 nPortIndex);
        OMX_ERRORTYPE GetComponentPortsPara();
        OMX_ERRORTYPE RegistSysCallback(SYSCALLBACKTYPE sysCb, OMX_PTR pAppData);
        OMX_ERRORTYPE RegistSysBufferCallback(SYSBUFFERCB sysBufferCb, OMX_PTR pAppData);
        OMX_ERRORTYPE RegistBufferCallback(GM_BUFFERCB *pBufferCb, OMX_PTR pAppData);
        OMX_ERRORTYPE CbEventHandler(OMX_EVENTTYPE eEvent, OMX_U32 nData1, OMX_U32 nData2, OMX_PTR pEventData);
        OMX_ERRORTYPE CbEmptyBufferDone(OMX_BUFFERHEADERTYPE* pBuffer);
        OMX_ERRORTYPE CbFillBufferDone(OMX_BUFFERHEADERTYPE* pBuffer);
    private:
        GM_PORTINFO PortsInfo[MAX_PORT_NUM];
        OMX_BOOL bError;
        GM_CMDINFO ProcessingCmd;
        GM_CMDINFO DoneCmd;
        SYSCALLBACKTYPE SysEventHandler;
        OMX_PTR SysEventAppData;
        SYSBUFFERCB SysBufferHandler;
        OMX_PTR SysBufferAppData;
        GM_BUFFERCB *pBufferCallback;
        OMX_PTR pBufAppData;
        OMX_BOOL bResetTimeout;

        OMX_ERRORTYPE SendCommand(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData);
        OMX_ERRORTYPE WaitCommand(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData, OMX_TICKS timeout);
        OMX_ERRORTYPE DoLoaded2Idle();
        OMX_ERRORTYPE DoIdle2Loaded();
        OMX_ERRORTYPE DoExec2Idle(OMX_STATETYPE eState);
        OMX_ERRORTYPE DoStateTrans(OMX_STATETYPE eState, OMX_TICKS timeout);
        OMX_ERRORTYPE PortAllocateBuffers(OMX_U32 nPortIndex);
        OMX_ERRORTYPE PortUsePeerBuffers(OMX_U32 nPortIndex);
        OMX_ERRORTYPE PortUseClientBuffers(OMX_U32 nPortIndex);
        OMX_ERRORTYPE PortUseShareBuffers(OMX_U32 nPortIndex);
        OMX_ERRORTYPE PortFreeBuffers(OMX_U32 nPortIndex);
        OMX_ERRORTYPE PortUseEglImage(OMX_U32 nPortIndex);
        OMX_ERRORTYPE GetPortBuffer(OMX_U32 nPortIndex, OMX_U32 nBufferIndex, OMX_BUFFERHEADERTYPE **ppBufferHdr);
        OMX_ERRORTYPE WaitForOutput(OMX_U32 nPortIndex);
        OMX_ERRORTYPE HoldingOutPortBuffer(OMX_BUFFERHEADERTYPE *pBufferHdr);
        OMX_ERRORTYPE DoPortSettingChanged(OMX_U32 nPortIndex);
        OMX_ERRORTYPE SetPeerPortMediaFormat(OMX_U32 nPortIndex);
        OMX_ERRORTYPE PeerNeedBuffer();
};

#endif
