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
PROD	= wanrouter
VERSION	= 2.3.0
DEBUG	= 2
OS_TYPE =__LINUX__
WANPIPE_TDM_VOICE=NO
WANPIPE_TDM_VOICE_DCHAN=NO
WANPIPE_ADSL=NO
WANPIPE_ATM=NO
WANPIPE_ASYHDLC=NO
KERNEL_HDLC_ENABLED=NO

# Project File Paths.
TMPDIR	= tmp
OUTDIR	= mod
LOCINC	= /usr/src/linux/include
VPATH	= $(LOCINC):$(TMPDIR)
ARCH    = $(shell uname -m)
ANNEXG_INC=../include/annexg
ADSLDIR = ../adsl

MODDIR=modinfo

TARGETS	= $(OUTDIR)/wanrouter.o \
	  $(OUTDIR)/sdladrv.o \
	  $(OUTDIR)/wanpipe.o \
	  $(OUTDIR)/wanpipe_syncppp.o

#KERN := $(shell uname -r | grep 2.4)
KERN := $(shell grep 2.4 /usr/src/linux/include/linux/version.h)
KERN_V26 := $(shell grep 2.6 /usr/src/linux/include/linux/version.h)
LD_ELF=

ifneq "${KERN}" ""

#Found 2.4 Kernel

MODTYPE=o
K_WAN_DIR=drivers/net/wan

else 

#Found 2.6 Kernel
LD_ELF=-m elf_i386

MODTYPE=ko
K_WAN_DIR=drivers/net/wan

endif

PRODUCT_DEFINES= -DCONFIG_PRODUCT_WANPIPE_BASE -DCONFIG_PRODUCT_WANPIPE_BITSTRM -DCONFIG_PRODUCT_WANPIPE_BSCSTRM -DCONFIG_PRODUCT_WANPIPE_SS7 -DSS7_USER_ID=0x00 -DCONFIG_PRODUCT_WANPIPE_SDLC -DCONFIG_PRODUCT_WANPIPE_EDU -DCONFIG_PRODUCT_WANPIPE_POS -DCONFIG_PRODUCT_WANPIPE_FR -DCONFIG_PRODUCT_WANPIPE_CHDLC -DCONFIG_PRODUCT_WANPIPE_PPP -DCONFIG_PRODUCT_WANPIPE_MULTPROT -DCONFIG_PRODUCT_WANPIPE_MULTFR -DCONFIG_PRODUCT_WANPIPE_X25 -DCONFIG_PRODUCT_WANPIPE_AFT -DCONFIG_PRODUCT_WANPIPE_AFT_TE1 -DCONFIG_PRODUCT_WANPIPE_AFT_TE3 -DCONFIG_PRODUCT_WANPIPE_AFT_TE1_SS7 -DCONFIG_PRODUCT_WANPIPE_ADCCP -DCONFIG_PRODUCT_WANPIPE_ANNEXG

WANPIPE_FILE_LIST= sdlamain.o sdla_fr.o sdla_ppp.o sdla_chdlc.o sdla_x25.o sdla_ft1.o wanpipe_multppp.o sdla_te1.o sdla_56k.o sdla_edu.o sdla_bsc.o sdla_bitstrm.o sdla_bscstrm.o sdla_mp_fr.o sdla_sdlc.o sdla_pos.o sdla_xilinx.o sdla_aft_te1.o sdla_aft_te3.o sdla_te3.o wanpipe_utils.o sdla_adccp.o wanpipe_abstr.o wanpipe_linux_iface.o  

ifeq "${WANPIPE_ADSL}" "YES"
PRODUCT_DEFINES+= -DCONFIG_PRODUCT_WANPIPE_ADSL
WANPIPE_FILE_LIST+= sdla_adsl.o ../adsl/wanpipe_adsl.o
endif

ifeq "${WANPIPE_ATM}" "YES"
PRODUCT_DEFINES+= -DCONFIG_PRODUCT_WANPIPE_ATM
WANPIPE_FILE_LIST+= sdla_atm.o ../atm/wanpipe_atm.o
endif

ifeq "${WANPIPE_ASYHDLC}" "YES"
PRODUCT_DEFINES+= -DCONFIG_PRODUCT_WANPIPE_ASYHDLC
WANPIPE_FILE_LIST+= sdla_asyhdlc.o 
endif

ifeq "${WANPIPE_TDM_VOICE}" "YES"
PRODUCT_DEFINES+= -DCONFIG_PRODUCT_WANPIPE_TDM_VOICE
WANPIPE_FILE_LIST+= sdla_tdmv.o
CFLAGS+=-I/usr/src/zaptel 
endif

ifeq "${WANPIPE_TDM_VOICE_DCHAN}" "YES"
PRODUCT_DEFINES+= -DCONFIG_PRODUCT_WANPIPE_TDM_VOICE_DCHAN
endif

ifeq "${KERNEL_HDLC_ENABLED}" "YES"
PRODUCT_DEFINES+= -DCONFIG_PRODUCT_WANPIPE_GENERIC
endif

TARGETS	= $(OUTDIR)/wanrouter.$(MODTYPE) \
	  $(OUTDIR)/sdladrv.$(MODTYPE) \
	  $(OUTDIR)/wanpipe.$(MODTYPE) \
	  $(OUTDIR)/wanpipe_syncppp.$(MODTYPE) 

# Miscellaneous.
TAGFILE	= net/wanrouter/patchlevel

####### RULES ################################################################


all:	$(TARGETS)
	@echo "Ok."

patch:
	../../scripts/Mkpatch $(PROD) $(VERSION)

$(OUTDIR)/wanrouter.ko:	$(OUTDIR)/wanrouter.o $(MODDIR)/wanrouter.mod.o
	ld $(LD_ELF) -r -o $@ $^
	chmod 664 $@

$(OUTDIR)/sdladrv.ko:	$(OUTDIR)/sdladrv.o $(MODDIR)/sdladrv.mod.o
	ld $(LD_ELF) -r -o $@ $^
	chmod 664 $@

$(OUTDIR)/wanpipe.ko:	$(OUTDIR)/wanpipe.o $(MODDIR)/wanpipe.mod.o
	ld $(LD_ELF) -r -o $@ $^
	chmod 664 $@

$(OUTDIR)/wanpipe_syncppp.ko:	$(OUTDIR)/wanpipe_syncppp.o $(MODDIR)/wanpipe_syncppp.mod.o
	ld $(LD_ELF) -r -o $@ $^
	chmod 664 $@


$(MODDIR)/wanrouter.mod.o:	$(MODDIR)/wanrouter.mod.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanrouter -DKBUILD_MODNAME=wanrouter -c -o $@ $<
	chmod 664 $@
	
$(MODDIR)/sdladrv.mod.o:	$(MODDIR)/sdladrv.mod.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdladrv -DKBUILD_MODNAME=sdladrv -c -o $@ $<
	chmod 664 $@

$(MODDIR)/wanpipe.mod.o:	$(MODDIR)/wanpipe.mod.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(MODDIR)/wanpipe_syncppp.mod.o:	$(MODDIR)/wanpipe_syncppp.mod.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_syncppp  -DKBUILD_MODNAME=wanpipe_syncppp -c -o $@ $<
	chmod 664 $@




# ----- Multiprotocol WAN router module --------------------------------------

$(OUTDIR)/wanrouter.o:	wanmain.o wanproc.o waniface.o
	ld $(LD_ELF) -r -o $@ $^
	chmod 664 $@

$(TMPDIR)/wanmain.o:	wanmain.c 
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanmain -DKBUILD_MODNAME=wanrouter -c -o $@ $<

$(TMPDIR)/wanproc.o:	common/wanproc.c 
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanproc -DKBUILD_MODNAME=wanrouter -c -o $@ $<

$(TMPDIR)/waniface.o:	waniface.c 
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=waniface -DKBUILD_MODNAME=wanrouter -c -o $@ $<

# ----- SDLA Support Module --------------------------------------------------


$(OUTDIR)/sdladrv.o:	common/sdladrv.c 
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdladrv -DKBUILD_MODNAME=sdladrv -c -o $@ $<
	chmod 664 $@

$(OUTDIR)/wanpipe_syncppp.o:	wanpipe_syncppp.c 
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_syncppp -DKBUILD_MODNAME=wanpipe_syncppp -c -o $@ $<
	chmod 664 $@

# ----- WANPIPE(tm) Driver Module --------------------------------------------

$(OUTDIR)/wanpipe.o:	${WANPIPE_FILE_LIST}
	ld $(LD_ELF) -r -o $@ $^
	chmod 664 $@

$(TMPDIR)/sdlamain.o:	sdlamain.c
	$(CC) $(CFLAGS)  -DKBUILD_BASENAME=sdlamain -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_x25.o:	sdla_x25.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_x25 -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_fr.o:	sdla_fr.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_fr -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_ppp.o:	sdla_ppp.c 
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_ppp -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_chdlc.o:	sdla_chdlc.c 
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_chdlc -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_ft1.o:	sdla_ft1.c 
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_ft1 -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/wanpipe_multppp.o:	wanpipe_multppp.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_multppp -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_te1.o:	common/sdla_te1.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_te1 -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_56k.o:	common/sdla_56k.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_56k -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_bitstrm.o:	sdla_bitstrm.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_bitstrm -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_edu.o:	sdla_edu.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_edu -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_bsc.o:	sdla_bsc.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_bsc -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_ss7.o:	sdla_ss7.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_ss7 -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_bscstrm.o:	sdla_bscstrm.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_bscstrm -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_mp_fr.o:	sdla_mp_fr.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_mp_fr -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_sdlc.o:	sdla_sdlc.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_sdlc -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_asyhdlc.o:	sdla_asyhdlc.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_asyhdlc -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_tdmv.o:	common/sdla_tdmv.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_tdmv -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_atm.o:	common/sdla_atm.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_atm -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_xilinx.o:	common/sdla_xilinx.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_xilinx -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_aft_te1.o:	common/sdla_aft_te1.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_aft_te1 -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_aft_te3.o:	common/sdla_aft_te3.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_aft_te3 -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_aft_te1_ss7.o:	common/sdla_aft_te1_ss7.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_aft_te1_ss7 -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@	

$(TMPDIR)/sdla_te3.o:	common/sdla_te3.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_te3 -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_pos.o:	common/sdla_pos.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_pos -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/wanpipe_utils.o:	common/wanpipe_utils.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_utils -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/wanpipe_abstr.o:	common/wanpipe_abstr.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_abstr -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@	

$(TMPDIR)/sdla_adsl.o:  common/sdla_adsl.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_adsl -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/sdla_adccp.o:  sdla_adccp.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=sdla_adccp -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

$(TMPDIR)/wanpipe_linux_iface.o:  wanpipe_linux_iface.c
	$(CC) $(CFLAGS) -DKBUILD_BASENAME=wanpipe_linux_iface -DKBUILD_MODNAME=wanpipe -c -o $@ $<
	chmod 664 $@

../adsl/wanpipe_adsl.o:
	make -C ../adsl clean
	make -C ../adsl all

../atm/wanpipe_atm.o:
	make -C ../atm clean
	make -C ../atm all
	
clean:
	rm -f mod/*.*o
	rm -f tmp/*.*o
	rm -f modinfo/*.o
	rm -f *.o
	make -C ../adsl clean
	make -C ../atm clean

ver:
	@echo "Building 2.4 = ${KERN}"
	@echo "Building 2.6 = ${KERN_V26}"

install:
	install -D $(OUTDIR)/wanrouter.${MODTYPE} /lib/modules/$(shell uname -r)/kernel/net/wanrouter/wanrouter.${MODTYPE} 
	install -D $(OUTDIR)/sdladrv.${MODTYPE} /lib/modules/$(shell uname -r)/kernel/$(K_WAN_DIR)/sdladrv.${MODTYPE}
	install -D $(OUTDIR)/wanpipe_syncppp.${MODTYPE} /lib/modules/$(shell uname -r)/kernel/$(K_WAN_DIR)/wanpipe_syncppp.${MODTYPE}
	install -D $(OUTDIR)/wanpipe.${MODTYPE} /lib/modules/$(shell uname -r)/kernel/$(K_WAN_DIR)/wanpipe.${MODTYPE}


uninstall:
	\rm -f /lib/modules/$(shell uname -r)/kernel/net/wanrouter/wanrouter.* 
	\rm -f /lib/modules/$(shell uname -r)/kernel/$(K_WAN_DIR)/wanrouter.*
	\rm -f /lib/modules/$(shell uname -r)/kernel/$(K_WAN_DIR)/sdladrv.*
	\rm -f /lib/modules/$(shell uname -r)/kernel/$(K_WAN_DIR)/wanpipe_syncppp.*
	\rm -f /lib/modules/$(shell uname -r)/kernel/$(K_WAN_DIR)/wanpipe.*

###### END ###################################################################
