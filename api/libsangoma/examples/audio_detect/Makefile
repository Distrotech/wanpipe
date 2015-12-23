CC=gcc
WANINC=$(WAN_VIRTUAL)/usr/include/wanpipe

WLINC=../../../../patches/kdrivers/include
WEC=../../../../patches/kdrivers/wanec
WECOCT=../../../../patches/kdrivers/wanec/oct6100_api/include

INCLUDES=-I../ -I/usr/include -I/usr/src/libpri  -I$(WLINC) -I$(WEC) -I$(WECOCT) -I$(WANINC) -I$(WANINC)/oct6100_api -I$(WANINC)/oct6100_api/oct6100api -I. -Ihdlc -I/usr/local/include 
CFLAGS += -D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -O2 -DWANPIPE_TDM_API 
CCFLAGS += -Wall -Wstrict-prototypes -Wmissing-prototypes -g

all: audio_detect

everything:  audio_detect.c

audio_detect.o: audio_detect.c
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -c -o audio_detect.o audio_detect.c 

audio_detect: audio_detect.o  
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) $(LDFLAGS) -o audio_detect audio_detect.o -L. -lsangoma -lm -lpthread

clean:
	rm -fr audio_detect *.o *.so *~ *core* *.so* *.a

distclean: clean
	@echo OK
