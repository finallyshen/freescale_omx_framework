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

EXCLUDE_COMP=https_pipe\>

SUB_DIRS = ls -1d */ | grep -Ev "$(EXCLUDE_COMP)"

all: content_pipe 

content_pipe:
	for dir in `$(SUB_DIRS)`; do \
		$(MAKE) -C $$dir || exit $$? ;\
	done
		
clean:
	for dir in `$(SUB_DIRS)`; do\
		$(MAKE) -C $$dir clean;\
	done




