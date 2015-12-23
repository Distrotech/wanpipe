# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-1998 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= __LINUX__
DEBUG	= 2

# Tools options.
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I.

TARGETS=wanpipe_hdlc.o

####### RULES ################################################################

all:	$(TARGETS)  
	@echo "Ok."

wanpipe_hdlc.o:      wanpipe_hdlc.c 
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -f $(TARGETS)
