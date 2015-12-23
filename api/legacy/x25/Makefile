# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-1998 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= __LINUX__
DEBUG	= 2

# Wanpipe Include Directory ( can be changed using --builddir= on ./Setup or make WANINCDIR= )
ifndef WANINCDIR
	WANINCDIR=/usr/include/wanpipe
endif


# Tools options.
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I$(WANINCDIR)

####### RULES ################################################################

all:	server_v1 \
		server_v2 \
		svc_client \
		pvc_client \
		hdlc_client  
	@echo "Ok."

server_v1:	server_v1.c
	$(CC) $(CFLAGS) -o $@ $<
 
svc_client:	svc_client.c
	$(CC) $(CFLAGS) -o $@ $<

server_v2:  server_v2.c
	$(CC) $(CFLAGS) -o $@ $<

pvc_client:	pvc_client.c
	$(CC) $(CFLAGS) -o $@ $< 

hdlc_client:	hdlc_client.c
	$(CC) $(CFLAGS) -o $@ $< 


clean:
	rm -f server_v1
	rm -f server_v2
	rm -f svc_client
	rm -f pvc_client
	rm -f hdlc_client
