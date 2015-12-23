################################################################################
# Sangoma MGD
#
# Author:   Anthony Minessale II <anthmct@yahoo.com>
#           Nenad Corbic <ncorbic@sangoma.com>
#
# Copyright:   (c) 2005 Anthony Minessale II
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
################################################################################
   
SMG_DTMF=YES

#Default kernel directory to be overwritten by user
#Kernel version and location
ifndef KVER
       KVER=$(shell uname -r)
endif
ifndef KMOD
       KMOD=/lib/modules/$(KVER)
endif
ifndef KDIR
       KDIR=$(KMOD)/build
endif
ifndef KINSTDIR
       KINSTDIR=$(KMOD)/kernel
endif

CC = gcc

ifndef DESTDIR
	ifdef INSTALLPREFIX
		DESTDIR=$(INSTALLPREFIX)
	else
		DESTDIR=
	endif
endif

ifndef BRI
BRI:=NO
ifneq (,$(wildcard ./.bri))
        BRI=$(shell cat .bri)
endif
endif

ifndef PRI
PRI:=NO
ifneq (,$(wildcard ./.pri))
        PRI=$(shell cat .pri)
endif
endif


$(shell echo $(BRI) > .bri)
$(shell echo $(PRI) > .pri)


SAFE_D=/etc/wanpipe/safe_sangoma.d
SMGCTRL_D=/etc/wanpipe/smg_ctrl.d

INCLUDES = -I ../../ssmg/libsangoma.trunk -I ../../ssmg/libsangoma.trunk/wanec -I. -I ../../patches/kdrivers/include -I ../../patches/kdrivers/wanec/oct6100_api/include -I ../../patches/kdrivers/wanec -I/usr/local/include -I../../patches/kdrivers/include -I/usr/include/wanpipe -Ilib/libteletone/src

CFLAGS = -D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -O6 


ifeq "${PRI}" "YES"
	CFLAGS += -DPRI_PROT -DSMG_CALLING_NAME
	NO_SS7:=YES
else
	ifeq "${BRI}" "YES"
		CFLAGS += -DBRI_PROT -DSMG_CALLING_NAME
		NO_SS7:=YES
	else
		ifeq "${NO_SS7}" "YES"
			BRI:=YES
			CFLAGS += -DBRI_PROT
		endif
	endif
endif

CCFLAGS = -Wall -Wstrict-prototypes -Wmissing-prototypes -g
LDFLAGS=-L lib/libteletone/.libs -L. -L/usr/local/lib -L ../../ssmg/libsangoma.trunk/.libs -lpthread -lsangoma -lm

#Enable memory leak subsystem
#Not to be used in production
#CFLAGS += -DSMG_MEMORY_DEBUG


ifeq "${SMG_DTMF}" "YES"
LDFLAGS+= -lteletone
CFLAGS+= -DSMG_DTMF_ENABLE
endif


all: sangoma_mgd  

libs: 
	$(shell cd lib/libteletone; ./configure --prefix=$(DESTDIR); cd ../../; )
	$(MAKE) -C lib/libteletone all

switch_buffer.o: switch_buffer.c switch_buffer.h
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS)  -c -o switch_buffer.o switch_buffer.c

call_signal.o: call_signal.c call_signal.h
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS)  -c -o call_signal.o call_signal.c

sangoma_mgd_memdbg.o: sangoma_mgd_memdbg.c sangoma_mgd_memdbg.h 
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS)  -c -o sangoma_mgd_memdbg.o sangoma_mgd_memdbg.c

sangoma_mgd_logger.o: sangoma_mgd_logger.c 
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS)  -c -o sangoma_mgd_logger.o sangoma_mgd_logger.c

sangoma_mgd_ip_bridge.o: sangoma_mgd_ip_bridge.c 
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS)  -c -o sangoma_mgd_ip_bridge.o sangoma_mgd_ip_bridge.c

sangoma_mgd.o: sangoma_mgd.c sangoma_mgd.h sigboost.h
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -c -o sangoma_mgd.o sangoma_mgd.c 

sangoma_mgd: sangoma_mgd.o sangoma_mgd_memdbg.o  sangoma_mgd_logger.o sangoma_mgd_ip_bridge.o call_signal.o switch_buffer.o sigboost.h sangoma_mgd_memdbg.h
	rm -fr core*
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -o sangoma_mgd sangoma_mgd.o sangoma_mgd_memdbg.o  sangoma_mgd_logger.o sangoma_mgd_ip_bridge.o switch_buffer.o  call_signal.o $(LDFLAGS)



clean:  old_cleanup
	find . -name '*.o' | xargs rm -f
	rm -fr sangoma_mgd pritest *.o *.so *~ *core* *.so* *.a
	make -C lib/libteletone clean

distclean: clean
	@echo OK

install: all install_smg old_cleanup

install_smg: old_cleanup 
	install -D -m 755 sangoma_mgd $(DESTDIR)/usr/sbin/sangoma_mgd
	@if [ ! -e $(DESTDIR)/etc/sangoma_mgd.conf ]; then \
		install -D -m 755 sangoma_mgd.conf.sample $(DESTDIR)/etc/sangoma_mgd.conf; \
	fi
	
	install -D -m 755 ./safe_sangoma $(DESTDIR)/usr/sbin/safe_sangoma
	install -D -m 755 ./smg_ctrl $(DESTDIR)/usr/sbin/smg_ctrl

	@if [ ! -f  $(DESTDIR)/etc/wanpipe/safe_sangoma.rc ]; then \
	install -D -m 755 ./rc/safe_sangoma.rc $(DESTDIR)/etc/wanpipe/safe_sangoma.rc; \
	fi

	@if [ ! -d ${SAFE_D} ]; then   \
		mkdir -p ${SAFE_D}; \
	fi
	@if [ ! -d ${SMGCTRL_D} ]; then   \
		mkdir -p ${SMGCTRL_D}; \
	fi
	

ifeq "${PRI}" "YES"
	@echo "PRI control scripts installed"
	@if [ ! -e $(DESTDIR)/etc/sangoma_mgd.conf ]; then \
		install -D -m 755 sangoma_mgd.conf.sample.pri $(DESTDIR)/etc/sangoma_mgd.conf; \
	fi
	install -D -m 755 rc/smg.rc.pri $(DESTDIR)/etc/wanpipe/smg.rc
else
ifeq "${BRI}" "YES"
	@echo "BRI control scripts installed"
	@if [ ! -e $(DESTDIR)/etc/sangoma_mgd.conf ]; then \
		install -D -m 755 sangoma_mgd.conf.sample.bri $(DESTDIR)/etc/sangoma_mgd.conf; \
	fi
	install -D -m 755 rc/smg.rc.bri $(DESTDIR)/etc/wanpipe/smg.rc
else
	@echo "SS7 control scripts installed"
	@if [ ! -e $(DESTDIR)/etc/sangoma_mgd.conf ]; then \
		install -D -m 755 sangoma_mgd.conf.sample.ss7 $(DESTDIR)/etc/sangoma_mgd.conf; \
	fi
	@if [ -e $(DESTDIR)/usr/local/ss7box/ss7boost ]; then \
		echo "install -D -m 755 rc/smg.rc.ss7boost $(DESTDIR)/etc/wanpipe/smg.rc"; \
		install -D -m 755 rc/smg.rc.ss7boost $(DESTDIR)/etc/wanpipe/smg.rc; \
	else  \
		echo "install -D -m 755 rc/smg.rc.isupd $(DESTDIR)/etc/wanpipe/smg.rc"; \
		install -D -m 755 rc/smg.rc.isupd $(DESTDIR)/etc/wanpipe/smg.rc; \
	fi

	install -D -m 755 scripts/init.d/smgss7_init_ctrl $(DESTDIR)/etc/init.d/smgss7_init_ctrl
	install -D -m 755 scripts/init.d/smgss7_init_ctrl $(DESTDIR)/usr/sbin/smgss7_init_ctrl
	install -D -m 755 scripts/init.d/ss7boxd_monitor.sh $(DESTDIR)/usr/sbin/ss7boxd_monitor.sh

endif	
endif
	@echo "sangoma_mgd Installed"

old_cleanup:
	./scripts/old_cleanup.sh


install_all: all install_smg 

uninstall: 
	/bin/rm $(DESTDIR)/usr/sbin/sangoma_mgd $(DESTDIR)/etc/sangoma_mgd.conf



