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
	$(MAKE) -C resource_mgr
	$(MAKE) -C core
	$(MAKE) -C core_mgr 
	$(MAKE) -C content_pipe 
	$(MAKE) -C component
	$(MAKE) -C client
	#$(MAKE) -C graph_manager
clean:
	$(MAKE) clean -C resource_mgr
	$(MAKE) clean -C core 
	$(MAKE) clean -C core_mgr
	$(MAKE) clean -C content_pipe
	$(MAKE) clean -C component        
	$(MAKE) clean -C client
	#$(MAKE) clean -C graph_manager	





