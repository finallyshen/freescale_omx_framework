/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "VpuDecComponent.h"
#include "Mem.h"

#define Align(ptr,align)	(((OMX_U32)(ptr)+(align)-1)/(align)*(align))

#define DEFAULT_BUF_IN_CNT			0x1
#define DEFAULT_BUF_IN_SIZE		(1024*1024)
#define DEFAULT_BUF_OUT_CNT		0x3

#define DEFAULT_FRM_WIDTH			(176)
#define DEFAULT_FRM_HEIGHT		(144)
#define DEFAULT_FRM_RATE			(30 * Q16_SHIFT)

#define MAX_FRAME_NUM	(VPU_DEC_MAX_NUM_MEM)

#if  defined(ICS) && defined(MX5X)
#define FRAME_ALIGN		(64) //set 64 for gpu y/u/v alignment in iMX5 ICS 
#define CHROMA_ALIGN            (4096)
#else
#define FRAME_ALIGN		(32) //set 32 for gpu y/u/v alignment 
#define CHROMA_ALIGN            (1)  //default, needn't align
#endif

//nOutBufferCnt=nMinFrameBufferCount + FRAME_SURPLUS
#if 0
#define FRAME_SURPLUS	(0)
#define FRAME_MIN_FREE_THD		(((nOutBufferCnt-2)>2)?(nOutBufferCnt-2):2)//(nOutBufferCnt-1)
#else
#define FRAME_SURPLUS	(2)
//#define FRAME_MIN_FREE_THD	((nOutBufferCnt-FRAME_SURPLUS)-1)	// eg => FrameBuffers must be > (nMinFrameBufferCount - 1)
/*
to improve performance with limited frame buffers, we may set smaller threshold for FRAME_MIN_FREE_THD
for smaller FRAME_MIN_FREE_THD, vpu wrapper may return VPU_DEC_NO_ENOUGH_BUF_***
*/
#define FRAME_MIN_FREE_THD ((nOutBufferCnt-FRAME_SURPLUS)-3) //adjust performance: for clip: Divx5_1920x1080_30fps_19411kbps_MP3_44.1khz_112kbps_JS.avi
#endif

//#define VPUDEC_IN_PORT 		IN_PORT
#define VPUDEC_OUT_PORT 	OUT_PORT

#undef NULL
#define NULL (0)
#define INVALID (0xFFFFFFFF)		// make sure the match between "return FILTER_INPUT_CONSUMED" and SetInputBuffer()
typedef int INT32;
typedef unsigned int UINT32;

#define VPU_DEC_COMP_DROP_B	//drop B frames automatically
#ifdef VPU_DEC_COMP_DROP_B
#define DROP_B_THRESHOLD	30000
#endif

//#define VPU_COMP_DBGLOG
#ifdef VPU_COMP_DBGLOG
#define VPU_COMP_LOG	printf
#else
#define VPU_COMP_LOG(...)
#endif

//#define VPU_COMP_API_DBGLOG
#ifdef VPU_COMP_API_DBGLOG
#define VPU_COMP_API_LOG	printf
#else
#define VPU_COMP_API_LOG(...)
#endif

//#define VPU_COMP_ERR_DBGLOG
#ifdef VPU_COMP_ERR_DBGLOG
#define VPU_COMP_ERR_LOG	printf
#define ASSERT(exp)	if(!(exp)) {printf("%s: %d : assert condition !!!\r\n",__FUNCTION__,__LINE__);}
#else
#define VPU_COMP_ERR_LOG LOG_ERROR //(...)
#define ASSERT(...)
#endif

//#define VPU_COMP_DEBUG
#ifdef VPU_COMP_DEBUG
#define MAX_NULL_LOOP	(0xFFFFFFF)
#define MAX_DEC_FRAME  (0xFFFFFFF)
#define MAX_YUV_FRAME  (200)
#endif


#define VPU_COMP_NAME_H263DEC	"OMX.Freescale.std.video_decoder.h263.hw-based"
#define VPU_COMP_NAME_SORENSONDEC	"OMX.Freescale.std.video_decoder.sorenson.hw-based"
#define VPU_COMP_NAME_AVCDEC		"OMX.Freescale.std.video_decoder.avc.hw-based"
#define VPU_COMP_NAME_MPEG2DEC	"OMX.Freescale.std.video_decoder.mpeg2.hw-based"
#define VPU_COMP_NAME_MPEG4DEC	"OMX.Freescale.std.video_decoder.mpeg4.hw-based"
#define VPU_COMP_NAME_WMVDEC		"OMX.Freescale.std.video_decoder.wmv.hw-based"
#define VPU_COMP_NAME_DIV3DEC	"OMX.Freescale.std.video_decoder.div3.hw-based"
#define VPU_COMP_NAME_DIV4DEC	"OMX.Freescale.std.video_decoder.div4.hw-based"
#define VPU_COMP_NAME_DIVXDEC	"OMX.Freescale.std.video_decoder.divx.hw-based"
#define VPU_COMP_NAME_XVIDDEC	"OMX.Freescale.std.video_decoder.xvid.hw-based"
#define VPU_COMP_NAME_RVDEC		"OMX.Freescale.std.video_decoder.rv.hw-based"
#define VPU_COMP_NAME_MJPEGDEC	"OMX.Freescale.std.video_decoder.mjpeg.hw-based"
#define VPU_COMP_NAME_AVS	"OMX.Freescale.std.video_decoder.avs.hw-based"
#define VPU_COMP_NAME_VP8	"OMX.Freescale.std.video_decoder.vp8.hw-based"
#define VPU_COMP_NAME_MVC	"OMX.Freescale.std.video_decoder.mvc.hw-based"

//#define USE_PROCESS_SEM		//for debug : use semaphore between process
#ifdef USE_PROCESS_SEM
#include <semaphore.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
sem_t* sharedmem_sem_open(fsl_osal_s32 pshared, fsl_osal_u32 value);
efsl_osal_return_type_t fsl_osal_sem_init_process(fsl_osal_sem *sem_obj,fsl_osal_s32 pshared,fsl_osal_u32 value);
sem_t* sharedmem_sem_open(fsl_osal_s32 pshared, fsl_osal_u32 value)
{
	int fd;
	int ret;
	sem_t *pSem = NULL;
	char *shm_path, shm_file[256];
	shm_path = getenv("CODEC_SHM_PATH");      /*the CODEC_SHM_PATH is on a memory map the fs */

	if (shm_path == NULL)
		strcpy(shm_file, "/dev/shm");   /* default path */
	else
		strcpy(shm_file, shm_path);

	strcat(shm_file, "/");
	//strcat(shm_file, "codec.shm");
	strcat(shm_file, "vpudec.shm");
	fd = open(shm_file, O_RDWR, 0777);
	if (fd < 0)
	{
		/* first thread/process need codec protection come here */
		fd = open(shm_file, O_RDWR | O_CREAT | O_EXCL, 0777);
		if(fd < 0)
		{
			return NULL;
		}
		ftruncate(fd, sizeof(sem_t));
		/* map the semaphore variant in the file */
		pSem = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if((void *)(-1) == pSem)
		{
			return NULL;
		}
		/* do the semaphore initialization */
		ret = sem_init(pSem, pshared, value);
		if(-1 == ret)
		{
			return NULL;
		}
	}
	else
		pSem = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	return pSem;
}
efsl_osal_return_type_t fsl_osal_sem_init_process(fsl_osal_sem *psem_obj,fsl_osal_s32 pshared,fsl_osal_u32 value)
{
	ASSERT(1==pshared);
	*psem_obj = (sem_t *)sharedmem_sem_open(pshared,value);
	if(*psem_obj == NULL)
	{
		VPU_COMP_ERR_LOG("\n Creation of semaphore failed.");
		return E_FSL_OSAL_UNAVAILABLE;
	}
	return E_FSL_OSAL_SUCCESS;
}
efsl_osal_return_type_t fsl_osal_sem_destroy_process(fsl_osal_sem sem_obj)
{
#ifdef ANDROID_BUILD
	/* workaround for android semaphore usage */
	fsl_osal_sem_post(sem_obj);
#endif
	if (sem_destroy((sem_t *)sem_obj) != 0)
	{
		VPU_COMP_ERR_LOG("\n Error in destroying semaphore.");
		return E_FSL_OSAL_INVALIDPARAM ;
	}
	munmap((void *)sem_obj, sizeof(sem_t));
	return E_FSL_OSAL_SUCCESS;
}
#endif

#ifdef VPU_COMP_DEBUG
void printf_memory(OMX_U8* addr, OMX_S32 width, OMX_S32 height, OMX_S32 stride)
{
	OMX_S32 i,j;
	OMX_U8* ptr;

	ptr=addr;
	VPU_COMP_LOG("addr: 0x%X \r\n",(UINT32)addr);
	for(i=0; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			VPU_COMP_LOG("%2X ",ptr[j]);
		}
		VPU_COMP_LOG("\r\n");
		ptr+=stride;
	}
	VPU_COMP_LOG("\r\n");
	return;
}

void FileDumpBitstrem(FILE** ppFp, OMX_U8* pBits, OMX_U32 nSize)
{
	if(nSize==0)
	{
		return;
	}

	if(*ppFp==NULL)
	{
		*ppFp=fopen("temp.bit","wb");
		if(*ppFp==NULL)
		{
			VPU_COMP_LOG("open temp.bit failure \r\n");
			return;
		}
		VPU_COMP_LOG("open temp.bit OK \r\n");
	}

	fwrite(pBits,1,nSize,*ppFp);
	return;
}

void FileDumpYUV(FILE** ppFp, OMX_U8*  pY,OMX_U8*  pU,OMX_U8*  pV, OMX_U32 nYSize,OMX_U32 nCSize,OMX_COLOR_FORMATTYPE colorFormat)
{
	static OMX_U32 cnt=0;
	OMX_S32 nCScale=1;

	switch (colorFormat)
	{
	case OMX_COLOR_FormatYUV420SemiPlanar:
		VPU_COMP_ERR_LOG("interleave 420 color format : %d \r\n",colorFormat);
	case OMX_COLOR_FormatYUV420Planar:
		nCScale=1;
		break;
	case OMX_COLOR_FormatYUV422SemiPlanar:
		VPU_COMP_ERR_LOG("interleave 422 color format : %d \r\n",colorFormat);
	case OMX_COLOR_FormatYUV422Planar:
		//hor ???
		nCScale=2;
		break;
		//FIXME: add 4:0:0/4:4:4/...
	default:
		VPU_COMP_ERR_LOG("unsupported color format : %d \r\n",colorFormat);
		break;
	}

	if(*ppFp==NULL)
	{
		*ppFp=fopen("temp.yuv","wb");
		if(*ppFp==NULL)
		{
			VPU_COMP_LOG("open temp.yuv failure \r\n");
			return;
		}
		VPU_COMP_LOG("open temp.yuv OK \r\n");
	}

	if(cnt<=MAX_YUV_FRAME)
	{
		fwrite(pY,1,nYSize,*ppFp);
		fwrite(pU,1,nCSize*nCScale,*ppFp);
		fwrite(pV,1,nCSize*nCScale,*ppFp);
		cnt++;
	}

	return;
}
#endif

VpuDecRetCode VPU_DecGetMem_Wrapper(VpuMemDesc* pInOutMem,OMX_PARAM_MEM_OPERATOR* pMemOp)
{
	VpuDecRetCode ret;
	if((pMemOp->pfMalloc==NULL) || (pMemOp->pfFree==NULL))
	{
		//use default method
		ret=VPU_DecGetMem(pInOutMem);
	}
	else
	{
		OMX_MEM_DESC sOmxMem;
		sOmxMem.nSize=pInOutMem->nSize;
		if(OMX_TRUE==pMemOp->pfMalloc(&sOmxMem))
		{
			pInOutMem->nPhyAddr=sOmxMem.nPhyAddr;
			pInOutMem->nVirtAddr=sOmxMem.nVirtAddr;
			pInOutMem->nCpuAddr=sOmxMem.nCpuAddr;
			ret=VPU_DEC_RET_SUCCESS;
		}
		else
		{
			ret=VPU_DEC_RET_FAILURE;
		}
	}
	return ret;
}

VpuDecRetCode VPU_DecFreeMem_Wrapper(VpuMemDesc* pInMem,OMX_PARAM_MEM_OPERATOR* pMemOp)
{
	VpuDecRetCode ret;
	if((pMemOp->pfMalloc==NULL) || (pMemOp->pfFree==NULL))
	{
		//use default method
		ret=VPU_DecFreeMem(pInMem);
	}
	else
	{
		OMX_MEM_DESC sOmxMem;
		sOmxMem.nSize=pInMem->nSize;
		sOmxMem.nPhyAddr=pInMem->nPhyAddr;
		sOmxMem.nVirtAddr=pInMem->nVirtAddr;
		sOmxMem.nCpuAddr=pInMem->nCpuAddr;
		if(OMX_TRUE==pMemOp->pfFree(&sOmxMem))
		{
			ret=VPU_DEC_RET_SUCCESS;
		}
		else
		{
			ret=VPU_DEC_RET_FAILURE;
		}
	}
	return ret;
}

OMX_S32 MemFreeBlock(VpuDecoderMemInfo* pDecMem,OMX_PARAM_MEM_OPERATOR* pMemOp)
{
	OMX_S32 i;
	VpuMemDesc vpuMem;
	VpuDecRetCode vpuRet;
	OMX_S32 retOk=1;

	//free virtual mem
	//for(i=0;i<pDecMem->nVirtNum;i++)
	for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
	{
		if(pDecMem->virtMem[i])
		{
			fsl_osal_ptr ptr=(fsl_osal_ptr)pDecMem->virtMem[i];
			FSL_FREE(ptr);
			pDecMem->virtMem[i]=NULL;
		}
	}

	pDecMem->nVirtNum=0;

	//free physical mem
	//for(i=0;i<pDecMem->nPhyNum;i++)
	for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
	{
		if(pDecMem->phyMem_phyAddr[i])
		{
			vpuMem.nPhyAddr=pDecMem->phyMem_phyAddr[i];
			vpuMem.nVirtAddr=pDecMem->phyMem_virtAddr[i];
			vpuMem.nCpuAddr=pDecMem->phyMem_cpuAddr[i];
			vpuMem.nSize=pDecMem->phyMem_size[i];
			vpuRet=VPU_DecFreeMem_Wrapper(&vpuMem,pMemOp);
			if(vpuRet!=VPU_DEC_RET_SUCCESS)
			{
				VPU_COMP_LOG("%s: free vpu memory failure : ret=0x%X \r\n",__FUNCTION__,vpuRet);
				retOk=0;
			}
			pDecMem->phyMem_phyAddr[i]=NULL;
			pDecMem->phyMem_virtAddr[i]=NULL;
			pDecMem->phyMem_cpuAddr[i]=NULL;
			pDecMem->phyMem_size[i]=0;
		}
	}
	pDecMem->nPhyNum=0;

	return retOk;
}


OMX_S32 MemAddPhyBlock(VpuMemDesc* pInMemDesc,VpuDecoderMemInfo* pOutDecMem)
{
	OMX_S32 i;

	for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
	{
		// insert value into empty node
		if(NULL==pOutDecMem->phyMem_phyAddr[i])
		{
			pOutDecMem->phyMem_phyAddr[i]=pInMemDesc->nPhyAddr;
			pOutDecMem->phyMem_virtAddr[i]=pInMemDesc->nVirtAddr;
			pOutDecMem->phyMem_cpuAddr[i]=pInMemDesc->nCpuAddr;
			pOutDecMem->phyMem_size[i]=pInMemDesc->nSize;
			pOutDecMem->nPhyNum++;
			return 1;
		}
	}

	return 0;
}

OMX_S32 MemRemovePhyBlock(VpuMemDesc* pInMemDesc,VpuDecoderMemInfo* pOutDecMem)
{
	OMX_S32 i;
	for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
	{
		// clear matched node
		if(pInMemDesc->nPhyAddr==pOutDecMem->phyMem_phyAddr[i])
		{
			pOutDecMem->phyMem_phyAddr[i]=NULL;
			pOutDecMem->phyMem_virtAddr[i]=NULL;
			pOutDecMem->phyMem_cpuAddr[i]=NULL;
			pOutDecMem->phyMem_size[i]=0;
			pOutDecMem->nPhyNum--;
			return 1;
		}
	}

	return 0;
}

OMX_S32 MemQueryPhyBlock(OMX_PTR pInVirtAddr,VpuMemDesc* pOutMemDesc,VpuDecoderMemInfo* pInDecMem)
{
	OMX_S32 i;
	for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
	{
		// find matched node
		if((OMX_U32)pInVirtAddr==pInDecMem->phyMem_virtAddr[i])
		{
			pOutMemDesc->nPhyAddr=pInDecMem->phyMem_phyAddr[i];
			pOutMemDesc->nVirtAddr=pInDecMem->phyMem_virtAddr[i];
			pOutMemDesc->nCpuAddr=pInDecMem->phyMem_cpuAddr[i];
			pOutMemDesc->nSize=pInDecMem->phyMem_size[i];
			return 1;
		}
	}

	return 0;
}


OMX_S32 MemMallocVpuBlock(VpuMemInfo* pMemBlock,VpuDecoderMemInfo* pVpuMem,OMX_PARAM_MEM_OPERATOR* pMemOp)
{
	OMX_S32 i;
	OMX_U8 * ptr=NULL;
	OMX_S32 size;

	for(i=0; i<pMemBlock->nSubBlockNum; i++)
	{
		size=pMemBlock->MemSubBlock[i].nAlignment+pMemBlock->MemSubBlock[i].nSize;
		if(pMemBlock->MemSubBlock[i].MemType==VPU_MEM_VIRT)
		{
			ptr=(OMX_U8*)FSL_MALLOC(size);
			if(ptr==NULL)
			{
				VPU_COMP_LOG("%s: get virtual memory failure, size=%d \r\n",__FUNCTION__,(INT32)size);
				goto failure;
			}
			pMemBlock->MemSubBlock[i].pVirtAddr=(OMX_U8*)Align(ptr,pMemBlock->MemSubBlock[i].nAlignment);

			//record virtual base addr
			pVpuMem->virtMem[pVpuMem->nVirtNum]=(OMX_U32)ptr;
			pVpuMem->nVirtNum++;
		}
		else// if(memInfo.MemSubBlock[i].MemType==VPU_MEM_PHY)
		{
			VpuMemDesc vpuMem;
			VpuDecRetCode ret;
			vpuMem.nSize=size;
			ret=VPU_DecGetMem_Wrapper(&vpuMem,pMemOp);
			if(ret!=VPU_DEC_RET_SUCCESS)
			{
				VPU_COMP_LOG("%s: get vpu memory failure, size=%d, ret=0x%X \r\n",__FUNCTION__,(INT32)size,ret);
				goto failure;
			}
			pMemBlock->MemSubBlock[i].pVirtAddr=(OMX_U8*)Align(vpuMem.nVirtAddr,pMemBlock->MemSubBlock[i].nAlignment);
			pMemBlock->MemSubBlock[i].pPhyAddr=(OMX_U8*)Align(vpuMem.nPhyAddr,pMemBlock->MemSubBlock[i].nAlignment);

			//record physical base addr
			pVpuMem->phyMem_phyAddr[pVpuMem->nPhyNum]=(OMX_U32)vpuMem.nPhyAddr;
			pVpuMem->phyMem_virtAddr[pVpuMem->nPhyNum]=(OMX_U32)vpuMem.nVirtAddr;
			pVpuMem->phyMem_cpuAddr[pVpuMem->nPhyNum]=(OMX_U32)vpuMem.nCpuAddr;
			pVpuMem->phyMem_size[pVpuMem->nPhyNum]=size;
			pVpuMem->nPhyNum++;
		}
	}

	return 1;

failure:
	MemFreeBlock(pVpuMem,pMemOp);
	return 0;

}

OMX_S32 OutInfoQueryEmptyNode(VpuDecOutFrameInfo** ppOutFrameInfo, VpuDecoderOutMapInfo* pInOutMapInfo)
{
	OMX_S32 i;
	for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
	{
		//find empty node
		if(pInOutMapInfo->pVirtAddr[i]==NULL)
		{
			*ppOutFrameInfo=&pInOutMapInfo->outFrameInfo[i];
			return i;
		}
	}
	return -1;
}

OMX_S32 OutInfoRegisterVirtAddr(OMX_S32 index, OMX_PTR pInVirtAddr,VpuDecoderOutMapInfo* pOutOutMapInfo)
{
	//set node
	pOutOutMapInfo->pVirtAddr[index]=pInVirtAddr;
	return 1;
}

OMX_S32 OutInfoMap(OMX_PTR pInVirtAddr,VpuDecOutFrameInfo** ppOutFrameInfo,VpuDecoderOutMapInfo* pInOutMapInfo)
{
	OMX_S32 i;
	for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
	{
		//find matched node
		if(pInOutMapInfo->pVirtAddr[i]==pInVirtAddr)
		{
			*ppOutFrameInfo=&pInOutMapInfo->outFrameInfo[i];
			return i;
		}
	}
	return -1;
}

OMX_S32 OutInfoUnregisterVirtAddr(OMX_S32 index, VpuDecoderOutMapInfo* pOutOutMapInfo)
{
	//clear node
	pOutOutMapInfo->pVirtAddr[index]=NULL;
	return 1;
}

OMX_S32 OutInfoReSet(VpuDecoderMemInfo* pInDecMem,VpuDecoderOutMapInfo* pOutOutMapInfo,OMX_S32 nTotalCnt, VpuFrameBuffer** pInFrameBufInfo,OMX_S32 nInFrameBufNum)
{
	OMX_S32 i;
	OMX_S32 nCnt=0;

	//cleare all before re-set
	fsl_osal_memset(pOutOutMapInfo,0,sizeof(VpuDecoderOutMapInfo));

	//re-set value
	ASSERT(nInFrameBufNum==pInDecMem->nPhyNum);
	for(i=0; i<pInDecMem->nPhyNum; i++)
	{
#if 0
		if(((OMX_U32)NULL)!=pInDecMem->phyMem_virtAddr[i])
		{
			pOutOutMapInfo->pVirtAddr[nCnt]=(OMX_PTR)pInDecMem->phyMem_virtAddr[i];
			fsl_osal_memset(&pOutOutMapInfo->outFrameInfo[nCnt],NULL,sizeof(VpuDecOutFrameInfo));
			nCnt++;
		}
#else
		pOutOutMapInfo->outFrameInfo[nCnt].pDisplayFrameBuf=*pInFrameBufInfo++;
		pOutOutMapInfo->pVirtAddr[nCnt]=pOutOutMapInfo->outFrameInfo[nCnt].pDisplayFrameBuf->pbufVirtY;
		nCnt++;
#endif
	}

	ASSERT(nCnt==nTotalCnt);
	return 1;
}


OMX_S32 OutInfoOutputNum(VpuDecoderOutMapInfo* pInOutMapInfo)
{
	OMX_S32 i;
	OMX_S32 nCnt=0;

	//re-set value
	for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
	{
		if(NULL!=pInOutMapInfo->pVirtAddr[i])
		{
			nCnt++;
		}
	}

	return nCnt;
}


OMX_S32 FrameBufRegister(OMX_PTR pInVirtAddr,VpuDecoderMemInfo* pOutDecMem,OMX_S32* pOutIsPhy)
{
	OMX_S32 i;
	OMX_PTR pPhyAddr;

	//get physical info from resource manager
	if(OMX_ErrorNone!=GetHwBuffer(pInVirtAddr,&pPhyAddr))
	{
		return -1;
	}

	if(pPhyAddr==NULL)
	{
		//1 virtual space ??
		for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
		{
			//insert into empty node
			if(NULL==pOutDecMem->virtMem[i])
			{
				pOutDecMem->virtMem[i]=(OMX_U32)pInVirtAddr;
				pOutDecMem->nVirtNum++;
				*pOutIsPhy=0;
				return pOutDecMem->nPhyNum;
			}
		}
	}
	else
	{
		//physical space
		for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
		{
			//insert into empty node
			if(NULL==pOutDecMem->phyMem_phyAddr[i])
			{
				pOutDecMem->phyMem_phyAddr[i]=(OMX_U32)pPhyAddr;
				pOutDecMem->phyMem_virtAddr[i]=(OMX_U32)pInVirtAddr;
				//pOutDecMem->phyMem_cpuAddr[i]=
				//pOutDecMem->phyMem_size[i]=
				pOutDecMem->nPhyNum++;
				*pOutIsPhy=1;
				return pOutDecMem->nPhyNum;
			}
		}
	}

	return -1;
}

OMX_S32 FrameBufExist(OMX_PTR pInVirtAddr,VpuDecoderMemInfo* pOutDecMem, OMX_S32* pOutIsPhy)
{
	OMX_S32 i;
	OMX_PTR pPhyAddr;

	//get physical info from resource manager
	if(OMX_ErrorNone!=GetHwBuffer(pInVirtAddr,&pPhyAddr))
	{
		return 0;
	}

	if(pPhyAddr==NULL)
	{
		//1 virtual space ??
		for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
		{
			//search matched node
			if((OMX_U32)pInVirtAddr==pOutDecMem->virtMem[i])
			{
				*pOutIsPhy=0;
				return 1;
			}
		}
	}
	else
	{
		//physical space
		for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
		{
			//search matched node
			if((OMX_U32)pPhyAddr==pOutDecMem->phyMem_phyAddr[i])
			{
				*pOutIsPhy=1;
				return 1;
			}
		}
	}
	return 0;
}

OMX_S32 FrameBufPhyNum(VpuDecoderMemInfo* pInDecMem)
{
	return pInDecMem->nPhyNum;
}

OMX_S32 FrameBufClear(VpuDecoderMemInfo* pOutDecMem)
{
	OMX_S32 i;
	//clear all node
	for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
	{
		pOutDecMem->phyMem_virtAddr[i]=NULL;

		pOutDecMem->phyMem_phyAddr[i]=NULL;
		pOutDecMem->phyMem_virtAddr[i]=NULL;
		pOutDecMem->phyMem_cpuAddr[i]=NULL;
		pOutDecMem->phyMem_size[i]=0;
	}
	pOutDecMem->nVirtNum=0;
	pOutDecMem->nPhyNum=0;
	return 1;
}

OMX_S32 FrameBufCreateRegisterFrame(
    VpuFrameBuffer* pOutRegisterFrame,VpuDecoderMemInfo* pInDecMem,
    OMX_S32 nInCnt,OMX_S32 nPadW,OMX_S32 nPadH,
    VpuDecoderMemInfo* pOutDecMemInfo,
    OMX_PARAM_MEM_OPERATOR* pMemOp,OMX_COLOR_FORMATTYPE colorFormat,OMX_U32 nInChromaAlign)
{
	VpuDecRetCode ret;
	VpuMemDesc vpuMem;
	OMX_S32 i;
	OMX_S32 nCnt=0;
	OMX_S32 yStride;
	OMX_S32 uvStride;
	OMX_S32 ySize;
	OMX_S32 uvSize;
	OMX_S32 mvSize;
	OMX_S32 mvTotalSize;
	OMX_U8* ptr;
	OMX_U8* ptrVirt;

	yStride=nPadW;
	ySize=yStride*nPadH;

	switch (colorFormat)
	{
	case OMX_COLOR_FormatYUV420Planar:
	case OMX_COLOR_FormatYUV420SemiPlanar:
		uvStride=yStride/2;
		uvSize=ySize/4;
		mvSize=ySize/4;
		break;
	case OMX_COLOR_FormatYUV422Planar:
	case OMX_COLOR_FormatYUV422SemiPlanar:
		//hor ???
		uvStride=yStride/2;
		uvSize=ySize/2;
		mvSize=ySize/2;	//In fact, for MJPG, mv is useless
		break;
		//FIXME: add 4:0:0/4:4:4/...
	default:
		VPU_COMP_ERR_LOG("unsupported color format : %d \r\n",colorFormat);
		uvStride=yStride/2;
		uvSize=ySize/4;
		mvSize=ySize/4;
		break;
	}

	//fill StrideY/StrideC/Y/Cb/Cr info
	for(i=0; i<VPU_DEC_MAX_NUM_MEM; i++)
	{
		//search valid phy buff
		if(pInDecMem->phyMem_phyAddr[i]!=NULL)
		{
			ptr=(OMX_U8*)pInDecMem->phyMem_phyAddr[i];
			ptrVirt=(OMX_U8*)pInDecMem->phyMem_virtAddr[i];

			/* fill stride info */
			pOutRegisterFrame[nCnt].nStrideY=yStride;
			pOutRegisterFrame[nCnt].nStrideC=uvStride;

			/* fill phy addr*/
			pOutRegisterFrame[nCnt].pbufY=(OMX_U8*)Align(ptr,nInChromaAlign);
			pOutRegisterFrame[nCnt].pbufCb=(OMX_U8*)Align(pOutRegisterFrame[nCnt].pbufY+ySize,nInChromaAlign);
			pOutRegisterFrame[nCnt].pbufCr=(OMX_U8*)Align(pOutRegisterFrame[nCnt].pbufCb+uvSize,nInChromaAlign);
			//pOutRegisterFrame[nCnt].pbufMvCol=ptr+ySize+uvSize*2;

			/* fill virt addr */
			pOutRegisterFrame[nCnt].pbufVirtY=(OMX_U8*)Align(ptrVirt,nInChromaAlign);
			pOutRegisterFrame[nCnt].pbufVirtCb=(OMX_U8*)Align(pOutRegisterFrame[nCnt].pbufVirtY+ySize,nInChromaAlign);
			pOutRegisterFrame[nCnt].pbufVirtCr=(OMX_U8*)Align(pOutRegisterFrame[nCnt].pbufVirtCb+uvSize,nInChromaAlign);
			//pOutRegisterFrame[nCnt].pbufVirtMvCol=ptrVirt+ySize+uvSize*2;

			nCnt++;
			//if(nCnt>=nInCnt)
			//{
			//	break;
			//}
		}
	}

	if(nCnt<nInCnt)
	{
		return -1;
	}

	//malloc phy memory for mv
	mvTotalSize=mvSize*nCnt;

	vpuMem.nSize=mvTotalSize;
	ret=VPU_DecGetMem_Wrapper(&vpuMem,pMemOp);
	if(VPU_DEC_RET_SUCCESS!=ret)
	{
		VPU_COMP_ERR_LOG("%s: vpu malloc frame buf failure: ret=0x%X \r\n",__FUNCTION__,ret);
		return -1;//OMX_ErrorInsufficientResources;
	}

	//record memory info for release
	pOutDecMemInfo->phyMem_phyAddr[pOutDecMemInfo->nPhyNum]=vpuMem.nPhyAddr;
	pOutDecMemInfo->phyMem_virtAddr[pOutDecMemInfo->nPhyNum]=vpuMem.nVirtAddr;
	pOutDecMemInfo->phyMem_cpuAddr[pOutDecMemInfo->nPhyNum]=vpuMem.nCpuAddr;
	pOutDecMemInfo->phyMem_size[pOutDecMemInfo->nPhyNum]=vpuMem.nSize;
	pOutDecMemInfo->nPhyNum++;

	//fill mv info
	ptr=(OMX_U8*)vpuMem.nPhyAddr;
	ptrVirt=(OMX_U8*)vpuMem.nVirtAddr;
	for(i=0; i<nCnt; i++)
	{
		pOutRegisterFrame[i].pbufMvCol=ptr;
		pOutRegisterFrame[i].pbufVirtMvCol=ptrVirt;
		ptr+=mvSize;
		ptrVirt+=mvSize;
	}

	//we allow nCnt>nInCnt
	return nCnt;
}


OMX_S32 FrameBufFindOneUnOutputed(OMX_PTR * ppOutVirtBuf,VpuDecoderMemInfo* pInDecMem,VpuDecoderOutMapInfo* pInOutMapInfo)
{
	OMX_S32 i;
	VpuDecOutFrameInfo* pOutFrameInfo;
	for(i=0; i<pInDecMem->nPhyNum; i++)
	{
		//find valid node
		if(NULL!=pInDecMem->phyMem_virtAddr[i])
		{
			if(-1==OutInfoMap((OMX_PTR)pInDecMem->phyMem_virtAddr[i],&pOutFrameInfo , pInOutMapInfo))
			{
				//find one un-outputed frame
				*ppOutVirtBuf=(OMX_PTR)pInDecMem->phyMem_virtAddr[i];
				return i;
			}
		}
	}
	*ppOutVirtBuf=NULL;
	return -1;
}

OMX_ERRORTYPE OpenVpu(VpuDecHandle* pOutHandle, VpuMemInfo* pInMemInfo,
                      VpuCodStd eCodeFormat, OMX_S32 nPicWidth,OMX_S32 nPicHeight,OMX_COLOR_FORMATTYPE eColorFormat,OMX_S32 enableFileMode,VPUCompSemaphor psem)
{
	VpuDecOpenParam decOpenParam;
	VpuDecRetCode ret;
	OMX_S32 para;

	VPU_COMP_LOG("%s: codec format: %d \r\n",__FUNCTION__,eCodeFormat);
	//clear 0
	fsl_osal_memset(&decOpenParam, 0, sizeof(VpuDecOpenParam));
	//set open params
	decOpenParam.CodecFormat=eCodeFormat;

	//FIXME: for MJPG, we need to add check for 4:4:4/4:2:2 ver/4:0:0  !!!
	if((OMX_COLOR_FormatYUV420SemiPlanar==eColorFormat)
	        ||(OMX_COLOR_FormatYUV422SemiPlanar==eColorFormat))
	{
		decOpenParam.nChromaInterleave=1;
	}
	else
	{
		decOpenParam.nChromaInterleave=0;
	}
	decOpenParam.nReorderEnable=1;	//for H264
	switch(eCodeFormat)
	{
	case VPU_V_MPEG4:
	case VPU_V_DIVX4:
	case VPU_V_DIVX56:
	case VPU_V_XVID:
	case VPU_V_H263:
		decOpenParam.nEnableFileMode=enableFileMode;	//FIXME: set 1 in future
		break;
	case VPU_V_AVC:
		decOpenParam.nEnableFileMode=enableFileMode; 	//FIXME: set 1 in future
		break;
	case VPU_V_MPEG2:
		decOpenParam.nEnableFileMode=enableFileMode; 	//FIXME: set 1 in future
		break;
	case VPU_V_VC1:
	case VPU_V_VC1_AP:
	case VPU_V_DIVX3:
	case VPU_V_RV:
		//for VC1/RV/DivX3: we should use filemode, otherwise, some operations are unstable(such as seek...)
		decOpenParam.nEnableFileMode=enableFileMode;
		break;
	case VPU_V_MJPG:
		//must file mode ???
		decOpenParam.nEnableFileMode=enableFileMode;
		break;
	default:
		decOpenParam.nEnableFileMode=enableFileMode;
		break;
	}

	//for special formats, such as package VC1 header,...
	decOpenParam.nPicWidth=nPicWidth;
	decOpenParam.nPicHeight=nPicHeight;

	//open vpu
	VPU_COMP_SEM_LOCK(psem);
	ret=VPU_DecOpen(pOutHandle, &decOpenParam, pInMemInfo);
	VPU_COMP_SEM_UNLOCK(psem);
	if (ret!=VPU_DEC_RET_SUCCESS)
	{
		VPU_COMP_ERR_LOG("%s: vpu open failure: ret=0x%X \r\n",__FUNCTION__,ret);
		return OMX_ErrorHardware;
	}

	//set default config
	VPU_COMP_SEM_LOCK(psem);
	para=VPU_DEC_SKIPNONE;
	ret=VPU_DecConfig(*pOutHandle, VPU_DEC_CONF_SKIPMODE, &para);
	if(VPU_DEC_RET_SUCCESS!=ret)
	{
		VPU_COMP_ERR_LOG("%s: vpu config failure: config=0x%X, ret=%d \r\n",__FUNCTION__,(UINT32)VPU_DEC_CONF_SKIPMODE,ret);
		VPU_DecClose(*pOutHandle);
		VPU_COMP_SEM_UNLOCK(psem);
		return OMX_ErrorHardware;
	}
	VPU_COMP_SEM_UNLOCK(psem);
	return OMX_ErrorNone;
}

#ifdef VPU_DEC_COMP_DROP_B
OMX_ERRORTYPE ConfigVpu(VpuDecHandle InHandle,OMX_TICKS nTimeStamp,OMX_PTR pClock,VPUCompSemaphor psem)
{
	VpuDecRetCode ret;
	OMX_TIME_CONFIG_TIMESTAMPTYPE sCur;
	VpuDecConfig config;
	OMX_S32 param;

	if(pClock!=NULL)
	{
		OMX_INIT_STRUCT(&sCur, OMX_TIME_CONFIG_TIMESTAMPTYPE);
		OMX_GetConfig(pClock, OMX_IndexConfigTimeCurrentMediaTime, &sCur);
		if(sCur.nTimestamp > (nTimeStamp - DROP_B_THRESHOLD))
		{
			VPU_COMP_LOG("drop B frame \r\n");
			config=VPU_DEC_CONF_SKIPMODE;
			param=VPU_DEC_SKIPB;
		}
		else
		{
			config=VPU_DEC_CONF_SKIPMODE;
			param=VPU_DEC_SKIPNONE;
		}
		VPU_COMP_SEM_LOCK(psem);
		ret=VPU_DecConfig(InHandle, config, (void*)(&param));
		VPU_COMP_SEM_UNLOCK(psem);
		if(VPU_DEC_RET_SUCCESS!=ret)
		{
			VPU_COMP_ERR_LOG("%s: vpu config failure: config=0x%X, ret=%d \r\n",__FUNCTION__,config,ret);
			return OMX_ErrorHardware;
		}
	}

	return OMX_ErrorNone;
}
#endif

OMX_COLOR_FORMATTYPE ConvertMjpgColorFormat(OMX_S32 sourceFormat,OMX_COLOR_FORMATTYPE oriColorFmt)
{
	OMX_COLOR_FORMATTYPE	colorformat;
	OMX_S32 interleave=0;

	switch(oriColorFmt)
	{
	case OMX_COLOR_FormatYUV420SemiPlanar:
	case OMX_COLOR_FormatYUV422SemiPlanar:
		//FIXME: add 4:0:0/4:4:4/...
		interleave=1;
		break;
	default:
		break;
	}
	switch(sourceFormat)
	{
	case 0:	//4:2:0
		colorformat=(1==interleave)?OMX_COLOR_FormatYUV420SemiPlanar:OMX_COLOR_FormatYUV420Planar;
		break;
	case 1:	//4:2:2 hor
		colorformat=(1==interleave)?OMX_COLOR_FormatYUV422SemiPlanar:OMX_COLOR_FormatYUV422Planar;
		break;
#if 0	//FIXME
	case 2:	//4:2:2 ver
		VPU_COMP_ERR_LOG("unsupport mjpg 4:2:2 ver color format: %d \r\n",sourceFormat);
		colorformat=OMX_COLOR_FormatYUV422Planar;
		break;
	case 3:	//4:4:4
		VPU_COMP_ERR_LOG("unsupport mjpg 4:4:4 color format: %d \r\n",sourceFormat);
		colorformat=OMX_COLOR_FormatYUV444Planar;
		break;
	case 4:	//4:0:0
		VPU_COMP_ERR_LOG("unsupport mjpg 4:0:0 color format: %d \r\n",sourceFormat);
		colorformat=OMX_COLOR_FormatYUV400Planar;
		break;
#endif
	default:	//unknow
		VPU_COMP_ERR_LOG("unknown mjpg color format: %d \r\n",(INT32)sourceFormat);
		colorformat=OMX_COLOR_FormatYUV420Planar;
		break;
	}
	return colorformat;
}

VpuDecoder::VpuDecoder()
{
	VPU_COMP_API_LOG("%s: \r\n",__FUNCTION__);

	// version info
	//fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_decoder.vpu.v2");
	ComponentVersion.s.nVersionMajor = 0x0;
	ComponentVersion.s.nVersionMinor = 0x1;
	ComponentVersion.s.nRevision = 0x0;
	ComponentVersion.s.nStep = 0x0;

	//set default
	fsl_osal_strcpy((fsl_osal_char*)name, "OMX.Freescale.std.video_decoder.mpeg4.hw-based");

	fsl_osal_memset(&sInFmt, 0, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
	sInFmt.nFrameWidth = DEFAULT_FRM_WIDTH;
	sInFmt.nFrameHeight = DEFAULT_FRM_HEIGHT;
	sInFmt.xFramerate = DEFAULT_FRM_RATE;
	sInFmt.eColorFormat = OMX_COLOR_FormatUnused;
	sInFmt.eCompressionFormat = OMX_VIDEO_CodingMPEG4;

	fsl_osal_memset(&sOutFmt, 0, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));
	sOutFmt.nFrameWidth = DEFAULT_FRM_WIDTH;
	sOutFmt.nFrameHeight = DEFAULT_FRM_HEIGHT;
	sOutFmt.eColorFormat = OMX_COLOR_FormatYUV420Planar;
	sOutFmt.eCompressionFormat = OMX_VIDEO_CodingUnused;

	bFilterSupportPartilInput = OMX_TRUE;
	nInBufferCnt = DEFAULT_BUF_IN_CNT;
	nInBufferSize = DEFAULT_BUF_IN_SIZE;
	nOutBufferCnt = DEFAULT_BUF_OUT_CNT;
	nOutBufferSize = sOutFmt.nFrameWidth * sOutFmt.nFrameHeight * pxlfmt2bpp(sOutFmt.eColorFormat) / 8;

	//clear internal variable 0
	fsl_osal_memset(&sMemInfo,0,sizeof(VpuMemInfo));
	fsl_osal_memset(&sVpuMemInfo,0,sizeof(VpuDecoderMemInfo));
	fsl_osal_memset(&sAllocMemInfo,0,sizeof(VpuDecoderMemInfo));
	fsl_osal_memset(&sFrameMemInfo,0,sizeof(VpuDecoderMemInfo));
	fsl_osal_memset(&sInitInfo,0,sizeof(VpuDecInitInfo));
	fsl_osal_memset(&sOutMapInfo,0,sizeof(VpuDecoderOutMapInfo));

	nHandle=0;

#if 0
	// role info
	role_cnt = 10;	//Mpeg2;Mpeg4;DivX3;DivX4;Divx56;XviD;H263;H264;RV;VC1
	role_cnt = 8;
	role[0] = "video_decoder.avc";
	role[1] = "video_decoder.wmv9";
	role[2] = "video_decoder.mpeg2";
	role[3] = "video_decoder.mpeg4";
	role[4] = "video_decoder.divx3";
	role[5] = "video_decoder.divx4";
	role[6] = "video_decoder.divx";
	role[7] = "video_decoder.xvid";
	role[8] = "video_decoder.h263";
	role[9] = "video_decoder.rv";
#endif

	eFormat = VPU_V_MPEG2;//VPU_V_MPEG4;
	nPadWidth=DEFAULT_FRM_WIDTH;
	nPadHeight=DEFAULT_FRM_HEIGHT;
	OMX_INIT_STRUCT(&sOutCrop, OMX_CONFIG_RECTTYPE);
	//sOutCrop.nPortIndex = VPUDEC_OUT_PORT;
	sOutCrop.nLeft = sOutCrop.nTop = 0;
	sOutCrop.nWidth = sInFmt.nFrameWidth;
	sOutCrop.nHeight = sInFmt.nFrameHeight;

	pInBuffer=(OMX_PTR)INVALID;
	nInSize=0;
	bInEos=OMX_FALSE;

	//pOutBuffer=NULL;
	//bOutLast=OMX_FALSE;

	eVpuDecoderState=VPU_COM_STATE_NONE;

	pLastOutVirtBuf=NULL;

	nFreeOutBufCnt=0;

	nNeedFlush=0;
	nNeedSkip=0;

	fsl_osal_memset(&sMemOperator,0,sizeof(OMX_PARAM_MEM_OPERATOR));

	ePlayMode=DEC_FILE_MODE;
	nChromaAddrAlign=CHROMA_ALIGN;
	pClock=NULL;
	nCapability=0;

	// init semaphore
#ifdef USE_PROCESS_SEM
	VPU_COMP_SEM_INIT_PROCESS(&psemaphore);
#else
	VPU_COMP_SEM_INIT(&psemaphore);
#endif

	//return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::SetRoleFormat(OMX_STRING role)
{
	OMX_VIDEO_CODINGTYPE CodingType = OMX_VIDEO_CodingUnused;
	//OMX_PARAM_PORTDEFINITIONTYPE sPortDef;

	VPU_COMP_API_LOG("%s: role: %s \r\n",__FUNCTION__,role);

	if(fsl_osal_strcmp(role, "video_decoder.mpeg2") == 0)
	{
		CodingType = OMX_VIDEO_CodingMPEG2;
		eFormat=VPU_V_MPEG2;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_MPEG2DEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.h263") == 0)
	{
		CodingType = OMX_VIDEO_CodingH263;
		eFormat=VPU_V_H263;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_H263DEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.sorenson") == 0)
	{
		CodingType = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_SORENSON263;//OMX_VIDEO_CodingH263;
		eFormat=VPU_V_H263;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_SORENSONDEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.mpeg4") == 0)
	{
		CodingType = OMX_VIDEO_CodingMPEG4;
		eFormat=VPU_V_MPEG4;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_MPEG4DEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.wmv9") == 0)
	{
		CodingType = (OMX_VIDEO_CODINGTYPE )OMX_VIDEO_CodingWMV9;
		//CodingType = OMX_VIDEO_CodingWMV; ???
		//1 we need to use SetParameter() to set eFormat  !!!
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_WMVDEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.rv") == 0)
	{
		CodingType = OMX_VIDEO_CodingRV;
		eFormat=VPU_V_RV;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_RVDEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.avc") == 0)
	{
		CodingType = OMX_VIDEO_CodingAVC;
		eFormat=VPU_V_AVC;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_AVCDEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.divx") == 0)
	{
		CodingType = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingDIVX;
		eFormat=VPU_V_DIVX56;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_DIVXDEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.div3") == 0)
	{
		CodingType = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingDIV3;
		eFormat=VPU_V_DIVX3;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_DIV3DEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.div4") == 0)
	{
		CodingType = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingDIV4;
		eFormat=VPU_V_DIVX4;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_DIV4DEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.xvid") == 0)
	{
		CodingType = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingXVID;
		eFormat=VPU_V_XVID;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_XVIDDEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.mjpeg") == 0)
	{
		CodingType = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingMJPEG;
		eFormat=VPU_V_MJPG;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_MJPEGDEC);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.avs") == 0)
	{
		//1 CodingType = (OMX_VIDEO_CODINGTYPE);
		VPU_COMP_ERR_LOG("%s: failure: avs unsupported: %s \r\n",__FUNCTION__,role);
		eFormat=VPU_V_AVS;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_AVS);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.vp8") == 0)
	{
		CodingType = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_VP8;
		eFormat=VPU_V_VP8;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_VP8);
	}
	else if(fsl_osal_strcmp(role, "video_decoder.mvc") == 0)
	{
		//1 CodingType = (OMX_VIDEO_CODINGTYPE);
		VPU_COMP_ERR_LOG("%s: failure: mvc unsupported: %s \r\n",__FUNCTION__,role);
		eFormat=VPU_V_AVC_MVC;
		fsl_osal_strcpy((fsl_osal_char*)name, VPU_COMP_NAME_MVC);
	}
	else
	{
		CodingType = OMX_VIDEO_CodingUnused;
		//eFormat=;
		VPU_COMP_ERR_LOG("%s: failure: unknow role: %s \r\n",__FUNCTION__,role);
		return OMX_ErrorUndefined;
	}

	//check input change
	if(sInFmt.eCompressionFormat!=CodingType)
	{
		sInFmt.eCompressionFormat=CodingType;
		InputFmtChanged();
	}
#if 0
	OMX_INIT_STRUCT(&sPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
	sPortDef.nPortIndex = VPUDEC_IN_PORT;
	ports[VPUDEC_IN_PORT]->GetPortDefinition(&sPortDef);
	sPortDef.format.video.eCompressionFormat = CodingType;
	ports[VPUDEC_IN_PORT]->SetPortDefinition(&sPortDef);
#endif

	return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::GetParameter(
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR pComponentParameterStructure)
{
	VPU_COMP_API_LOG("%s: nParamIndex=0x%X, \r\n",__FUNCTION__,nParamIndex);

	//check state
	switch(eVpuDecoderState)
	{
	default:
		break;
	}

	if(NULL==pComponentParameterStructure)
	{
		VPU_COMP_ERR_LOG("%s: failure: param is null \r\n",__FUNCTION__);
		return OMX_ErrorBadParameter;
	}

	if(nParamIndex==OMX_IndexParamStandardComponentRole)
	{
		fsl_osal_strcpy((OMX_STRING)((OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure)->cRole,(OMX_STRING)cRole);
		return OMX_ErrorNone;
	}
	else if(nParamIndex ==  OMX_IndexParamVideoWmv)
	{
		OMX_VIDEO_PARAM_WMVTYPE  *pPara;
		pPara = (OMX_VIDEO_PARAM_WMVTYPE *)pComponentParameterStructure;
		switch(eFormat)
		{
		case VPU_V_VC1:
			pPara->eFormat = OMX_VIDEO_WMVFormat9;
			break;
		default:
			pPara->eFormat = OMX_VIDEO_WMVFormatUnused;
			break;
		}
		return OMX_ErrorNone;
	}
	else if(nParamIndex ==  OMX_IndexParamVideoProfileLevelQuerySupported)
	{
		struct CodecProfileLevel
		{
			OMX_U32 mProfile;
			OMX_U32 mLevel;
		};

		static const CodecProfileLevel kH263ProfileLevels[] =
		{
			{ OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10 },
			{ OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20 },
			{ OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30 },
			{ OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45 },
			{ OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50 },
			{ OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60 },
			{ OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70 },
			{ OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level10 },
			{ OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level20 },
			{ OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level30 },
			{ OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level45 },
			{ OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level50 },
			{ OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level60 },
			{ OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level70 },
		};

		static const CodecProfileLevel kProfileLevels[] =
		{
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1  },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11 },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12 },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13 },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2  },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21 },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22 },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3  },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31 },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32 },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4  },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41 },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42 },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5  },
			{ OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1  },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11 },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12 },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13 },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2  },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21 },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22 },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3  },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31 },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32 },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4  },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41 },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42 },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel5  },
			{ OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1  },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11 },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12 },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13 },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2  },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21 },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22 },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3  },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31 },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32 },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4  },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41 },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42 },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel5  },
			{ OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51 },
		};

		static const CodecProfileLevel kM4VProfileLevels[] =
		{
			{ OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0 },
			{ OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0b },
			{ OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level1 },
			{ OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level2 },
			{ OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3 },
			{ OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0 },
			{ OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0b },
			{ OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level1 },
			{ OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level2 },
			{ OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level3 },
			{ OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4 },
			{ OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4a },
			{ OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5 },
		};

		static const CodecProfileLevel kMpeg2ProfileLevels[] =
		{
			{ OMX_VIDEO_MPEG2ProfileSimple, OMX_VIDEO_MPEG2LevelLL },
			{ OMX_VIDEO_MPEG2ProfileSimple, OMX_VIDEO_MPEG2LevelML },
			{ OMX_VIDEO_MPEG2ProfileSimple, OMX_VIDEO_MPEG2LevelH14},
			{ OMX_VIDEO_MPEG2ProfileSimple, OMX_VIDEO_MPEG2LevelHL},
			{ OMX_VIDEO_MPEG2ProfileMain, OMX_VIDEO_MPEG2LevelLL },
			{ OMX_VIDEO_MPEG2ProfileMain, OMX_VIDEO_MPEG2LevelML },
			{ OMX_VIDEO_MPEG2ProfileMain, OMX_VIDEO_MPEG2LevelH14},
			{ OMX_VIDEO_MPEG2ProfileMain, OMX_VIDEO_MPEG2LevelHL},
		};

		OMX_VIDEO_PARAM_PROFILELEVELTYPE  *pPara;
		OMX_S32 index;
		OMX_S32 nProfileLevels;

		pPara = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
		switch(eFormat)
		{
		case VPU_V_H263:
			index = pPara->nProfileIndex;

			nProfileLevels =sizeof(kH263ProfileLevels) / sizeof(kH263ProfileLevels[0]);
			if (index >= nProfileLevels)
			{
				return OMX_ErrorNoMore;
			}

			pPara->eProfile = kH263ProfileLevels[index].mProfile;
			pPara->eLevel = kH263ProfileLevels[index].mLevel;
			break;
		case VPU_V_AVC:
			index = pPara->nProfileIndex;

			nProfileLevels =sizeof(kProfileLevels) / sizeof(kProfileLevels[0]);
			if (index >= nProfileLevels)
			{
				return OMX_ErrorNoMore;
			}

			pPara->eProfile = kProfileLevels[index].mProfile;
			pPara->eLevel = kProfileLevels[index].mLevel;
			break;
		case VPU_V_MPEG4:
			index = pPara->nProfileIndex;

			nProfileLevels =sizeof(kM4VProfileLevels) / sizeof(kM4VProfileLevels[0]);
			if (index >= nProfileLevels)
			{
				return OMX_ErrorNoMore;
			}

			pPara->eProfile = kM4VProfileLevels[index].mProfile;
			pPara->eLevel = kM4VProfileLevels[index].mLevel;
			break;
		case VPU_V_MPEG2:
			index = pPara->nProfileIndex;

			nProfileLevels =sizeof(kMpeg2ProfileLevels) / sizeof(kMpeg2ProfileLevels[0]);
			if (index >= nProfileLevels)
			{
				return OMX_ErrorNoMore;
			}

			pPara->eProfile = kMpeg2ProfileLevels[index].mProfile;
			pPara->eLevel = kMpeg2ProfileLevels[index].mLevel;
			break;
		default:
			return OMX_ErrorUnsupportedIndex;
			//break;
		}
		return OMX_ErrorNone;
	}
	else
	{
		VPU_COMP_ERR_LOG("%s: failure: unsupported index: 0x%X \r\n",__FUNCTION__,nParamIndex);
		return OMX_ErrorUnsupportedIndex;
	}
}

OMX_ERRORTYPE VpuDecoder::SetParameter(
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR pComponentParameterStructure)
{
	OMX_ERRORTYPE ret = OMX_ErrorNone;
	VPU_COMP_API_LOG("%s: nParamIndex=0x%X \r\n",__FUNCTION__,nParamIndex);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_LOADED:
		break;
	case VPU_COM_STATE_OPENED:
		if(nParamIndex ==  OMX_IndexParamVideoWmv)
		{
			break;
		}
	default:
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure: error state transition, current state=%d, nParamIndex=0x%X, role=%s \r\n",__FUNCTION__,eVpuDecoderState,nParamIndex,(OMX_STRING)cRole);
		return OMX_ErrorIncorrectStateTransition;
	}


	if(NULL==pComponentParameterStructure)
	{
		VPU_COMP_ERR_LOG("%s: failure: param is null \r\n",__FUNCTION__);
		return OMX_ErrorBadParameter;
	}

	if(nParamIndex==OMX_IndexParamStandardComponentRole)
	{
		fsl_osal_strcpy( (fsl_osal_char *)cRole,(fsl_osal_char *)((OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure)->cRole);
		if(OMX_ErrorNone!=SetRoleFormat((OMX_STRING)cRole))
		{
			VPU_COMP_ERR_LOG("%s: set role format failure \r\n",__FUNCTION__);
			return OMX_ErrorBadParameter;
		}
	}
	else if(nParamIndex ==  OMX_IndexParamVideoWmv)
	{
		OMX_VIDEO_PARAM_WMVTYPE  *pPara;
		pPara = (OMX_VIDEO_PARAM_WMVTYPE *)pComponentParameterStructure;

		switch(pPara->eFormat)
		{
		case OMX_VIDEO_WMVFormat7:
		case OMX_VIDEO_WMVFormat8:
		case OMX_VIDEO_WMVFormat9a:
			ret = OMX_ErrorBadParameter;
			break;
		case OMX_VIDEO_WMVFormat9:
			eFormat = VPU_V_VC1;
			VPU_COMP_LOG("%s: set OMX_VIDEO_WMVFormat9 \r\n",__FUNCTION__);
			break;
		case OMX_VIDEO_WMVFormatWVC1:
			eFormat = VPU_V_VC1_AP;
			VPU_COMP_LOG("%s: set OMX_VIDEO_WMVFormatWVC1 \r\n",__FUNCTION__);
			break;
		default:
			//eFormat = ;
			VPU_COMP_ERR_LOG("%s: failure: unsupported format: 0x%X \r\n",__FUNCTION__,pPara->eFormat);
			ret = OMX_ErrorBadParameter;
			break;
		}
	}
	else if (nParamIndex==OMX_IndexParamMemOperator)
	{
		//should be set before open vpu(eg. malloc bitstream/frame buffers)
		if(VPU_COM_STATE_LOADED!=eVpuDecoderState)
		{
			ret=OMX_ErrorInvalidState;
		}
		else
		{
			OMX_PARAM_MEM_OPERATOR * pPara;
			pPara=(OMX_PARAM_MEM_OPERATOR *)pComponentParameterStructure;
			VPU_COMP_LOG("%s: set OMX_IndexParamMemOperator \r\n",__FUNCTION__);
			sMemOperator=*pPara;
		}
	}
	else if(nParamIndex==OMX_IndexParamDecoderPlayMode)
	{
		OMX_DECODE_MODE* pMode=(OMX_DECODE_MODE*)pComponentParameterStructure;
		ePlayMode=*pMode;
		VPU_COMP_LOG("%s: set OMX_IndexParamDecoderPlayMode: %d \r\n",__FUNCTION__,*pMode);
	}
	else if(nParamIndex==OMX_IndexParamVideoDecChromaAlign)
	{
		OMX_U32* pAlignVal=(OMX_U32*)pComponentParameterStructure;
		nChromaAddrAlign=*pAlignVal;
		VPU_COMP_LOG("%s: set OMX_IndexParamVideoDecChromaAlign: %d \r\n",__FUNCTION__,nChromaAddrAlign);
		if(nChromaAddrAlign==0) nChromaAddrAlign=1;
	}
	else
	{
		VPU_COMP_ERR_LOG("%s: failure: unsupported index: 0x%X \r\n",__FUNCTION__,nParamIndex);
		ret = OMX_ErrorUnsupportedIndex;
	}

	return ret;
}

OMX_ERRORTYPE VpuDecoder::GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pComponentParameterStructure)
{
	VPU_COMP_API_LOG("%s: nParamIndex=0x%X, \r\n",__FUNCTION__,nParamIndex);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_NONE:
	case VPU_COM_STATE_LOADED:
	case VPU_COM_STATE_OPENED:	//allow user get wrong value before opened ???
		//forbidden
		VPU_COMP_LOG("%s: failure: error state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorIncorrectStateTransition;
	default:
		break;
	}

	if(NULL==pComponentParameterStructure)
	{
		VPU_COMP_ERR_LOG("%s: failure: param is null  \r\n",__FUNCTION__);
		return OMX_ErrorBadParameter;
	}

	switch (nParamIndex)
	{
	case OMX_IndexConfigCommonOutputCrop:
	{
		OMX_CONFIG_RECTTYPE *pRecConf = (OMX_CONFIG_RECTTYPE*)pComponentParameterStructure;
		if(pRecConf->nPortIndex == VPUDEC_OUT_PORT)
		{
			pRecConf->nTop = sOutCrop.nTop;
			pRecConf->nLeft = sOutCrop.nLeft;
			pRecConf->nWidth = sOutCrop.nWidth;
			pRecConf->nHeight = sOutCrop.nHeight;
			VPU_COMP_LOG("%s: [top,left,width,height]=[%d,%d,%d,%d], \r\n",__FUNCTION__,(INT32)pRecConf->nTop,(INT32)pRecConf->nLeft,(INT32)pRecConf->nWidth,(INT32)pRecConf->nHeight);
		}
	}
	break;
	case OMX_IndexConfigCommonScale:
	{
		OMX_CONFIG_SCALEFACTORTYPE *pDispRatio = (OMX_CONFIG_SCALEFACTORTYPE *)pComponentParameterStructure;
		if(pDispRatio->nPortIndex == VPUDEC_OUT_PORT)
		{
			pDispRatio->xWidth = sDispRatio.xWidth;
			pDispRatio->xHeight = sDispRatio.xHeight;
		}
	}
	break;
	case OMX_IndexConfigVideoOutBufPhyAddr:
	{
		OMX_CONFIG_VIDEO_OUTBUFTYPE * param = (OMX_CONFIG_VIDEO_OUTBUFTYPE *)pComponentParameterStructure;
		if(OMX_ErrorNone!=GetHwBuffer(param->pBufferHdr->pBuffer,&param->nPhysicalAddr))
		{
			return OMX_ErrorBadParameter;
		}
	}
	break;
	default:
		VPU_COMP_ERR_LOG("%s: failure: unsupported index: 0x%X \r\n",__FUNCTION__,nParamIndex);
		return OMX_ErrorUnsupportedIndex;
		//break;
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::SetConfig(OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure)
{
	OMX_ERRORTYPE eRetVal = OMX_ErrorNone;
	OMX_CONFIG_CLOCK *pC;

	if(pComponentConfigStructure == NULL)
	{
		return OMX_ErrorBadParameter;

	}
	switch(nIndex)
	{
	case OMX_IndexConfigClock:
		pC = (OMX_CONFIG_CLOCK*) pComponentConfigStructure;
		pClock = pC->hClock;
		break;
	default:
		return eRetVal;
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::InitFilterComponent()
{
	VpuDecRetCode ret;
	//VpuVersionInfo ver;
	//VpuMemInfo sMemInfo;

	VPU_COMP_API_LOG("%s: \r\n",__FUNCTION__);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_NONE:
		break;
	default:
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure: error state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorIncorrectStateTransition;

	}

	//load vpu
	VPU_COMP_SEM_LOCK(psemaphore);
	ret=VPU_DecLoad();
	VPU_COMP_SEM_UNLOCK(psemaphore);
	if (ret!=VPU_DEC_RET_SUCCESS)
	{
		VPU_COMP_ERR_LOG("%s: vpu load failure: ret=0x%X \r\n",__FUNCTION__,ret);
		return OMX_ErrorHardware;
	}

	//version info
	VPU_COMP_SEM_LOCK(psemaphore);
	ret=VPU_DecGetVersionInfo(&sVpuVer);
	if (ret!=VPU_DEC_RET_SUCCESS)
	{
		VPU_COMP_ERR_LOG("%s: vpu get version failure: ret=0x%X \r\n",__FUNCTION__,ret);
		VPU_DecUnLoad();
		VPU_COMP_SEM_UNLOCK(psemaphore);
		return OMX_ErrorHardware;
	}
	VPU_COMP_SEM_UNLOCK(psemaphore);
	VPU_COMP_LOG("vpu lib version : rel.major.minor=%d.%d.%d \r\n",sVpuVer.nLibRelease,sVpuVer.nLibMajor,sVpuVer.nLibMinor);
	VPU_COMP_LOG("vpu fw version : rel.major.minor=%d.%d.%d \r\n",sVpuVer.nFwRelease,sVpuVer.nFwMajor,sVpuVer.nFwMinor);

	//move memory and open related operations into FilterOneBuffer() to allow user to set some parameters before allocate mem or open vpu.

	//update state
	eVpuDecoderState=VPU_COM_STATE_LOADED;

	return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::DeInitFilterComponent()
{
	VpuDecRetCode ret;
	OMX_ERRORTYPE omx_ret=OMX_ErrorNone;
	VPU_COMP_API_LOG("%s: \r\n",__FUNCTION__);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_NONE:
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure: error state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorIncorrectStateTransition;
	default:
		break;
	}

	//1 TODO: for error case, we need to check state.
	//such as, vpu may be not opened at all, so we should not call vpu close.

	//close vpu
	if(NULL==nHandle)
	{
		//user may called DeInitFilterComponent directly after InitFilterComponent()
	}
	else
	{
		VPU_COMP_SEM_LOCK(psemaphore);
		ret=VPU_DecClose(nHandle);
		VPU_COMP_SEM_UNLOCK(psemaphore);
		if (ret!=VPU_DEC_RET_SUCCESS)
		{
			VPU_COMP_ERR_LOG("%s: vpu close failure: ret=0x%X \r\n",__FUNCTION__,ret);
			omx_ret=OMX_ErrorHardware;
		}
	}

	//release mem
	if(0==MemFreeBlock(&sVpuMemInfo,&sMemOperator))
	{
		VPU_COMP_ERR_LOG("%s: free memory failure !  \r\n",__FUNCTION__);
		omx_ret=OMX_ErrorHardware;
	}

	//avoid memory leak !!!
	if(0==MemFreeBlock(&sAllocMemInfo,&sMemOperator))
	{
		VPU_COMP_ERR_LOG("%s: free memory failure !!!  \r\n",__FUNCTION__);
		omx_ret=OMX_ErrorHardware;
	}

	//it is released by user(In fact, it may be overlay with 'sAllocMemInfo')
	//if(0==MemFreeBlock(&sFrameMemInfo))
	//{
	//	VPU_COMP_LOG("%s: free memory failure:  \r\n",__FUNCTION__);
	//	omx_ret=OMX_ErrorHardware;
	//}

	//clear handle
	nHandle=0;

	//unload
	VPU_COMP_SEM_LOCK(psemaphore);
	ret=VPU_DecUnLoad();
	VPU_COMP_SEM_UNLOCK(psemaphore);
	if (ret!=VPU_DEC_RET_SUCCESS)
	{
		VPU_COMP_ERR_LOG("%s: vpu unload failure: ret=0x%X \r\n",__FUNCTION__,ret);
#ifdef USE_PROCESS_SEM
		VPU_COMP_SEM_DESTROY_PROCESS(psemaphore);
#else
		VPU_COMP_SEM_DESTROY(psemaphore);
#endif
		omx_ret=OMX_ErrorHardware;
	}

#ifdef USE_PROCESS_SEM
	VPU_COMP_SEM_DESTROY_PROCESS(psemaphore);
#else
	VPU_COMP_SEM_DESTROY(psemaphore);
#endif
	//update state
	eVpuDecoderState=VPU_COM_STATE_NONE;

	return omx_ret;
}

OMX_ERRORTYPE VpuDecoder::SetInputBuffer(OMX_PTR pBuffer, OMX_S32 nSize, OMX_BOOL bLast)
{
	pInBuffer=pBuffer;
	nInSize=nSize;
	bInEos=bLast;

	VPU_COMP_API_LOG("%s: state: %d, size: %d, last: %d \r\n",__FUNCTION__,eVpuDecoderState,(UINT_32)nInSize,(UINT_32)bInEos);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_NONE:
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure: error state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorIncorrectStateTransition;
	case VPU_COM_STATE_EOS:
		//if user want to repeat play, user should call the last getoutput (to make state change from eos to decode)
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure: error state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorIncorrectStateTransition;
	default:
		break;
	}

	//check data length, we don't allow zero-length-buf
	if(0>=nInSize)
	{
		pInBuffer=NULL;
	}

	if(1==nNeedFlush)
	{
		VPU_COMP_LOG("flush internal !!!! \r\n");
		if(OMX_ErrorNone!=FlushFilter())
		{
			VPU_COMP_ERR_LOG("internal flush filter failure \r\n");
		}
		//nNeedFlush=0;
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::SetOutputBuffer(OMX_PTR pBuffer)
{
	VpuDecRetCode ret;
	VpuDecOutFrameInfo * pFrameInfo;
	OMX_S32 nPhyFrameNum;
	OMX_S32 nIsPhy=0;
	OMX_S32 index;

	VPU_COMP_API_LOG("%s: state: %d, buffer: 0x%X \r\n",__FUNCTION__,eVpuDecoderState,(UINT_32)pBuffer);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_NONE:
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure: error state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorIncorrectStateTransition;
	case VPU_COM_STATE_EOS:
		//if user want to repeat play, user should call the last getoutput (to make state change from eos to decode)
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure: error state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorIncorrectStateTransition;
	default:
		break;
	}

	//pOutBuffer=pBuffer;

	if(0==FrameBufExist(pBuffer,&sFrameMemInfo,&nIsPhy))
	{
		//register frame buffer
		nPhyFrameNum=FrameBufRegister(pBuffer,&sFrameMemInfo,&nIsPhy);
		if(-1==nPhyFrameNum)
		{
			VPU_COMP_ERR_LOG("%s: failure: unvalid buffer ! \r\n",__FUNCTION__);
			//sFrameMemInfo is full
			return OMX_ErrorInsufficientResources;
		}

		//update frame pool's state
		//if(nPhyFrameNum>=nOutBufferCnt)
		//{
		//	eVpuDecoderState=VPU_COM_STATE_REG_FRM;
		//}
	}
	else
	{
		if(nIsPhy)
		{
			//find map info
			index=OutInfoMap(pBuffer, &pFrameInfo, &sOutMapInfo);
			if(-1!=index)
			{
				if(NULL!=pFrameInfo->pDisplayFrameBuf)  //it will be cleared to NULL at state: VPU_COM_STATE_RE_WAIT_FRM
				{
					//clear displayed frame
					//TODO: check frame to avoid clear repeatedly ?? It is important for (1) nFreeOutBufCnt and (2) state of VPU_COM_STATE_RE_WAIT_FRM
					VPU_COMP_SEM_LOCK(psemaphore);
					ret=VPU_DecOutFrameDisplayed(nHandle,pFrameInfo->pDisplayFrameBuf);
					VPU_COMP_SEM_UNLOCK(psemaphore);
					if(VPU_DEC_RET_SUCCESS!=ret)
					{
						VPU_COMP_ERR_LOG("%s: vpu clear frame display failure: ret=0x%X \r\n",__FUNCTION__,ret);
						return OMX_ErrorHardware;
					}
				}
				//clear info
				OutInfoUnregisterVirtAddr(index, &sOutMapInfo);

				//update buffer state
				nFreeOutBufCnt++;
				VPU_COMP_LOG("set output: 0x%X , nFreeOutBufCnt: %d \r\n",(UINT32)pBuffer,(UINT32)nFreeOutBufCnt);
			}
			else
			{
				//user have set output repeatly !!!
				VPU_COMP_ERR_LOG("%s: failure: repeat setting output buffer: 0x%X \r\n",__FUNCTION__,(UINT32)pBuffer);
				return OMX_ErrorIncorrectStateOperation;
			}
		}
		else
		{
			//clear frame node ...
			//...
		}
	}

	if(0==nIsPhy)
	{
		VPU_COMP_ERR_LOG("%s: failure: virtual address is not supported ! \r\n",__FUNCTION__);
		//TODO: Now, virtual address is not supported !!!!!, it will introduce more complex logic.
		return OMX_ErrorUnsupportedSetting;
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::InitFilter()
{
	VpuDecRetCode ret;
	VpuFrameBuffer frameBuf[MAX_FRAME_NUM];
	OMX_S32 BufNum;

	VPU_COMP_API_LOG("%s: \r\n",__FUNCTION__);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_DO_INIT:
		break;
	default:
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure: error state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorIncorrectStateTransition;
	}

	//fill frameBuf[]
	ASSERT(nOutBufferCnt<=MAX_FRAME_NUM);
	BufNum=FrameBufCreateRegisterFrame(frameBuf, &sFrameMemInfo,nOutBufferCnt,nPadWidth, nPadHeight, &sVpuMemInfo,&sMemOperator,sOutFmt.eColorFormat,nChromaAddrAlign);
	if(-1==BufNum)
	{
		VPU_COMP_ERR_LOG("%s: create register frame failure \r\n",__FUNCTION__);
		return OMX_ErrorInsufficientResources;
	}

	//register frame buffs
	VPU_COMP_SEM_LOCK(psemaphore);
	ret=VPU_DecRegisterFrameBuffer(nHandle, frameBuf, BufNum);
	VPU_COMP_SEM_UNLOCK(psemaphore);
	if(VPU_DEC_RET_SUCCESS!=ret)
	{
		VPU_COMP_ERR_LOG("%s: vpu register frame failure: ret=0x%X \r\n",__FUNCTION__,ret);
		return OMX_ErrorHardware;
	}

	//update state
	eVpuDecoderState=VPU_COM_STATE_DO_DEC;

	//update buffer state
	nFreeOutBufCnt=nOutBufferCnt;

	return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::DeInitFilter()
{
	OMX_ERRORTYPE omx_ret=OMX_ErrorNone;
	VPU_COMP_API_LOG("%s: \r\n",__FUNCTION__);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_NONE:
	case VPU_COM_STATE_LOADED:
	case VPU_COM_STATE_OPENED:
	case VPU_COM_STATE_WAIT_FRM:
	case VPU_COM_STATE_DO_INIT:
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure: error state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorIncorrectStateTransition;
	default:
		break;
	}

	//release vpu's registered frame ??

	//update state
	//eVpuDecoderState=VPU_COM_STATE_OPENED; //???

	return omx_ret;
}

FilterBufRetCode VpuDecoder::FilterOneBuffer()
{
	VpuDecRetCode ret;
	VpuBufferNode InData;
	OMX_S32 bufRetCode;
	FilterBufRetCode bufRet=FILTER_OK;

	OMX_U8* pBitstream;
	OMX_S32 readbytes;
	OMX_U8  dummy;
	OMX_S32 enableFileMode=0;
	OMX_S32 capability=0;
#ifdef VPU_COMP_DEBUG
	static OMX_U32 nNotUsedCnt=0;
	static OMX_U32 nOutFrameCnt=0;
	static FILE* fpBitstream=NULL;
#endif

	VPU_COMP_API_LOG("%s: state: %d, InBuf: 0x%X, data size: %d, bInEos: %d \r\n",__FUNCTION__,eVpuDecoderState,(UINT_32)pInBuffer,(UINT_32)nInSize,bInEos);

	if(1==nNeedSkip)
	{
		VPU_COMP_LOG("skip directly: we need to get one time stamp \r\n");
		nNeedSkip=0;
		return FILTER_SKIP_OUTPUT;
	}

RepeatPlay:
	//check state
	switch(eVpuDecoderState)
	{
		//forbidden state
	case VPU_COM_STATE_NONE:
	case VPU_COM_STATE_DO_INIT:
	case VPU_COM_STATE_DO_OUT:
	case VPU_COM_STATE_EOS:
		bufRet=FILTER_ERROR;
		VPU_COMP_ERR_LOG("%s: failure: error state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return bufRet;
		//allowed state
	case VPU_COM_STATE_WAIT_FRM:
		if(FrameBufPhyNum(&sFrameMemInfo)>=(OMX_S32)nOutBufferCnt)
		{
			//ready to call InitFilter()
			eVpuDecoderState=VPU_COM_STATE_DO_INIT;
			bufRet =FILTER_DO_INIT;
		}
		else
		{
			//do nothing, wait for more output buffer
			bufRet=FILTER_NO_OUTPUT_BUFFER;
		}
		VPU_COMP_LOG("%s: waiting frames ready, return and do nothing \r\n",__FUNCTION__);
		return bufRet;//OMX_ErrorNone;
	case VPU_COM_STATE_LOADED:
		//query memory
		VPU_COMP_SEM_LOCK(psemaphore);
		ret=VPU_DecQueryMem(&sMemInfo);
		VPU_COMP_SEM_UNLOCK(psemaphore);
		if (ret!=VPU_DEC_RET_SUCCESS)
		{
			VPU_COMP_ERR_LOG("%s: vpu query memory failure: ret=0x%X \r\n",__FUNCTION__,ret);
			return FILTER_ERROR;
		}
		//malloc memory for vpu wrapper
		if(0==MemMallocVpuBlock(&sMemInfo,&sVpuMemInfo,&sMemOperator))
		{
			VPU_COMP_ERR_LOG("%s: malloc memory failure: \r\n",__FUNCTION__);
			return FILTER_ERROR;
		}
		//open vpu
		enableFileMode=(ePlayMode==DEC_FILE_MODE)?1:0;
		if(OMX_ErrorNone!=OpenVpu(&nHandle, &sMemInfo, eFormat, sInFmt.nFrameWidth, sInFmt.nFrameHeight,sOutFmt.eColorFormat,enableFileMode,psemaphore)) //1 sInFmt is valid ???
		{
			VPU_COMP_ERR_LOG("%s: open vpu failure \r\n",__FUNCTION__);
			return FILTER_ERROR;
		}
		//check capability
		VPU_COMP_SEM_LOCK(psemaphore);
		ret=VPU_DecGetCapability(nHandle, VPU_DEC_CAP_FRAMESIZE, (INT32*)&capability);
		if((ret==VPU_DEC_RET_SUCCESS)&&capability)
		{
			nCapability|=VPU_COM_CAPABILITY_FRMSIZE;
			bFilterSupportFrmSizeRpt=OMX_TRUE;
		}
		VPU_COMP_SEM_UNLOCK(psemaphore);
		//update state
		eVpuDecoderState=VPU_COM_STATE_OPENED;
	case VPU_COM_STATE_OPENED:
		break;
	case VPU_COM_STATE_DO_DEC:
		//check buffer state
		//if(nFreeOutBufCnt<=0)	//1 not enough, may hang up ???
		if((OMX_S32)nFreeOutBufCnt<=(OMX_S32)FRAME_MIN_FREE_THD)
		{
			//notify user release outputed buffer
			VPU_COMP_LOG("%s: no output buffer, do nothing, current free buf numbers: %d \r\n",__FUNCTION__,(INT32)nFreeOutBufCnt);
			return FILTER_NO_OUTPUT_BUFFER;
		}
		break;
	case VPU_COM_STATE_RE_WAIT_FRM:
		//FIXME: for iMX6(now its major version==2), we needn't to return all frame buffers.
		if(sVpuVer.nFwMajor!=2)
		{
			//need to user SetOutputBuffer() for left buffers, otherwise vpu may return one buffer which is not set by user
			if((OMX_U32)nFreeOutBufCnt<nOutBufferCnt)
			{
				VPU_COMP_LOG("%s: no output buffer, do nothing, current free buf numbers: %d \r\n",__FUNCTION__,(INT32)nFreeOutBufCnt);
				return FILTER_NO_OUTPUT_BUFFER;
			}
		}
		else
		{
			VPU_COMP_LOG("for iMX6: needn't return all frame buffers \r\n");
		}
		eVpuDecoderState=VPU_COM_STATE_DO_DEC;
		goto RepeatPlay;
		break;
		//unknow state
	default:
		VPU_COMP_ERR_LOG("%s: failure state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return FILTER_ERROR;
	}

	//for all codecs
	pBitstream=(pInBuffer==(OMX_PTR)INVALID)?NULL:(OMX_U8*)pInBuffer;
	readbytes=nInSize;

	//check eos or null data
	if(pBitstream==NULL)
	{
		if(bInEos==OMX_TRUE)
		{
			//create and send EOS data (with length=0)
			pBitstream=&dummy;
			readbytes=0;
			//bInEos=OMX_FALSE;
		}
		else
		{
			//no new input setting , return directly
			//VPU_COMP_LOG("%s: pInBuffer==NULL, set consumed and return directly(do nothing)  \r\n",__FUNCTION__);
			//bufRet=(FilterBufRetCode)(bufRet|FILTER_INPUT_CONSUMED);
			//return FILTER_OK;//OMX_ErrorNone;
		}
	}

	//EOS: 		0==readbytesp && Bitstream!=NULL
	//non-EOS: 	0==readbytesp && Bitstream==NULL

	//reset bOutLast for any non-zero data ??
	//bOutLast=OMX_FALSE;

	//seq init
	//decode bitstream buf
	VPU_COMP_LOG("%s: pBitstream=0x%X, readbytes=%d  \r\n",__FUNCTION__,(UINT32)pBitstream,(INT32)readbytes);
	InData.nSize=readbytes;
	InData.pPhyAddr=NULL;
	InData.pVirAddr=pBitstream;
	InData.sCodecData.pData=(OMX_U8*)pCodecData;
	InData.sCodecData.nSize=nCodecDataLen;

#ifdef VPU_DEC_COMP_DROP_B
	{
		OMX_TICKS nTimeStamp;
		nTimeStamp=QueryStreamTs();
		if(nTimeStamp >= 0)
		{
			if(OMX_ErrorNone!=ConfigVpu(nHandle,nTimeStamp,pClock,psemaphore))
			{
				return FILTER_ERROR;
			}
		}
	}
#endif

	VPU_COMP_SEM_LOCK(psemaphore);
	ret=VPU_DecDecodeBuf(nHandle, &InData,(INT32*)&bufRetCode);
	//VPU_COMP_SEM_UNLOCK(&sSemaphore);
	if(VPU_DEC_RET_SUCCESS!=ret)
	{
		VPU_COMP_ERR_LOG("%s: vpu dec buf failure: ret=0x%X \r\n",__FUNCTION__,ret);
		if(VPU_DEC_RET_FAILURE_TIMEOUT==ret)
		{
			//VPU_COMP_SEM_LOCK(psemaphore);
			VPU_DecReset(nHandle);
			SendEvent(OMX_EventError, OMX_ErrorStreamCorrupt, 0, NULL);
		}
		VPU_COMP_SEM_UNLOCK(psemaphore);
		return FILTER_ERROR; //OMX_ErrorHardware;
	}
	VPU_COMP_SEM_UNLOCK(psemaphore);
	VPU_COMP_LOG("%s: bufRetCode: 0x%X  \r\n",__FUNCTION__,bufRetCode);
	//check input buff
	if(bufRetCode&VPU_DEC_INPUT_USED)
	{
#ifdef VPU_COMP_DEBUG
		FileDumpBitstrem(&fpBitstream,pBitstream,readbytes);
#endif

		if(pInBuffer!=(OMX_PTR)INVALID)
		{
			bufRet=(FilterBufRetCode)(bufRet|FILTER_INPUT_CONSUMED);
			pInBuffer=(OMX_PTR)INVALID;  //clear input
			nInSize=0;
		}
	}
	else
	{
		//not used
		VPU_COMP_LOG("%s: not used  \r\n",__FUNCTION__);
#ifdef VPU_COMP_DEBUG
		nNotUsedCnt++;
		if(nNotUsedCnt>MAX_NULL_LOOP)
		{
			VPU_COMP_ERR_LOG("%s: too many(%d times) null loop: return failure \r\n",__FUNCTION__, (INT32)nNotUsedCnt);
			return FILTER_ERROR;
		}
#endif
	}

	//check init info
	if(bufRetCode&VPU_DEC_INIT_OK)
	{
		FilterBufRetCode ret;
		ret=ProcessVpuInitInfo();
		bufRet=(FilterBufRetCode)((OMX_U32)bufRet|(OMX_U32)ret);
		return bufRet;
	}

	//check decoded info
	if(nCapability&VPU_COM_CAPABILITY_FRMSIZE)
	{
		if(bufRetCode&VPU_DEC_ONE_FRM_CONSUMED)
		{
			VPU_COMP_LOG("%s: one frame is decoded \r\n");
			bufRet=(FilterBufRetCode)(bufRet|FILTER_ONE_FRM_DECODED);
		}
	}

	//check output buff
	//VPU_COMP_LOG("%s: bufRetCode=0x%X \r\n",__FUNCTION__,bufRetCode);
	if(bufRetCode&VPU_DEC_OUTPUT_DIS)
	{
		bufRet=(FilterBufRetCode)(bufRet|FILTER_HAS_OUTPUT);
		eVpuDecoderState=VPU_COM_STATE_DO_OUT;
#ifdef VPU_COMP_DEBUG
		nOutFrameCnt++;
		if(nOutFrameCnt>MAX_DEC_FRAME)
		{
			VPU_COMP_ERR_LOG("already output %d frames, return failure \r\n",(INT32)nOutFrameCnt);
			return FILTER_ERROR;
		}
#endif
		nOutBufferSize=sOutFmt.nFrameWidth * sOutFmt.nFrameHeight*pxlfmt2bpp(sOutFmt.eColorFormat)/8;
	}
	else if (bufRetCode&VPU_DEC_OUTPUT_MOSAIC_DIS)
	{
		bufRet=(FilterBufRetCode)(bufRet|FILTER_HAS_OUTPUT);
		eVpuDecoderState=VPU_COM_STATE_DO_OUT;
		//send out one frame with length=0
		nOutBufferSize=0;
	}
	else if (bufRetCode&VPU_DEC_OUTPUT_EOS)
	{
		bufRet=(FilterBufRetCode)(bufRet|FILTER_LAST_OUTPUT);
		//bOutLast=OMX_TRUE;
		eVpuDecoderState=VPU_COM_STATE_EOS;
	}
	//else if (bufRetCode&VPU_DEC_OUTPUT_NODIS)
	//{
	//	bufRet=(FilterBufRetCode)(bufRet|FILTER_SKIP_OUTPUT);
	//}
	else if (bufRetCode&VPU_DEC_OUTPUT_REPEAT)
	{
		bufRet=(FilterBufRetCode)(bufRet|FILTER_SKIP_OUTPUT);
	}
	else if (bufRetCode&VPU_DEC_OUTPUT_DROPPED)
	{
		bufRet=(FilterBufRetCode)(bufRet|FILTER_SKIP_OUTPUT);
	}
	else
	{
		//bufRet=(FilterBufRetCode)(bufRet|FILTER_NO_OUTPUT);
	}

	//check "no enough buf"
	if(bufRetCode&VPU_DEC_NO_ENOUGH_BUF)
	{
		//FIXME: if have one output , we ignore this "not enough buf" flag, since video filter may only check one bit among them
		//only consider VPU_DEC_OUTPUT_MOSAIC_DIS/VPU_DEC_OUTPUT_DIS/VPU_DEC_OUTPUT_NODIS
		if(bufRetCode&VPU_DEC_OUTPUT_NODIS)
		{
			bufRet=(FilterBufRetCode)(bufRet|FILTER_NO_OUTPUT_BUFFER);
		}
		else if(bufRetCode&VPU_DEC_OUTPUT_DIS)
		{
			bufRet=(FilterBufRetCode)(bufRet|FILTER_HAS_OUTPUT);
			eVpuDecoderState=VPU_COM_STATE_DO_OUT;
			nOutBufferSize=sOutFmt.nFrameWidth * sOutFmt.nFrameHeight*pxlfmt2bpp(sOutFmt.eColorFormat)/8;
		}
		else if(bufRetCode&VPU_DEC_OUTPUT_MOSAIC_DIS)
		{
			bufRet=(FilterBufRetCode)(bufRet|FILTER_HAS_OUTPUT);
			eVpuDecoderState=VPU_COM_STATE_DO_OUT;
			nOutBufferSize=0;
		}
	}

	if(bufRetCode&VPU_DEC_SKIP)
	{
		//notify user to get one time stamp.
		ASSERT(bufRet&FILTER_HAS_OUTPUT);	//only for this case now !!
		//bufRet=(FilterBufRetCode)(bufRet|FILTER_SKIP_TS);
		nNeedSkip=1;
	}

	if(bufRetCode&VPU_DEC_NO_ENOUGH_INBUF)
	{
		//in videofilter: these flags are exclusively, so we must be careful !!!
		if(bufRet==(FILTER_OK|FILTER_INPUT_CONSUMED))
		{
			bufRet=(FilterBufRetCode)(bufRet|FILTER_NO_INPUT_BUFFER);
		}
		else if((bufRet==FILTER_OK)&&(pInBuffer==(OMX_PTR)INVALID))  //check this since we added one state for pInBuffer=INVALID
			//else if((bufRet==FILTER_OK)&&(bufRetCode&VPU_DEC_INPUT_USED))  //check this since we added one state for pInBuffer=INVALID
		{
			bufRet=(FilterBufRetCode)(bufRet|FILTER_NO_INPUT_BUFFER);
		}
	}

	if(bufRetCode&VPU_DEC_FLUSH)
	{
		//flush operation is recommended by vpu wrapper
		//we call flush filter at setinput step to avoid missing getoutput
		nNeedFlush=1;
	}

	VPU_COMP_LOG("%s: return OMX_ErrorNone \r\n",__FUNCTION__);
	return bufRet;//OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder:: GetDecBuffer(OMX_PTR *ppBuffer,OMX_S32 * pOutStuffSize,OMX_S32* pOutFrmSize)
{
	VpuDecFrameLengthInfo sLengthInfo;
	VpuDecRetCode ret;
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_DO_DEC:
	case VPU_COM_STATE_DO_OUT:
		break;
	default:
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		//return OMX_ErrorIncorrectStateTransition;
	}
	VPU_COMP_SEM_LOCK(psemaphore);
	ret=VPU_DecGetConsumedFrameInfo(nHandle, &sLengthInfo);
	VPU_COMP_SEM_UNLOCK(psemaphore);
	if(VPU_DEC_RET_SUCCESS!=ret)
	{
		VPU_COMP_ERR_LOG("%s: vpu get decoded frame info failure: ret=0x%X \r\n",__FUNCTION__,ret);
		return OMX_ErrorHardware;
	}
	*pOutStuffSize=sLengthInfo.nStuffLength;
	*pOutFrmSize=sLengthInfo.nFrameLength;
	if(sLengthInfo.pFrame==NULL)
	{
		*ppBuffer=NULL;	//the frame is skipped
	}
	else
	{
		*ppBuffer=(OMX_PTR)sLengthInfo.pFrame->pbufVirtY;
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder:: GetOutputBuffer(OMX_PTR *ppOutVirtBuf,OMX_S32* pOutSize)
{
	VpuDecRetCode ret;
	VpuDecOutFrameInfo* pFrameInfo;
	OMX_S32 index;
	OMX_BOOL bOutLast=OMX_FALSE;

#ifdef VPU_COMP_DEBUG
	static FILE* fpYUV=NULL;
#endif

	VPU_COMP_API_LOG("%s: state: %d \r\n",__FUNCTION__,eVpuDecoderState);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_DO_OUT:
		//update state
		eVpuDecoderState=VPU_COM_STATE_DO_DEC;
		break;
	case VPU_COM_STATE_EOS:
		bOutLast=OMX_TRUE;
		//update to decode state for repeat play ??
		//eVpuDecoderState=VPU_COM_STATE_DO_DEC;
		eVpuDecoderState=VPU_COM_STATE_RE_WAIT_FRM;
		VPU_COMP_LOG("%s: nFreeOutBufCnt: %d, outputed num: %d \r\n",__FUNCTION__,(INT32)nFreeOutBufCnt, (INT32)OutInfoOutputNum(&sOutMapInfo));
		//we should not re set out map info
		//OutInfoReSet(&sFrameMemInfo,&sOutMapInfo,nOutBufferCnt);
		//nFreeOutBufCnt=0;
		break;
	default:
		//forbidden
		VPU_COMP_ERR_LOG("%s: failure state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorIncorrectStateTransition;
	}

	if(ppOutVirtBuf==NULL)
	{
		VPU_COMP_ERR_LOG("%s: failure: ppOutVirtBuf==NULL !!! \r\n",__FUNCTION__);
		return OMX_ErrorBadParameter;
	}

	//get output frame
	if(OMX_TRUE==bOutLast)
	{
#if 0
		VPU_COMP_LOG("%s:  reach EOS, repeat the last output frame: 0x%X  \r\n",__FUNCTION__,(UINT32)pLastOutVirtBuf);
		//return last output frame for the EOS case
		*ppOutVirtBuf=pLastOutVirtBuf;
#else
		//return NULL for EOS case, in this case, we need not use variable pLastOutVirtBuf again.
		//*ppOutVirtBuf=NULL;



		index=OutInfoQueryEmptyNode(&pFrameInfo, &sOutMapInfo);
		if(-1==index)
		{
			//sOutMapInfo is full
			VPU_COMP_ERR_LOG("%s: query empty node failure ! \r\n",__FUNCTION__);
			return OMX_ErrorInsufficientResources;
		}

		if(-1==FrameBufFindOneUnOutputed(ppOutVirtBuf,&sFrameMemInfo,&sOutMapInfo))
		{
			//not find one unoutputed buffer
			VPU_COMP_ERR_LOG("%s: find unoutputed frame failure \r\n",__FUNCTION__);
			return OMX_ErrorInsufficientResources;
		}
		//set pDisplayFrameBuf to NULL to avoid clear operation in SetOutputBuffer()
		pFrameInfo->pDisplayFrameBuf=NULL;
		//record out frame info
		OutInfoRegisterVirtAddr(index, *ppOutVirtBuf, &sOutMapInfo);
		//update buffer state
		nFreeOutBufCnt--;
#endif
		VPU_COMP_LOG("%s:  reach EOS , set OK  \r\n",__FUNCTION__);
		//In theory, for eos, user should call getouput only once !!!
		//We can set pLastOutVirtBuf=NULL to avoid user call getoutput repeatedly(dead loop) ???
		//pLastOutVirtBuf=NULL;
		*pOutSize=0;
	}
	else
	{
		index=OutInfoQueryEmptyNode(&pFrameInfo, &sOutMapInfo);
		if(-1==index)
		{
			//sOutMapInfo is full
			VPU_COMP_ERR_LOG("%s: query empty node failure !!! \r\n",__FUNCTION__);
			return OMX_ErrorInsufficientResources;
		}

		VPU_COMP_SEM_LOCK(psemaphore);
		ret=VPU_DecGetOutputFrame(nHandle, pFrameInfo);
		VPU_COMP_SEM_UNLOCK(psemaphore);
		if(VPU_DEC_RET_SUCCESS!=ret)
		{
			VPU_COMP_ERR_LOG("%s: vpu get output frame failure: ret=0x%X \r\n",__FUNCTION__,ret);
			return OMX_ErrorHardware;
		}
		*ppOutVirtBuf=(OMX_PTR)pFrameInfo->pDisplayFrameBuf->pbufVirtY;
		//*pSize=nPadWidth*nPadHeight*3/2;

#ifdef VPU_COMP_DEBUG
		FileDumpYUV(&fpYUV,pFrameInfo->pDisplayFrameBuf->pbufVirtY,
		            pFrameInfo->pDisplayFrameBuf->pbufVirtCb,
		            pFrameInfo->pDisplayFrameBuf->pbufVirtCr,
		            nPadWidth*nPadHeight,nPadWidth*nPadHeight/4,sOutFmt.eColorFormat);
#endif

		//record out frame info
		OutInfoRegisterVirtAddr(index, pFrameInfo->pDisplayFrameBuf->pbufVirtY, &sOutMapInfo);

		//update the last output frame
		pLastOutVirtBuf=(OMX_PTR)pFrameInfo->pDisplayFrameBuf->pbufVirtY;

		//update buffer state
		nFreeOutBufCnt--;
		VPU_COMP_LOG("%s: return output: 0x%X, nFreeOutBufCnt: %d \r\n",__FUNCTION__,(UINT32)(*ppOutVirtBuf),(UINT32)nFreeOutBufCnt);
		*pOutSize=nOutBufferSize;
	}

	return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::FlushFilter()
{
	VpuDecRetCode ret;
	VPU_COMP_LOG("%s: \r\n",__FUNCTION__);

	VPU_COMP_SEM_LOCK(psemaphore);
	ret=VPU_DecFlushAll(nHandle);
	//VPU_COMP_SEM_UNLOCK(psemaphore);
	if(VPU_DEC_RET_SUCCESS!=ret)
	{
		VPU_COMP_ERR_LOG("%s: vpu flush failure: ret=0x%X \r\n",__FUNCTION__,ret);
		if(VPU_DEC_RET_FAILURE_TIMEOUT==ret)
		{
			VPU_DecReset(nHandle);
			SendEvent(OMX_EventError, OMX_ErrorStreamCorrupt, 0, NULL);
		}
		VPU_COMP_SEM_UNLOCK(psemaphore);
		return OMX_ErrorHardware;
	}
	VPU_COMP_SEM_UNLOCK(psemaphore);
	//since vpu will auto clear all buffers(is equal to setoutput() operation), we need to add additional protection(set VPU_COM_STATE_WAIT_FRM).
	//otherwise, vpu may return one buffer which is still not set by user.
	eVpuDecoderState=VPU_COM_STATE_RE_WAIT_FRM;
	//nFreeOutBufCntRp=0;

	nNeedFlush=0;
	nNeedSkip=0;
	return OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::FlushInputBuffer()
{
	OMX_ERRORTYPE ret=OMX_ErrorNone;

	VPU_COMP_API_LOG("%s: state: %d \r\n",__FUNCTION__,eVpuDecoderState);

	//clear input buffer
	pInBuffer=(OMX_PTR)INVALID;
	nInSize=0;

	//need to clear bInEos
	//fixed case: SetInputBuffer(,bInEos), and FlushInputBuffer(), then FilterOneBuffer() before SetInputBuffer().
	//As result, user will get one output(EOS) without calling SetInputBuffer().
	bInEos=OMX_FALSE;

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_NONE:
	case VPU_COM_STATE_LOADED:
	case VPU_COM_STATE_OPENED:
	case VPU_COM_STATE_WAIT_FRM:	// have not registered frames, so can not call flushfilter
	case VPU_COM_STATE_DO_INIT:
	case VPU_COM_STATE_DO_OUT:
		//forbidden !!!
		VPU_COMP_ERR_LOG("%s: failure state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorNone; //for conformance test: don't return OMX_ErrorIncorrectStateTransition;
	case VPU_COM_STATE_DO_DEC:
	case VPU_COM_STATE_EOS:
	case VPU_COM_STATE_RE_WAIT_FRM:
		break;
	default:
		VPU_COMP_ERR_LOG("%s: unknown state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorNone; //for conformance test: don't return OMX_ErrorIncorrectStateTransition;
	}

	//flush vpu input/output
	ret=FlushFilter();

	//should not re-set(or clear) out map info !!!
	//OutInfoReSet(sFrameMemInfo,&sOutMapInfo,nOutBufferCnt);
	//nFreeOutBufCnt=0;

	return ret;
}

OMX_ERRORTYPE VpuDecoder::FlushOutputBuffer()
{
	OMX_ERRORTYPE ret=OMX_ErrorNone;
	VpuFrameBuffer* vpuFrameBuffer[VPU_DEC_MAX_NUM_MEM];
	OMX_S32 num;
	VPU_COMP_API_LOG("%s: state: %d  \r\n",__FUNCTION__,eVpuDecoderState);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_NONE:
	case VPU_COM_STATE_LOADED:
	case VPU_COM_STATE_OPENED:
	case VPU_COM_STATE_WAIT_FRM:
		break;
#if 1
	case VPU_COM_STATE_DO_DEC:
	case VPU_COM_STATE_EOS:
	case VPU_COM_STATE_RE_WAIT_FRM:
		//flush vpu input/output
		ret=FlushFilter();

		//re set out map info: simulate: all frames have been returned by vpu and been recorded into sOutMapInfo
		VPU_COMP_SEM_LOCK(psemaphore);
		VPU_DecAllRegFrameInfo(nHandle, vpuFrameBuffer, (INT32*)&num);
		VPU_COMP_SEM_UNLOCK(psemaphore);
		if(num<=0)
		{
			//in theory, shouldn't enter here if all states are protected correctly.
			VPU_COMP_ERR_LOG("%s: no buffers registered \r\n",__FUNCTION__);
			return OMX_ErrorNone;
		}
		OutInfoReSet(&sFrameMemInfo,&sOutMapInfo,nOutBufferCnt,vpuFrameBuffer, num);
		nFreeOutBufCnt=0;
		return ret;
#endif
	case VPU_COM_STATE_DO_INIT:
	case VPU_COM_STATE_DO_OUT:
		//forbidden !!!
		VPU_COMP_ERR_LOG("%s: failure state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorNone; //for conformance test: don't return OMX_ErrorIncorrectStateTransition;
	default:
		//forbidden !!!
		VPU_COMP_ERR_LOG("%s: unknown state transition, current state=%d \r\n",__FUNCTION__,eVpuDecoderState);
		return OMX_ErrorNone; //for conformance test: don't return OMX_ErrorIncorrectStateTransition;
	}

	//clear out frame info
	FrameBufClear(&sFrameMemInfo);

	return ret;

}

OMX_PTR VpuDecoder::AllocateOutputBuffer(OMX_U32 nSize)
{
	VpuDecRetCode ret;
	VpuMemDesc vpuMem;

	VPU_COMP_API_LOG("%s: state: %d \r\n",__FUNCTION__,eVpuDecoderState);

	//check state
	switch(eVpuDecoderState)
	{
	case VPU_COM_STATE_NONE:
		// case VPU_COM_STATE_LOADED:
		//case VPU_COM_STATE_OPENED:
		//1 how to avoid conflict memory operators
		VPU_COMP_ERR_LOG("%s: error state: %d \r\n",__FUNCTION__,eVpuDecoderState);
		return (OMX_PTR)NULL;
	default:
		break;
	}

	//malloc physical memory through vpu
	vpuMem.nSize=nSize;
	ret=VPU_DecGetMem_Wrapper(&vpuMem,&sMemOperator);
	if(VPU_DEC_RET_SUCCESS!=ret)
	{
		VPU_COMP_ERR_LOG("%s: vpu malloc frame buf failure: size=%d, ret=0x%X \r\n",__FUNCTION__,(INT32)nSize,ret);
		return (OMX_PTR)NULL;//OMX_ErrorInsufficientResources;
	}

	//record memory for release
	if(0==MemAddPhyBlock(&vpuMem, &sAllocMemInfo))
	{
		VPU_DecFreeMem_Wrapper(&vpuMem,&sMemOperator);
		VPU_COMP_ERR_LOG("%s:add phy block failure \r\n",__FUNCTION__);
		return (OMX_PTR)NULL;
	}

	//register memory info into resource manager
	if(OMX_ErrorNone!=AddHwBuffer((OMX_PTR)vpuMem.nPhyAddr, (OMX_PTR)vpuMem.nVirtAddr))
	{
		MemRemovePhyBlock(&vpuMem, &sAllocMemInfo);
		VPU_DecFreeMem_Wrapper(&vpuMem,&sMemOperator);
		VPU_COMP_ERR_LOG("%s:add hw buffer failure \r\n",__FUNCTION__);
		return (OMX_PTR)NULL;
	}

	//return virtual address
	return (OMX_PTR)vpuMem.nVirtAddr;//OMX_ErrorNone;
}

OMX_ERRORTYPE VpuDecoder::FreeOutputBuffer(OMX_PTR pBuffer)
{
	VpuDecRetCode ret;
	VpuMemDesc vpuMem;

	VPU_COMP_API_LOG("%s: \r\n",__FUNCTION__);

	//query related mem info for release
	if(0==MemQueryPhyBlock(pBuffer,&vpuMem,&sAllocMemInfo))
	{
		VPU_COMP_ERR_LOG("%s: query phy block failure \r\n",__FUNCTION__);
		return OMX_ErrorResourcesLost;
	}

	//release physical memory through vpu
	ret=VPU_DecFreeMem_Wrapper(&vpuMem,&sMemOperator);
	if(ret!=VPU_DEC_RET_SUCCESS)
	{
		VPU_COMP_ERR_LOG("%s: free vpu memory failure : ret=0x%X \r\n",__FUNCTION__,ret);
		return OMX_ErrorHardware;
	}

	//remove mem info
	if(0==MemRemovePhyBlock(&vpuMem, &sAllocMemInfo))
	{
		VPU_COMP_ERR_LOG("%s: remove phy block failure \r\n",__FUNCTION__);
		return OMX_ErrorResourcesLost;
	}

	//unregister memory info from resource manager
	if(OMX_ErrorNone!=RemoveHwBuffer(pBuffer))
	{
		VPU_COMP_ERR_LOG("%s: remove hw buffer failure \r\n",__FUNCTION__);
		return OMX_ErrorResourcesLost;
	}

	return OMX_ErrorNone;
}

FilterBufRetCode VpuDecoder::ProcessVpuInitInfo()
{
	FilterBufRetCode bufRet=FILTER_OK;
	VpuDecRetCode ret;
	OMX_S32 nChanged=0;
	//process init info
	VPU_COMP_SEM_LOCK(psemaphore);
	ret=VPU_DecGetInitialInfo(nHandle, &sInitInfo);
	VPU_COMP_SEM_UNLOCK(psemaphore);
	if(VPU_DEC_RET_SUCCESS!=ret)
	{
		VPU_COMP_ERR_LOG("%s: vpu get init info failure: ret=0x%X \r\n",__FUNCTION__,ret);
		return FILTER_ERROR;//OMX_ErrorHardware;
	}

	//set resolution info
	sInFmt.nFrameWidth=sInitInfo.nPicWidth;
	sInFmt.nFrameHeight=sInitInfo.nPicHeight;
	nPadWidth = Align(sInFmt.nFrameWidth,FRAME_ALIGN);//(sInFmt.nFrameWidth +15)&(~15);
	if(sInitInfo.nInterlace)
	{
		nPadHeight = Align(sInFmt.nFrameHeight ,2*FRAME_ALIGN);//(sInFmt.nFrameHeight +31)&(~31);
	}
	else
	{
		nPadHeight = Align(sInFmt.nFrameHeight ,FRAME_ALIGN);//(sInFmt.nFrameHeight +15)&(~15);
	}

	//check change for nFrameWidth/nFrameHeight
	if(((OMX_S32)sOutFmt.nFrameWidth !=nPadWidth)||((OMX_S32)sOutFmt.nFrameHeight != nPadHeight))
	{
		sOutFmt.nFrameWidth = nPadWidth;
		sOutFmt.nFrameHeight = nPadHeight;
		sOutFmt.nStride=nPadWidth;
		nChanged=1;
	}

	//check color format, only for mjpg : 4:2:0/4:2:2(ver/hor)/4:4:4/4:0:0
	if(VPU_V_MJPG==eFormat)
	{
		OMX_COLOR_FORMATTYPE colorFormat;
		colorFormat=ConvertMjpgColorFormat(sInitInfo.nMjpgSourceFormat,sOutFmt.eColorFormat);
		if(colorFormat!=sOutFmt.eColorFormat)
		{
			sOutFmt.eColorFormat=colorFormat;
			nChanged=1;
		}
	}

	//TODO: check change for sOutCrop ?
	//...

	//set crop info
	VPU_COMP_LOG("%s: original init info: [top,left,bottom,right]=[%d,%d,%d,%d], \r\n",__FUNCTION__,
	             (INT32)sInitInfo.PicCropRect.nTop,(INT32)sInitInfo.PicCropRect.nLeft,(INT32)sInitInfo.PicCropRect.nBottom,(INT32)sInitInfo.PicCropRect.nRight);

	sOutCrop.nLeft = sInitInfo.PicCropRect.nLeft;
	sOutCrop.nTop = sInitInfo.PicCropRect.nTop;
	//here, we am not responsible to 8 pixels limitation at display end. !!!
	sOutCrop.nWidth = sInitInfo.PicCropRect.nRight-sInitInfo.PicCropRect.nLeft;// & (~7);
	sOutCrop.nHeight= sInitInfo.PicCropRect.nBottom-sInitInfo.PicCropRect.nTop;

#if 1	//user need to know the non-pad width/height in portchange process ???
	//it may have potential risk if crop info is not equal to non-pad info !!!
	sInFmt.nFrameWidth=sOutCrop.nWidth;
	sInFmt.nFrameHeight=sOutCrop.nHeight;
	if(((nPadWidth-sOutCrop.nWidth)>=16)||((sInitInfo.nInterlace)&&(nPadHeight-sOutCrop.nHeight)>=32)||((0==sInitInfo.nInterlace)&&(nPadHeight-sOutCrop.nHeight)>=16))
	{
		VPU_COMP_LOG("potential risk for sInFmt [widthxheight]: [%dx%d] \r\n",(INT32)sOutCrop.nWidth,(INT32)sOutCrop.nHeight);
	}
#endif

	//check change for nOutBufferCnt
	if((sInitInfo.nMinFrameBufferCount+FRAME_SURPLUS)!=(OMX_S32)nOutBufferCnt)
	{
		nOutBufferCnt=sInitInfo.nMinFrameBufferCount+FRAME_SURPLUS;
		nChanged=1;
	}
#if 1	//workaround for ENGR00171878: we should make sure porting change event always be triggered
	else
	{
		VPU_COMP_LOG("plus one buffer cnt manually: original cnt: %d, new cnt: %d \r\n",nOutBufferCnt,nOutBufferCnt+1);
		nOutBufferCnt++;
		sInitInfo.nMinFrameBufferCount++;
		nChanged=1;
	}
#endif

	VPU_COMP_LOG("%s: Init OK, [width x height]=[%d x %d] \r\n",__FUNCTION__,sInitInfo.nPicWidth,sInitInfo.nPicHeight);
	VPU_COMP_LOG("%s: [top,left,width,height]=[%d,%d,%d,%d], \r\n",__FUNCTION__,
	             (INT32)sOutCrop.nLeft,(INT32)sOutCrop.nTop,(INT32)sOutCrop.nWidth,(INT32)sOutCrop.nHeight);
	VPU_COMP_LOG("nOutBufferCnt:%d ,nPadWidth: %d, nPadHeight: %d \r\n",(INT32)nOutBufferCnt, (INT32)nPadWidth, (INT32)nPadHeight);

	//get aspect ratio info
	sDispRatio.xWidth=sInitInfo.nQ16ShiftWidthDivHeightRatio;
	sDispRatio.xHeight=Q16_SHIFT;
	VPU_COMP_LOG("%s: ratio: width: 0x%X, height: 0x%X \r\n",__FUNCTION__,sDispRatio.xWidth,sDispRatio.xHeight);

	if(nChanged)
	{
		nOutBufferSize=sOutFmt.nFrameWidth * sOutFmt.nFrameHeight*pxlfmt2bpp(sOutFmt.eColorFormat)/8;
		OutputFmtChanged();
	}

	//bufRet|=FILTER_DO_INIT;
	bufRet=(FilterBufRetCode)(bufRet|FILTER_NO_OUTPUT_BUFFER); //request enough output buffer before do InitFilter() operation

	//update state
	eVpuDecoderState=VPU_COM_STATE_WAIT_FRM;
	VPU_COMP_LOG("%s: enter wait frame state \r\n",__FUNCTION__);
	return bufRet;	//OMX_ErrorNone;
}

/**< C style functions to expose entry point for the shared library */
extern "C"
{
	OMX_ERRORTYPE VpuDecoderInit(OMX_IN OMX_HANDLETYPE pHandle)
	{
		OMX_ERRORTYPE ret = OMX_ErrorNone;
		VpuDecoder *obj = NULL;
		ComponentBase *base = NULL;
		VPU_COMP_API_LOG("%s: \r\n",__FUNCTION__);

		obj = FSL_NEW(VpuDecoder, ());
		if(obj == NULL)
		{
			VPU_COMP_ERR_LOG("%s: vpu decoder new failure: ret=0x%X \r\n",__FUNCTION__,ret);
			return OMX_ErrorInsufficientResources;
		}

		base = (ComponentBase*)obj;
		ret = base->ConstructComponent(pHandle);
		if(ret != OMX_ErrorNone)
		{
			VPU_COMP_ERR_LOG("%s: vpu decoder construct failure: ret=0x%X \r\n",__FUNCTION__,ret);
			return ret;
		}
		return ret;
	}
}

/* File EOF */
