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
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_  -I../lib
CFLAGS	+= -I/usr/include/wanpipe -I/usr/include -I../../patches/kdrivers/include/  

TARGETS = aft_tdm_hp

####### RULES ################################################################

all:	$(TARGETS)  
	@echo "Ok."

install:
	for x in $(TARGETS); do \
                install -D -m 755 $$x /usr/bin/$$x; \
        done;


aft_tdm_hp: aft_tdm_hp.c  
	$(CC) $(CFLAGS) -o $@ $^ -lpthread -lsangoma -lm

clean:
	rm -f $(TARGETS)
