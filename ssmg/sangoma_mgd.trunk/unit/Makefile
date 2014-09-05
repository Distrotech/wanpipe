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
INSTALLPREFIX=

INCLUDES = -I$(KDIR)/include -I../ -I ../../ssmg/libsangoma.trunk -I. -I../ -I ../../patches/kdrivers/include -I ../../patches/kdrivers/wanec/oct6100_api/include -I ../../patches/kdrivers/wanec -I/usr/local/include -I../../patches/kdrivers/include -I/usr/include/wanpipe -Ilib/libteletone/src

CFLAGS = -D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -O6 -march=i686
CCFLAGS = -Wall -Wstrict-prototypes -Wmissing-prototypes -g -DSANGOMA_UNIT_TESTER
LDFLAGS=-L lib/libteletone/.libs -L. -L/usr/local/lib -L ../../ssmg/libsangoma.trunk/.libs -lpthread -lsangoma -lm


ifeq "${SMG_DTMF}" "YES"
LDFLAGS+= -lteletone
CFLAGS+= -DSMG_DTMF_ENABLE
endif


all: sangoma_mgd_unit  

libs: 
	$(shell cd lib/libteletone; ./configure --prefix=$(INSTALLPREFIX); cd ../../; )
	$(MAKE) -C lib/libteletone all

switch_buffer.o: ../switch_buffer.c ../switch_buffer.h
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS)  -c -o switch_buffer.o ../switch_buffer.c

call_signal.o: ../call_signal.c ../call_signal.h
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS)  -c -o call_signal.o ../call_signal.c

sangoma_mgd_unit: sangoma_mgd_unit.o call_signal.o switch_buffer.o ../sigboost.h
	rm -fr core*
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -o sangoma_mgd_unit sangoma_mgd_unit.o switch_buffer.o call_signal.o $(LDFLAGS)

sangoma_mgd_unit.o: sangoma_mgd_unit.c
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -c -o sangoma_mgd_unit.o sangoma_mgd_unit.c


clean:	
	rm -f sangoma_mgd_unit
	rm -f *.o

