ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := core_register
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := core_register
LOCAL_MODULE_TAGS := eng
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := component_register
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := component_register
LOCAL_MODULE_TAGS := eng
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := contentpipe_register
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := contentpipe_register
LOCAL_MODULE_TAGS := eng
include $(BUILD_PREBUILT)

endif
endif



