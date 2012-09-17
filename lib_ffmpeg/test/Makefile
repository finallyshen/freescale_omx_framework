#####################################################################################
#
#   Copyright (c) 2009-2010, Freescale Semiconductors Inc.,
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

MODULE_NAME = stack_test
BINARY_NAME = $(MODULE_NAME)_$(PROCESSOR)_$(OSTYPE)

CXXFLAGS += -I../ -I../libavformat -I../libavcodec -I../libavutil
LFLAGS+= -l_ffmpeg_$(PROCESSOR)_$(OSTYPE)

SOURCE_FILES = $(SRC_DIR)/test.c

OBJECTS = $(SOURCE_FILES:.c=.o)

all: $(BINARY_NAME)

$(BINARY_NAME):$(OBJECTS)
	$(CC) -Wl,-rpath,$(RELEASE_LIB_DIR) -o $(BINARY_NAME) $(OBJECTS) $(LFLAGS) -lpthread -ldl 
	#$(CC) -o $(BINARY_NAME) $(OBJECTS) $(LFLAGS) -lpthread -ldl 

	mv $(BINARY_NAME) $(RELEASE_EXE_DIR)/$(BINARY_NAME)
	@echo "-----------------------------------------------------------------------------------"

.c.o:
	$(CC) $(CXXFLAGS) -o $@ -c $<

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_EXE_DIR)/$(BINARY_NAME)
			


