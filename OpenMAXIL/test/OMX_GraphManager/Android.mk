ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gm_app.c
		
LOCAL_CFLAGS += $(FSL_OMX_CFLAGS) -DDIVXINT_USE_STDINT
 
LOCAL_C_INCLUDES += $(FSL_OMX_INCLUDES)

LOCAL_SHARED_LIBRARIES := lib_omx_osal_v2_arm11_elinux \
                          lib_omx_client_arm11_elinux
	
LOCAL_MODULE:= omx_graph_manager_arm11_elinux

include $(BUILD_EXECUTABLE)

endif
endif
