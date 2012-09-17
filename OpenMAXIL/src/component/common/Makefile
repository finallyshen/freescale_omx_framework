#####################################################################################
#
#   Copyright (c) 2009-2011, Freescale Semiconductors Inc.,
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

COMPONENT_SO_NAME=$(COMMON_SO_NAME)

LFLAGS+= -l_omx_core_mgr_v2_$(PROCESSOR)_$(OSTYPE)

SOURCE_FILES = $(SRC_DIR)/ComponentBase.cpp \
               $(SRC_DIR)/ExecutingState.cpp \
               $(SRC_DIR)/IdleState.cpp \
               $(SRC_DIR)/InvalidState.cpp \
               $(SRC_DIR)/LoadedState.cpp \
               $(SRC_DIR)/PauseState.cpp \
               $(SRC_DIR)/Port.cpp \
               $(SRC_DIR)/State.cpp \
               $(SRC_DIR)/WaitForResourcesState.cpp \
               $(SRC_DIR)/AudioParserBase.cpp \
               $(SRC_DIR)/AudioFilter.cpp \
               $(SRC_DIR)/AudioRender.cpp \
	       $(SRC_DIR)/Parser.cpp \
	       $(SRC_DIR)/Muxer.cpp \
	       $(SRC_DIR)/VideoFilter.cpp \
	       $(SRC_DIR)/VideoSource.cpp \
	       $(SRC_DIR)/AudioSource.cpp \
	       $(SRC_DIR)/VideoRender.cpp
              
#Include rules Makefile
include $(OMX_COMPONENT_DEFINE_DIR)/Makefile.rules	



