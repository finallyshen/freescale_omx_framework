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
#   This file will build whole OMX project.
#
######################################################################################


all: 
	$(MAKE) -C OSAL 
	$(MAKE) -C utils 
	$(MAKE) -C lib_ffmpeg
	$(MAKE) -C OpenMAXIL        

clean:
	$(MAKE) clean -C OSAL 
	$(MAKE) clean -C utils
	$(MAKE) clean -C lib_ffmpeg
	$(MAKE) clean -C OpenMAXIL        





