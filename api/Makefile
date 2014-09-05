# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-2000 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

# Wanpipe Include Directory ( can be changed using --builddir= on ./Setup or make WANINCDIR= )
ifndef WANINCDIR
	WANINCDIR=/usr/include/wanpipe
endif

LIB_INSTALL_DIR=/usr

####### RULES ################################################################

all:	
	make -C tdm_api WANINCDIR=$(WANINCDIR)
	make -C legacy WANINCDIR=$(WANINCDIR)
	make -C sample_c WANINCDIR=$(WANINCDIR)
	make -C sample_cpp WANINCDIR=$(WANINCDIR)

clean:
	make -C tdm_api clean
	make -C legacy clean
	make -C lib/hdlc clean
	make -C sample_c clean
	make -C sample_cpp clean
