/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file VpuDecComponent.h
 *  @brief Class definition of VpuDecoder Component
 *  @ingroup VpuDecoder
 */

#ifndef VpuDecoder_h
#define VpuDecoder_h

#include "VideoFilter.h"
#include "vpu_wrapper.h"

#define VPU_DEC_MAX_NUM_MEM	(30)
typedef fsl_osal_sem VPUCompSemaphor;
#define VPU_COMP_SEM_INIT(ppsem)    fsl_osal_sem_init((ppsem), 0/*process shared*/,1/*number*/)
#define VPU_COMP_SEM_INIT_PROCESS(ppsem)    fsl_osal_sem_init_process((ppsem), 1/*process shared*/,1/*number*/)
#define VPU_COMP_SEM_DESTROY(psem)  fsl_osal_sem_destroy((psem))
#define VPU_COMP_SEM_DESTROY_PROCESS(psem)  fsl_osal_sem_destroy_process((psem))
#define VPU_COMP_SEM_LOCK(psem)     fsl_osal_sem_wait((psem))
#define VPU_COMP_SEM_TRYLOCK(psem)     fsl_osal_sem_trywait((psem))
#define VPU_COMP_SEM_UNLOCK(psem)   fsl_osal_sem_post((psem))

typedef struct
{
	//virtual mem info
	OMX_S32 nVirtNum;
	OMX_U32 virtMem[VPU_DEC_MAX_NUM_MEM];

	//phy mem info
	OMX_S32 nPhyNum;
	OMX_U32 phyMem_virtAddr[VPU_DEC_MAX_NUM_MEM];
	OMX_U32 phyMem_phyAddr[VPU_DEC_MAX_NUM_MEM];
	OMX_U32 phyMem_cpuAddr[VPU_DEC_MAX_NUM_MEM];
	OMX_U32 phyMem_size[VPU_DEC_MAX_NUM_MEM];	
}VpuDecoderMemInfo;

typedef struct
{
	OMX_PTR pVirtAddr[VPU_DEC_MAX_NUM_MEM];
	VpuDecOutFrameInfo outFrameInfo[VPU_DEC_MAX_NUM_MEM];
}VpuDecoderOutMapInfo;

typedef enum
{
	VPU_COM_STATE_NONE=0,
	VPU_COM_STATE_LOADED,
	VPU_COM_STATE_OPENED,
	VPU_COM_STATE_WAIT_FRM,
	VPU_COM_STATE_DO_INIT,
	VPU_COM_STATE_DO_DEC,
	VPU_COM_STATE_DO_OUT,
	VPU_COM_STATE_EOS,
	VPU_COM_STATE_RE_WAIT_FRM,
}VpuDecoderState;

typedef enum
{
	VPU_COM_CAPABILITY_FILEMODE=0x1,
	VPU_COM_CAPABILITY_TILE=0x2,
	VPU_COM_CAPABILITY_FRMSIZE=0x4,
}VpuDecoderCapability;

class VpuDecoder : public VideoFilter {
	public:
		VpuDecoder();
		OMX_S32 DoGetBitstream(OMX_U32 nLen, OMX_U8 *pBuffer, OMX_S32 *pEndOfFrame);
	private:
		VPUCompSemaphor psemaphore;		//in fact, it is one ptr
		VpuMemInfo sMemInfo;				// required by vpu wrapper
		VpuDecoderMemInfo sVpuMemInfo;		// memory info for vpu wrapper itself
		VpuDecoderMemInfo sAllocMemInfo;	// memory info for AllocateOutputBuffer()
		VpuDecoderMemInfo sFrameMemInfo;	// memory info for output frame
		VpuDecoderOutMapInfo sOutMapInfo;	// output info
		
		VpuDecInitInfo sInitInfo;		// seqinit info
		VpuDecHandle nHandle;		// pointer to vpu object

		OMX_U8 cRole[OMX_MAX_STRINGNAME_SIZE];
		
		//sWmv9DecObjectType sDecObj;
		VpuCodStd eFormat;
		OMX_S32 nPadWidth;
		OMX_S32 nPadHeight;
		//OMX_S32 nFrameSize;
		//OMX_PTR pFrameBuffer;
		OMX_CONFIG_RECTTYPE sOutCrop;
		OMX_CONFIG_SCALEFACTORTYPE sDispRatio;

		OMX_PTR pInBuffer;
		OMX_S32 nInSize;
		OMX_BOOL bInEos;
		
		//OMX_PTR pOutBuffer;
		//OMX_BOOL bOutLast;
		
		//OMX_BOOL bFrameBufReady;
		VpuDecoderState eVpuDecoderState;	

		OMX_PTR pLastOutVirtBuf;		//record the last output frame

		OMX_S32 nFreeOutBufCnt;		//frame buffer numbers which can be used by vpu
		OMX_S32 nFreeOutBufCntRp;	//frame buffer numbers which can be used by vpu after flush filter

		OMX_S32 nNeedFlush;		//flush automatically according return value from vpu wrapper
		OMX_S32 nNeedSkip;			//output one skip to get one timestamp

		OMX_PARAM_MEM_OPERATOR sMemOperator;
		OMX_DECODE_MODE ePlayMode;
		OMX_U32 nChromaAddrAlign;	//the address of Y,Cb,Cr may need to alignment.

		OMX_PTR pClock;
		VpuVersionInfo sVpuVer;		//vpu version info
		OMX_U32 nCapability;			//vpu capability

		OMX_ERRORTYPE SetRoleFormat(OMX_STRING role);
		OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
		OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
		OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);
		OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure);

		/* virtual function implementation */
		OMX_ERRORTYPE InitFilterComponent();
		OMX_ERRORTYPE DeInitFilterComponent();
		OMX_ERRORTYPE SetInputBuffer(OMX_PTR pBuffer, OMX_S32 nSize, OMX_BOOL bLast);
		OMX_ERRORTYPE SetOutputBuffer(OMX_PTR pBuffer);
		OMX_ERRORTYPE InitFilter();
		OMX_ERRORTYPE DeInitFilter();
		FilterBufRetCode FilterOneBuffer();
		OMX_ERRORTYPE GetDecBuffer(OMX_PTR *ppBuffer,OMX_S32 * pOutStuffSize,OMX_S32* pOutFrmSize);
		OMX_ERRORTYPE GetOutputBuffer(OMX_PTR *ppBuffer,OMX_S32 * pOutSize);
		OMX_ERRORTYPE FlushFilter();
		OMX_ERRORTYPE FlushInputBuffer();
		OMX_ERRORTYPE FlushOutputBuffer();

		//OMX_PTR AllocateInputBuffer(OMX_U32 nSize);
		//OMX_ERRORTYPE FreeInputBuffer(OMX_PTR pBuffer);
		OMX_PTR AllocateOutputBuffer(OMX_U32 nSize);
		OMX_ERRORTYPE FreeOutputBuffer(OMX_PTR pBuffer);		

		FilterBufRetCode ProcessVpuInitInfo();
};

#endif
/* File EOF */
