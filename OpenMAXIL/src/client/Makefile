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

GRAPH_MANAGER_LIB_NAME=client
GRAPH_MANAGER_SO_NAME= lib_omx_$(GRAPH_MANAGER_LIB_NAME)_$(PROCESSOR)_$(OSTYPE).so

CXXFLAGS += -I../graph_manager_interface \
            -I../../../fsl_mad_multimedia_codec/ghdr \
	    -I../../../fsl_mad_multimedia_layer/ghdr \
	    -I../../../fsl_mad_multimedia_layer/ghdr/common \
	    -I../../../utils/colorconvert/include \
	    -I$(BSP_RELATED_INC_DIR)

LFLAGS+= -l$(OSAL_LIB_NAME) \
	 -l$(UTILS_LIB_NAME) \
	 -l_omx_core_mgr_v2_$(PROCESSOR)_$(OSTYPE) \
	 -L../../../fsl_mad_multimedia_codec/release/lib -l_jpeg_enc_$(PROCESSOR)_$(OSTYPE) \
	 -L../../../fsl_mad_multimedia_layer/release/lib \
	 -L$(BSP_RELATED_LIB_DIR) -lipu 

SOURCE_FILES = $(SRC_DIR)/GMPlayerWrapper.cpp \
	       $(SRC_DIR)/GMPlayer.cpp \
	       $(SRC_DIR)/GMComponent.cpp \
	       $(SRC_DIR)/OMX_MetadataExtractor.cpp \
	       $(SRC_DIR)/OMX_Recorder.cpp \
	       $(SRC_DIR)/GMRecorder.cpp \
	       $(SRC_DIR)/MediaInspector.cpp
		   

CSOURCE_FILES = $(SRC_DIR)/image_convert.c

VIDEORENDER = IPULIB

ifeq ($(VIDEORENDER),V4L)
CXXFLAGS += -DV4L_RENDER
endif

ifeq ($(VIDEORENDER),IPULIB)
CXXFLAGS += -DIPULIB_RENDER
endif

ifeq ($(VIDEORENDER),FB)
CXXFLAGS += -DFB_RENDER
endif

ifeq ($(VIDEORENDER),EGL)
CXXFLAGS += -DEGL_RENDER
LFLAGS += -legl13X -lgles20 -lm -lX11 -lXau -lxcb -lXdmcp -lgsl -lpanel2 -lc2d
SOURCE_FILES += $(SRC_DIR)/GMEgl.cpp
CSOURCE_FILES += $(SRC_DIR)/eglRender.c
endif

OBJECTS = $(SOURCE_FILES:.cpp=.o)
OBJECTS += $(CSOURCE_FILES:.c=.o)

all: $(GRAPH_MANAGER_SO_NAME)

$(GRAPH_MANAGER_SO_NAME):$(OBJECTS)
	$(CC) -shared -Wl,-soname,$(GRAPH_MANAGER_SO_NAME) -o $(GRAPH_MANAGER_SO_NAME) $(OBJECTS) $(LFLAGS) 

	mv $(GRAPH_MANAGER_SO_NAME) $(RELEASE_LIB_DIR)/$(GRAPH_MANAGER_SO_NAME)
	@echo "-----------------------------------------------------------------------------------"

.cpp.o:
	@echo "build ..." $@ $<
	$(CC) $(CXXFLAGS) -o $@ -c $<
	
.c.o:
	@echo "build ..." $@ $<
	$(CC) $(CXXFLAGS) -o $@ -c $<	

clean:
	@echo "cleaning ..."
	rm  -f $(SRC_DIR)/*.o
	rm  -f $(RELEASE_LIB_DIR)/$(GRAPH_MANAGER_SO_NAME)
