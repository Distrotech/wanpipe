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

ARCH    = $(shell uname -m)

OUTDIR  = mod
TMPDIR  = tmp
MODDIR  = modinfo
WANDIR  = ../wanpipe
FRDIR   = ../fr
LAPBDIR = ../lapb
LAPDDIR = ../lapd
XDLCDIR = ../xdlc
SPPPDIR = ../sppp
XMTP2DIR= ../xmtp2
X25DIR  = ../x25
LIP_ATM_DIR  = ../lip_atm
WAN_ROUTER_DIR = /wanpipe/code/router

WAN_TTY_SUPPORT=YES
WAN_FR_SUPPORT=YES
WAN_X25_SUPPORT=NO
WAN_CHDLC_PPP_SUPPORT=YES
WAN_XDLC_SUPPORT=YES
WAN_LAPB_SUPPORT=NO
WAN_XMTP2_SUPPORT=NO
WANPIPE_LIP_ATM=YES
WAN_LAPD_SUPPORT=NO
REGPARM=YES

LIP_LINKED_PROTS=

KERN := $(shell grep 2.4 /usr/src/linux/include/linux/version.h)
KERN_V26 := $(shell grep 2.6 /usr/src/linux/include/linux/version.h)

ifneq "${KERN}" ""

MODTYPE=o
K_WAN_DIR=drivers/net/wan

else

ifneq "${KERN_V26}" ""

ifeq "${ARCH}" "x86_64"
#LDFLAGS=-m elf_x86_64
LD_ELF=-m elf_x86_64
else
#LDFLAGS=-m elf_i386
LD_ELF=-m elf_i386
endif

MODTYPE=ko
K_WAN_DIR=drivers/net/wan

else

MODTYPE=o
K_WAN_DIR=drivers/net

endif

endif

CFLAGS=$(shell cat /wanpipe/GCFLAGS)
CFLAGS+=-DWANLIP_DRIVER
#CFLAGS=-Wp,-MD,.wanpipe_lip.o.d -nostdinc -iwithprefix include -D__KERNEL__ -D__LINUX__ -Iinclude -I/usr/src/linux/include  -Wall -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -pipe -mpreferred-stack-boundary=2 -march=$(ARCH) -I/usr/src/linux/include/asm-i386/mach-default -O2  -DWANLIP_DRIVER -DMODULE

#CFLAGS+= -DCONFIG_PRODUCT_WANPIPE_FR -DCONFIG_PRODUCT_WANPIPE_CHDLC -DCONFIG_PRODUCT_WANPIPE_PPP -DCONFIG_PRODUCT_WANPIPE_XDLC -DCONFIG_PRODUCT_WANPIPE_LAPB -DCONFIG_PRODUCT_WANPIPE_XMTP2

ifeq "$(WAN_TTY_SUPPORT)" "YES"
CFLAGS+= -DWPLIP_TTY_SUPPORT
TTY_SUPPORT_FILE=$(TMPDIR)/wanpipe_lip_tty.o
endif

ifeq "$(WAN_FR_SUPPORT)" "YES"
CFLAGS+= -DCONFIG_PRODUCT_WANPIPE_FR
LIP_LINKED_PROTS+= $(FRDIR)/wanpipe_fr.o
endif

ifeq "${WANPIPE_LIP_ATM}" "YES"
CFLAGS+=-DCONFIG_PRODUCT_WANPIPE_LIP_ATM
LIP_LINKED_PROTS+= $(LIP_ATM_DIR)/wanpipe_lip_atm.o
endif

ifeq "$(WAN_X25_SUPPORT)" "YES"
CFLAGS+= -DCONFIG_PRODUCT_WANPIPE_X25
LIP_LINKED_PROTS+= $(X25DIR)/wanpipe_x25.o
endif

ifeq "$(WAN_CHDLC_PPP_SUPPORT)" "YES"
CFLAGS+= -DCONFIG_PRODUCT_WANPIPE_CHDLC -DCONFIG_PRODUCT_WANPIPE_PPP
LIP_LINKED_PROTS+= $(SPPPDIR)/wanpipe_sppp.o
endif

ifeq "$(WAN_XDLC_SUPPORT)" "YES"
CFLAGS+= -DCONFIG_PRODUCT_WANPIPE_XDLC
LIP_LINKED_PROTS+= $(XDLCDIR)/wanpipe_xdlc.o
endif

ifeq "$(WAN_LAPB_SUPPORT)" "YES"
CFLAGS+= -DCONFIG_PRODUCT_WANPIPE_LAPB
LIP_LINKED_PROTS+= $(LAPBDIR)/wanpipe_lapb.o
endif

ifeq "$(WAN_LAPD_SUPPORT)" "YES"
CFLAGS+= -DCONFIG_PRODUCT_WANPIPE_LAPD
LIP_LINKED_PROTS+= $(LAPDDIR)/wanpipe_lapd.o
endif

ifeq "$(WAN_XMTP2_SUPPORT)" "YES"
CFLAGS+= -DCONFIG_PRODUCT_WANPIPE_XMTP2
LIP_LINKED_PROTS+= $(XMTP2DIR)/wanpipe_xmtp2.o
endif
####### RULES ################################################################

all:	$(OUTDIR)/wanpipe_lip.$(MODTYPE)
	@echo "Ok."

# ----- Multiprotocol WAN router module --------------------------------------


$(OUTDIR)/wanpipe_lip.ko: $(OUTDIR)/wanpipe_lip.o $(MODDIR)/wanpipe_lip.mod.o
	ld $(LD_ELF) -r -o $@ $^
	chmod 664 $@

$(MODDIR)/wanpipe_lip.mod.o:	$(MODDIR)/wanpipe_lip.mod.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_lip -DKBUILD_MODNAME=wanpipe_lip -c -o $@ $^
	chmod 664 $@

#----------------------------------------------------

$(OUTDIR)/wanpipe_lip.o: $(TMPDIR)/wanpipe_lip_iface.o $(TMPDIR)/wanpipe_lip_sub.o $(TMPDIR)/wanpipe_lip_netdev.o $(TMPDIR)/wanpipe_lip_prot.o $(TMPDIR)/wanpipe_lip_bh.o $(TMPDIR)/wanpipe_lip_ipx.o $(TMPDIR)/wanpipe_abstr.o $(TMPDIR)/wanpipe_linux_iface.o $(TTY_SUPPORT_FILE) $(LIP_LINKED_PROTS)
	ld $(LD_ELF) -r -o $@ $^
	chmod 664 $@


#-----------------------------------------------------

$(TMPDIR)/wanpipe_lip_iface.o:	wanpipe_lip_iface.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_lip_iface -DKBUILD_MODNAME=wanpipe_lip -c -o $@ $^
	chmod 664 $@

$(TMPDIR)/wanpipe_lip_sub.o:	wanpipe_lip_sub.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_lip_sub -DKBUILD_MODNAME=wanpipe_lip -c -o $@ $^
	chmod 664 $@

$(TMPDIR)/wanpipe_lip_netdev.o:	wanpipe_lip_netdev.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_lip_netdev -DKBUILD_MODNAME=wanpipe_lip -c -o $@ $^
	chmod 664 $@

$(TMPDIR)/wanpipe_lip_prot.o:	wanpipe_lip_prot.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_lip_prot -DKBUILD_MODNAME=wanpipe_lip -c -o $@ $^
	chmod 664 $@

$(TMPDIR)/wanpipe_lip_bh.o:	wanpipe_lip_bh.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_lip_bh -DKBUILD_MODNAME=wanpipe_lip -c -o $@ $^
	chmod 664 $@

$(TMPDIR)/wanpipe_lip_tty.o: 	wanpipe_lip_tty.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_lip_tty -DKBUILD_MODNAME=wanpipe_lip -c -o $@ $^
	chmod 664 $@

$(TMPDIR)/wanpipe_lip_ipx.o:   wanpipe_lip_ipx.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_lip_ipx -DKBUILD_MODNAME=wanpipe_lip -c -o $@ $^
	chmod 664 $@

$(TMPDIR)/wanpipe_abstr.o:	$(WANDIR)/wanpipe_abstr.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_abstr -DKBUILD_MODNAME=wanpipe_lip -c -o $@ $^
	chmod 664 $@

$(TMPDIR)/wanpipe_linux_iface.o:   $(WAN_ROUTER_DIR)/wanpipe_linux_iface.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_linux_iface -DKBUILD_MODNAME=wanpipe_lip -c -o $@ $^
	chmod 664 $@


$(FRDIR)/wanpipe_fr.o:
	make -C $(FRDIR) clean
	make -C $(FRDIR) all REGPARM=$(REGPARM)

$(LIP_ATM_DIR)/wanpipe_lip_atm.o:
	make -C $(LIP_ATM_DIR) clean
	make -C $(LIP_ATM_DIR) all REGPARM=$(REGPARM)


$(LAPBDIR)/wanpipe_lapb.o: 
	make -C $(LAPBDIR) clean
	make -C $(LAPBDIR) all REGPARM=$(REGPARM)
	
$(LAPDDIR)/wanpipe_lapd.o: 
	make -C $(LAPDDIR) clean
	make -C $(LAPDDIR) all REGPARM=$(REGPARM)

$(XDLCDIR)/wanpipe_xdlc.o:
	make -C $(XDLCDIR) clean
	make -C $(XDLCDIR) all REGPARM=$(REGPARM)

$(SPPPDIR)/wanpipe_sppp.o:
	make -C $(SPPPDIR) clean
	make -C $(SPPPDIR) all REGPARM=$(REGPARM)

$(XMTP2DIR)/wanpipe_xmtp2.o:
	make -C $(XMTP2DIR) clean
	make -C $(XMTP2DIR) all REGPARM=$(REGPARM)

$(X25DIR)/wanpipe_x25.o:
	make -C $(X25DIR) clean
	make -C $(X25DIR) all REGPARM=$(REGPARM)

clean:
	rm -f wanpipe_lip.$(MODTYPE)
	rm -f *.o*
	rm -f mod/*.*o
	rm -f tmp/*.*o
	rm -f modinfo/*.*o
	make -C ../fr clean
	make -C ../lapb clean
	make -C ../lapd clean
	make -C ../xdlc clean
	make -C $(SPPPDIR) clean
	make -C $(XMTP2DIR) clean
	make -C ../x25 clean

install:
	install -D $(OUTDIR)/wanpipe_lip.${MODTYPE} /lib/modules/$(shell uname -r)/kernel/net/wanrouter/wanpipe_lip.${MODTYPE}

uninstall:
	rm -f /lib/modules/$(shell uname -r)/kernel/net/wanrouter/wanpipe_lip.${MODTYPE}
