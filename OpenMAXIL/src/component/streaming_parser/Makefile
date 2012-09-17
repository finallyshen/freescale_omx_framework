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


#Set the root directory
OMX_ROOT_DIR=../../../..

#Set the component directory
OMX_COMPONENT_DEFINE_DIR=..

# Set the BUILD information 
include $(OMX_COMPONENT_DEFINE_DIR)/Makefile_component.defines

COMPONENT_NAME=streaming_parser
COMPONENT_SO_NAME=lib_omx_$(COMPONENT_NAME)_$(PROCESSOR)_$(OSTYPE).so

SOURCE_FILES = $(SRC_DIR)/StreamingParser.cpp \

all: $(COMPONENT_SO_NAME)

CXXFLAGS += -I$(FFMPEG_DIR) -I$(FFMPEG_DIR)/libavformat -I$(FFMPEG_DIR)/libavcodec -I$(FFMPEG_DIR)/libavutil

LFLAGS += -l$(FFMPEG_LIB_NAME) -l$(UTILS_LIB_NAME)

#Include rules Makefile
include $(OMX_COMPONENT_DEFINE_DIR)/Makefile.rules	




