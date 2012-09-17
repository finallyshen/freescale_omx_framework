ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        GMPlayerWrapper.cpp \
	GMPlayer.cpp \
	OMX_MetadataExtractor.cpp \
	OMX_Recorder.cpp \
	GMRecorder.cpp \
	GMComponent.cpp \
	MediaInspector.cpp \
	PMemory.cpp \
	image_convert.c
		
LOCAL_CFLAGS += $(FSL_OMX_CFLAGS) 

LOCAL_LDFLAGS += $(FSL_OMX_LDFLAGS)
 
LOCAL_C_INCLUDES += $(FSL_OMX_INCLUDES) 

LOCAL_SHARED_LIBRARIES := lib_omx_osal_v2_arm11_elinux \
                          lib_omx_utils_v2_arm11_elinux \
			  lib_omx_core_mgr_v2_arm11_elinux \
			  lib_omx_res_mgr_v2_arm11_elinux \
			  libfsl_jpeg_enc_arm11_elinux \
			  libipu

LOCAL_PRELINK_MODULE := false
	
LOCAL_MODULE:= lib_omx_client_arm11_elinux
LOCAL_MODULE_TAGS := eng
include $(BUILD_SHARED_LIBRARY)

endif
endif
