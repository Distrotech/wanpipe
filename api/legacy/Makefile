# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-2000 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

# Wanpipe Include Directory ( can be changed using --builddir= on ./Setup or make WANINCDIR= )
ifndef WANINCDIR
	WANINCDIR=/usr/include/wanpipe
endif


####### RULES ################################################################

all:	
	make -C x25 all  WANINCDIR=$(WANINCDIR)
	make -C chdlc all  WANINCDIR=$(WANINCDIR)
	make -C fr all  WANINCDIR=$(WANINCDIR)
	make -C bitstrm all WANINCDIR=$(WANINCDIR)
	make -C bisync all WANINCDIR=$(WANINCDIR)
	make -C aft all WANINCDIR=$(WANINCDIR)

clean:
	make -C x25 clean 
	make -C chdlc clean 
	make -C fr clean 
	make -C bitstrm clean
	make -C aft clean
	make -C bisync clean

