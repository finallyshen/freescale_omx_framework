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

#include "libavutil/opt.h"
#include "libavformat/http.h"

extern URLProtocol ff_http_protocol;
extern URLProtocol ff_mmsh_protocol;
extern URLProtocol ff_mmst_protocol;

static void *DoReadStream(void *arg) 
{
    HttpsCache *h = (HttpsCache*) arg;
    while(OMX_ErrorNone == h->GetStreamData());
    return NULL;
}

static OMX_S32 UrlHeaderPos(OMX_STRING url) 
{
    OMX_S32 len = fsl_osal_strlen(url) + 1;
    OMX_S32 i;
    OMX_BOOL bHasHeader = OMX_FALSE;
    for(i=0; i<len; i++) {
        if(url[i] == '\n') {
            bHasHeader = OMX_TRUE;
            break;
        }
    }

    if(bHasHeader == OMX_TRUE)
        return i+1;
    else
        return 0;
}

static OMX_BOOL bAbort = OMX_FALSE;
static int abort_cb(void)
{
    if(bAbort == OMX_TRUE) {
        bAbort = OMX_FALSE;
        return 1;
    }

    return 0;
}

HttpsCache::HttpsCache()
{
    mBlockSize = mBlockNum = 0;
    mCacheBuffer = NULL;
    mCacheNodeRoot = NULL;
    mContentLength = mOffsetStart = mOffsetEnd = 0;
    mStartNode = mEndNode = 0;
    SemReadStream = NULL;
    pThread = NULL;
    bStopThread = OMX_FALSE;
    bHandleError = OMX_FALSE;
    bStreamEOS = OMX_FALSE;
    bReset = OMX_FALSE;
    uc = NULL;
}

OMX_ERRORTYPE HttpsCache::Create(
        OMX_STRING url, 
        OMX_U32 nBlockSize, 
        OMX_U32 nBlockNum)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_S32 i;
    CacheNode *pNode = NULL;
    OMX_STRING URL, mURL, mHeader;

    av_register_all();
    //avio_set_interrupt_cb(abort_cb);

    URL = mURL = mHeader = NULL;
    OMX_S32 pos = UrlHeaderPos(url);
    if(pos > 0) {
        OMX_S32 url_len = fsl_osal_strlen(url) + 1;
        URL = (OMX_STRING)FSL_MALLOC(url_len);
        if(URL == NULL)
            return OMX_ErrorInsufficientResources;
        fsl_osal_strcpy(URL, url);
        URL[pos - 1] = '\0';
        mURL = URL;
        mHeader = URL + pos;
    }
    else
        mURL = url;

    LOG_DEBUG("mURL: %s\n", mURL);
    LOG_DEBUG("mHeader: %s\n", mHeader);

    uc = (URLContext*)FSL_MALLOC(sizeof(URLContext) + fsl_osal_strlen(mURL) + 1);
    if(uc == NULL) {
        FSL_FREE(URL);
        LOG_ERROR("Allocate for http context failed.\n");
        return OMX_ErrorInsufficientResources;
    }

    uc->filename = (char *) &uc[1];
    fsl_osal_strcpy(uc->filename, mURL);
    uc->flags = AVIO_RDONLY;
    uc->is_streamed = 0; /* default = not streamed */
    uc->max_packet_size = 0; /* default: stream file */

    if(fsl_osal_strncmp(mURL, "http://", 7) == 0)
        uc->prot = &ff_http_protocol;
    else if(fsl_osal_strncmp(mURL, "mms://", 6) == 0)
        uc->prot = &ff_mmsh_protocol;
    else {
        FSL_FREE(uc);
        FSL_FREE(URL);
        return OMX_ErrorNotImplemented;
    }

    if (uc->prot->priv_data_size) {
        uc->priv_data = FSL_MALLOC(uc->prot->priv_data_size);
        if(uc->priv_data == NULL) {
            FSL_FREE(uc);
            FSL_FREE(URL);
            return OMX_ErrorInsufficientResources;
        }
        fsl_osal_memset(uc->priv_data, 0, uc->prot->priv_data_size);
        if (uc->prot->priv_data_class) {
            *(const AVClass**)uc->priv_data = uc->prot->priv_data_class;
            av_opt_set_defaults(uc->priv_data);
        }
    }

    if(mHeader)
        ff_http_set_headers(uc, mHeader);

    FSL_FREE(URL);

    LOG_DEBUG("Connecting %s\n", uc->filename);

    int err = 0;
    err = uc->prot->url_open(uc, uc->filename, uc->flags);
    if (err) {
        LOG_ERROR("Connect %s failed.\n", uc->filename);
        FSL_FREE(uc->priv_data);
        FSL_FREE(uc);
        return OMX_ErrorUndefined;
    }

    uc->is_connected = 1;
    LOG_DEBUG("Connect to %s success.\n", uc->filename);

    if(uc->prot->url_seek)
        mContentLength = uc->prot->url_seek(uc, 0, AVSEEK_SIZE);
    if(mContentLength == (OMX_U64)-1)
        mContentLength = 0;
    LOG_DEBUG("Content Length: %lld\n", mContentLength);

    if(mContentLength == 0 && (fsl_osal_strncmp(uc->filename, "http://localhost", 16) == 0))
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
    bAbort = OMX_TRUE;

    if(pThread != NULL) {
        bStopThread = OMX_TRUE;
        fsl_osal_sem_post(SemReadStream);
        fsl_osal_thread_destroy(pThread);
    }

    if(SemReadStream != NULL) {
        fsl_osal_sem_destroy(SemReadStream);
    }

    if(uc != NULL) {
        uc->prot->url_close(uc);
        FSL_FREE(uc->priv_data);
        FSL_FREE(uc);
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
        OMX_S64 nLeft = mLength - nOffset;
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
        OMX_U64 nLeft = mContentLength - nOffset;
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
    if(bStopThread == OMX_TRUE) {
        LOG_DEBUG("Read stream thread is stoped.\n");
        return OMX_ErrorNoMore;
    }

    OMX_U32 nFreeCache = mBlockSize * mBlockNum - (mOffsetEnd - mOffsetStart);
    if(nFreeCache < mBlockSize) {
        LOG_DEBUG("No free cache to read, nFreeCache: %d, mBlockSize: %d.\n", nFreeCache, mBlockSize);
        fsl_osal_sleep(100000);
        return OMX_ErrorNone;
    }

    if(mContentLength > 0 && mOffsetEnd >= mContentLength) {
        fsl_osal_sleep(1000000);
        return OMX_ErrorNone;
    }

    if(bReset == OMX_TRUE) {
        if(uc->prot->url_seek) {
            OMX_S64 offset = uc->prot->url_seek(uc, mOffsetStart, SEEK_SET);
            if(offset < 0) {
                LOG_ERROR("Http seek to %lld error, ret: %lld\n", mOffsetStart, offset);
                return OMX_ErrorNone;
            }
            else if(offset < (OMX_S64)mOffsetStart) {
                LOG_DEBUG("http seek not return correctly, request: %lld, return: %lld\n", mOffsetStart, offset);
                OMX_U8 buffer[512];
                OMX_S32 diff = mOffsetStart - offset;
                while(diff > 0) {
                    if(bStopThread == OMX_TRUE)
                        return OMX_ErrorNoMore;
                    OMX_S32 read = 0;
                    read = uc->prot->url_read(uc, (unsigned char*)buffer, diff > 512 ? 512 : diff);
                    if(read > 0)
                        diff -= read;
                }
            }
        }
        bReset = OMX_FALSE;
    }

    pNode = mCacheNodeRoot + mEndNode;
    pNode->nOffset = mOffsetEnd;
    pNode->nLength = 0;

    LOG_DEBUG("+++++++Want Fill cache node [%d] at offset %d\n", mEndNode, mOffsetEnd);

    OMX_U8 *pBuffer = (OMX_U8*)pNode->pData;
    OMX_S32 nWant = mBlockSize;

again:
    OMX_S32 n = uc->prot->url_read(uc, (unsigned char*)pBuffer, nWant);

    if(bStopThread == OMX_TRUE)
        return OMX_ErrorUndefined;

    if(bReset == OMX_TRUE)
        return OMX_ErrorNone;

    if(n <= 0) {
        LOG_DEBUG("read http source failed, ret = %d\n", n);
        if(bHandleError == OMX_TRUE) {
            bStreamEOS = OMX_TRUE;
            return OMX_ErrorUndefined;
        }
        else {
            fsl_osal_sleep(1000000);
            if(mContentLength > 0 && mOffsetEnd >= mContentLength)
                return OMX_ErrorNone;
            else {
                if(uc->prot->url_seek) {
                    while (uc->prot->url_seek(uc, mOffsetEnd, SEEK_SET) < 0) {
                        fsl_osal_sleep(1000000);
                        if(bStopThread)
                            return OMX_ErrorUndefined;
                    }
                }
                goto again;
            }
        }
    }

    if(n > 0) {
        pNode->nLength += n;
        mOffsetEnd += n;
        LOG_DEBUG("+++++++Fill cache node [%d], [%lld:%d]\n", mEndNode, pNode->nOffset, pNode->nLength);

        if(pNode->nLength < mBlockSize) {
            pBuffer += n;
            nWant -= n;
            goto again;
        }
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
    for(i=0; i<(OMX_S32)mBlockNum; i++) {
        mCacheNodeRoot[i].nOffset = 0;
        mCacheNodeRoot[i].nLength = 0;
    }

    mOffsetStart = mOffsetEnd = nOffset;
    mCacheNodeRoot[0].nOffset = nOffset;
    mStartNode = mEndNode = 0;
    bReset = OMX_TRUE;

    for(i=0; i<(OMX_S32)mBlockNum; i++)
        fsl_osal_sem_post(SemReadStream);


    return OMX_ErrorNone;
}


/*EOF*/




