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
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ 

####### RULES ################################################################

all:	bstrm_file_client \
	bstrm_file_server \
	bstrm_gen_file
	@echo "Ok."

bstrm_file_client:      	bstrm_file_client.c
	$(CC) $(CFLAGS) -o $@ $<


bstrm_file_server:	bstrm_file_server.c
	$(CC) $(CFLAGS) -o $@ $<

bstrm_gen_file:	bstrm_gen_file.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f bstrm_file_server
	rm -f bstrm_file_client
	rm -f bstrm_gen_file
