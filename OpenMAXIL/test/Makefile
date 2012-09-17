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
#   This file will build OMX player.
#
######################################################################################

SUB_DIRS = ls -1d */

all:
	for dir in `$(SUB_DIRS)`; do \
		$(MAKE) -C $$dir || exit $$? ;\
	done
		
clean:
	for dir in `$(SUB_DIRS)`; do\
		$(MAKE) -C $$dir clean;\
	done


