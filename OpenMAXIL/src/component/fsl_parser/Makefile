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

COMPONENT_NAME=fsl_parser_v2
COMPONENT_SO_NAME=lib_omx_$(COMPONENT_NAME)_$(PROCESSOR)_$(OSTYPE).so

SOURCE_FILES = $(SRC_DIR)/FslParser.cpp \

all: $(COMPONENT_SO_NAME)

CXXFLAGS+=-I../../../../fsl_mad_multimedia_layer/ghdr -I../../../../fsl_mad_multimedia_layer/ghdr/common

LFLAGS+=-l_omx_core_mgr_v2_$(PROCESSOR)_$(OSTYPE)

LFLAGS+=-L../../../../fsl_mad_multimedia_layer/release/lib -l_omx_utils_v2_$(PROCESSOR)_$(OSTYPE)
#Include rules Makefile
include $(OMX_COMPONENT_DEFINE_DIR)/Makefile.rules	




