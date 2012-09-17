/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <linux/android_pmem.h>
#include "PMemory.h"
#include "Mem.h"
#include "Log.h"

#define MAX_BUFFER_NUM 32

typedef struct {
    OMX_S32 fd;
    OMX_U32 nSize;
    OMX_U32 nBufferSize;
    OMX_U32 nBufferNum;
    OMX_PTR VAddrBase;
    OMX_PTR PAddrBase;
    PMEMADDR BufferSlot[MAX_BUFFER_NUM];
    OMX_BOOL bSlotAllocated[MAX_BUFFER_NUM];
}PMCONTEXT;

static OMX_ERRORTYPE InitPMemAllocator(PMCONTEXT *Context, OMX_U32 nBufferSize, OMX_U32 nBufferNum)
{
    struct pmem_region region;
    OMX_S32 pagesize = getpagesize();

    if(nBufferNum > MAX_BUFFER_NUM)
        return OMX_ErrorBadParameter;

    nBufferSize = (nBufferSize + pagesize-1) & ~(pagesize-1);
    Context->fd = open("/dev/pmem_adsp", O_RDWR);
    if (Context->fd < 0) {
        LOG_ERROR("InitPMemAllocator Error,cannot open pmem dev.\n");
        return OMX_ErrorHardware;
    }

    ioctl(Context->fd, PMEM_GET_TOTAL_SIZE, &region);
    LOG_DEBUG("Get pmem total size %d",region.len);

    Context->nBufferSize = nBufferSize;
    Context->nBufferNum = nBufferNum;
    Context->nSize = nBufferSize * nBufferNum;

    Context->VAddrBase = (OMX_PTR)mmap(0, Context->nSize, PROT_READ|PROT_WRITE, MAP_SHARED, Context->fd, 0);
    if (Context->VAddrBase == MAP_FAILED) {
        LOG_ERROR("Error!mmap(fd=%d, size=%u) failed.\n", Context->fd, Context->nSize);
        close(Context->fd);
        return OMX_ErrorHardware;
    }

    fsl_osal_memset(&region, 0, sizeof(region));

    if (ioctl(Context->fd, PMEM_GET_PHYS, &region) < 0) {
        LOG_ERROR("Error!Failed to get physical address of source!\n");
        munmap(Context->VAddrBase, Context->nSize);
        close(Context->fd);
        return OMX_ErrorHardware;
    }
    Context->PAddrBase = (OMX_PTR)region.offset;

    for(OMX_U32 i=0; i<nBufferNum; i++) {
        Context->BufferSlot[i].pVirtualAddr = (OMX_U8 *)Context->VAddrBase + Context->nBufferSize * i;
        Context->BufferSlot[i].pPhysicAddr = (OMX_U8 *)Context->PAddrBase + Context->nBufferSize * i;
    }

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE DeInitPMemAllocator(PMCONTEXT *Context)
{
    if(Context == NULL)
        return OMX_ErrorBadParameter;

    if(Context->fd) {
        munmap(Context->VAddrBase, Context->nSize);
        close(Context->fd);
    }

    fsl_osal_memset(Context, 0, sizeof(PMCONTEXT));

    return OMX_ErrorNone;
}

OMX_PTR CreatePMeoryContext()
{
    OMX_PTR Context = NULL;

    Context = (OMX_PTR)FSL_MALLOC(sizeof(PMCONTEXT));
    if(Context == NULL) {
        LOG_ERROR("Failed allocate PMemory context.\n");
        return NULL;
    }
    fsl_osal_memset(Context, 0, sizeof(PMCONTEXT));

    return Context;
}

OMX_ERRORTYPE DestroyPMeoryContext(OMX_PTR Context)
{
    if(Context != NULL) {
        DeInitPMemAllocator((PMCONTEXT*)Context);
        FSL_FREE(Context);
    }

    return OMX_ErrorNone;
}

PMEMADDR GetPMBuffer(OMX_PTR Context, OMX_U32 nSize, OMX_U32 nNum)
{
    PMCONTEXT *pContext = (PMCONTEXT*)Context;
    PMEMADDR sMemAddr;

    fsl_osal_memset(&sMemAddr, 0, sizeof(PMEMADDR));

    if(pContext == NULL)
        return sMemAddr;

    if(pContext->fd == 0)
        if(OMX_ErrorNone != InitPMemAllocator(pContext, nSize, nNum))
            return sMemAddr;

    OMX_S32 i;
    for(i=0; i<(OMX_S32)pContext->nBufferNum; i++)
        if(pContext->bSlotAllocated[i] != OMX_TRUE)
            break;

    if(i == (OMX_S32)pContext->nBufferNum) {
        LOG_ERROR("No freed P memory for allocate.\n");
        return sMemAddr;
    }

    pContext->bSlotAllocated[i] = OMX_TRUE;

    LOG_DEBUG("PM allocate: %p\n", pContext->BufferSlot[i]);

    return pContext->BufferSlot[i];
}

OMX_ERRORTYPE FreePMBuffer(OMX_PTR Context, OMX_PTR pBuffer)
{
    PMCONTEXT *pContext = (PMCONTEXT*)Context;

    if(pContext == NULL)
        return OMX_ErrorBadParameter;

    LOG_DEBUG("PM free: %p\n", pBuffer);

    OMX_S32 i;
    for(i=0; i<(OMX_S32)pContext->nBufferSize; i++)
        if(pBuffer == pContext->BufferSlot[i].pVirtualAddr)
            break;

    if(i == (OMX_S32)pContext->nBufferNum) {
        LOG_ERROR("Bad PMemory address for free.\n");
        return OMX_ErrorBadParameter;
    }

    pContext->bSlotAllocated[i] = OMX_FALSE;

    return OMX_ErrorNone;
}

/* File EOF */
