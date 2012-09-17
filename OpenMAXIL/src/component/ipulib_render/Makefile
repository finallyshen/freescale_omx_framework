#####################################################################################
#
#   Copyright (c) 2010, Freescale Semiconductors Inc.,
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

COMPONENT_NAME=ipulib_render
COMPONENT_SO_NAME=lib_omx_$(COMPONENT_NAME)_$(PROCESSOR)_$(OSTYPE).so

ifeq ($(HWPLATFORM),MX6X)
SOURCE_FILES = $(SRC_DIR)/IpulibRender_mx6x.cpp
else
SOURCE_FILES = $(SRC_DIR)/IpulibRender.cpp
LFLAGS +=-L$(BSP_RELATED_LIB_DIR) -lipu
endif
               
#Include rules Makefile
include $(OMX_COMPONENT_DEFINE_DIR)/Makefile.rules	



