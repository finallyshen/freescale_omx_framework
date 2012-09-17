include $(SUBDIR)../config.mak

NAME = avformat
FFLIBS = avcodec avutil

HEADERS = avformat.h avio.h version.h

OBJS = allformats.o         \
       cutils.o             \
       id3v1.o              \
       id3v2.o              \
       metadata.o           \
       metadata_compat.o    \
       options.o            \
       os_support.o         \
       sdp.o                \
       seek.o               \
       utils.o              \

# muxers/demuxers

OBJS-$(CONFIG_RM_DEMUXER)                += rmdec.o rm.o
OBJS-$(CONFIG_MOV_DEMUXER)               += mov.o riff.o isom.o
OBJS-$(CONFIG_ASF_DEMUXER)               += asfdec.o asf.o asfcrypt.o \
                                            riff.o avlanguage.o
OBJS-$(CONFIG_MPEGTS_DEMUXER)            += mpegts.o isom.o


OBJS-$(CONFIG_RTP_MUXER)                 += rtp.o         \
                                            rtpenc_aac.o     \
                                            rtpenc_amr.o     \
                                            rtpenc_h263.o    \
                                            rtpenc_mpv.o     \
                                            rtpenc.o      \
                                            rtpenc_h264.o \
                                            rtpenc_vp8.o  \
                                            rtpenc_xiph.o \
                                            avc.o
OBJS-$(CONFIG_RTPDEC)                    += rdt.o         \
                                            rtp.o         \
                                            rtpdec.o      \
                                            rtpdec_amr.o  \
                                            rtpdec_asf.o  \
                                            rtpdec_h263.o \
                                            rtpdec_h264.o \
                                            rtpdec_latm.o \
                                            rtpdec_mpeg4.o \
                                            rtpdec_qcelp.o \
                                            rtpdec_qdm2.o \
                                            rtpdec_qt.o   \
                                            rtpdec_svq3.o \
                                            rtpdec_vp8.o  \
                                            rtpdec_xiph.o
OBJS-$(CONFIG_RTSP_DEMUXER)              += rtsp.o rtspdec.o httpauth.o
OBJS-$(CONFIG_RTSP_MUXER)                += rtsp.o rtspenc.o httpauth.o \
                                            rtpenc_chain.o
OBJS-$(CONFIG_SAP_DEMUXER)               += sapdec.o
OBJS-$(CONFIG_SAP_MUXER)                 += sapenc.o rtpenc_chain.o
OBJS-$(CONFIG_SDP_DEMUXER)               += rtsp.o

# protocols I/O
OBJS+= avio.o aviobuf.o

OBJS-$(CONFIG_APPLEHTTP_PROTOCOL)        += applehttpproto.o
OBJS-$(CONFIG_HTTP_PROTOCOL)             += http.o httpauth.o
OBJS-$(CONFIG_MMSH_PROTOCOL)             += mmsh.o mms.o asf.o
OBJS-$(CONFIG_MMST_PROTOCOL)             += mmst.o mms.o asf.o

RTMP-OBJS-$(!CONFIG_LIBRTMP)              = rtmpproto.o rtmppkt.o
OBJS-$(CONFIG_RTMP_PROTOCOL)             += $(RTMP-OBJS-yes)

OBJS-$(CONFIG_RTP_PROTOCOL)              += rtpproto.o
OBJS-$(CONFIG_TCP_PROTOCOL)              += tcp.o
OBJS-$(CONFIG_UDP_PROTOCOL)              += udp.o

include $(SUBDIR)../subdir.mak
