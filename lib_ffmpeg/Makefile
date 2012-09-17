#####################################################################################
#
#   Copyright (c) 2009, Freescale Semiconductors Inc.,
#   All Rights Reserved.
# 
#   The following programs are the sole property of Freescale Semiconductors Inc.,
#   and contain its proprietary and confidential information.
# 
####################################################################################### 
#
#   This file will compile all cpp file and build dynimic library. 
#
######################################################################################


# Set the root directory
OMX_ROOT_DIR=..

# Set the BUILD information
include $(OMX_ROOT_DIR)/Makefile.defines

FFMPEG_SO_NAME= lib_ffmpeg_$(PROCESSOR)_$(OSTYPE).so

LFLAGS+= -l$(OSAL_LIB_NAME) -l$(UTILS_LIB_NAME)


LIBAVFORMAT_OBJS = libavformat/allformats.o         	\
		    libavformat/cutils.o             	\
		    libavformat/id3v1.o              	\
		    libavformat/id3v2.o              	\
		    libavformat/metadata.o           	\
		    libavformat/metadata_compat.o    	\
		    libavformat/options.o            	\
		    libavformat/os_support.o         	\
		    libavformat/sdp.o                	\
		    libavformat/seek.o               	\
		    libavformat/utils.o              	\
		    libavformat/rmdec.o 	     	\
		    libavformat/rm.o			\
		    libavformat/mov.o 		     	\
		    libavformat/riff.o 	     		\
		    libavformat/isom.o			\
		    libavformat/asfdec.o 	     	\
		    libavformat/asf.o 		     	\
		    libavformat/asfcrypt.o 	     	\
		    libavformat/avlanguage.o		\
		    libavformat/mpegts.o 	     	\
		    libavformat/rtp.o         		\
		    libavformat/rtpenc_aac.o     	\
		    libavformat/rtpenc_amr.o     	\
		    libavformat/rtpenc_h263.o    	\
		    libavformat/rtpenc_mpv.o     	\
		    libavformat/rtpenc.o      		\
		    libavformat/rtpenc_h264.o 		\
		    libavformat/rtpenc_vp8.o  		\
		    libavformat/rtpenc_xiph.o 		\
		    libavformat/avc.o			\
		    libavformat/rdt.o         		\
		    libavformat/rtpdec.o      		\
		    libavformat/rtpdec_amr.o  		\
		    libavformat/rtpdec_asf.o  		\
		    libavformat/rtpdec_h263.o 		\
		    libavformat/rtpdec_h264.o 		\
		    libavformat/rtpdec_latm.o 		\
		    libavformat/rtpdec_mpeg4.o		\
		    libavformat/rtpdec_qcelp.o		\
		    libavformat/rtpdec_qdm2.o 		\
		    libavformat/rtpdec_qt.o   		\
		    libavformat/rtpdec_svq3.o 		\
		    libavformat/rtpdec_vp8.o  		\
		    libavformat/rtpdec_xiph.o		\
		    libavformat/rtsp.o 			\
		    libavformat/rtspdec.o 		\
		    libavformat/httpauth.o		\
		    libavformat/rtspenc.o 		\
		    libavformat/rtpenc_chain.o		\
		    libavformat/sapdec.o		\
		    libavformat/sapenc.o 		\
		    libavformat/avio.o 			\
		    libavformat/aviobuf.o		\
		    libavformat/applehttp.o		\
		    libavformat/applehttpproto.o	\
		    libavformat/http.o	 		\
		    libavformat/mmsh.o	 		\
		    libavformat/mms.o 			\
		    libavformat/mmst.o 			\
		    libavformat/rtmpproto.o 		\
		    libavformat/rtmppkt.o		\
		    libavformat/rtpproto.o		\
		    libavformat/tcp.o			\
		    libavformat/udp.o			\

LIBAVCODEC_OBJS= libavcodec/avpacket.o          	\
		 libavcodec/utils.o			\
		 libavcodec/bitstream_filter.o		\
		 libavcodec/parser.o			\
		 libavcodec/options.o			\
		 libavcodec/xiph.o			\
		 libavcodec/raw.o			\
		 libavcodec/imgconvert.o		\
		 libavcodec/missing.o			\

LIBAVUTIL_OBJS= libavutil/audioconvert.o                \
		libavutil/avstring.o                    \
		libavutil/base64.o                      \
		libavutil/crc.o                         \
		libavutil/des.o                         \
		libavutil/eval.o                        \
		libavutil/imgutils.o                    \
		libavutil/intfloat_readwrite.o          \
		libavutil/lfg.o                         \
		libavutil/log.o                         \
		libavutil/mathematics.o                 \
		libavutil/md5.o                         \
		libavutil/mem.o                         \
		libavutil/opt.o                         \
		libavutil/parseutils.o                  \
		libavutil/pixdesc.o                     \
		libavutil/random_seed.o                 \
		libavutil/rational.o                    \
		libavutil/rc4.o                         \
		libavutil/samplefmt.o                   \
		libavutil/sha.o                         \

SOURCE_OBJS = $(LIBAVFORMAT_OBJS)

OBJECTS = $(LIBAVFORMAT_OBJS) $(LIBAVCODEC_OBJS) $(LIBAVUTIL_OBJS)

CXXFLAGS += -I. -I./libavformat -I./libavcodec -I./libavutil -I../utils

all: $(FFMPEG_SO_NAME)

$(FFMPEG_SO_NAME):$(OBJECTS)
	$(CXX) -shared -Wl,-soname,$(FFMPEG_SO_NAME) -o $(FFMPEG_SO_NAME) $(OBJECTS) $(LFLAGS) 

	mv $(FFMPEG_SO_NAME) $(RELEASE_LIB_DIR)/$(FFMPEG_SO_NAME)
	@echo "-----------------------------------------------------------------------------------"

.c.o:
	$(CC) $(CXXFLAGS) -o $@ -c $<	

clean:
	@echo "cleaning ..."
	rm  -f libavformat/*.o  libavcodec/*.o libavutil/*.o
	rm  -f $(RELEASE_LIB_DIR)/$(FFMPEG_SO_NAME)


