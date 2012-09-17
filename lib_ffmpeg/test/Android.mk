ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := test.c
		
LOCAL_CFLAGS += $(FSL_OMX_CFLAGS)

LOCAL_LDFLAGS += $(FSL_OMX_LDFLAGS)
 
LOCAL_C_INCLUDES += $(FSL_OMX_INCLUDES)

LOCAL_SHARED_LIBRARIES := lib_ffmpeg_arm11_elinux
	
LOCAL_MODULE:= omx_stack_test_arm11_elinux
LOCAL_MODULE_TAGS := eng
include $(BUILD_EXECUTABLE)

endif
endif
