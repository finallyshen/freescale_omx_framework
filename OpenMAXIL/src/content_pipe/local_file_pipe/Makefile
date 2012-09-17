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

LOCAL_FILE_PIPE_LIB_NAME=local_file_pipe_v2
LOCAL_FILE_PIPE_SO_NAME= lib_omx_$(LOCAL_FILE_PIPE_LIB_NAME)_$(PROCESSOR)_$(OSTYPE).so

LFLAGS+= -l$(OSAL_LIB_NAME) -l$(UTILS_LIB_NAME)

SOURCE_FILES = $(SRC_DIR)/LocalFilePipe.cpp \

OBJECTS = $(SOURCE_FILES:.cpp=.o)

all: $(LOCAL_FILE_PIPE_SO_NAME)

$(LOCAL_FILE_PIPE_SO_NAME):$(OBJECTS)
	$(CXX) -shared -Wl,-soname,$(LOCAL_FILE_PIPE_SO_NAME) -o $(LOCAL_FILE_PIPE_SO_NAME) $(OBJECTS) $(LFLAGS) 

	mv $(LOCAL_FILE_PIPE_SO_NAME) $(RELEASE_LIB_DIR)/$(LOCAL_FILE_PIPE_SO_NAME)
	@echo "-----------------------------------------------------------------------------------"

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_LIB_DIR)/$(LOCAL_FILE_PIPE_SO_NAME)
			


