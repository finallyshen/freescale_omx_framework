/**
 *  Copyright (c) 2010-2012, Freescale Semiconductor Inc.,
 *  All Rights Reserved.
 *
 *  The following programs are the sole property of Freescale Semiconductor Inc.,
 *  and contain its proprietary and confidential information.
 *
 */
 
#define LOG_TAG "FSLOMXPlugin"

#include <media/stagefright/OMXPluginBase.h>
#include <media/stagefright/MediaDebug.h>
#include <cutils/log.h>      
#include <dlfcn.h>
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Audio.h"
#include "OMX_Image.h"
#include "OMX_Video.h"
#include "OMX_Other.h"


typedef OMX_ERRORTYPE   OMX_APIENTRY (*tpOMX_Init)(void);
typedef OMX_ERRORTYPE   OMX_APIENTRY (*tpOMX_Deinit)(void);
typedef OMX_ERRORTYPE   OMX_APIENTRY (*tpOMX_ComponentNameEnum)(
        OMX_OUT OMX_STRING cComponentName,
        OMX_IN  OMX_U32 nNameLength,
        OMX_IN  OMX_U32 nIndex);

typedef OMX_ERRORTYPE   OMX_APIENTRY (*tpOMX_GetHandle)(
        OMX_OUT OMX_HANDLETYPE* pHandle, 
        OMX_IN  OMX_STRING cComponentName,
        OMX_IN  OMX_PTR pAppData,
        OMX_IN  OMX_CALLBACKTYPE*   pCallBacks);

typedef OMX_ERRORTYPE   OMX_APIENTRY (*tpOMX_FreeHandle)(
        OMX_IN  OMX_HANDLETYPE hComponent);

typedef OMX_ERRORTYPE   OMX_APIENTRY (*tpOMX_SetupTunnel)(
        OMX_IN  OMX_HANDLETYPE hOutput,
        OMX_IN  OMX_U32 nPortOutput,
        OMX_IN  OMX_HANDLETYPE hInput,
        OMX_IN  OMX_U32 nPortInput);

typedef OMX_ERRORTYPE       (*tpOMX_GetContentPipe)(
        OMX_OUT OMX_HANDLETYPE *hPipe,
        OMX_IN OMX_STRING   szURI);

typedef OMX_ERRORTYPE   (*tpOMX_GetComponentsOfRole) ( 
        OMX_IN          OMX_STRING role,
        OMX_INOUT       OMX_U32 *pNumComps,
        OMX_INOUT       OMX_U8  **compNames);

typedef OMX_ERRORTYPE   (*tpOMX_GetRolesOfComponent)(   
        OMX_IN          OMX_STRING compName, 
        OMX_INOUT       OMX_U32 *pNumRoles,
        OMX_OUT         OMX_U8 **roles);

#define OMX_CORE_LIBRARY "lib_omx_core_v2_arm11_elinux.so"

class   FSLOMXInterface{
    public:
        void* pShareLibHandle;
        FSLOMXInterface()
        {
            pShareLibHandle = dlopen(OMX_CORE_LIBRARY, RTLD_NOW);
            if (NULL == pShareLibHandle)
            {       
                pOMX_Init   =   NULL;
                pOMX_Deinit =   NULL;
                pOMX_ComponentNameEnum = NULL;
                pOMX_GetHandle = NULL;
                pOMX_FreeHandle =   NULL;
                pOMX_GetComponentsOfRole = NULL;
                pOMX_GetRolesOfComponent = NULL;
                pOMX_SetupTunnel = NULL;
                pOMX_GetContentPipe =   NULL;
                const char* lError = dlerror();
                if (lError)
                {
                    LOGE("Error: %s\n", lError);
                }

                LOGE("LoadLibrary %s failed\n",OMX_CORE_LIBRARY);        
            }
            else
            {
                pOMX_Init   =   (tpOMX_Init)dlsym(pShareLibHandle, "OMX_Init");
                pOMX_Deinit =   (tpOMX_Deinit)dlsym(pShareLibHandle, "OMX_Deinit");
                pOMX_ComponentNameEnum = (tpOMX_ComponentNameEnum)dlsym(pShareLibHandle, "OMX_ComponentNameEnum");
                pOMX_GetHandle = (tpOMX_GetHandle)dlsym(pShareLibHandle, "OMX_GetHandle");
                pOMX_FreeHandle =   (tpOMX_FreeHandle)dlsym(pShareLibHandle, "OMX_FreeHandle");
                pOMX_GetComponentsOfRole = (tpOMX_GetComponentsOfRole)dlsym(pShareLibHandle, "OMX_GetComponentsOfRole");
                pOMX_GetRolesOfComponent = (tpOMX_GetRolesOfComponent)dlsym(pShareLibHandle, "OMX_GetRolesOfComponent");
                pOMX_SetupTunnel = (tpOMX_SetupTunnel)dlsym(pShareLibHandle, "OMX_SetupTunnel");
                pOMX_GetContentPipe =   (tpOMX_GetContentPipe)dlsym(pShareLibHandle, "OMX_GetContentPipe");
            }

        };   

        ~FSLOMXInterface()
        {
            if ( pShareLibHandle && (0 !=   dlclose(pShareLibHandle)))
            {
                LOGE("UnloadLibrary %s failed\n",OMX_CORE_LIBRARY);
            }
            pShareLibHandle =   NULL;
        };
        
        tpOMX_Init                          pOMX_Init;
        tpOMX_Deinit                        pOMX_Deinit;
        tpOMX_ComponentNameEnum             pOMX_ComponentNameEnum;
        tpOMX_GetHandle                     pOMX_GetHandle;
        tpOMX_FreeHandle                    pOMX_FreeHandle;
        tpOMX_GetComponentsOfRole           pOMX_GetComponentsOfRole;
        tpOMX_GetRolesOfComponent           pOMX_GetRolesOfComponent;
        tpOMX_SetupTunnel                   pOMX_SetupTunnel;
        tpOMX_GetContentPipe                pOMX_GetContentPipe;

};


namespace   android{
class   OMXFSLPlugin : public   OMXPluginBase   {
    public:
        OMXFSLPlugin();
        virtual ~OMXFSLPlugin();

        virtual OMX_ERRORTYPE   makeComponentInstance(
                const   char *name,
                const   OMX_CALLBACKTYPE *callbacks,
                OMX_PTR appData,
                OMX_COMPONENTTYPE   **component);

        virtual OMX_ERRORTYPE   destroyComponentInstance(
                OMX_COMPONENTTYPE   *component);

        virtual OMX_ERRORTYPE   enumerateComponents(
                OMX_STRING name,
                size_t size,
                OMX_U32 index);

        virtual OMX_ERRORTYPE   getRolesOfComponent(
                const   char *name,
                Vector<String8> *roles);
            OMXPluginBase* Instance();
    private:
            FSLOMXInterface *pFSLOMX;

    };
    
    OMXFSLPlugin::OMXFSLPlugin() {
        pFSLOMX =   new FSLOMXInterface();
        if(pFSLOMX &&   pFSLOMX->pOMX_Init)
            pFSLOMX->pOMX_Init();
    }

    OMXFSLPlugin::~OMXFSLPlugin()   {
        if(pFSLOMX){
            if( pFSLOMX->pOMX_Deinit)
                pFSLOMX->pOMX_Deinit();
            delete  pFSLOMX;
            pFSLOMX =   NULL;
        }
    }


    OMXPluginBase* OMXFSLPlugin::Instance()
    {
        return reinterpret_cast<OMXPluginBase *> (new OMXFSLPlugin());
    };

    OMX_ERRORTYPE OMXFSLPlugin::makeComponentInstance(
        const   char *name,
        const   OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE   **component) {
        if(pFSLOMX &&   pFSLOMX->pOMX_GetHandle){
            return pFSLOMX->pOMX_GetHandle(
        reinterpret_cast<OMX_HANDLETYPE *>(component),
        const_cast<char *>(name),
        appData,
        const_cast<OMX_CALLBACKTYPE *>(callbacks));     
        }else{
            return OMX_ErrorNotImplemented;
        }
    
        return OMX_ErrorNotImplemented;
    }

    OMX_ERRORTYPE   OMXFSLPlugin::destroyComponentInstance(
        OMX_COMPONENTTYPE   *component) {
        if(pFSLOMX &&   pFSLOMX->pOMX_FreeHandle){
            return pFSLOMX->pOMX_FreeHandle(reinterpret_cast<OMX_HANDLETYPE *>(component));
        }else{
            return OMX_ErrorNotImplemented;
        }
    }

    OMX_ERRORTYPE   OMXFSLPlugin::enumerateComponents(
        OMX_STRING name,
        size_t size,
        OMX_U32 index) {
        if(pFSLOMX  && pFSLOMX->pOMX_ComponentNameEnum){
            return  pFSLOMX->pOMX_ComponentNameEnum(name, size, index);
        }else{
            return OMX_ErrorNotImplemented;
        }
        return OMX_ErrorNotImplemented;
    }

    OMX_ERRORTYPE   OMXFSLPlugin::getRolesOfComponent(
        const   char *name,
        Vector<String8> *roles) {
        if( NULL== pFSLOMX ||   NULL ==pFSLOMX->pOMX_GetRolesOfComponent){
            return OMX_ErrorNotImplemented;
        }   

        roles->clear();
        OMX_U32 numRoles;
        OMX_ERRORTYPE   err =
            pFSLOMX->pOMX_GetRolesOfComponent(
                const_cast<char *>(name),
                &numRoles,
                NULL);
        if (err != OMX_ErrorNone) {
            return err;
        }

        if (numRoles > 0) {
            OMX_U8 **array = new OMX_U8 *[numRoles];
            for (OMX_U32 i = 0; i   <   numRoles;   ++i) {
                array[i] = new OMX_U8[OMX_MAX_STRINGNAME_SIZE];
            }
            OMX_U32 numRoles2;
            err =   pFSLOMX->pOMX_GetRolesOfComponent(
                const_cast<char *>(name),   &numRoles2, array);
            if (err != OMX_ErrorNone) {
                return err;
            }

            for (OMX_U32 i = 0; i < numRoles; ++i) {
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
        return reinterpret_cast<OMXPluginBase *> (new OMXFSLPlugin());
    }
}

