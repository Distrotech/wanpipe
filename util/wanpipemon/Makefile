# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-2000 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= __LINUX__
DEBUG	= 2

PROD=wanpipemon

ifndef SYSINC
	SYSINC=/usr/include/wanpipe
endif

WLINC=../../patches/kdrivers/include
WEC=../../patches/kdrivers/wanec
WECOCT=../../patches/kdrivers/wanec/oct6100_api/include
WINC=/usr/include/wanpipe   

PWD=$(shell pwd)

WAN_VIRTUAL=
LXDIR=../lxdialog
ENABLE_GUI=YES
ENABLE_HDLC=YES
ENABLE_WANPIPEMON_ZAP=
PROTOCOL_DEFS=
ASTBROOT=/usr
#-DCONFIG_PRODUCT_WANPIPE_ADSL

CC = gcc 
CPP = gcc -E
CFLAGS = -Wall  -fno-strict-aliasing -Wstrict-prototypes 
CFLAGS += $(EXTRA_FLAGS) $(PROTOCOL_DEFS)
CFLAGS += $(GLOBAL_CFLAGS)
CFLAGS +=  -D$(OS_TYPE) -I../libsangoma -I../libsangoma/wanec -I$(WLINC) -I$(WEC) -I$(WECOCT) -I$(WINC)
CFLAGS += -I$(SYSINC) -I$(SYSINC)/oct6100_api -I$(SYSINC)/oct6100_api/oct6100api -I../ft1 -O2 -fomit-frame-pointer -DLOCALE -D_DEBUG_=$(DEBUG)
WAN_VIRTUAL=


# Tools options.

ifeq "${ENABLE_GUI}" "NO"
LX_OBJ =
GUI_OBJ =
LDLIBS =
else
LX_OBJ = $(LXDIR)/checklist.o $(LXDIR)/menubox.o $(LXDIR)/inputbox.o $(LXDIR)/util.o 
GUI_OBJ = wangui.o
LDLIBS = -lncurses -lsangoma -lm -L $(PWD)/../.lib/usr/lib


CFLAGS += -DWANPIPEMON_GUI -DLOCALE 

ifeq "${ENABLE_WANPIPEMON_ZAP}" "YES"
CFLAGS += -DWANPIPEMON_ZAP
LDLIBS += -lstdc++
endif

CFLAGS += -DCURSES_LOC="<ncurses.h>"       
endif

OBJS= wanpipemon.o fpipemon.o cpipemon.o ppipemon.o xpipemon.o atmpipemon.o aftpipemon.o usbpipemon.o dslpipemon.o prot_trace.o ss7pipemon.o bpipemon.o fe_lib.o xml_lib.o ../ft1/unixio.o
OBJS+=$(GUI_OBJ)

ifeq "${ENABLE_WANPIPEMON_ZAP}" "YES"
OBJS+=zapmon.o
endif

ifeq "${ENABLE_HDLC}" "YES"
CFLAGS += -DWANPIPEMON_HDLC_TRACE -I../wan_hdlc_eng/ 
OBJS += ../wan_hdlc_eng/wanpipe_hdlc.o
endif 

SRCS = $(OBJS:.o=.c)

OBJS+=$(LX_OBJ)


####### RULES ################################################################

all:    $(PROD) 
	@echo "Ok."

$(PROD):  $(OBJS)  

ncurses:
	@x=`find $(ASTBROOT)/lib/ /lib/ /usr/lib/ /usr/lib64/ /usr/local/lib/ -maxdepth 1 -name 'libncurses.*' 2> /dev/null` ;\
	if [ ! "$$x" ]; then \
		echo -e "\007" ;\
		echo ">> Unable to find the Ncurses libraries." ;\
		echo ">>" ;\
		echo ">> You must have Ncurses installed in order" ;\
		echo ">> to use 'make menuconfig'" ;\
		echo ;\
		exit 1 ;\
	fi

nenad:
	@echo "Ok. "  
	
install:
	@echo "Installing Wanpipe utilites in $(WAN_VIRTUAL)/usr/sbin"
	install -D $(PROD)	  $(WAN_VIRTUAL)/usr/sbin/$(PROD)

uninstall:
	@echo "Un-installing Wanpipe utilites from $(WAN_VIRTUAL)/usr/sbin"
	rm -f $(WAN_VIRTUAL)/usr/sbin/$(PROD)
clean:
	rm -f *.o
	rm -f $(PROD)
