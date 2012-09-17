/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "SorensonDec.h"
#include "Mem.h"

/*****************************************************************************
 * <Typedefs>
 *****************************************************************************/
#define SUPPORTED_UNCLOSED_GOP		//for seek issue: only decode I frame after seek to avoid mosaic output

#define BUFF_ALLOCATED  0x1
#define BUFF_DECODED	0x2
#define BUFF_DISPLAYED	0x4
#define BUFF_NOT_REF    0x8

#ifndef ALIGN16
#define ALIGN16(x)  (((x)+15)&(~15))
#endif
#define ALIGN32(x)  ((x+31)&(~31))

#if defined(ICS) && defined(MX5X)
#define ALIGN_STRIDE(x)  (((x)+63)&(~63))
#define ALIGN_CHROMA(x) (((x) + 4095)&(~4095))
#else
#define ALIGN_STRIDE(x)  (((x)+31)&(~31))
#define ALIGN_CHROMA(x) (x)
#endif

#define BUFFER_NUM 3


#define SET_BIT_FLAG(flag, state) ((flag)|=(state))
#define CLEAR_BIT_FLAG(flag, state) ((flag)&=~(state))
#define IS_FLAGS_SET(flag, state) ((flag) & (state))

#ifdef SUPPORTED_UNCLOSED_GOP
#define PSC_LENGTH        17
#define PCT_INTRA           0
#define PCT_INTER           1
//#define SOREN_BITS_LOG	printf
#define SOREN_BITS_LOG(...)
typedef struct ld
{
	OMX_U32  word;
	OMX_S32  BitsLeft;
	OMX_U8 *inbufptr;
} BITSTREAM_T;

#if 0
void printf_memory(OMX_U8* addr, OMX_S32 width, OMX_S32 height, OMX_S32 stride)
{
	OMX_S32 i,j;
	OMX_U8* ptr;

	ptr=addr;
	SOREN_BITS_LOG("addr: 0x%X \r\n",(OMX_U32)addr);
	for(i=0; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			SOREN_BITS_LOG("%2X ",ptr[j]);
		}
		SOREN_BITS_LOG("\r\n");
		ptr+=stride;
	}
	SOREN_BITS_LOG("\r\n");
	return;
}
#endif

OMX_U32 h263_getbits(BITSTREAM_T *ld,OMX_S32 numberofbits)
{
	OMX_U32 retval;

	SOREN_BITS_LOG("%s: bitsleft: %d, numberofbits: %d \r\n",__FUNCTION__,ld->BitsLeft,numberofbits);
	if (ld->BitsLeft >= numberofbits)
	{
		retval = (ld->word >> (32 - numberofbits));
		ld->word = ld->word<<numberofbits;
		ld->BitsLeft=ld->BitsLeft-numberofbits;
	}
	else
	{
		OMX_U32 upper;
		OMX_U32 next32bits;
		upper=((ld->word>>(32 - ld->BitsLeft)) <<(numberofbits-ld->BitsLeft));
		next32bits=(ld->inbufptr[0]<<24)|(ld->inbufptr[1]<<16)|(ld->inbufptr[2]<<8)|(ld->inbufptr[3]);
		retval=upper | (next32bits >>(32-numberofbits+ld->BitsLeft));
		ld->word=next32bits<<(numberofbits-ld->BitsLeft);
		ld->BitsLeft=(32-numberofbits+ld->BitsLeft);
		ld->inbufptr+=4;
	}
	SOREN_BITS_LOG("retval: 0x%X , BitsLeft: %d , word: 0x%X \r\n",retval,ld->BitsLeft,ld->word);
	return retval;
}

OMX_U32 h263_pre_fill_bitstream(BITSTREAM_T *pBitStream,OMX_S32 numberofbits)
{
	OMX_U8  *pptr ;
	OMX_S32 num_bytes = numberofbits/8;

	pptr = pBitStream->inbufptr;
	pBitStream->BitsLeft = numberofbits;
	for(; num_bytes>0; num_bytes--)
	{
		pBitStream->word <<= 8;
		pBitStream->word |= *pptr; //  ((*pptr & 0xff)<<24) + ((*(pptr +1) & 0xff)<<16)+((*(pptr+ 2) & 0xff)<<8)+ *(pptr +3);
		pptr++;
	}
	pBitStream->inbufptr = pptr;
	pBitStream->word <<= (32-pBitStream->BitsLeft);
	return 1;
}
OMX_S32 h263_is_keyframe(NAL_BUF_T * pNalBuf)
{
	BITSTREAM_T bitstream;
	BITSTREAM_T *ld;
	OMX_U32 code;
	OMX_S32 version;
	OMX_S8 pict_type;
	OMX_U32 temp;

	//printf_memory(pNalBuf->buf, 10, 1, 10);

	ld=&bitstream;
	ld->inbufptr = pNalBuf->buf;
	ld->BitsLeft = 0;
	ld->word = 0;
	temp = (OMX_U32)ld->inbufptr;
	if ((temp&3) != 0)
	{
		h263_pre_fill_bitstream(ld, (4-(temp&3))*8);
	}

	/* look for startcode */
	if(h263_getbits(ld,PSC_LENGTH) != 1l)
	{
		return -1;
	}

	// Presently only supports h263v1 based on FLV v9
	version = h263_getbits (ld,5);
	if (!(version == 0 || version == 1))
	{
		return -1;
	}

	// Parse video payload
	h263_getbits (ld,8);	//temp_ref

	// Picture size : Refer to H263VIDEOPACKET in FLV spec
	code = h263_getbits(ld,3);
	if (code <= 1)
	{
		h263_getbits(ld,(code+1)*8);	//width
		h263_getbits(ld,(code+1)*8);	//height
	}

	pict_type = h263_getbits(ld,2);
	SOREN_BITS_LOG("pictype: %d \r\n",pict_type);
	if(PCT_INTRA==pict_type)
	{
		return 1;	//key frame
	}
	else
	{
		return 0;	//non key frame
	}
}
#endif	// SUPPORTED_UNCLOSED_GOP

unsigned char *SorensonDec::GetFrameBuffer()
{
	OMX_U32 i;
	for (i = 0; i < BUFFER_NUM; i++)
	{
		if (nFrameBufState[i] == BUFF_ALLOCATED)
		{
			LOG_DEBUG("%s,buffer %d sent to decoder.\n",__FUNCTION__,i);
			nFrameBufState[i] = BUFF_DECODED;
			return FrameBuffer[i];
		}
	}

	LOG_ERROR("%s,no buffer available.",__FUNCTION__);
	return NULL;

}

void SorensonDec::RejectFrameBuffer( unsigned char * mem_ptr)
{
	OMX_U32 i;

	for (i = 0; i < BUFFER_NUM; i++)
	{
		if (FrameBuffer[i] == mem_ptr)
		{
			LOG_DEBUG("%s,buffer %d rejected.\n",__FUNCTION__,i);
			if (nFrameBufState[i] == BUFF_DECODED)
			{
				LOG_ERROR("%s,a buffer still in reference has been sent to decoder again.",__FUNCTION__);
				nFrameBufState[i] = BUFF_ALLOCATED;
			}
		}
	}

}

void SorensonDec::ReleaseFrameBuffer( unsigned char * mem_ptr)
{
	OMX_U32 i;
	for (i = 0; i < BUFFER_NUM; i++)
	{
		if (FrameBuffer[i] == mem_ptr && IS_FLAGS_SET(nFrameBufState[i],BUFF_DECODED))
		{
			LOG_DEBUG("%s,buffer %d released.\n",__FUNCTION__,i);
			SET_BIT_FLAG(nFrameBufState[i], BUFF_NOT_REF);
			if (IS_FLAGS_SET(nFrameBufState[i],BUFF_DISPLAYED))
			{
				nFrameBufState[i] = BUFF_ALLOCATED;
			}
		}
	}
}


static unsigned char * cbGetFrameBuffer( void* dutobj)
{
	SorensonDec *h = (SorensonDec *)dutobj;
	return h->GetFrameBuffer();
}

static void cbRejectFrameBuffer( unsigned char * mem_ptr, void* dutobj)
{
	SorensonDec *h = (SorensonDec *)dutobj;
	h->RejectFrameBuffer(mem_ptr);
}

static void cbReleaseFrameBuffer( unsigned char * mem_ptr, void* dutobj)
{
	SorensonDec *h = (SorensonDec *)dutobj;
	h->ReleaseFrameBuffer(mem_ptr);
}

SorensonDec::SorensonDec()
{
	fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_decoder.wmv.sw-based");
	ComponentVersion.s.nVersionMajor = 0x0;
	ComponentVersion.s.nVersionMinor = 0x1;
	ComponentVersion.s.nRevision = 0x0;
	ComponentVersion.s.nStep = 0x0;
	role_cnt = 1;
	role[0] = (OMX_STRING)"video_decoder.sorenson";

	fsl_osal_memset(&sInFmt, 0, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
	sInFmt.nFrameWidth = 320;
	sInFmt.nFrameHeight = 240;
	sInFmt.xFramerate = 30 * Q16_SHIFT;
	sInFmt.eColorFormat = OMX_COLOR_FormatYUV420Planar;
	sInFmt.eCompressionFormat =(OMX_VIDEO_CODINGTYPE)OMX_VIDEO_SORENSON263;

	sOutFmt.nFrameWidth = 320;
	sOutFmt.nFrameHeight = 240;
	sOutFmt.eColorFormat = OMX_COLOR_FormatYUV420Planar;
	sOutFmt.eCompressionFormat = OMX_VIDEO_CodingUnused;

	bFilterSupportPartilInput = OMX_FALSE;
	nInBufferCnt = 1;
	nInBufferSize = 128*1024;
	nOutBufferCnt = 1;
	nOutBufferSize=sOutFmt.nFrameWidth * sOutFmt.nFrameHeight * pxlfmt2bpp(sOutFmt.eColorFormat) / 8;;

	pInBuffer = pOutBuffer = NULL;
	nInputSize = nInputOffset = 0;
	bInEos = OMX_FALSE;
	nFrameSize = 0;;
	pFrameBuffer = NULL;

	OMX_INIT_STRUCT(&sOutCrop, OMX_CONFIG_RECTTYPE);
	sOutCrop.nPortIndex = OUT_PORT;
	sOutCrop.nLeft = sOutCrop.nTop = 0;
	sOutCrop.nWidth = sInFmt.nFrameWidth;
	sOutCrop.nHeight = sInFmt.nFrameHeight;
	DecStructSize       = 0;
	nOutputBufGot = 0;
	FrameBufSize = 0;
	MinFrameBufferCount = 0;
	DecFrameHeight = DecFrameWidth = 0;
	fsl_osal_memset(&Info,0,sizeof(INFO_STRUCT_T));
	fsl_osal_memset(&NalBuf,0,sizeof(NAL_BUF_T));
	fsl_osal_memset(FrameBuffer,0,sizeof(OMX_U8 *)*16);
	fsl_osal_memset(nFrameBufState,0,sizeof(OMX_U32)*16);

	eSorensonDecState = SORENSONDEC_LOADED;
}


OMX_ERRORTYPE SorensonDec::GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure)
{
	if(nParamIndex == OMX_IndexConfigCommonOutputCrop)
	{
		OMX_CONFIG_RECTTYPE *pRecConf = (OMX_CONFIG_RECTTYPE*)pComponentParameterStructure;
		if(pRecConf->nPortIndex == OUT_PORT)
		{
			pRecConf->nTop = sOutCrop.nTop;
			pRecConf->nLeft = sOutCrop.nLeft;
			pRecConf->nWidth = sOutCrop.nWidth;
			pRecConf->nHeight = sOutCrop.nHeight;
		}
		return OMX_ErrorNone;
	}
	else
		return OMX_ErrorUnsupportedIndex;
}

OMX_ERRORTYPE SorensonDec::SetInputBuffer(
    OMX_PTR pBuffer,
    OMX_S32 nSize,
    OMX_BOOL bLast)
{
	LOG_DEBUG("%s,%d,is in buffer last %d\n",__FUNCTION__,__LINE__,bLast);
	if(pBuffer == NULL)
		return OMX_ErrorBadParameter;

	pInBuffer = pBuffer;
	nInputSize = nSize;
	nInputOffset = 0;
	bInEos = bLast;

	return OMX_ErrorNone;
}

OMX_ERRORTYPE SorensonDec::SetOutputBuffer(OMX_PTR pBuffer)
{
	OMX_U32 i;
	if(pBuffer == NULL)
		return OMX_ErrorBadParameter;

	LOG_DEBUG("%s,%d, buffer got %x\n",__FUNCTION__,__LINE__,pBuffer);
	pOutBuffer = pBuffer;
	return OMX_ErrorNone;
}

OMX_ERRORTYPE SorensonDec::InitFilter()
{
	sSorensonDecFrameManager    frameManager;


	if(OMX_ErrorNone != AllocateDecoderMemory())
		return OMX_ErrorInsufficientResources;

	frameManager.GetterBuffer   = cbGetFrameBuffer ;
	frameManager.RejectorBuffer	= cbRejectFrameBuffer;
	frameManager.ReleaseBuffer  = cbReleaseFrameBuffer;
	frameManager.pvAppContext   = (void *)this;

	if (0 != H263_InitDecoder(
	            sInFmt.nFrameWidth,
	            sInFmt.nFrameHeight,
	            DecStruct,
	            &frameManager,
	            (&Info)))
	{
		LOG_ERROR("%s call H263_InitDecoder failed!", __FUNCTION__);
		FreeDecoderMemory();
		return OMX_ErrorUndefined;
	}
	LOG_DEBUG("decoded width %d,height %d, luma stride %d\n",Info.width,Info.height,Info.stride);

	eSorensonDecState = SORENSONDEC_RUN;

	return OMX_ErrorNone;
}

OMX_ERRORTYPE SorensonDec::DeInitFilter()
{
	OMX_U32 i;
	FreeDecoderMemory();
	for (i = 0; i < BUFFER_NUM; i++)
	{
		if (FrameBuffer[i])
		{
			fsl_osal_dealloc(FrameBuffer[i]);
			FrameBuffer[i] = NULL;
		}
	}
	eSorensonDecState = SORENSONDEC_LOADED;

	return OMX_ErrorNone;
}

FilterBufRetCode SorensonDec::FilterOneBuffer()
{
	FilterBufRetCode ret = FILTER_OK;

	switch(eSorensonDecState)
	{
	case SORENSONDEC_LOADED:
		DetectOutputFmt();
		break;
	case SORENSONDEC_INIT:
		ret = FILTER_DO_INIT;
		break;
	case SORENSONDEC_RUN:
#ifdef SUPPORTED_UNCLOSED_GOP
	case SORENSONDEC_SEEK:
#endif
		ret = DecodeOneFrame();
		break;
	case SORENSONDEC_STOP:
		break;
	default:
		break;
	}

	return ret;
}

OMX_ERRORTYPE SorensonDec::GetOutputBuffer(OMX_PTR *ppBuffer,OMX_S32* pOutSize)
{

	LOG_DEBUG("%s,%d, buffer got %x\n",__FUNCTION__,__LINE__,pOutBuffer);

	*ppBuffer = pOutBuffer;
	pOutBuffer = NULL;
	*pOutSize = nOutBufferSize;
	nOutBufferSize=0;    // make sure return size=0 when eos

	return OMX_ErrorNone;
}

OMX_ERRORTYPE SorensonDec::FlushInputBuffer()
{
	pInBuffer = NULL;
	nInputSize = 0;
	nInputOffset = 0;
	bInEos = OMX_FALSE;

#ifdef SUPPORTED_UNCLOSED_GOP
	eSorensonDecState=SORENSONDEC_SEEK;
#endif

	return OMX_ErrorNone;
}

OMX_ERRORTYPE SorensonDec::FlushOutputBuffer()
{
	pOutBuffer = NULL;
	nOutBufferSize = 0;
	return OMX_ErrorNone;
}

OMX_ERRORTYPE SorensonDec::DetectOutputFmt()
{
	OMX_U32 i;

	nPadWidth = ALIGN_STRIDE(sInFmt.nFrameWidth);
	nPadHeight = ALIGN_STRIDE(sInFmt.nFrameHeight);

	H263_GetMemoryInfo(nPadWidth,
	                   nPadHeight,
	                   (int *)&DecStructSize,
	                   (int *)&FrameBufSize,
	                   (int *)&MinFrameBufferCount);
	LOG_DEBUG("MemoryInfo width %d height %u FrameBufSize %u MinFrameBufferCount %u\n",
	          nPadWidth, nPadHeight, FrameBufSize, MinFrameBufferCount);

	for (i = 0; i < BUFFER_NUM; i++)
	{
		FrameBuffer[i] = (OMX_U8 *)fsl_osal_malloc_new(FrameBufSize);
		if (!FrameBuffer[i])
		{
			LOG_ERROR("%s,%d.\n",__FUNCTION__,__LINE__);
			return OMX_ErrorInsufficientResources;
		}
		nFrameBufState[i] = BUFF_ALLOCATED;
	}

	nOutBufferCnt = 3;
	sOutFmt.nFrameWidth = nPadWidth;
	sOutFmt.nFrameHeight = nPadHeight;

	sOutCrop.nLeft = 0;
	sOutCrop.nTop = 0;
	sOutCrop.nWidth = sInFmt.nFrameWidth & (~7);
	sOutCrop.nHeight = sInFmt.nFrameHeight & (~7);

	VideoFilter::OutputFmtChanged();

	eSorensonDecState = SORENSONDEC_INIT;

	return OMX_ErrorNone;
}


OMX_BOOL SorensonDec::CheckResolutionChanged()
{
	int DecRet;
	OMX_U32 width, height;
	DecRet = h263_get_resolution (&NalBuf, (int*)&width, (int*)&height);
	LOG_DEBUG("resolution:width %d,height %d\n",width,height);

	if (DecFrameWidth && DecFrameHeight && width != DecFrameWidth && height != DecFrameHeight)
		return OMX_TRUE;
	DecFrameWidth = width;
	DecFrameHeight = height;
	return OMX_FALSE;
}

void SorensonDec::CopyOutputFrame(OMX_U8 *pOutputFrame)
{
	OMX_S32 i,aligned_height;

	aligned_height = ALIGN16(Info.height);

	OMX_U8 *y_addr = (OMX_U8 *)pOutBuffer;
	OMX_U8 *u_addr = (OMX_U8 *)ALIGN_CHROMA(((int)y_addr) + nPadWidth * nPadHeight);
	OMX_U8 *v_addr = (OMX_U8 *)ALIGN_CHROMA(((int)u_addr) + nPadWidth * nPadHeight/4);
	for (i = 0; i < aligned_height; i++)
		fsl_osal_memcpy(y_addr + i*nPadWidth, pOutputFrame + i*Info.stride, Info.width);
	for (i = 0; i < aligned_height/2; i++)
		fsl_osal_memcpy(u_addr + i*nPadWidth/2,
		                pOutputFrame + Info.stride * aligned_height + i*Info.stride/2, Info.width/2);
	for (i = 0; i < aligned_height/2; i++)
		fsl_osal_memcpy(v_addr + i*nPadWidth/2,
		                pOutputFrame + Info.stride * aligned_height*5/4 + i*Info.stride/2, Info.width/2);

	for (i = 0; i < BUFFER_NUM; i++)
	{
		if (FrameBuffer[i] == pOutputFrame)
		{
			LOG_DEBUG("%s,buffer %d released.\n",__FUNCTION__,i);
			SET_BIT_FLAG(nFrameBufState[i], BUFF_DISPLAYED);
			if (IS_FLAGS_SET(nFrameBufState[i],BUFF_NOT_REF))
			{
				nFrameBufState[i] = BUFF_ALLOCATED;
			}
		}
	}
	nOutBufferSize=sOutFmt.nFrameWidth * sOutFmt.nFrameHeight * pxlfmt2bpp(sOutFmt.eColorFormat) / 8;;
}


FilterBufRetCode SorensonDec::DecodeOneFrame()
{
	FilterBufRetCode ret = FILTER_OK;
	OMX_U8  *pOutputFrame = NULL;
	OMX_S32 DecRet = 0;

	if(pOutBuffer == NULL)
		return FILTER_NO_OUTPUT_BUFFER;

	if(pInBuffer == NULL && bInEos != OMX_TRUE)
		return FILTER_NO_INPUT_BUFFER;


	NalBuf.buf = (unsigned char*)pInBuffer;
	NalBuf.len = nInputSize;
	NalBuf.max_len = nInputSize;
	if(pInBuffer != NULL && nInputSize > 0)
	{
		if (CheckResolutionChanged())
		{
			ret = (FilterBufRetCode) (FILTER_INPUT_CONSUMED | FILTER_LAST_OUTPUT);
			LOG_WARNING("%s,%d\n",__FUNCTION__,__LINE__);
			return ret;
		}
#ifdef SUPPORTED_UNCLOSED_GOP
		if(eSorensonDecState==SORENSONDEC_SEEK)
		{
			OMX_S32 isKeyFrame=0;
			isKeyFrame=h263_is_keyframe(&NalBuf);
			if(1!=isKeyFrame)
			{
				SOREN_BITS_LOG("skip current frame \r\n");
				ret = FILTER_INPUT_CONSUMED;

				//FIMXME: how about (bInEos == OMX_TRUE) ???: (1) set pOutBuffer=NULL ?? or (2) process it at next round DecodeOneFrame(): (pInBuffer == NULL)
#if 0
				ret = (FilterBufRetCode) (ret | FILTER_SKIP_OUTPUT);
#else       //to avoid seek timeout, we need to send out one valid buffer with lenght=0. !!!!
				ret = (FilterBufRetCode) (ret | FILTER_HAS_OUTPUT);
				nOutBufferSize=0;
#endif
				pInBuffer=NULL;	//must clear it
				nInputSize=0;
				return ret;
			}
			else
			{
				SOREN_BITS_LOG("find key frame \r\n");
				eSorensonDecState=SORENSONDEC_RUN;
			}
		}
#endif
		DecRet = H263_DecodeFrame(&NalBuf, DecStruct, 1, (unsigned char**)&pOutputFrame);

		if (-1 == DecRet)
		{
			LOG_WARNING("Decode: failed %d OutputFrame ptr 0x%p", DecRet, pOutputFrame);
			ret = FILTER_INPUT_CONSUMED;
			if(bInEos == OMX_TRUE)
				ret = (FilterBufRetCode) (ret | FILTER_LAST_OUTPUT);
			else
				ret = (FilterBufRetCode) (ret | FILTER_SKIP_OUTPUT);

			pInBuffer=NULL;	//eagle: must clear it
			nInputSize=0;
			return ret;
		}
		else if (1 == DecRet)
		{
			LOG_DEBUG("Decode: valid FLV packet is found");
		}
		else
		{
			LOG_DEBUG("Decode: all frames are decoded");
			ret = (FilterBufRetCode) (ret | FILTER_LAST_OUTPUT);
		}
	}


	ret = FILTER_INPUT_CONSUMED;

	if(bInEos == OMX_TRUE)
	{
		ret = (FilterBufRetCode) (ret | FILTER_LAST_OUTPUT);
		LOG_DEBUG("%s,%d,last output buffer %x\n",__FUNCTION__,__LINE__,pOutputFrame);
		eSorensonDecState = SORENSONDEC_STOP;
	}

	if(pOutputFrame != NULL)
	{
		ret = (FilterBufRetCode) (ret | FILTER_HAS_OUTPUT);
		CopyOutputFrame(pOutputFrame);
	}

	return ret;
}

OMX_ERRORTYPE SorensonDec::AllocateDecoderMemory()
{
	DecStruct = fsl_osal_malloc_new(DecStructSize);
	if (DecStruct == NULL)
	{
		LOG_ERROR("%s malloc Codec struct failed!", __FUNCTION__);
		return OMX_ErrorInsufficientResources;
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE SorensonDec::FreeDecoderMemory()
{
	if (DecStruct)
	{
		fsl_osal_dealloc(DecStruct);
		DecStruct = NULL;
	}
	return OMX_ErrorNone;
}


/**< C style functions to expose entry point for the shared library */
extern "C" {
	OMX_ERRORTYPE SorensonDecoderInit(OMX_IN OMX_HANDLETYPE pHandle)
	{
		OMX_ERRORTYPE ret = OMX_ErrorNone;
		SorensonDec *obj = NULL;
		ComponentBase *base = NULL;

		obj = FSL_NEW(SorensonDec, ());
		if(obj == NULL)
			return OMX_ErrorInsufficientResources;

		base = (ComponentBase*)obj;
		ret = base->ConstructComponent(pHandle);
		if(ret != OMX_ErrorNone)
			return ret;

		return ret;
	}
}

/* File EOF */
