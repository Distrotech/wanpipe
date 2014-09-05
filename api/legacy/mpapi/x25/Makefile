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

ifneq (,$(wildcard /wanpipe/code/Makefile.in))
	include /wanpipe/code/Makefile.in
	EXTRA_FLAGS+=$(GLOBAL_CFLAGS)
endif


# Tools options.
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I../ -I../../lib  -I$(WANINCDIR) $(EXTRA_FLAGS)

####### RULES ################################################################

all:	server_v1 \
        server_v2 \
		svc_client \
		pvc_client
	@echo "Ok."


server_v1:      	server_v1.c  ../../lib/lib_api.c
	$(CC) $(CFLAGS) -o $@ $^
 
server_v2:	server_v2.c  ../../lib/lib_api.c
	$(CC) $(CFLAGS) -o $@ $^

svc_client:	svc_client.c ../../lib/lib_api.c
	$(CC) $(CFLAGS) -o $@ $^
	
pvc_client:	pvc_client.c ../../lib/lib_api.c

clean:
	rm -f server_v1
	rm -f server_v2
	rm -f svc_client
	rm -f pvc_client

