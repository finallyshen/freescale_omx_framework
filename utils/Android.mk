ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
SOURCE_FILES = \
	       Mem.cpp \
	       Queue.cpp \
	       RegistryAnalyser.cpp \
	       RingBuffer.cpp \
	       FadeInFadeOut.cpp \
	       ShareLibarayMgr.cpp \
	       mfw_gst_ts.c \
	       Tsm_wrapper.c \
               colorconvert/src/cczoomrotation16.cpp \
               colorconvert/src/cczoomrotationbase.cpp 
		
LOCAL_CFLAGS += $(FSL_OMX_CFLAGS)

LOCAL_LDFLAGS += $(FSL_OMX_LDFLAGS)
 
LOCAL_C_INCLUDES += $(FSL_OMX_INCLUDES) \
		    $(LOCAL_PATH)/colorconvert/include

LOCAL_SHARED_LIBRARIES := libdl lib_omx_osal_v2_arm11_elinux

LOCAL_PRELINK_MODULE := false
	
LOCAL_MODULE:= lib_omx_utils_v2_arm11_elinux
LOCAL_MODULE_TAGS := eng
include $(BUILD_SHARED_LIBRARY)

endif
endif
