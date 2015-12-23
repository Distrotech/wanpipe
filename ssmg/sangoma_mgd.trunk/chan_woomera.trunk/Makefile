################################################################################
# chan_woomera: Woomera channel driver for Asterisk/CallWeaver
#
# Author:   Nenad Corbic <ncorbic@sangoma.com>
#	    Anthony Minessale II <anthmct@yahoo.com>
#
# Copyright:   (c) 2007 Nenad Corbic <ncorbic@sangoma.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
################################################################################

ifndef PBXDIR
	PBXDIR=/usr/include/asterisk
ifneq (,$(wildcard ./.pbxdir))
	PBXDIR=$(shell cat .pbxdir)
endif
endif

CHAN_NAME=WOOMERA

$(shell echo $(PBXDIR) > .pbxdir)	

PBXMODDIR=/usr/lib/asterisk/modules
PBXCFGDIR=/etc/asterisk
PBXDEFINES=
PBXFLAGS_EXTRA=


#Check if PBXDIR is asterisk source or include

INCDIR=$(PBXDIR)
INCLUDES=

ifneq (,$(wildcard $(PBXDIR)/include/asterisk/frame.h))
	INCLUDES= -I$(PBXDIR) -I$(PBXDIR)/include 
	INCDIR=$(PBXDIR)/include/asterisk
endif
ifneq (,$(wildcard $(PBXDIR)/include/callweaver/cli.h))
	INCLUDES= -I/$(PBXDIR) -I$(PBXDIR)/include 
	INCDIR=$(PBXDIR)/include/callweaver
endif



#Check for Asterisk AST_CONTROL_SRC feature
DUMMY:=$(shell ./check_for.sh AST_CONTROL_SRC $(INCDIR)/frame.h .ast_src_check;)

ifeq (1,$(shell cat .ast_src_check))
	PBXDEFINES+=-DWOO_CONTROL_SRC_FEATURE
endif


#Check for Callweaver opbx_cli_entry
DUMMY:=$(shell ./check_for.sh opbx_cli_entry $(INCDIR)/cli.h .opbx_cli_entry_check;)

ifneq (0,$(shell cat .opbx_cli_entry_check))
        PBXFLAGS_EXTRA+=-DCALLWEAVER_OPBX_CLI_ENTRY
endif


#Check for callweaver cw_cli_entry
DUMMY:=$(shell ./check_for.sh cw_cli_entry $(INCDIR)/cli.h .cw_cli_entry_check;)

ifneq (0,$(shell cat .cw_cli_entry_check))
        PBXFLAGS_EXTRA+=-DCALLWEAVER_CW_CLI_ENTRY
endif


ifneq (,$(wildcard $(INCDIR)/cwobj.h))
        PBXFLAGS_EXTRA+=-DCALLWEAVER_CWOBJ
endif


ifneq (,$(wildcard $(INCDIR)/../callweaver.h))
	PBXFLAGS_EXTRA+=-DCALLWEAVER -DHAVE_CONFIG_H
	PBXMODDIR=/usr/local/lib/callweaver/modules
	PBXCFGDIR=/usr/local/etc/callweaver

	#Check for callweaver MODULE_INFO
	DUMMY:=$(shell ./check_for.sh MODULE_INFO $(INCDIR)/module.h .cw_module_info_check;)

	ifneq (0,$(shell cat .cw_module_info_check))
        	PBXFLAGS_EXTRA+=-DCALLWEAVER_MODULE_INFO
	endif


else 

DUMMY:=$(shell ./check_for.sh 1.8  $(PBXDIR)/.version .ast18_check;)
ifneq (0,$(shell cat .ast18_check))
        PBXFLAGS_EXTRA+=-DAST16 -DAST18
endif
#Check for Asterisk 1.6
DUMMY:=$(shell ./check_for.sh 1.6  $(PBXDIR)/.version .ast16_check;)
ifneq (0,$(shell cat .ast16_check))
        PBXFLAGS_EXTRA+=-DAST16
endif

DUMMY:=$(shell ./check_for.sh 1.4  $(PBXDIR)/.version .ast14_check;)
ifneq (0,$(shell cat .ast14_check))
        PBXFLAGS_EXTRA+=-DAST14
endif

DUMMY:=$(shell ./check_for.sh 1.6  $(INCDIR)/version.h .ast16_check;)
ifneq (0,$(shell cat .ast16_check))
        PBXFLAGS_EXTRA+=-DAST16
endif

DUMMY:=$(shell ./check_for.sh 1.4  $(INCDIR)/version.h .ast14_check;)
ifneq (0,$(shell cat .ast14_check))
        PBXFLAGS_EXTRA+=-DAST14
endif


endif



PBXFLAGS_EXTRA+= -DAST_MODULE=\"chan_woomera\"

CC = gcc


CFLAGS = -D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -O6 
CCFLAGS = -Wall -Wstrict-prototypes -Wmissing-prototypes -g
LDFLAGS=-L lib/libteletone/.libs -L. -L/usr/local/lib -L ../../ssmg/libsangoma.trunk/.libs -lpthread -lsangoma -lm


PBXFLAGS= $(INCLUDES) -pipe  -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -g3 -D_REENTRANT  -DWOOMERA_CHAN_NAME=\"$(CHAN_NAME)\" -D_GNU_SOURCE $(PBXDEFINES)  -O6 -fomit-frame-pointer  -fPIC


all: chan_woomera.so 

chan_woomera.so: chan_woomera.o
	$(CC) -shared -Xlinker -x -o chan_woomera.so chan_woomera.o

chan_woomera.o: chan_woomera.c
	$(CC)  $(PBXFLAGS) $(PBXFLAGS_EXTRA) -c chan_woomera.c -o chan_woomera.o

clean:  
	find . -name '*.*o' | xargs rm -f

distclean: clean
	@echo OK

install: all  
	@if [ ! -d $(INSTALLPREFIX)$(PBXMODDIR) ]; then \
		mkdir -p $(INSTALLPREFIX)$(PBXMODDIR); \
	fi; 
	install -D -m 755 chan_woomera.so $(INSTALLPREFIX)$(PBXMODDIR)/chan_woomera.so
	@if [ ! -d $(INSTALLPREFIX)$(PBXCFGDIR) ]; then \
		mkdir -p $(INSTALLPREFIX)$(PBXCFGDIR); \
	fi; 
	@if [ -f woomera.conf ] && [ ! -e $(INSTALLPREFIX)$(PBXCFGDIR)/woomera.conf ]; then \
		install -D -m 755 woomera.conf $(INSTALLPREFIX)$(PBXCFGDIR)/woomera.conf; \
	fi
	@echo "chan_woomera Installed"

