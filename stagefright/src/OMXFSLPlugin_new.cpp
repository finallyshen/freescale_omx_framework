/**
 *  Copyright (c) 2011-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
#include <media/stagefright/OMXPluginBase.h>
#include <media/stagefright/HardwareAPI.h>
#include <ui/GraphicBuffer.h>
#include <gralloc_priv.h>

#include "OMX_Index.h"
#include "PlatformResourceMgrItf.h"

#define MAX_BUFFER_CNT (32)

#define OMX_IndexParamEnableAndroidNativeBuffers ((OMX_INDEXTYPE)(OMX_IndexMax - 1))
#define OMX_IndexParamNativeBufferUsage          ((OMX_INDEXTYPE)(OMX_IndexMax - 2))
#define OMX_IndexParamStoreMetaDataInBuffers     ((OMX_INDEXTYPE)(OMX_IndexMax - 3))
#define OMX_IndexParamUseAndroidNativeBuffer     ((OMX_INDEXTYPE)(OMX_IndexMax - 4))

namespace android {

class FSLOMXWrapper {
    public:
        FSLOMXWrapper();
        OMX_COMPONENTTYPE *MakeWapper(OMX_HANDLETYPE pHandle);
        OMX_COMPONENTTYPE *GetComponentHandle();

        OMX_ERRORTYPE GetVersion(OMX_STRING pComponentName, OMX_VERSIONTYPE* pComponentVersion, 
                                 OMX_VERSIONTYPE* pSpecVersion, OMX_UUIDTYPE* pComponentUUID) {
            return OMX_GetComponentVersion(
                    ComponentHandle, pComponentName, pComponentVersion, pSpecVersion, pComponentUUID);
        }
        OMX_ERRORTYPE SendCommand(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData) {
            return OMX_SendCommand(ComponentHandle, Cmd, nParam1, pCmdData);
        }
        OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure);
        OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure) {
            return OMX_GetConfig(ComponentHandle, nParamIndex, pStructure);
        }
        OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE nParamIndex, OMX_PTR pStructure) {
            return OMX_SetConfig(ComponentHandle, nParamIndex, pStructure);
        }
        OMX_ERRORTYPE GetExtensionIndex(OMX_STRING cParameterName, OMX_INDEXTYPE* pIndexType);
        OMX_ERRORTYPE GetState(OMX_STATETYPE* pState) {
            return OMX_GetState(ComponentHandle, pState);
        }
        OMX_ERRORTYPE TunnelRequest(OMX_U32 nPortIndex, OMX_HANDLETYPE hTunneledComp,
                                    OMX_U32 nTunneledPort, OMX_TUNNELSETUPTYPE* pTunnelSetup) {
            return ComponentHandle->ComponentTunnelRequest(
                    ComponentHandle, nPortIndex, hTunneledComp, nTunneledPort, pTunnelSetup);
        }
        OMX_ERRORTYPE UseBuffer(OMX_BUFFERHEADERTYPE** ppBuffer, OMX_U32 nPortIndex,
                                OMX_PTR pAppPrivate, OMX_U32 nSizeBytes, OMX_U8* pBuffer) {
            return OMX_UseBuffer(ComponentHandle, ppBuffer, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
        }
        OMX_ERRORTYPE AllocateBuffer(OMX_BUFFERHEADERTYPE** ppBuffer, OMX_U32 nPortIndex,
                                     OMX_PTR pAppPrivate, OMX_U32 nSizeBytes) {
            return OMX_AllocateBuffer(ComponentHandle, ppBuffer, nPortIndex, pAppPrivate, nSizeBytes);
        }
        OMX_ERRORTYPE FreeBuffer(OMX_U32 nPortIndex, OMX_BUFFERHEADERTYPE* pBuffer);
        OMX_ERRORTYPE EmptyThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer) {
            return OMX_EmptyThisBuffer(ComponentHandle, pBuffer);
        }
        OMX_ERRORTYPE FillThisBuffer(OMX_BUFFERHEADERTYPE* pBuffer) {
            return OMX_FillThisBuffer(ComponentHandle, pBuffer);
        }
        OMX_ERRORTYPE SetCallbacks(OMX_CALLBACKTYPE* pCbs, OMX_PTR pAppData) {
            return ComponentHandle->SetCallbacks(ComponentHandle, pCbs, pAppData);
        }
        OMX_ERRORTYPE ComponentDeInit() {
            return ComponentHandle->ComponentDeInit(ComponentHandle);
        }
        OMX_ERRORTYPE UseEGLImage(OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex,
                                  OMX_PTR pAppPrivate, void *eglImage) {
            return OMX_UseEGLImage(ComponentHandle, ppBufferHdr, nPortIndex, pAppPrivate, eglImage);
        }
        OMX_ERRORTYPE ComponentRoleEnum(OMX_U8 *cRole, OMX_U32 nIndex) {
            return ComponentHandle->ComponentRoleEnum(ComponentHandle, cRole, nIndex);
        }

    private:
        typedef struct {
            OMX_BUFFERHEADERTYPE *pBufferHdr;
            sp<GraphicBuffer> mGraphicBuffer;
        }BufferMapper;

        OMX_COMPONENTTYPE WrapperHandle;
        OMX_COMPONENTTYPE *ComponentHandle;
        OMX_BOOL bEnableNativeBuffers;
        OMX_U32 nNativeBuffersUsage;
        OMX_BOOL bStoreMetaData;
        BufferMapper sBufferMapper[MAX_BUFFER_CNT];
        OMX_S32 nBufferCnt;

        OMX_ERRORTYPE DoUseNativeBuffer(UseAndroidNativeBufferParams *pNativBufferParam);
};

class FSLOMXPlugin : public OMXPluginBase {
    public:
        FSLOMXPlugin() {
            OMX_Init();
        };

        virtual ~FSLOMXPlugin() {
            OMX_Deinit();
        };

        OMX_ERRORTYPE makeComponentInstance(
                const char *name,
                const OMX_CALLBACKTYPE *callbacks,
                OMX_PTR appData,
                OMX_COMPONENTTYPE **component);

        OMX_ERRORTYPE destroyComponentInstance(
                OMX_COMPONENTTYPE *component);

        OMX_ERRORTYPE enumerateComponents(
                OMX_STRING name,
                size_t size,
                OMX_U32 index);

        OMX_ERRORTYPE getRolesOfComponent(
                const char *name,
                Vector<String8> *roles);
    private:
};

#define GET_WRAPPER(handle) \
    ({\
        FSLOMXWrapper *wrapper = NULL; \
        if(handle == NULL) return OMX_ErrorInvalidComponent; \
        OMX_COMPONENTTYPE *hComp = (OMX_COMPONENTTYPE*)handle; \
        wrapper = (FSLOMXWrapper*)(hComp->pComponentPrivate); \
        wrapper; \
    })

static OMX_ERRORTYPE WrapperGetComponentVersion(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STRING pComponentName,
        OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
        OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
        OMX_OUT OMX_UUIDTYPE* pComponentUUID)
{
    return GET_WRAPPER(hComponent)->GetVersion(pComponentName, pComponentVersion, pSpecVersion, pComponentUUID);
}

static OMX_ERRORTYPE WrapperSendCommand(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_COMMANDTYPE Cmd,
            OMX_IN  OMX_U32 nParam1,
            OMX_IN  OMX_PTR pCmdData)
{
    return GET_WRAPPER(hComponent)->SendCommand(Cmd, nParam1, pCmdData);
}

static OMX_ERRORTYPE WrapperGetParameter(
            OMX_IN  OMX_HANDLETYPE hComponent, 
            OMX_IN  OMX_INDEXTYPE nParamIndex,  
            OMX_INOUT OMX_PTR pStructure)
{
    return GET_WRAPPER(hComponent)->GetParameter(nParamIndex, pStructure);
}

static OMX_ERRORTYPE WrapperSetParameter(
            OMX_IN  OMX_HANDLETYPE hComponent, 
            OMX_IN  OMX_INDEXTYPE nParamIndex,  
            OMX_INOUT OMX_PTR pStructure)
{
    return GET_WRAPPER(hComponent)->SetParameter(nParamIndex, pStructure);
}

static OMX_ERRORTYPE WrapperGetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex, 
            OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    return GET_WRAPPER(hComponent)->GetConfig(nIndex, pComponentConfigStructure);
}


static OMX_ERRORTYPE WrapperSetConfig(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_INDEXTYPE nIndex, 
            OMX_IN  OMX_PTR pComponentConfigStructure)
{
    return GET_WRAPPER(hComponent)->SetConfig(nIndex, pComponentConfigStructure);
}

static OMX_ERRORTYPE WrapperGetExtensionIndex(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_STRING cParameterName,
            OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    return GET_WRAPPER(hComponent)->GetExtensionIndex(cParameterName, pIndexType);
}

static OMX_ERRORTYPE WrapperGetState(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_OUT OMX_STATETYPE* pState)
{
    return GET_WRAPPER(hComponent)->GetState(pState);
}

static OMX_ERRORTYPE WrapperComponentTunnelRequest(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_U32 nPort,
        OMX_IN  OMX_HANDLETYPE hTunneledComp,
        OMX_IN  OMX_U32 nTunneledPort,
        OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    return GET_WRAPPER(hComponent)->TunnelRequest(nPort, hTunneledComp, nTunneledPort, pTunnelSetup);
}


static OMX_ERRORTYPE WrapperUseBuffer(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer)
{
    return GET_WRAPPER(hComponent)->UseBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
}


static OMX_ERRORTYPE WrapperAllocateBuffer(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes)
{
    return GET_WRAPPER(hComponent)->AllocateBuffer(ppBuffer, nPortIndex, pAppPrivate, nSizeBytes);
}

static OMX_ERRORTYPE WrapperFreeBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_U32 nPortIndex,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    return GET_WRAPPER(hComponent)->FreeBuffer(nPortIndex, pBuffer);
}

static OMX_ERRORTYPE WrapperEmptyThisBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    return GET_WRAPPER(hComponent)->EmptyThisBuffer(pBuffer);
}


static OMX_ERRORTYPE WrapperFillThisBuffer(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    return GET_WRAPPER(hComponent)->FillThisBuffer(pBuffer);
}


static OMX_ERRORTYPE WrapperSetCallbacks(
            OMX_IN  OMX_HANDLETYPE hComponent,
            OMX_IN  OMX_CALLBACKTYPE* pCbs, 
            OMX_IN  OMX_PTR pAppData)
{
    return GET_WRAPPER(hComponent)->SetCallbacks(pCbs, pAppData);
}


static OMX_ERRORTYPE WrapperComponentDeInit(
            OMX_IN  OMX_HANDLETYPE hComponent)
{
    return GET_WRAPPER(hComponent)->ComponentDeInit();
}


static OMX_ERRORTYPE WrapperUseEGLImage(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN void* eglImage)
{
    return GET_WRAPPER(hComponent)->UseEGLImage(ppBufferHdr, nPortIndex, pAppPrivate, eglImage);
}


static OMX_ERRORTYPE WrapperComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
		OMX_OUT OMX_U8 *cRole,
		OMX_IN OMX_U32 nIndex)
{
    return GET_WRAPPER(hComponent)->ComponentRoleEnum(cRole, nIndex);
}

FSLOMXWrapper::FSLOMXWrapper()
{
    nBufferCnt = 0;
    ComponentHandle = NULL;
    nNativeBuffersUsage = GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_NEVER | GRALLOC_USAGE_FORCE_CONTIGUOUS;
    memset(&WrapperHandle, 0, sizeof(OMX_COMPONENTTYPE));
    memset(sBufferMapper, 0, sizeof(BufferMapper) * MAX_BUFFER_CNT);
}

OMX_COMPONENTTYPE *FSLOMXWrapper::MakeWapper(OMX_HANDLETYPE pHandle)
{
    if(pHandle == NULL)
        return NULL;

    ComponentHandle = (OMX_COMPONENTTYPE*)pHandle;

    WrapperHandle.pComponentPrivate = this;
    WrapperHandle.GetComponentVersion = WrapperGetComponentVersion;
    WrapperHandle.SendCommand = WrapperSendCommand;
    WrapperHandle.GetParameter = WrapperGetParameter;
    WrapperHandle.SetParameter = WrapperSetParameter;
    WrapperHandle.GetConfig = WrapperGetConfig;
    WrapperHandle.SetConfig = WrapperSetConfig;
    WrapperHandle.GetExtensionIndex = WrapperGetExtensionIndex;
    WrapperHandle.GetState = WrapperGetState;
    WrapperHandle.ComponentTunnelRequest = WrapperComponentTunnelRequest;
    WrapperHandle.UseBuffer = WrapperUseBuffer;
    WrapperHandle.AllocateBuffer = WrapperAllocateBuffer;
    WrapperHandle.FreeBuffer = WrapperFreeBuffer;
    WrapperHandle.EmptyThisBuffer = WrapperEmptyThisBuffer;
    WrapperHandle.FillThisBuffer = WrapperFillThisBuffer;
    WrapperHandle.SetCallbacks = WrapperSetCallbacks;
    WrapperHandle.ComponentDeInit = WrapperComponentDeInit;
    WrapperHandle.UseEGLImage = WrapperUseEGLImage;
    WrapperHandle.ComponentRoleEnum = WrapperComponentRoleEnum;

    return &WrapperHandle;
}

OMX_COMPONENTTYPE * FSLOMXWrapper::GetComponentHandle()
{
    return ComponentHandle;
}

OMX_ERRORTYPE FSLOMXWrapper::GetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(pStructure == NULL)
        return OMX_ErrorBadParameter;

    switch(nParamIndex) {
        case OMX_IndexParamEnableAndroidNativeBuffers:
            {
                EnableAndroidNativeBuffersParams *pParams = (EnableAndroidNativeBuffersParams*)pStructure;
                if(pParams->nPortIndex != 1)
                    return OMX_ErrorUnsupportedIndex;
                pParams->enable = bEnableNativeBuffers;
            }
            break;
        case OMX_IndexParamNativeBufferUsage:
            {
                GetAndroidNativeBufferUsageParams *pParams = (GetAndroidNativeBufferUsageParams*)pStructure;
                if(pParams->nPortIndex != 1)
                    return OMX_ErrorUnsupportedIndex;
                pParams->nUsage = nNativeBuffersUsage;
            }
            break;
        case OMX_IndexParamStoreMetaDataInBuffers:
            {
                StoreMetaDataInBuffersParams *pParams = (StoreMetaDataInBuffersParams*)pStructure;
                if(pParams->nPortIndex != 1)
                    return OMX_ErrorUnsupportedIndex;
                pParams->bStoreMetaData = bStoreMetaData;
            }
            break;
        default:
            ret = OMX_GetParameter(ComponentHandle, nParamIndex, pStructure);
    };

    return ret;
}

OMX_ERRORTYPE FSLOMXWrapper::SetParameter(
        OMX_INDEXTYPE nParamIndex, 
        OMX_PTR pStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(pStructure == NULL)
        return OMX_ErrorBadParameter;

    switch(nParamIndex) {
        case OMX_IndexParamEnableAndroidNativeBuffers:
            {
                EnableAndroidNativeBuffersParams *pParams = (EnableAndroidNativeBuffersParams*)pStructure;
                if(pParams->nPortIndex != 1)
                    return OMX_ErrorUnsupportedIndex;
                bEnableNativeBuffers = pParams->enable;
            }
            break;
        case OMX_IndexParamNativeBufferUsage:
            {
                GetAndroidNativeBufferUsageParams *pParams = (GetAndroidNativeBufferUsageParams*)pStructure;
                if(pParams->nPortIndex != 1)
                    return OMX_ErrorUnsupportedIndex;
                nNativeBuffersUsage = pParams->nUsage;
            }
            break;
        case OMX_IndexParamStoreMetaDataInBuffers:
            {
                StoreMetaDataInBuffersParams *pParams = (StoreMetaDataInBuffersParams*)pStructure;
                if(pParams->nPortIndex != 1)
                    return OMX_ErrorUnsupportedIndex;
                bStoreMetaData = pParams->bStoreMetaData;
            }
            break;
        case OMX_IndexParamUseAndroidNativeBuffer:
            {
                UseAndroidNativeBufferParams *pParams = (UseAndroidNativeBufferParams*)pStructure;
                if(pParams->nPortIndex != 1)
                    return OMX_ErrorUnsupportedIndex;
                ret = DoUseNativeBuffer(pParams);
            }
            break;
        default:
            ret = OMX_SetParameter(ComponentHandle, nParamIndex, pStructure);
    };

    return ret;
}

OMX_ERRORTYPE FSLOMXWrapper::GetExtensionIndex(
        OMX_STRING cParameterName, 
        OMX_INDEXTYPE* pIndexType)
{
    if(!strcmp(cParameterName, "OMX.google.android.index.enableAndroidNativeBuffers"))
        *pIndexType = OMX_IndexParamEnableAndroidNativeBuffers;
    else if(!strcmp(cParameterName, "OMX.google.android.index.getAndroidNativeBufferUsage"))
        *pIndexType = OMX_IndexParamNativeBufferUsage;
    else if(!strcmp(cParameterName, "OMX.google.android.index.storeMetaDataInBuffers"))
        *pIndexType = OMX_IndexParamStoreMetaDataInBuffers;
    else if(!strcmp(cParameterName, "OMX.google.android.index.useAndroidNativeBuffer"))
        *pIndexType = OMX_IndexParamUseAndroidNativeBuffer;
    else
        return OMX_GetExtensionIndex(ComponentHandle, cParameterName, pIndexType);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FSLOMXWrapper::FreeBuffer(
        OMX_U32 nPortIndex, 
        OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    for(OMX_S32 i=0; i<MAX_BUFFER_CNT; i++) {
        if(pBufferHdr == sBufferMapper[i].pBufferHdr) {
            sBufferMapper[i].mGraphicBuffer->unlock();
            RemoveHwBuffer(pBufferHdr->pBuffer);
            sBufferMapper[i].mGraphicBuffer = NULL;
            memset(&sBufferMapper[i], 0, sizeof(BufferMapper));
            nBufferCnt--;
            break;
        }
    }

    OMX_FreeBuffer(ComponentHandle, nPortIndex, pBufferHdr);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FSLOMXWrapper::DoUseNativeBuffer(
        UseAndroidNativeBufferParams *pNativBufferParam)
{
    if(pNativBufferParam == NULL || pNativBufferParam->nativeBuffer == NULL
            || pNativBufferParam->pAppPrivate == NULL)
        return OMX_ErrorBadParameter;


    GraphicBuffer *pGraphicBuffer = static_cast<GraphicBuffer*>(pNativBufferParam->nativeBuffer.get());
    private_handle_t *prvHandle = (private_handle_t*)pGraphicBuffer->getNativeBuffer()->handle;

    OMX_PTR vAddr = NULL;
    pGraphicBuffer->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, &vAddr);
    if(vAddr == NULL) {
        LOGE("Failed to get native buffer virtual address.\n");
        return OMX_ErrorUndefined;
    }

    LOGV("native buffer handle %p, phys %p, virs %p, size %d\n", prvHandle, (OMX_PTR)prvHandle->phys, vAddr, prvHandle->size);

    AddHwBuffer((OMX_PTR)prvHandle->phys, vAddr);

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    ret = OMX_UseBuffer(ComponentHandle, pNativBufferParam->bufferHeader, 
            pNativBufferParam->nPortIndex, pNativBufferParam->pAppPrivate, 
            prvHandle->size, (OMX_U8*)vAddr);
    if(ret != OMX_ErrorNone) {
        RemoveHwBuffer(vAddr);
        pGraphicBuffer->unlock();
        LOGE("Failed to use native buffer.\n");
        return ret;
    }

    sBufferMapper[nBufferCnt].pBufferHdr = *pNativBufferParam->bufferHeader;
    sBufferMapper[nBufferCnt].mGraphicBuffer = pGraphicBuffer;
    nBufferCnt ++;

    return OMX_ErrorNone;
}

// FSLOMXPlugin class

OMX_ERRORTYPE FSLOMXPlugin::makeComponentInstance(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOGV("makeComponentInstance, appData: %p", appData);

    OMX_HANDLETYPE handle = NULL;
    ret = OMX_GetHandle(&handle, (char*)name, appData, (OMX_CALLBACKTYPE*)callbacks);
    if(ret != OMX_ErrorNone)
        return ret;

    FSLOMXWrapper *pWrapper = new FSLOMXWrapper;
    if(pWrapper == NULL)
        return OMX_ErrorInsufficientResources;

    *component = pWrapper->MakeWapper(handle);
    if(*component == NULL)
        return OMX_ErrorUndefined;

    LOGV("makeComponentInstance done, instance is: %p", *component);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FSLOMXPlugin::destroyComponentInstance(
            OMX_COMPONENTTYPE *component)
{
    LOGV("destroyComponentInstance, %p", component);

    FSLOMXWrapper *pWrapper = (FSLOMXWrapper*)component->pComponentPrivate;
    OMX_FreeHandle(pWrapper->GetComponentHandle());
    delete pWrapper;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FSLOMXPlugin::enumerateComponents(
            OMX_STRING name,
            size_t size,
            OMX_U32 index)
{
    return OMX_ComponentNameEnum(name, size, index);
}

OMX_ERRORTYPE FSLOMXPlugin::getRolesOfComponent(
            const char *name,
            Vector<String8> *roles)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_U32 numRoles;

    LOGV("Call getRolesOfComponent.\n");

    roles->clear();
    ret = OMX_GetRolesOfComponent((char*)name, &numRoles, NULL);
    if(ret != OMX_ErrorNone)
        return ret;

    if (numRoles > 0) {
        OMX_U8 **array = new OMX_U8 *[numRoles];
        OMX_S32 i;
        for (i = 0; i   < (OMX_S32)numRoles;   ++i) {
            array[i] = new OMX_U8[OMX_MAX_STRINGNAME_SIZE];
        }

        OMX_U32 numRoles2;
        ret = OMX_GetRolesOfComponent((char*)name, &numRoles2, array);
        if(ret != OMX_ErrorNone)
            return ret;

        for (i = 0; i < (OMX_S32)numRoles; ++i) {
            String8 s((const char   *)array[i]);
            roles->push(s);
            delete[] array[i];
            array[i] = NULL;
        }

        delete[] array;
        array   =   NULL;
    }

    return OMX_ErrorNone;
}

OMXPluginBase* createOMXPlugin()
{
    LOGV("createOMXPlugin");
    return (new FSLOMXPlugin());
}

}
