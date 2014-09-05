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
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I$(WANINCDIR)

####### RULES ################################################################

all:	bsc_cmd \
	bsc_api
	@echo "Ok."

bsc_cmd:      	bsc_cmd.c
	$(CC) $(CFLAGS) -o $@ $^

bsc_api:      	bsc_api.c
	$(CC) $(CFLAGS) -o $@ $^


clean:
	rm -f bsc_cmd
	rm -f bsc_api
