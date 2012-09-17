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
#   This file will build all component.
#
######################################################################################

EXCLUDE_COMP=common|android_audio_render|overlay_render|surface_render|camera_source|android_audio_source|aac_enc|vpu_wrapper|vpu_dec\>|video_processor

SUB_DIRS = ls -1d */ | grep -Ev "$(EXCLUDE_COMP)"

all: common_src components

common_src:
	$(MAKE) -C common

components:
	for dir in `$(SUB_DIRS)`; do \
		$(MAKE) -C $$dir || exit $$? ;\
	done
		
clean:
	make clean -C common	
	for dir in `$(SUB_DIRS)`; do\
		$(MAKE) -C $$dir clean;\
	done




