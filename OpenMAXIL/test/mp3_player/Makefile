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


# Set the root directory
OMX_ROOT_DIR=../../..

# Set the BUILD information
include $(OMX_ROOT_DIR)/Makefile.defines

MODULE_NAME = mp3_player
BINARY_NAME = $(MODULE_NAME)_$(PROCESSOR)_$(OSTYPE)

CORE_LIB_NAME=core_mgr_v2
LFLAGS+= -l_omx_$(CORE_LIB_NAME)_$(PROCESSOR)_$(OSTYPE) -l$(OSAL_LIB_NAME) -l$(UTILS_LIB_NAME)

SOURCE_FILES = $(SRC_DIR)/mp3_player.c 

OBJECTS = $(SOURCE_FILES:.c=.o)

all: $(BINARY_NAME)
	#cp ../../release/bin/mp3_player_arm11_elinux ../../../../test/omx_release/OpenMAXIL/release/bin -f

$(BINARY_NAME):$(OBJECTS)
	$(CC) -o $(BINARY_NAME) $(OBJECTS) $(LFLAGS) -lpthread -ldl 

	mv $(BINARY_NAME) $(RELEASE_EXE_DIR)/$(BINARY_NAME)
	@echo "-----------------------------------------------------------------------------------"

.c.o:
	$(CXX) $(CXXFLAGS) -o $@ -c $< -I $(OMX_ROOT_DIR)/OpenMAXIL/ghdr -g

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_EXE_DIR)/$(BINARY_NAME)
			


