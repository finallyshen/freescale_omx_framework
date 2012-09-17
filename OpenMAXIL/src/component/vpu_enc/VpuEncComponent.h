/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file VpuEncComponent.h
 *  @brief Class definition of VpuEncoder Component
 *  @ingroup VpuEncoder
 */

#ifndef VpuEncoder_h
#define VpuEncoder_h

#include "vpu_wrapper.h"
#include "VideoFilter.h"

typedef unsigned int VpuEncHandle;

typedef struct {
	VpuCodStd eFormat;
	int nPicWidth;
	int nPicHeight;	
	int nWidthStride;
	int nHeightStride;
	int nRotAngle;
	int nFrameRate;
	int nBitRate;			/*unit: bps*/
	int nGOPSize;
	int nChromaInterleave;
	VpuEncMirrorDirection sMirror;
	int nQuantParam;

	int nEnableAutoSkip;
	int nIDRPeriod;		//for H.264
	int nRefreshIntra;	//IDR for H.264
} VpuEncInputParam;

typedef enum
{
	VPU_ENC_COM_STATE_NONE=0,
	VPU_ENC_COM_STATE_LOADED,
	VPU_ENC_COM_STATE_OPENED,
	//VPU_ENC_COM_STATE_WAIT_FRM,
	VPU_ENC_COM_STATE_DO_INIT,
	VPU_ENC_COM_STATE_DO_DEC,
	VPU_ENC_COM_STATE_DO_OUT,
	//VPU_ENC_COM_STATE_EOS,
	//VPU_ENC_COM_STATE_RE_WAIT_FRM,
}VpuEncoderState;


#define VPU_ENC_MAX_NUM_MEM	(30)
typedef struct
{
	//virtual mem info
	OMX_S32 nVirtNum;
	OMX_U32 virtMem[VPU_ENC_MAX_NUM_MEM];

	//phy mem info
	OMX_S32 nPhyNum;
	OMX_U32 phyMem_virtAddr[VPU_ENC_MAX_NUM_MEM];
	OMX_U32 phyMem_phyAddr[VPU_ENC_MAX_NUM_MEM];
	OMX_U32 phyMem_cpuAddr[VPU_ENC_MAX_NUM_MEM];
	OMX_U32 phyMem_size[VPU_ENC_MAX_NUM_MEM];	
}VpuEncoderMemInfo;


class VpuEncoder : public VideoFilter {
	public:
		VpuEncoder();
		//OMX_S32 DoGetBitstream(OMX_U32 nLen, OMX_U8 *pBuffer, OMX_S32 *pEndOfFrame);
	private:
		VpuEncoderMemInfo sVpuEncMemInfo;		// memory info for vpu : component internal structure/vpu_EncRegisterFrameBuffer()
		VpuEncoderMemInfo sEncAllocInMemInfo;	// memory info for AllocateInputBuffer()
		VpuEncoderMemInfo sEncAllocOutMemInfo;	// memory info for AllocateOutputBuffer()
		VpuEncoderMemInfo sEncOutFrameInfo;		// memory info for output frames: it may be overlapped with sEncAllocOutMemInfo
		OMX_PARAM_MEM_OPERATOR sMemOperator;

		VpuEncInitInfo sEncInitInfo;	// seqinit info
		VpuEncHandle nHandle;		// pointer to vpu object

		OMX_U8 cRole[OMX_MAX_STRINGNAME_SIZE];
		
		//VpuCodStd eFormat;
		//OMX_S32 nPadWidth;
		//OMX_S32 nPadHeight;

		OMX_PTR pInBufferPhy;
		OMX_PTR pInBufferVirt;
		OMX_S32 nInSize;
		OMX_BOOL bInEos;
		OMX_PTR pOutBufferPhy;
		OMX_S32 nOutSize;	
		OMX_S32 nOutGOPFrameCnt;	// used for H.264: [1,2,...,GOPSize]

		VpuEncoderState eVpuEncoderState;	

		VpuEncInputParam sVpuEncInputPara;
		
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
		OMX_ERRORTYPE GetOutputBuffer(OMX_PTR *ppBuffer);
		OMX_ERRORTYPE GetOutputBuffer(OMX_PTR *ppBuffer,OMX_S32* pOutSize);
		OMX_ERRORTYPE FlushInputBuffer();
		OMX_ERRORTYPE FlushOutputBuffer();

		OMX_PTR AllocateInputBuffer(OMX_U32 nSize);	//implement this api to support direct input
		OMX_ERRORTYPE FreeInputBuffer(OMX_PTR pBuffer);
		OMX_PTR AllocateOutputBuffer(OMX_U32 nSize);
		OMX_ERRORTYPE FreeOutputBuffer(OMX_PTR pBuffer);		

		//OMX_ERRORTYPE GetOutputBufferSize(OMX_S32 *pOutSize);

#if 1 //1 temporary
	OMX_ERRORTYPE FmtChanged(OMX_U32 nPortIndex,OMX_VIDEO_PORTDEFINITIONTYPE* pFmt);
	//OMX_ERRORTYPE BufferChanged(OMX_U32 nPortIndex,OMX_VIDEO_PORTDEFINITIONTYPE* pFmt,OMX_U32 nBuffCnt);
#endif

};

#endif	// #ifdef VpuEncoder_h
/* File EOF */

