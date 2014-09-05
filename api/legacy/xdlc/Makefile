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

# Tools options.
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I../lib 

####### RULES ################################################################

all:	xdlc_api xdlc_mps
	@echo "Ok."

xdlc_api:     	xdlc_api.c ../lib/lib_api.c
	$(CC) $(CFLAGS) -o $@ $^

xdlc_mps:    	xdlc_mps.c ../lib/lib_api.c
	$(CC) $(CFLAGS) -o $@ $^
	
clean:
	rm -f xdlc_api
	rm -f xdlc_mps
