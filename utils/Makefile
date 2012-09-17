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
OMX_ROOT_DIR=..

# Set the BUILD information
include $(OMX_ROOT_DIR)/Makefile.defines

UTILS_SO_NAME= lib$(UTILS_LIB_NAME).so

LFLAGS+= -l$(OSAL_LIB_NAME)

SOURCE_FILES = $(SRC_DIR)/Mem.cpp \
	$(SRC_DIR)/Queue.cpp \
	$(SRC_DIR)/RegistryAnalyser.cpp \
	$(SRC_DIR)/RingBuffer.cpp \
	$(SRC_DIR)/FadeInFadeOut.cpp \
	$(SRC_DIR)/ShareLibarayMgr.cpp \
        $(SRC_DIR)/colorconvert/src/cczoomrotation16.cpp \
        $(SRC_DIR)/colorconvert/src/cczoomrotationbase.cpp \


CSOURCE_FILES = $(SRC_DIR)/mfw_gst_ts.c \
	              $(SRC_DIR)/Tsm_wrapper.c \
	              
OBJECTS = $(SOURCE_FILES:.cpp=.o)
OBJECTS += $(CSOURCE_FILES:.c=.o)

CXXFLAGS += -I./colorconvert/include

all: $(UTILS_SO_NAME)

$(UTILS_SO_NAME):$(OBJECTS)
	$(CXX) -shared -Wl,-soname,$(UTILS_SO_NAME) -o $(UTILS_SO_NAME) $(OBJECTS) $(LFLAGS) 

	mv $(UTILS_SO_NAME) $(RELEASE_LIB_DIR)/$(UTILS_SO_NAME)
	@echo "-----------------------------------------------------------------------------------"

.cpp.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $<

.c.o:
	$(CC) $(CXXFLAGS) -o $@ -c $<	

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o $(SRC_DIR)/colorconvert/src/*.o
	rm  -f $(RELEASE_LIB_DIR)/$(UTILS_SO_NAME)
			


