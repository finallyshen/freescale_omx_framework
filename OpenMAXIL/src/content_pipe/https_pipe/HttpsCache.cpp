/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

#include "HttpsCache.h"
#include "Mem.h"
#include "Log.h"

static void *DoReadStream(void *arg) 
{
    HttpsCache *h = (HttpsCache*) arg;
    while(OMX_ErrorNone == h->GetStreamData());
    return NULL;
}

HttpsCache::HttpsCache()
{
    mURL = NULL;
    mBlockSize = mBlockNum = 0;
    mCacheBuffer = NULL;
    mCacheNodeRoot = NULL;
    mContentLength = mOffsetStart = mOffsetEnd = 0;
    mStartNode = mEndNode = 0;
    SemReadStream = NULL;
    pThread = NULL;
    bStopThread = OMX_FALSE;
    bDiscardData = OMX_FALSE;
    bHandleError = OMX_FALSE;
    bStreamEOS = OMX_FALSE;
}

OMX_ERRORTYPE HttpsCache::Create(
        OMX_STRING url, 
        OMX_U32 nBlockSize, 
        OMX_U32 nBlockNum)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_S32 i;
    CacheNode *pNode = NULL;

    //fsl_osal_timeval tv1, tv2;
    //fsl_osal_systime(&tv1);

    mURL = url;
#ifdef FROYO 
    mHttpSource = new android::HTTPDataSource(mURL, NULL);
    if(mHttpSource->connect() != android::OK) {
#else
    mHttpSource = new android::NuHTTPDataSource();
    if(mHttpSource->connect(mURL, NULL) != android::OK) {
#endif
        LOG_ERROR("Connect %s failed.\n", mURL);
        mHttpSource.clear();

        //fsl_osal_systime(&tv2);
        //OMX_S32 diff = (OMX_S32)(tv2.sec - tv1.sec)*1000 + (OMX_S32)(tv2.usec - tv1.usec)/1000;
        //if(diff > 1000)
        //    printf("Connet failed, cost time: %d\n", diff);

        return OMX_ErrorUndefined;
    }

    //fsl_osal_systime(&tv2);
    //OMX_S32 diff = (OMX_S32)(tv2.sec - tv1.sec)*1000 + (OMX_S32)(tv2.usec - tv1.usec)/1000;
    //if(diff > 1000)
    //    printf("Connet succes, cost time: %d\n", diff);

    LOG_DEBUG("Connect to %s success.\n", mURL);

#ifdef HONEY_COMB
    off64_t size = 0;
#else
    off_t size = 0;
#endif
    mHttpSource->getSize(&size);
    mContentLength = size;

    LOG_DEBUG("Content length: %lld\n", mContentLength);

    if(mContentLength == 0 && (fsl_osal_strncmp(mURL, "http://localhost", 16) == 0))
        bHandleError = OMX_TRUE;
    
    mBlockSize = nBlockSize;
    mBlockNum = nBlockNum;
    mCacheBuffer = FSL_MALLOC(mBlockSize * mBlockNum);
    if(mCacheBuffer == NULL) {
        LOG_ERROR("Failed to allocate memory for HttpsCache, size %d\n", mBlockSize * mBlockNum);
        ret = OMX_ErrorInsufficientResources;
        goto err;
    }

    mCacheNodeRoot = (CacheNode*) FSL_MALLOC(mBlockNum * sizeof(CacheNode));
    if(mCacheNodeRoot == NULL) {
        LOG_ERROR("Failed to allocate memory for HttpsCache node.\n");
        ret = OMX_ErrorInsufficientResources;
        goto err;
    }

    pNode = mCacheNodeRoot;
    for(i=0; i<(OMX_S32)mBlockNum; i++) {
        pNode->nOffset = 0;
        pNode->nLength = 0;
        pNode->pData = (OMX_PTR)((OMX_U32)mCacheBuffer + i * mBlockSize);
        pNode ++;
    }

    if(E_FSL_OSAL_SUCCESS != fsl_osal_sem_init(&SemReadStream, 0, mBlockNum)) {
        LOG_ERROR("Failed create Semphore for HttpsCache.\n");
        ret = OMX_ErrorInsufficientResources;
        goto err;
    }

    if(E_FSL_OSAL_SUCCESS != fsl_osal_thread_create(&pThread, NULL, DoReadStream, this)) {
        LOG_ERROR("Failed create Thread for HttpsCache.\n");
        ret = OMX_ErrorInsufficientResources;
        goto err;
    }

    return OMX_ErrorNone;

err:
    Destroy();
    return ret;
}

OMX_ERRORTYPE HttpsCache::Destroy()
{
    if(pThread != NULL) {
        bStopThread = OMX_TRUE;
        fsl_osal_sem_post(SemReadStream);
        fsl_osal_thread_destroy(pThread);
    }

    if(SemReadStream != NULL) {
        fsl_osal_sem_destroy(SemReadStream);
    }

    if(mHttpSource != NULL) {
        mHttpSource->disconnect();
        mHttpSource.clear();
    }

    if(mCacheNodeRoot != NULL) {
        FSL_FREE(mCacheNodeRoot);
    }

    if(mCacheBuffer != NULL) {
        FSL_FREE(mCacheBuffer);
    }

    return OMX_ErrorNone;
}

OMX_U64 HttpsCache::GetContentLength()
{
    return mContentLength;
}

OMX_S32 HttpsCache::ReadAt(
        OMX_U64 nOffset, 
        OMX_U32 nSize, 
        OMX_PTR pBuffer)
{
    LOG_DEBUG("Read data to %p size %d offset %lld\n", pBuffer, nSize, nOffset);
    LOG_DEBUG("Read data from cache at [%lld:%d]\n", nOffset, nSize);

    OMX_U64 mLength = (bStreamEOS == OMX_TRUE) ? mOffsetEnd : mContentLength;
    if(mLength > 0) {
        OMX_S32 nLeft = mLength - nOffset;
        if(nLeft <= 0)
            return 0;

        if(nSize > nLeft)
            nSize = nLeft;
    }

    if(OMX_TRUE != AvailableAt(nOffset, nSize)) {
        LOG_DEBUG("Not enough data at [%lld:%d]\n", nOffset, nSize);
        return 0;
    }

    CacheNode *pNode = mCacheNodeRoot + mStartNode;
    LOG_DEBUG("Cache node [%d], [%lld:%d]\n", mStartNode, pNode->nOffset, pNode->nLength);
    while(nOffset > pNode->nOffset + pNode->nLength) {
        mStartNode ++;
        pNode ++;
        if(mStartNode >= mBlockNum) {
            mStartNode = 0;
            pNode = mCacheNodeRoot;
        }
        fsl_osal_sem_post(SemReadStream);
        LOG_DEBUG("Cache node [%d], [%lld:%d]\n", mStartNode, pNode->nOffset, pNode->nLength);
        LOG_DEBUG("%s:%d, Post sem, pNode=%p.\n", __FUNCTION__, __LINE__, pNode);
    }

    LOG_DEBUG("Read from cache node %d, offset=%lld, length=%d\n", mStartNode, pNode->nOffset, pNode->nLength);

    if((nOffset + nSize) <= (pNode->nOffset + pNode->nLength)) {
        OMX_PTR src = (OMX_PTR)((OMX_U32)pNode->pData + (nOffset - pNode->nOffset));
        fsl_osal_memcpy(pBuffer, src, nSize);
        //printf("1:Copy from cache node %d, offset=%lld, length=%d\n", mStartNode, nOffset, nSize);
    }
    else {
        OMX_PTR src = (OMX_PTR)((OMX_U32)pNode->pData + (nOffset - pNode->nOffset));
        OMX_U32 left = pNode->nLength - (nOffset - pNode->nOffset);
        fsl_osal_memcpy(pBuffer, src, left);
        //printf("2:Copy from cache node %d, offset=%lld, length=%d\n", mStartNode, nOffset, nSize);
        //printf("2:Copy from cache node %d, offset=%lld, length=%d\n", mStartNode, nOffset, left);

        OMX_U64 offset = nOffset + left;
        OMX_U32 size = nSize - left;
        OMX_PTR dest = (OMX_PTR)((OMX_U32)pBuffer + left);
        do {
            mStartNode ++;
            pNode ++;
            if(mStartNode >= mBlockNum) {
                mStartNode = 0;
                pNode = mCacheNodeRoot;
            }
            fsl_osal_sem_post(SemReadStream);
            LOG_DEBUG("Cache node [%d], [%lld:%d]\n", mStartNode, pNode->nOffset, pNode->nLength);
            LOG_DEBUG("%s:%d, Post sem.\n", __FUNCTION__, __LINE__);

            OMX_U32 copy = size >= pNode->nLength ? pNode->nLength : size;
            fsl_osal_memcpy(dest, pNode->pData, copy);
            //printf("2:Copy from cache node %d, offset=%lld, length=%d\n", mStartNode, nOffset, copy);
            offset += copy;
            size -= copy;
            dest = (OMX_PTR)((OMX_U32)dest + copy);
        } while(size > 0);
    }

    LOG_DEBUG("Read data from cache at [%lld:%d] done\n", nOffset, nSize);

    mOffsetStart = nOffset + nSize;
    
    return nSize;
}

OMX_BOOL HttpsCache::AvailableAt(
        OMX_U64 nOffset, 
        OMX_U32 nSize)
{
    if(bStreamEOS == OMX_TRUE)
        return OMX_TRUE;

    OMX_U32 nTotal = (mBlockNum - 1) * mBlockSize;
    if(nSize > nTotal) {
        LOG_DEBUG("CheckAvailableBytes %d larger than cache total size %d, ajust to %d\n", nSize, nTotal, nTotal);
        nSize = nTotal;
    }

    if(mContentLength > 0) {
        OMX_U32 nLeft = mContentLength - nOffset;
        if(nSize > nLeft) {
            LOG_DEBUG("CheckAvailableBytes %d larger than left %d, adjust to %d\n", nSize, nLeft, nLeft);
            nSize = nLeft;
        }
    }

    CacheNode *pNode = mCacheNodeRoot + mStartNode;
    LOG_DEBUG("CheckAvailableBytes at [%lld:%d], current: [%lld:%d]\n",
            nOffset, nSize, pNode->nOffset, mOffsetEnd - pNode->nOffset);

    if(nOffset < pNode->nOffset || nOffset > mOffsetEnd) {
        LOG_DEBUG("Reset Cache to %lld:%d, current Node %d [%lld:%lld]\n", nOffset, nSize, mStartNode, pNode->nOffset, mOffsetEnd);
        ResetCache(nOffset);
        return OMX_FALSE;
    }

    if(nOffset + nSize > mOffsetEnd) {
        while(nOffset > pNode->nOffset + pNode->nLength) {
            mStartNode ++;
            pNode ++;
            if(mStartNode >= mBlockNum) {
                mStartNode = 0;
                pNode = mCacheNodeRoot;
            }
            fsl_osal_sem_post(SemReadStream);
            LOG_DEBUG("%s:%d, Post sem.\n", __FUNCTION__, __LINE__);
        }
        return OMX_FALSE;
    }

    return OMX_TRUE;
}

OMX_S32 HttpsCache::AvailableBytes(OMX_U64 nOffset)
{
    if(mOffsetEnd <= nOffset)
        return 0;

    return mOffsetEnd - nOffset;
}

OMX_ERRORTYPE HttpsCache::GetStreamData()
{
    CacheNode *pNode = NULL;

    fsl_osal_sem_wait(SemReadStream);
    if(bStopThread == OMX_TRUE)
        return OMX_ErrorNoMore;

    OMX_U32 nFreeCache = mBlockSize * mBlockNum - (mOffsetEnd - mOffsetStart);
    if(nFreeCache < mBlockSize) {
        LOG_DEBUG("No free cache to read.\n");
        return OMX_ErrorNone;
    }

    pNode = mCacheNodeRoot + mEndNode;
    pNode->nOffset = mOffsetEnd;
    pNode->nLength = 0;

    if(mContentLength > 0 && mOffsetEnd >= mContentLength) {
        fsl_osal_sleep(1000000);
        return OMX_ErrorNone;
    }

    LOG_DEBUG("+++++++Want Fill cache node [%d] at offset %d\n", mEndNode, mOffsetEnd);
    bDiscardData = OMX_FALSE;

    //fsl_osal_timeval tv1, tv2;
    //fsl_osal_systime(&tv1);

    ssize_t n = mHttpSource->readAt(mOffsetEnd, pNode->pData, mBlockSize);

    //fsl_osal_systime(&tv2);
    //OMX_S32 diff = (OMX_S32)(tv2.sec - tv1.sec)*1000 + (OMX_S32)(tv2.usec - tv1.usec)/1000;
    //if(diff > 1000)
    //    printf("read %d data cost: %d\n", mBlockSize, diff);

    if(bDiscardData == OMX_TRUE) {
        bDiscardData = OMX_FALSE;
        fsl_osal_sem_post(SemReadStream);
        return OMX_ErrorNone;
    }

    if(n <= 0) {
        LOG_DEBUG("read http source failed, ret = %d\n", n);
        if(bHandleError == OMX_TRUE) {
            bStreamEOS = OMX_TRUE;
            return OMX_ErrorUndefined;
        }
        else
            fsl_osal_sleep(1000000);
    }

    if(n > 0) {
        pNode->nLength = n;
        mOffsetEnd += n;
        LOG_DEBUG("+++++++Fill cache node [%d], [%lld:%d]\n", mEndNode, pNode->nOffset, pNode->nLength);
        mEndNode ++;
        if(mEndNode >= mBlockNum)
            mEndNode = 0;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE HttpsCache::ResetCache(OMX_U64 nOffset)
{
    while(E_FSL_OSAL_SUCCESS == fsl_osal_sem_trywait(SemReadStream));

    OMX_S32 i;
    for(i=0; i<mBlockNum; i++) {
        mCacheNodeRoot[i].nOffset = 0;
        mCacheNodeRoot[i].nLength = 0;
    }

    mOffsetStart = mOffsetEnd = nOffset;
    mCacheNodeRoot[0].nOffset = nOffset;
    mStartNode = mEndNode = 0;
    bDiscardData = OMX_TRUE;

    for(i=0; i<mBlockNum; i++)
        fsl_osal_sem_post(SemReadStream);


    return OMX_ErrorNone;
}


/*EOF*/




