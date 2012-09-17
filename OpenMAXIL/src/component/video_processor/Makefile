###############################################################################
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

CFLAGS += -I$(BSP_RELATED_INC_DIR)
CXXFLAGS += -I$(BSP_RELATED_INC_DIR)

#CODEC_NAME = ipu
LFLAGS +=-L$(BSP_RELATED_LIB_DIR) -lipu

COMPONENT_NAME=omx_video_processor
COMPONENT_LIB=omx_video_processor_$(PROCESSOR)_$(OSTYPE)

COMPONENT_LIB_NAME=lib_$(COMPONENT_LIB).a
COMPONENT_SO_NAME=lib_$(COMPONENT_LIB).so
	

SOURCE_FILES = $(SRC_DIR)/VideoProcessorComponent.cpp \
               
#Include rules Makefile
include $(OMX_COMPONENT_DEFINE_DIR)/Makefile.rules	







