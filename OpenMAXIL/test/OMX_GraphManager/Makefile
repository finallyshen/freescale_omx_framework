###############################################################################
#
#  Freescale Semiconductor, Inc.
#
#                  This code is the property of Freescale.
#        (C) Copyright 2007 Freescale Semiconductor, Inc. All Rights Reserved.
#
#  History :
#	Date		Author		Version		Description
#
#  July,2007      	r12344c		   0.1        Created
#
###############################################################################

###############################################################################
#	This makefile links OMX Avc Enc ILclient to core, component, OSAL and 
#	resource manager libraries
#
#	options: all | clean
#
###############################################################################

#Set the root directory
OMX_ROOT_DIR=../../..


# Set the BUILD information
include $(OMX_ROOT_DIR)/Makefile.defines

CFLAGS +=-I../../../fsl_mad_multimedia_layer/ghdr/divx_drm/  -I../../ghdr -I../../../OSAL/ghdr -g

LFLAGS +=-L../../release/lib -l_omx_client_$(PROCESSOR)_$(OSTYPE) -l_omx_osal_v2_$(PROCESSOR)_$(OSTYPE) -l_omx_utils_v2_$(PROCESSOR)_$(OSTYPE) -Wl,-rpath-link,$(RELEASE_LIB_DIR),-rpath-link,$(BSP_RELATED_LIB_DIR),-rpath-link,../../../fsl_mad_multimedia_codec/release/lib,-rpath-link,../../../fsl_mad_multimedia_layer/release/lib

#

SOURCE_FILES = $(SRC_DIR)/gm_app.c 
BINARY_NAME = omxgraph_manager_$(PROCESSOR)_$(OSTYPE)

OBJECTS = $(SOURCE_FILES:.c=.o)

all: $(BINARY_NAME)

$(BINARY_NAME):$(OBJECTS)
	$(CC)   -o $(BINARY_NAME) $(OBJECTS) $(LFLAGS)  -lpthread -ldl 

	mv $(BINARY_NAME) $(RELEASE_EXE_DIR)/$(BINARY_NAME)
	@echo "-----------------------------------------------------------------------------------"

.c.o:
	$(CC) $(CFLAGS)  -o $@ -c $< 

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_EXE_DIR)/$(BINARY_NAME)


