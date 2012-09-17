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
OMX_ROOT_DIR=../../../..

# Set the BUILD information
include $(OMX_ROOT_DIR)/Makefile.defines

HTTPS_PIPE_LIB_NAME=https_pipe_v2
HTTPS_PIPE_SO_NAME= lib_omx_$(HTTPS_PIPE_LIB_NAME)_$(PROCESSOR)_$(OSTYPE).so

CXXFLAGS += -I$(FFMPEG_DIR) -I$(FFMPEG_DIR)/libavformat -I$(FFMPEG_DIR)/libavcodec -I$(FFMPEG_DIR)/libavutil
LFLAGS+= -l$(OSAL_LIB_NAME) -l$(UTILS_LIB_NAME) -l$(FFMPEG_LIB_NAME)

SOURCE_FILES = $(SRC_DIR)/HttpsPipe.cpp \
	       $(SRC_DIR)/HttpsCache.cpp \

OBJECTS = $(SOURCE_FILES:.cpp=.o)

all: $(HTTPS_PIPE_SO_NAME)

$(HTTPS_PIPE_SO_NAME):$(OBJECTS)
	$(CXX) -shared -Wl,-soname,$(HTTPS_PIPE_SO_NAME) -o $(HTTPS_PIPE_SO_NAME) $(OBJECTS) $(LFLAGS) 

	mv $(HTTPS_PIPE_SO_NAME) $(RELEASE_LIB_DIR)/$(HTTPS_PIPE_SO_NAME)
	@echo "-----------------------------------------------------------------------------------"

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_LIB_DIR)/$(HTTPS_PIPE_SO_NAME)
