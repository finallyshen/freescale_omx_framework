/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file VideoFilter.h
 *  @brief Class definition of VideoFilter base Component
 *  @ingroup VideoFilter
 */

#ifndef VideoFilter_h
#define VideoFilter_h

#include "ComponentBase.h"
#include "Mem.h"

#define NUM_PORTS 2
#define IN_PORT   0
#define OUT_PORT  1

typedef enum
{
	FILTER_OK = 0,
	FILTER_DO_INIT,
	FILTER_NO_INPUT_BUFFER,
	FILTER_NO_OUTPUT_BUFFER,
	FILTER_HAS_OUTPUT,
	FILTER_SKIP_OUTPUT,
	FILTER_LAST_OUTPUT,
	FILTER_ERROR,
	FILTER_INPUT_CONSUMED = 0x100,
	FILTER_ONE_FRM_DECODED = 0x200,
	/*related flags for encoder*/
	FILTER_FLAGS_MASK =0xF000,
	FILTER_FLAG_CODEC_DATA =0x1000,
	FILTER_FLAG_NONKEY_FRAME =0x2000,
	FILTER_FLAG_KEY_FRAME =0x4000,
} FilterBufRetCode;

class VideoFilter : public ComponentBase
{
public:
	VideoFilter();
	OMX_ERRORTYPE DoAllocateBuffer(OMX_PTR *buffer, OMX_U32 nSize,OMX_U32 nPortIndex);
	OMX_ERRORTYPE DoFreeBuffer(OMX_PTR buffer,OMX_U32 nPortIndex);
	OMX_ERRORTYPE FlushComponent(OMX_U32 nPortIndex);
	OMX_ERRORTYPE ComponentReturnBuffer(OMX_U32 nPortIndex);
	OMX_ERRORTYPE InputFmtChanged();
	OMX_ERRORTYPE OutputFmtChanged();
	OMX_TICKS QueryStreamTs();
protected:
	OMX_VIDEO_PORTDEFINITIONTYPE sInFmt;
	OMX_VIDEO_PORTDEFINITIONTYPE sOutFmt;
	OMX_BOOL bFilterSupportPartilInput;
	OMX_U32 nInBufferCnt;
	OMX_U32 nInBufferSize;
	OMX_U32 nOutBufferCnt;
	OMX_U32 nOutBufferSize;
	OMX_PTR pCodecData;
	OMX_U32 nCodecDataLen;
	OMX_CONFIG_ROTATIONTYPE Rotation;
	OMX_BOOL bFilterSupportFrmSizeRpt;
private:
	OMX_BUFFERHEADERTYPE PartialInputHdr;
	OMX_BUFFERHEADERTYPE *pInBufferHdr;
	List<OMX_BUFFERHEADERTYPE> OutBufferHdrList;
	OMX_PTR hTsHandle;
	OMX_S32 nDecodeOnly;
	OMX_BOOL bGetNewSegment;
	OMX_BOOL bNewSegment;
	OMX_BOOL bLastInput;
	OMX_BOOL bLastOutput;
	OMX_BOOL bInReturnBufferState;
	OMX_BOOL bNeedOutBuffer;
	OMX_BOOL bNeedInputBuffer;
	OMX_BOOL bInit;

	OMX_ERRORTYPE InitComponent();
	OMX_ERRORTYPE DeInitComponent();
	OMX_ERRORTYPE PortFormatChanged(OMX_U32 nPortIndex);
	OMX_ERRORTYPE ProcessDataBuffer();
	OMX_ERRORTYPE ProcessInputBuffer();
	OMX_ERRORTYPE ProcessOutputBuffer();
	OMX_ERRORTYPE DoIdle2Loaded();
	OMX_ERRORTYPE ProcessInBufferFlags();
	OMX_ERRORTYPE ProcessPartialInput();
	OMX_ERRORTYPE ReturnInputBuffer();
	OMX_ERRORTYPE ReturnOutputBuffer(OMX_BUFFERHEADERTYPE *pBufferHdr,OMX_U32 flags);
	OMX_BOOL HasFreeTsSlot();
	OMX_ERRORTYPE AddOneTimeStampToTM(OMX_TICKS ts);
	OMX_TICKS GetOneTimeStampFromTM();
	OMX_TICKS GetOneTimeStamp();
	OMX_BUFFERHEADERTYPE *GetOutBufferHdrFromList(OMX_PTR pBuffer);
	OMX_BUFFERHEADERTYPE *GetFirstOutBufferHdrFromList();
	OMX_ERRORTYPE HandleLastOutput(OMX_U32 flags);
	OMX_ERRORTYPE CheckPortResource(OMX_U32 nPortIndex);

	virtual OMX_ERRORTYPE InitFilterComponent();
	virtual OMX_ERRORTYPE DeInitFilterComponent();
	virtual OMX_ERRORTYPE SetInputBuffer(OMX_PTR pBuffer, OMX_S32 nSize, OMX_BOOL bLast) = 0;
	virtual OMX_ERRORTYPE SetOutputBuffer(OMX_PTR pBuffer) = 0;
	virtual OMX_ERRORTYPE InitFilter() = 0;
	virtual OMX_ERRORTYPE DeInitFilter() = 0;
	virtual FilterBufRetCode FilterOneBuffer() = 0;
	virtual OMX_ERRORTYPE GetDecBuffer(OMX_PTR *ppBuffer,OMX_S32 * pOutStuffSize,OMX_S32* pOutFrmSize);
	virtual OMX_ERRORTYPE GetOutputBuffer(OMX_PTR *ppBuffer,OMX_S32 * pOutSize) = 0;
	virtual OMX_ERRORTYPE FlushInputBuffer() = 0;
	virtual OMX_ERRORTYPE FlushOutputBuffer() = 0;
	virtual OMX_PTR AllocateInputBuffer(OMX_U32 nSize);
	virtual OMX_ERRORTYPE FreeInputBuffer(OMX_PTR pBuffer);
	virtual OMX_PTR AllocateOutputBuffer(OMX_U32 nSize);
	virtual OMX_ERRORTYPE FreeOutputBuffer(OMX_PTR pBuffer);
};

#endif
/* File EOF */
