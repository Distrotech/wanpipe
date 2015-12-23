# ============================================================================
# Makefile	Multiprotocol WAN Router for Linux.  Make Script.
#
# Copyright	(c) 1995-1997 Sangoma Technologies Inc.  All Rights Reserved.
# ----------------------------------------------------------------------------
# Mar 27  2000  Nenad Corbic	Version 2.0.5 to 2.1.2
# Jan 07, 1999	Jaspreet Singh	Version 2.0.4
# Aug 25, 1998	Jaspreet Singh  Version 2.0.3
# Nov 06, 1997	Jaspreet Singh	Version 2.0.0
# Jul 28, 1997	Jaspreet Singh  Version 1.0.5
# Jul 10, 1997  Jaspreet Singh	Version 1.0.4
# June 3, 1997	Jaspreet Singh	Version 1.0.3	
# Jan 15, 1997	Gene Kozin	Version 1.0.1.
# Dec 31, 1996	Gene Kozin	Initial version.
# ============================================================================

####### DEFINES ##############################################################

# Build Options.
#
OS_TYPE	= __LINUX__
DEBUG	= 2

PROD=wan_aftup

WAN_VIRTUAL=
ifndef SYSINC
	SYSINC=/usr/include/wanpipe
endif

# Wanpipe Include Directory ( can be changed using --builddir= on ./Setup or make WANINCDIR= )
ifndef WANINCDIR
	WANINCDIR=/usr/include/wanpipe
endif

# Tools options.
SRCS	= wan_aftup.c wan_aft_prg.c wan_aft_flash.c wan_aft_flash_shark.c wan_aft_flash_shark_116.c wan_aft_flash_shark_t116_base.c
#wan_aft_flash_shark_ds.c wan_aft_flash_shark_116.c 
SRCS	+= wan_pcie_ctrl.c wan_pcie_ctrl_plx.c wan_pcie_ctrl_tundra.c wan_aft_flash_a600.c wan_aft_flash_t116.c
SRC2	+= wan_usb_fwupdate.cpp mem.cpp
CFLAGS  = -Wall -O0 -g -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) 
CFLAGS += $(EXTRA_FLAGS)
CFLAGS += -I/usr/include -I$(WANINCDIR) -I$(SYSINC)

BINS=`ls *.BIN`


####### RULES ################################################################

all:	$(PROD) 
	@echo "Ok."

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

wan_aftup: $(patsubst %.c,%.o,${SRCS}) $(patsubst %.cpp,%.o,${SRC2})
	$(CXX) $(CFLAGS) -o $@ $^ -lstdc++
 


install:
	@echo "Installing Wanpipe Firmware  update utility in $(WAN_VIRTUAL)/etc/wanpipe/util/wan_aftup"
	install -D $(PROD)	  $(WAN_VIRTUAL)/usr/sbin/$(PROD)
	install -d $(WAN_VIRTUAL)/etc/wanpipe/util/wan_aftup/scripts
	install -m755 wan_aftup $(WAN_VIRTUAL)/etc/wanpipe/util/wan_aftup/wan_aftup
	install -m755 update_aft_firm.sh $(WAN_VIRTUAL)/etc/wanpipe/util/wan_aftup/update_aft_firm.sh
	install -m755 scripts/load.sh $(WAN_VIRTUAL)/etc/wanpipe/util/wan_aftup/scripts/load.sh
	install -m755 scripts/unload.sh $(WAN_VIRTUAL)/etc/wanpipe/util/wan_aftup/scripts/unload.sh
	@if [ ! -z "$(BINS)" ]; then \
		echo "Installing fimware files in /etc/wanpipe/util/wan_aftup/"; \
		cp -f *.BIN $(WAN_VIRTUAL)/etc/wanpipe/util/wan_aftup/; \
	fi

uninstall:
	@echo "Uninstalling WAN_AFTUP in $(WAN_VIRTUAL)/etc/wanpipe/util/wan_aftup"
	@if [ -e $(WAN_VIRTUAL)/etc/wanpipe/util/wan_aftup ]; then \
		rm -rf $(WAN_VIRTUAL)/etc/wanpipe/util/wan_aftup;   \
	fi
	@rm -f $(WAN_VIRTUAL)/usr/sbin/$(PROD)

clean:
	rm -f $(PROD)
	rm -f *.o
