ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)

# LOCAL_PATH will be changed by each Android.mk under this. So save it firstly
FSL_OMX_PATH := $(LOCAL_PATH)

include $(CLEAR_VARS)
FSL_OMX_CFLAGS := -DANDROID_BUILD -D_POSIX_SOURCE -DOMX_STEREO_OUTPUT -UDOMX_MEM_CHECK -Werror

FSL_OMX_LDFLAGS := -Wl,--fatal-warnings

FSL_OMX_INCLUDES := \
	$(LOCAL_PATH)/OSAL/ghdr \
	$(LOCAL_PATH)/utils \
	$(LOCAL_PATH)/utils/colorconvert/include \
	$(LOCAL_PATH)/lib_ffmpeg \
	$(LOCAL_PATH)/lib_ffmpeg/libavformat \
	$(LOCAL_PATH)/lib_ffmpeg/libavcodec \
	$(LOCAL_PATH)/lib_ffmpeg/libavutil \
	$(LOCAL_PATH)/OpenMAXIL/ghdr \
	$(LOCAL_PATH)/OpenMAXIL/src/core_mgr \
	$(LOCAL_PATH)/OpenMAXIL/src/core \
	$(LOCAL_PATH)/OpenMAXIL/src/content_pipe \
	$(LOCAL_PATH)/OpenMAXIL/src/resource_mgr \
	$(LOCAL_PATH)/OpenMAXIL/src/graph_manager_interface \
	$(LOCAL_PATH)/OpenMAXIL/src/client \
	$(LOCAL_PATH)/OpenMAXIL/src/component/common \
	$(LOCAL_PATH)/../linux-lib/ipu \
	$(LOCAL_PATH)/../linux-lib/vpu \
	$(LOCAL_PATH)/../../frameworks/base/include \
	$(LOCAL_PATH)/../../frameworks/base/include/media \
	$(LOCAL_PATH)/../../frameworks/base/include/utils \
	$(LOCAL_PATH)/../../frameworks/base/include/surfaceflinger \
	$(LOCAL_PATH)/../../frameworks/base/media/libmediaplayerservice \
	$(LOCAL_PATH)/../../system/core/include 


# Check Android Version
ifeq ($(findstring x2.2,x$(PLATFORM_VERSION)), x2.2)
    FSL_OMX_CFLAGS += -DFROYO
endif

ifeq ($(findstring x2.3,x$(PLATFORM_VERSION)), x2.3)
    FSL_OMX_CFLAGS += -DGINGER_BREAD -DMEDIA_SCAN_2_3_3_API
    FSL_OMX_INCLUDES += $(LOCAL_PATH)/../../frameworks/base/media/libstagefright/include \
			$(LOCAL_PATH)/../../device/fsl/proprietary/codec/ghdr \
			$(LOCAL_PATH)/../../device/fsl/proprietary/codec/ghdr/common \
			$(LOCAL_PATH)/../../device/fsl/proprietary/codec/ghdr/wma10
else
    FSL_OMX_INCLUDES += $(LOCAL_PATH)/../../device/fsl-proprietary/codec/ghdr \
			$(LOCAL_PATH)/../../device/fsl-proprietary/codec/ghdr/common \
			$(LOCAL_PATH)/../../device/fsl-proprietary/codec/ghdr/wma10
endif

ifeq ($(findstring x3.,x$(PLATFORM_VERSION)), x3.)
    FSL_OMX_CFLAGS += -DHONEY_COMB -DMEDIA_SCAN_2_3_3_API -DHWC_RENDER
    FSL_OMX_INCLUDES += $(LOCAL_PATH)/../../frameworks/base/media/libstagefright/include \
	$(LOCAL_PATH)/../../frameworks/base/include/ui/egl
    ifeq ($(TARGET_BOARD_PLATFORM), imx5x)
        FSL_OMX_INCLUDES += $(LOCAL_PATH)/../../hardware/imx/mx5x/libgralloc
    endif
    ifeq ($(TARGET_BOARD_PLATFORM), imx6)
        FSL_OMX_INCLUDES += $(LOCAL_PATH)/../../hardware/imx/mx6/libgralloc
    endif
endif

ifeq ($(findstring x4.,x$(PLATFORM_VERSION)), x4.)
    FSL_OMX_CFLAGS += -DHONEY_COMB -DICS -DMEDIA_SCAN_2_3_3_API
    FSL_OMX_INCLUDES += $(LOCAL_PATH)/../../frameworks/base/media/libstagefright/include \
	$(LOCAL_PATH)/../../frameworks/base/include/ui/egl
    ifeq ($(TARGET_BOARD_PLATFORM), imx5x)
        FSL_OMX_INCLUDES += $(LOCAL_PATH)/../../hardware/imx/mx5x/libgralloc
        FSL_OMX_CFLAGS += -DMX5X
    endif
    ifeq ($(TARGET_BOARD_PLATFORM), imx6)
        FSL_OMX_INCLUDES += $(LOCAL_PATH)/../../hardware/imx/mx6/libgralloc_wrapper
	FSL_OMX_CFLAGS += -DMX6X
    endif
endif

include $(FSL_OMX_PATH)/OpenMAXIL/src/component/vpu_wrapper/Android.mk
include $(FSL_OMX_PATH)/utils/id3_parser/Android.mk

include $(FSL_OMX_PATH)/OSAL/linux/Android.mk
include $(FSL_OMX_PATH)/utils/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/resource_mgr/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/core_mgr/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/core/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/content_pipe/local_file_pipe/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/content_pipe/shared_fd_pipe/Android.mk
#include $(FSL_OMX_PATH)/OpenMAXIL/src/content_pipe/https_pipe/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/content_pipe/https_pipe_v2/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/common/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/clock/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/fsl_parser/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/streaming_parser/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/fsl_muxer/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/mp3_parser/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/flac_parser/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/aac_parser/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/wav_parser/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/mp3_dec/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/mp3_enc/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/amr_dec/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/amr_enc/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/flac_dec/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/aac_dec/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/vorbis_dec/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/pcm_dec/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/audio_processor/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/android_audio_render/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/android_audio_source/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/audio_fake_render/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/sorenson_dec/Android.mk
ifeq ($(findstring x2.3,x$(PLATFORM_VERSION)), x2.3)
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/overlay_render/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/camera_source/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/aac_enc/Android.mk
endif
ifeq ($(findstring x3.,x$(PLATFORM_VERSION)), x3.)
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/camera_source/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/aac_enc/Android.mk
endif
ifeq ($(findstring x4.,x$(PLATFORM_VERSION)), x4.)
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/camera_source/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/aac_enc/Android.mk
endif
ifeq ($(BOARD_HAVE_VPU),true)
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/vpu_dec_v2/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/vpu_enc/Android.mk
include $(FSL_OMX_PATH)/stagefright/Android.mk
endif
ifeq ($(HAVE_FSL_IMX_IPU),true)
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/ipulib_render/Android.mk
endif
include $(FSL_OMX_PATH)/OpenMAXIL/src/client/Android.mk
#include $(FSL_OMX_PATH)/OpenMAXIL/test/OMX_GraphManager/Android.mk
#include $(FSL_OMX_PATH)/OpenMAXIL/test/vpu_test/Android.mk
#include $(FSL_OMX_PATH)/OpenMAXIL/test/vpu_enc_test/Android.mk
include $(FSL_OMX_PATH)/Android/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/release/registry/Android.mk

include $(FSL_OMX_PATH)/OpenMAXIL/src/component/wma_dec/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/wmv_dec/Android.mk
#include $(FSL_OMX_PATH)/OpenMAXIL/src/component/ac3_parser/Android.mk
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/ac3_dec/Android.mk

include $(FSL_OMX_PATH)/lib_ffmpeg/Android.mk
#include $(FSL_OMX_PATH)/lib_ffmpeg/test/Android.mk

ifeq ($(findstring x3.,x$(PLATFORM_VERSION)), x3.)
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/surface_render/Android.mk
endif

ifeq ($(findstring x4.,x$(PLATFORM_VERSION)), x4.)
include $(FSL_OMX_PATH)/OpenMAXIL/src/component/surface_render/Android.mk
endif

endif
endif
