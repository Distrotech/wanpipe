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
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I ../lib -I../../lib 

####### RULES ################################################################

all:	ss7_monitor_api 
	@echo "Ok."

ss7_monitor_api:     	ss7_monitor_api.c ../../lib/lib_api.c
	$(CC) $(CFLAGS) -o $@ $^

bitstrm_api:     	bitstrm_api.c ../../lib/lib_api.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -f ss7_monitor_api
