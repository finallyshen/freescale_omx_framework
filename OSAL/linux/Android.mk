ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        	fsl_osal_linux_file.cpp \
		fsl_osal_linux_mem.cpp \
		fsl_osal_linux_string.cpp \
		fsl_osal_linux_mutex.cpp \
		fsl_osal_linux_sem.cpp \
		fsl_osal_linux_thread.cpp \
		fsl_osal_linux_time.cpp \
		fsl_osal_linux_condition.cpp \
		../logger/Log.cpp
		
LOCAL_CFLAGS += $(FSL_OMX_CFLAGS)

LOCAL_LDFLAGS += $(FSL_OMX_LDFLAGS)
 
LOCAL_C_INCLUDES += $(FSL_OMX_INCLUDES) 

LOCAL_SHARED_LIBRARIES := libutils libc

LOCAL_PRELINK_MODULE := false
	
LOCAL_MODULE:= lib_omx_osal_v2_arm11_elinux
LOCAL_MODULE_TAGS := eng
include $(BUILD_SHARED_LIBRARY)

endif
endif
