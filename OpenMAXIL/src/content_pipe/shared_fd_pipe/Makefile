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
OMX_ROOT_DIR=../../../..

# Set the BUILD information
include $(OMX_ROOT_DIR)/Makefile.defines

SHARED_FD_PIPE_LIB_NAME=shared_fd_pipe
SHARED_FD_PIPE_SO_NAME= lib_omx_$(SHARED_FD_PIPE_LIB_NAME)_$(PROCESSOR)_$(OSTYPE).so

LFLAGS+= -l$(OSAL_LIB_NAME) -l$(UTILS_LIB_NAME)

SOURCE_FILES = $(SRC_DIR)/SharedFdPipe.cpp \

OBJECTS = $(SOURCE_FILES:.cpp=.o)

all: $(SHARED_FD_PIPE_SO_NAME)

$(SHARED_FD_PIPE_SO_NAME):$(OBJECTS)
	$(CXX) -shared -Wl,-soname,$(SHARED_FD_PIPE_SO_NAME) -o $(SHARED_FD_PIPE_SO_NAME) $(OBJECTS) $(LFLAGS) 

	mv $(SHARED_FD_PIPE_SO_NAME) $(RELEASE_LIB_DIR)/$(SHARED_FD_PIPE_SO_NAME)
	@echo "-----------------------------------------------------------------------------------"

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_LIB_DIR)/$(SHARED_FD_PIPE_SO_NAME)
			


