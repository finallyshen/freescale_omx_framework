ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	vpu_enc_test.cpp
		
LOCAL_CFLAGS += $(FSL_OMX_CFLAGS) -DDIVXINT_USE_STDINT
 
LOCAL_C_INCLUDES += $(FSL_OMX_INCLUDES)
LOCAL_SHARED_LIBRARIES := lib_omx_common_v2_arm11_elinux \
                          lib_omx_osal_v2_arm11_elinux \
			  lib_omx_utils_v2_arm11_elinux \
			  lib_aac_parser_arm11_elinux \
			  lib_aac_dec_v2_arm12_elinux


LOCAL_SHARED_LIBRARIES := lib_omx_osal_v2_arm11_elinux \
	lib_omx_utils_v2_arm11_elinux \
	lib_omx_core_mgr_v2_arm11_elinux
	
LOCAL_MODULE:= omx_vpu_enc_test_arm11_elinux

LOCAL_MODULE_TAGS := eng
include $(BUILD_EXECUTABLE)

endif
endif
