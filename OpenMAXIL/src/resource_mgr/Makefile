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

RESOURCE_MGR_SO_NAME= lib$(RESOURCE_MGR_LIB_NAME).so

LFLAGS+= -l$(OSAL_LIB_NAME)

SOURCE_FILES = $(SRC_DIR)/PlatformResourceMgr.cpp

OBJECTS = $(SOURCE_FILES:.cpp=.o)

all: $(RESOURCE_MGR_SO_NAME)

$(RESOURCE_MGR_SO_NAME):$(OBJECTS)
	$(CXX) -shared -Wl,-soname,$(RESOURCE_MGR_SO_NAME) -o $(RESOURCE_MGR_SO_NAME) $(OBJECTS) $(LFLAGS) 

	mv $(RESOURCE_MGR_SO_NAME) $(RELEASE_LIB_DIR)/$(RESOURCE_MGR_SO_NAME)
	@echo "-----------------------------------------------------------------------------------"

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_LIB_DIR)/$(RESOURCE_MGR_SO_NAME)
			


