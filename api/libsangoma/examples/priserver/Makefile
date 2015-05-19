CC=gcc
LINUXINC=/lib/modules/$(shell uname -r)/build/include
WANINC=../../../patches/kdrivers/include
INCLUDES=-I../../ -I/usr/include -I/usr/include/wanpipe -I/usr/src/libpri -I$(WANINC) -I.  -I/usr/local/include 
CFLAGS=-D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -O6 -march=i686 -DAFT_A104
CCFLAGS=-Wall -Wstrict-prototypes -Wmissing-prototypes -g

all: priserver 

everything:  priserver

priserver: priserver.o 
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -o priserver priserver.o -L. -lpri -lsangoma -lm

priserver.o: priserver.c
	$(CC) $(CCFLAGS) $(INCLUDES) $(CFLAGS) -c -o priserver.o priserver.c

clean:
	rm -fr priserver *.o *.so *~ *core* *.so* *.a

distclean: clean
	@echo OK
