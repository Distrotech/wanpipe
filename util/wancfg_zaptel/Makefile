# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-2000 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
WZDIR=/etc/wanpipe/wancfg_zaptel
WAN_VIRTUAL=

all:

install:
	@echo "Installing Wanconfig Zaptel/SMG utilites in $(WAN_VIRTUAL)/$(WZDIR)"
	$(shell ./install.sh $(WZDIR) $(WAN_VIRTUAL) )

uninstall:
	@echo "Un-installing Wanpipe utilites from $(WAN_VIRTUAL)/$(WZDIR)"
	$(shell ./uninstall.sh $(WZDIR) $(WAN_VIRTUAL) )

clean:
	@echo "Removing temporary wancfg_zaptel files"
	$(shell ./clean.sh)
