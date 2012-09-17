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


# Set the root directory
OMX_ROOT_DIR=../../..

# Set the BUILD information
include $(OMX_ROOT_DIR)/Makefile.defines

MODULE_NAME = recorder_test
BINARY_NAME = $(MODULE_NAME)_$(PROCESSOR)_$(OSTYPE)

CXXFLAGS += -I../../ghdr -I../../../OSAL/ghdr -g

LFLAGS +=-L../../release/lib -l_omx_client_$(PROCESSOR)_$(OSTYPE) -l_omx_osal_v2_$(PROCESSOR)_$(OSTYPE) -l_omx_utils_v2_$(PROCESSOR)_$(OSTYPE) -Wl,-rpath-link,$(RELEASE_LIB_DIR),-rpath-link,$(BSP_RELATED_LIB_DIR),-rpath-link,../../../fsl_mad_multimedia_codec/release/lib,-rpath-link,../../../fsl_mad_multimedia_layer/release/lib

SOURCE_FILES = $(SRC_DIR)/recorder_test.cpp

OBJECTS = $(SOURCE_FILES:.cpp=.o)

all: $(BINARY_NAME)

$(BINARY_NAME):$(OBJECTS)
	$(CXX) -Wl,-rpath,$(RELEASE_LIB_DIR) -o $(BINARY_NAME) $(OBJECTS) $(LFLAGS) -lpthread -ldl 

	mv $(BINARY_NAME) $(RELEASE_EXE_DIR)/$(BINARY_NAME)
	@echo "-----------------------------------------------------------------------------------"

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_EXE_DIR)/$(BINARY_NAME)
			


