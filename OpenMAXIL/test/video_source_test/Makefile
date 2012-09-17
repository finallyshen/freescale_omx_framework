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

MODULE_NAME = video_source_test
BINARY_NAME = $(MODULE_NAME)_$(PROCESSOR)_$(OSTYPE)

CORE_LIB_NAME=core_mgr_v2
LFLAGS+= -l_omx_$(CORE_LIB_NAME)_$(PROCESSOR)_$(OSTYPE) -l$(OSAL_LIB_NAME) -l$(UTILS_LIB_NAME)

#SOURCE_FILES = $(SRC_DIR)/component_test.cpp
SOURCE_FILES = $(SRC_DIR)/video_source_test.cpp

OBJECTS = $(SOURCE_FILES:.cpp=.o)

all: $(BINARY_NAME)

$(BINARY_NAME):$(OBJECTS)
	#$(CXX) -Wl,-rpath,$(RELEASE_LIB_DIR) -o $(BINARY_NAME) $(OBJECTS) $(LFLAGS) -lpthread -ldl 
	$(CXX) -o $(BINARY_NAME) $(OBJECTS) $(LFLAGS) -lpthread -ldl 

	mv $(BINARY_NAME) $(RELEASE_EXE_DIR)/$(BINARY_NAME)
	@echo "-----------------------------------------------------------------------------------"

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_EXE_DIR)/$(BINARY_NAME)
			


