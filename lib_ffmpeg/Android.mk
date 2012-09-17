ifeq ($(HAVE_FSL_IMX_CODEC),true)
ifneq ($(PREBUILT_FSL_IMX_OMX),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LIBAVFORMAT_OBJS = libavformat/allformats.c         	\
		    libavformat/cutils.c             	\
		    libavformat/id3v1.c              	\
		    libavformat/id3v2.c              	\
		    libavformat/metadata.c           	\
		    libavformat/metadata_compat.c    	\
		    libavformat/options.c            	\
		    libavformat/os_support.c         	\
		    libavformat/sdp.c                	\
		    libavformat/seek.c               	\
		    libavformat/utils.c              	\
		    libavformat/rmdec.c 	     	\
		    libavformat/rm.c			\
		    libavformat/mov.c 		     	\
		    libavformat/riff.c 	     		\
		    libavformat/isom.c			\
		    libavformat/asfdec.c 	     	\
		    libavformat/asf.c 		     	\
		    libavformat/asfcrypt.c 	     	\
		    libavformat/avlanguage.c		\
		    libavformat/mpegts.c 	     	\
		    libavformat/rtp.c         		\
		    libavformat/rtpenc_aac.c     	\
		    libavformat/rtpenc_amr.c     	\
		    libavformat/rtpenc_h263.c    	\
		    libavformat/rtpenc_mpv.c     	\
		    libavformat/rtpenc.c      		\
		    libavformat/rtpenc_h264.c 		\
		    libavformat/rtpenc_vp8.c  		\
		    libavformat/rtpenc_xiph.c 		\
		    libavformat/avc.c			\
		    libavformat/rdt.c         		\
		    libavformat/rtpdec.c      		\
		    libavformat/rtpdec_amr.c  		\
		    libavformat/rtpdec_asf.c  		\
		    libavformat/rtpdec_h263.c 		\
		    libavformat/rtpdec_h264.c 		\
		    libavformat/rtpdec_latm.c 		\
		    libavformat/rtpdec_mpeg4.c		\
		    libavformat/rtpdec_qcelp.c		\
		    libavformat/rtpdec_qdm2.c 		\
		    libavformat/rtpdec_qt.c   		\
		    libavformat/rtpdec_svq3.c 		\
		    libavformat/rtpdec_vp8.c  		\
		    libavformat/rtpdec_xiph.c		\
		    libavformat/rtsp.c 			\
		    libavformat/rtspdec.c 		\
		    libavformat/httpauth.c		\
		    libavformat/rtspenc.c 		\
		    libavformat/rtpenc_chain.c		\
		    libavformat/sapdec.c		\
		    libavformat/sapenc.c 		\
		    libavformat/avio.c 			\
		    libavformat/aviobuf.c		\
		    libavformat/applehttp.c		\
		    libavformat/applehttpproto.c	\
		    libavformat/http.c	 		\
		    libavformat/mmsh.c	 		\
		    libavformat/mms.c 			\
		    libavformat/mmst.c 			\
		    libavformat/rtmpproto.c 		\
		    libavformat/rtmppkt.c		\
		    libavformat/rtpproto.c		\
		    libavformat/tcp.c			\
		    libavformat/udp.c			\

LIBAVCODEC_OBJS= libavcodec/avpacket.c          	\
		 libavcodec/utils.c			\
		 libavcodec/bitstream_filter.c		\
		 libavcodec/parser.c			\
		 libavcodec/options.c			\
		 libavcodec/xiph.c			\
		 libavcodec/raw.c			\
		 libavcodec/imgconvert.c		\
		 libavcodec/missing.c			\

LIBAVUTIL_OBJS= libavutil/audioconvert.c                \
		libavutil/avstring.c                    \
		libavutil/base64.c                      \
		libavutil/crc.c                         \
		libavutil/des.c                         \
		libavutil/eval.c                        \
		libavutil/imgutils.c                    \
		libavutil/intfloat_readwrite.c          \
		libavutil/lfg.c                         \
		libavutil/log.c                         \
		libavutil/mathematics.c                 \
		libavutil/md5.c                         \
		libavutil/mem.c                         \
		libavutil/opt.c                         \
		libavutil/parseutils.c                  \
		libavutil/pixdesc.c                     \
		libavutil/random_seed.c                 \
		libavutil/rational.c                    \
		libavutil/rc4.c                         \
		libavutil/samplefmt.c                   \
		libavutil/sha.c                         \

LOCAL_SRC_FILES := $(LIBAVFORMAT_OBJS) $(LIBAVCODEC_OBJS) $(LIBAVUTIL_OBJS)

LOCAL_CFLAGS += -DANDROID_BUILD -Werror

LOCAL_SHARED_LIBRARIES := libutils libc

LOCAL_PRELINK_MODULE := false
	
LOCAL_MODULE:= lib_ffmpeg_arm11_elinux
LOCAL_MODULE_TAGS := eng
include $(BUILD_SHARED_LIBRARY)

endif
endif
