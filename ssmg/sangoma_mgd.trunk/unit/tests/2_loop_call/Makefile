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

SMG_DIR=../../..
UNIT_CORE_DIR=../../core
TMP=tmp

INCLUDES = -I$(KDIR)/include -I../ -I$(SMG_DIR) -I$(UNIT_CORE_DIR)/ -I. -I../ -I/usr/local/include -I/usr/include/wanpipe

CFLAGS = -D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -O6 -march=i686 
CCFLAGS = -Wall -Wstrict-prototypes -Wmissing-prototypes -g -DSANGOMA_UNIT_TESTER
LDFLAGS=-L lib/libteletone/.libs -L. -L/usr/local/lib -L ../../ssmg/libsangoma.trunk/.libs -lpthread -lsangoma -lm


ifeq "${SMG_DTMF}" "YES"
LDFLAGS+= -lteletone
CFLAGS+= -DSMG_DTMF_ENABLE
endif


all: smg_unit  

$(TMP)/switch_buffer.o: $(SMG_DIR)/switch_buffer.c $(SMG_DIR)/switch_buffer.h
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS)  -c -o $(TMP)/switch_buffer.o $(SMG_DIR)/switch_buffer.c

$(TMP)/call_signal.o: $(SMG_DIR)/call_signal.c $(SMG_DIR)/call_signal.h
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS)  -c -o $(TMP)/call_signal.o $(SMG_DIR)/call_signal.c

$(TMP)/sangoma_mgd_unit.o: $(UNIT_CORE_DIR)/sangoma_mgd_unit.c
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -c -o $(TMP)/sangoma_mgd_unit.o $(UNIT_CORE_DIR)/sangoma_mgd_unit.c

$(TMP)/test.o:	test.c
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -c -o $(TMP)/test.o test.c 

smg_unit: $(TMP)/test.o $(TMP)/sangoma_mgd_unit.o $(TMP)/call_signal.o $(TMP)/switch_buffer.o $(SMG_DIR)/sigboost.h
	rm -fr core*
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -o smg_unit $(TMP)/test.o  $(TMP)/sangoma_mgd_unit.o $(TMP)/switch_buffer.o $(TMP)/call_signal.o $(LDFLAGS)


clean:	
	rm -f smg_unit
	rm -f tmp/*
	rm -f *.o

