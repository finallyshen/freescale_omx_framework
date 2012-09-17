ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_PLATFORM), imx6)
LOCAL_SRC_FILES := \
	IpulibRender_mx6x.cpp
else
LOCAL_SRC_FILES := \
	IpulibRender.cpp
endif
		
LOCAL_CFLAGS += $(FSL_OMX_CFLAGS)

LOCAL_LDFLAGS += $(FSL_OMX_LDFLAGS)
 
LOCAL_C_INCLUDES += $(FSL_OMX_INCLUDES)

LOCAL_SHARED_LIBRARIES := lib_omx_common_v2_arm11_elinux \
                          lib_omx_osal_v2_arm11_elinux \
                          lib_omx_utils_v2_arm11_elinux \
			  lib_omx_res_mgr_v2_arm11_elinux \
			  libipu libcutils

LOCAL_PRELINK_MODULE := false
	
LOCAL_MODULE:= lib_omx_ipulib_render_arm11_elinux
LOCAL_MODULE_TAGS := eng
include $(BUILD_SHARED_LIBRARY)

endif
endif
