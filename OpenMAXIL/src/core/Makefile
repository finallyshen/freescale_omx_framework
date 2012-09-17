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
OMX_ROOT_DIR=../../..

# Set the BUILD information
include $(OMX_ROOT_DIR)/Makefile.defines

CORE_LIB_NAME=core_v2
CORE_SO_NAME= lib_omx_$(CORE_LIB_NAME)_$(PROCESSOR)_$(OSTYPE).so

LFLAGS+= -l$(OSAL_LIB_NAME) -l$(UTILS_LIB_NAME) -l$(RESOURCE_MGR_LIB_NAME)

SOURCE_FILES = $(SRC_DIR)/OMXCore.cpp \

OBJECTS = $(SOURCE_FILES:.cpp=.o)

all: $(CORE_SO_NAME)

$(CORE_SO_NAME):$(OBJECTS)
	$(CXX) -shared -Wl,-soname,$(CORE_SO_NAME) -o $(CORE_SO_NAME) $(OBJECTS) $(LFLAGS) 

	mv $(CORE_SO_NAME) $(RELEASE_LIB_DIR)/$(CORE_SO_NAME)
	@echo "-----------------------------------------------------------------------------------"

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_LIB_DIR)/$(CORE_SO_NAME)
			


