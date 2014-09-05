# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-1999 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= _LINUX_
DEBUG	= 2

# Project file paths.
SYSINC=../../patches/kdrivers/include

# Wanpipe Include Directory ( can be changed using --builddir= on ./Setup or make WANINCDIR= )
ifndef WANINCDIR
	WANINCDIR=/usr/include/wanpipe
	#/lib/modules/$(shell uname -r)/build/include
endif

EXTRA_FLAGS=

# Tools options.
CFLAGS  = -D__LINUX__ -Wall -O2 -D_GNUC_ -I$(WANINCDIR) -I$(SYSINC) -D$(OS_TYPE) -D_DEBUG_=$(DEBUG)  
LIBS = -lncurses

CFLAGS += $(EXTRA_FLAGS)

####### RULES ################################################################

all:	wanpipe_ft1exec
	@echo "Ok."

wanpipe_ft1exec:	ft1_config.c	unixio.c
	$(CC) $(CFLAGS) -o $@ $^
	chmod 755 $@

clean:
	rm -f wanpipe_ft1exec
	rm -f *.o

install:
	install -D -m 755 wanpipe_ft1exec $(WAN_VIRTUAL)/usr/sbin/wanpipe_ft1exec
	install -D -m 755 wanpipe_cfgft1 $(WAN_VIRTUAL)/usr/sbin/cfgft1

uninstall:
	rm -f $(WAN_VIRTUAL)/usr/sbin/wanpipe_ft1exec
	rm -f $(WAN_VIRTUAL)/usr/sbin/cfgft1
