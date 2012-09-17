/**
 *  Copyright (c) 2009-2011, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */

/**
 *  @file HttpsCache.h
 *  @brief Class definition of HttpsCache
 *  @ingroup Https
 */

#ifndef HttpsCache_h
#define HttpsCache_h

#include "fsl_osal.h"
#include "OMX_Core.h"
#ifdef FROYO
#include <stagefright/HTTPDataSource.h>
#else
#include <NuHTTPDataSource.h>
#endif

class HttpsCache {
    public:
        HttpsCache();
        OMX_ERRORTYPE Create(OMX_STRING url, OMX_U32 nBlockSize, OMX_U32 nBlockNum);
        OMX_ERRORTYPE Destroy();
        OMX_U64 GetContentLength();
        OMX_S32 ReadAt(OMX_U64 nOffset, OMX_U32 nSize, OMX_PTR pBuffer);
        OMX_BOOL AvailableAt(OMX_U64 nOffset, OMX_U32 nSize);
        OMX_S32 AvailableBytes(OMX_U64 nOffset);
        OMX_ERRORTYPE GetStreamData();
    private:
        typedef struct{
            OMX_U64 nOffset;
            OMX_U32 nLength;
            OMX_PTR pData;
        } CacheNode;

        OMX_STRING mURL;
        OMX_U32 mBlockSize;
        OMX_U32 mBlockNum;
        OMX_PTR mCacheBuffer;
        CacheNode *mCacheNodeRoot;
        OMX_U64 mContentLength;
        OMX_U64 mOffsetEnd;
        OMX_U64 mOffsetStart;
        OMX_U32 mStartNode;
        OMX_U32 mEndNode;
        fsl_osal_sem SemReadStream;
        OMX_PTR pThread;
        OMX_BOOL bStopThread;
        OMX_BOOL bDiscardData;
        OMX_BOOL bStreamEOS;
        OMX_BOOL bHandleError;
#ifdef FROYO
        android::sp<android::HTTPDataSource> mHttpSource;
#else
        android::sp<android::NuHTTPDataSource> mHttpSource;
#endif

        OMX_ERRORTYPE ResetCache(OMX_U64 nOffset);
};

#endif
/* File EOF */
