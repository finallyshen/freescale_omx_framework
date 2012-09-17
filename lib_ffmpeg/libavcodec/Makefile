include $(SUBDIR)../config.mak

NAME = avcodec
FFLIBS = avutil

HEADERS = avcodec.h avfft.h dxva2.h opt.h vaapi.h vdpau.h version.h xvmc.h

OBJS = avpacket.o                                                       \
       utils.o								\
       bitstream_filter.o						\
       parser.o								\
       options.o							\
       xiph.o								\
       raw.o								\
       imgconvert.o							\
       missing.o							\



# parts needed for many different codecs

include $(SUBDIR)../subdir.mak

