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
OMX_ROOT_DIR=../..

# Set the BUILD information
include $(OMX_ROOT_DIR)/Makefile.defines

OSAL_SO_NAME= lib$(OSAL_LIB_NAME).so

SOURCE_FILES = $(SRC_DIR)/fsl_osal_linux_file.cpp \
	$(SRC_DIR)/fsl_osal_linux_mem.cpp \
	$(SRC_DIR)/fsl_osal_linux_string.cpp \
	$(SRC_DIR)/fsl_osal_linux_mutex.cpp \
	$(SRC_DIR)/fsl_osal_linux_sem.cpp \
	$(SRC_DIR)/fsl_osal_linux_thread.cpp \
	$(SRC_DIR)/fsl_osal_linux_time.cpp \
	$(SRC_DIR)/fsl_osal_linux_condition.cpp \
	$(SRC_DIR)/../logger/Log.cpp

OBJECTS = $(SOURCE_FILES:.cpp=.o)

all: $(OSAL_SO_NAME)

$(OSAL_SO_NAME):$(OBJECTS)
	$(CXX) -shared -Wl,-soname,$(OSAL_SO_NAME) -o $(OSAL_SO_NAME) $(OBJECTS)  

	mv $(OSAL_SO_NAME) $(RELEASE_LIB_DIR)/$(OSAL_SO_NAME)
	@echo "-----------------------------------------------------------------------------------"

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(SRC_DIR)/../logger/*.o
	rm  -f $(RELEASE_LIB_DIR)/$(OSAL_SO_NAME)
			


