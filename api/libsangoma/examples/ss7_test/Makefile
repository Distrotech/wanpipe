CC=gcc
WANINC=$(WAN_VIRTUAL)/usr/include/wanpipe

WLINC=../../../../patches/kdrivers/include
WEC=../../../../patches/kdrivers/wanec
WECOCT=../../../../patches/kdrivers/wanec/oct6100_api/include

INCLUDES=-I../ -I/usr/include -I/usr/src/libpri  -I$(WLINC) -I$(WEC) -I$(WECOCT) -I$(WANINC) -I$(WANINC)/oct6100_api -I$(WANINC)/oct6100_api/oct6100api -I. -Ihdlc -I/usr/local/include 
CFLAGS += -D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -DWANPIPE_TDM_API 
CCFLAGS += -Wall -Wstrict-prototypes -Wmissing-prototypes -g

all: hdlc_test

everything:  hdlc_test.c

hdlc_test.o: hdlc_test.c
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -c -o hdlc_test.o hdlc_test.c 

hdlc_test: hdlc_test.o  
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) $(LDFLAGS) -o hdlc_test hdlc_test.o hdlc/wanpipe_hdlc.c -L. -lsangoma -lm -lpthread

clean:
	rm -fr hdlc_test *.o *.so *~ *core* *.so* *.a

distclean: clean
	@echo OK
