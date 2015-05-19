# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-2000 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= __LINUX__
DEBUG	= 2

# Project file paths.
SYSINC=/lib/modules/$(shell uname -r)/build/include
WAN_VIRTUAL=

# Wanpipe Include Directory ( can be changed using --builddir= on ./Setup or make WANINCDIR= )
ifndef WANINCDIR
	WANINCDIR=/usr/include/wanpipe
endif

# Tools options.
CFLAGS  = -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -I$(SYSINC) -I$(WANINCDIR) $(HOSTCFLAGS)

####### RULES ################################################################

all: wpkbdmon \
     wp_x25_event_read \
     wanconfig_client
	@echo "Ok."

wpkbdmon:	wpkbdmon.c
	$(CC) -D_GNUC_ $(CFLAGS) -o $@ $<

wanconfig_client: 	wanconfig_client.c	
	$(CC) -D_GNUC_ $(CFLAGS) -o $@ $<

wp_x25_event_read: 	wp_x25_event_read.c
	$(CC) -D_GNUC_ $(CFLAGS) -o $@ $<

install:
	@echo "Installing Misc utilites in $(WAN_VIRTUAL)/usr/sbin"
	install -D wpkbdmon   $(WAN_VIRTUAL)/usr/sbin/wpkbdmon
	install -D wanpipe_setup $(WAN_VIRTUAL)/usr/sbin/wanpipe_setup
	install -D wanconfig_client $(WAN_VIRTUAL)/usr/sbin/wanconfig_client
	install -D wp_x25_event_read $(WAN_VIRTUAL)/usr/sbin/wp_x25_event_read

uninstall:
	@echo "Un-installing Wanpipe utilites from $(WAN_VIRTUAL)/usr/sbin"
	rm -f $(WAN_VIRTUAL)/usr/sbin/wpkbdmon
	rm -f $(WAN_VIRTUAL)/usr/sbin/wanpipe_setup
	rm -f $(WAN_VIRTUAL)/usr/sbin/wanconfig_client
	rm -f $(WAN_VIRTUAL)/usr/sbin/wp_x25_event_read

clean:
	rm -f *.0
	rm -f wpkbdmon
	rm -f wanconfig_client
	rm -f wp_x25_event_read
