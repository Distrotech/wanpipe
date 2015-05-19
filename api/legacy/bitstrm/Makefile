# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-1998 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= __LINUX__
DEBUG	= 2

# Project file paths.
# Wanpipe Include Directory ( can be changed using --builddir= on ./Setup or make WANINCDIR= )
ifndef WANINCDIR
	WANINCDIR=/usr/include/wanpipe
endif

# Tools options.
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I ../lib  -I$(WANINCDIR)

####### RULES ################################################################

all:	bitstrm_api \
        bitstrm_switch \
	bitstrm_txrx 
	@echo "Ok."

bitstrm_api:     	bitstrm_api.c ../lib/lib_api.c
	$(CC) $(CFLAGS) -o $@ $^

bitstrm_switch:     	bitstrm_switch.c ../lib/lib_api.c
	$(CC) $(CFLAGS) -o $@ $^

bitstrm_txrx:     	bitstrm_txrx.c ../lib/lib_api.c
	$(CC) $(CFLAGS) -o $@ $^


clean:
	rm -f bitstrm_api
	rm -f bitstrm_switch
	rm -f bitstrm_txrx
