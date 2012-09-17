LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libstagefrighthw

LOCAL_ARM_MODE := arm

LOCAL_LDFLAGS += $(FSL_OMX_LDFLAGS)

LOCAL_C_INCLUDES := $(FSL_OMX_INCLUDES)

ifeq ($(findstring x2.3,x$(PLATFORM_VERSION)), x2.3)
LOCAL_SRC_FILES := src/OMXFSLPlugin.cpp \
    		   src/FSLRenderer.cpp

LOCAL_SHARED_LIBRARIES := \
        libbinder         \
        libmedia          \
        libutils          \
        libcutils         \
        libsurfaceflinger_client \
	libdl \
	libipu lib_omx_res_mgr_v2_arm11_elinux
endif

ifeq ($(findstring x3.,x$(PLATFORM_VERSION)), x3.)
LOCAL_SRC_FILES := src/OMXFSLPlugin_new.cpp

LOCAL_SHARED_LIBRARIES := \
    	lib_omx_core_v2_arm11_elinux \
	lib_omx_res_mgr_v2_arm11_elinux \
	libui libutils libcutils
endif

ifeq ($(findstring x4.,x$(PLATFORM_VERSION)), x4.)
LOCAL_SRC_FILES := src/OMXFSLPlugin_new.cpp

LOCAL_SHARED_LIBRARIES := \
    	lib_omx_core_v2_arm11_elinux \
	lib_omx_res_mgr_v2_arm11_elinux \
	libui libutils libcutils
endif

include $(BUILD_SHARED_LIBRARY)

