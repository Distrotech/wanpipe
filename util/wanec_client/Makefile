# ============================================================================
# Makefile	WAN Echo Canceler Chip configurator.  Make Script.
#
# Copyright	(c) 1995-1997 Sangoma Technologies Inc.  All Rights Reserved.
# ----------------------------------------------------------------------------
# Augc 25, 2005		Alex Feldman	Initial version.
# ============================================================================

####### DEFINES ##############################################################

# Build Options.
PROD	= wan_ec_setup
VERSION	= 1.0
DEBUG	= 2
YACC	= bison

# Project File Paths.
TMPDIR		= tmp
OUTDIR		= mod/
VPATH		= $(LOCINC):$(TMPDIR)
ARCH    	= $(shell uname -m)
#DIR_EC_APILIB	= ../wanec_apilib

PWD=$(shell pwd)

WANECDIR=../../patches/kdrivers/wanec
ifeq (,$(wildcard $(WANECDIR)))
	WANECDIR=/common/wanec
endif

# Wanpipe Include Directory ( can be changed using --builddir= on ./Setup or make WANINCDIR= )
ifndef WANINCDIR
	WANINCDIR=$(WAN_VIRTUAL)/usr/include/wanpipe
endif

PRODS=wan_ec_client
SRC_CLIENT=wan_ecmain.c wan_ec_argy.c wan_ec_argl.c

#SRC_EC_APILIB=$(DIR_EC_APILIB)/wanec_api.c $(DIR_EC_APILIB)/wanec_api_lib.c
INC_EC_APILIB = -I../libsangoma/wanec -I$(WANECDIR) -I$(DIR_EC_APILIB) -I$(WANECDIR)/oct6100_api/include
INC_EC_APILIB += -I../../patches/kdrivers/include -I$(WANINCDIR)

CFLAGS	= -Wall -O2 -D__LINUX__ -DCONFIG_PRODUCT_WANPIPE_TDMV_EC -D_DEBUG_=$(DEBUG) -D_GNUC_  -L $(PWD)/../.lib/usr/lib 
CFLAGS	+= $(INC_EC_APILIB)  $(EXTRA_FLAGS)

####### RULES ################################################################

all:	$(PRODS) 
	@echo "Ok."

wan_ec_client:	$(SRC_CLIENT) $(SRC_EC_APILIB)
	$(CC) $(CFLAGS) -o $@ $^ -lsangoma  -lm

wan_ec_argy.c:
	$(YACC) -d -o wan_ec_argy.c wan_ec_arg.y

wan_ec_argl.c:
	$(LEX)	-owan_ec_argl.c wan_ec_arg.l

install:
	@echo "Installing Wanpipe EC API in $(WAN_VIRTUAL)/usr/sbin"
	install $(PRODS) $(WAN_VIRTUAL)/usr/sbin/

uninstall:
	@echo "Un-installing Wanpipe EC API from $(WAN_VIRTUAL)/usr/sbin"
	rm -f $(WAN_VIRTUAL)/usr/sbin/$(PRODS)

clean:
	rm -f $(PRODS)
	rm -f wan_ec_argy.c
	rm -f wan_ec_argy.h
	rm -f wan_ec_argl.c
