#####################################################################################
#
#   Copyright (c) 2011, Freescale Semiconductors Inc.,
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

COMPONENT_NAME=file_write
COMPONENT_SO_NAME=lib_omx_$(COMPONENT_NAME)_$(PROCESSOR)_$(OSTYPE).so

SOURCE_FILES = $(SRC_DIR)/FileWrite.cpp \
               
#Include rules Makefile
include $(OMX_COMPONENT_DEFINE_DIR)/Makefile.rules	



