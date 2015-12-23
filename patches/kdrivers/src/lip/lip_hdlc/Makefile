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
ARCH=i686

# Project File Paths.
TMPDIR	= tmp
SYSINC	= /lib/modules/$(shell uname -r)/build/include
ARCH    := $(shell uname -m)
MODDIR  = modinfo
REGPARM = NO
ARCH_DEF= -mpreferred-stack-boundary=2 -march=$(ARCH)
LD=ld

ifeq "${ARCH}" "x86_64"

LDFLAGS=-m elf_x86_64

CFLAGS=-nostdinc -iwithprefix include -D__LINUX__ -Dlinux -D__KERNEL__ -DWANLIP_DRIVER -Iinclude/ -I../include -I$(SYSINC) -I../include/common  -Wall -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common  -mno-red-zone -mcmodel=kernel -pipe -fno-reorder-blocks    -Wno-sign-compare  -O2 -fomit-frame-pointer -DMODULE

else

LDFLAGS=-m elf_i386

CFLAGS=-nostdinc -iwithprefix include -DWANLIP_DRIVER -D__KERNEL__  -D__LINUX__ -I../include -Iinclude -I$(SYSINC) -Wall -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -pipe $(ARCH_DEF) -O2 

endif

ifeq "${REGPARM}" "YES"
	CFLAGS+= -mregparm=3
endif


####### RULES ################################################################

all:	wanpipe_hdlc.o
	@echo "Ok."

# ----- Multiprotocol WAN router module --------------------------------------
wanpipe_hdlc.o: $(TMPDIR)/wanpipe_hdlc_iface.o $(TMPDIR)/wanpipe_hdlc_eng.o 
	$(LD) $(LDFLAGS) -r -o $@ $^
	chmod 664 $@

$(TMPDIR)/wanpipe_hdlc_iface.o:	wanpipe_hdlc_iface.c 
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_hdlc_iface -DKBUILD_MODNAME=wanpipe_hdlc -c -o $@ $<

$(TMPDIR)/wanpipe_hdlc_eng.o:	wanpipe_hdlc_eng.c 
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_hdlc_iface -DKBUILD_MODNAME=wanpipe_hdlc -c -o $@ $<

clean:
	rm -f *.o
	rm -f tmp/*.o
	rm -f mod/*.o
