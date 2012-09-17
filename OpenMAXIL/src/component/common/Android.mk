ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	ComponentBase.cpp \
        ExecutingState.cpp \
        IdleState.cpp \
        InvalidState.cpp \
        LoadedState.cpp \
        PauseState.cpp \
        Port.cpp \
        State.cpp \
        WaitForResourcesState.cpp \
        AudioParserBase.cpp \
        AudioFilter.cpp \
        AudioRender.cpp \
	Parser.cpp \
	VideoFilter.cpp \
	VideoSource.cpp \
	AudioSource.cpp \
	Muxer.cpp \
	VideoRender.cpp

		
LOCAL_CFLAGS += $(FSL_OMX_CFLAGS)

LOCAL_LDFLAGS += $(FSL_OMX_LDFLAGS)
 
LOCAL_C_INCLUDES += $(FSL_OMX_INCLUDES) 

LOCAL_SHARED_LIBRARIES := lib_omx_osal_v2_arm11_elinux \
    			  lib_omx_utils_v2_arm11_elinux \
				  lib_omx_res_mgr_v2_arm11_elinux \
				  lib_omx_core_v2_arm11_elinux \
				  lib_id3_parser_arm11_elinux \
				  libutils 

LOCAL_PRELINK_MODULE := false
	
LOCAL_MODULE:= lib_omx_common_v2_arm11_elinux
LOCAL_MODULE_TAGS := eng
include $(BUILD_SHARED_LIBRARY)

endif
endif
