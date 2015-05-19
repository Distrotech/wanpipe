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
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_  -I$(WANINCDIR)

TARGETS = fr_api 
#TARGETS += fr_api_fast


####### RULES ################################################################

all:    $(TARGETS)
	@echo "Ok."

fr_api:      	fr_api.c
	$(CC) $(CFLAGS) -c -o $@ $<
#fr_api_fast:      	fr_api_fast.c
#	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGETS)
