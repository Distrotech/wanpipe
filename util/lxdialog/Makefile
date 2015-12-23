CC = gcc 
CPP = gcc -E
#Will be overwritten by ./Setup
SYSINC=/lib/modules/$(shell uname -r)/build/include

CFLAGS = -D__LINUX__ -Wall -Wstrict-prototypes -I$(SYSINC) -O2 -fomit-frame-pointer -DLOCALE 
LDFLAGS = -s -L .
LDLIBS = -lncurses
WAN_VIRTUAL=

CFLAGS += -I$(ASTBROOT)/include -DCURSES_LOC="<ncurses.h>"

#ifeq (/usr/include/ncurses/ncurses.h, $(wildcard /usr/include/ncurses/ncurses.h))
#        CFLAGS += -I/usr/include/ncurses -DCURSES_LOC="<ncurses.h>"
#else
#ifeq (/usr/include/ncurses/curses.h, $(wildcard /usr/include/ncurses/curses.h))
#        CFLAGS += -I/usr/include/ncurses -DCURSES_LOC="<ncurses/curses.h>"
#else
#ifeq (/usr/include/ncurses.h, $(wildcard /usr/include/ncurses.h))
#        CFLAGS += -DCURSES_LOC="<ncurses.h>"
#else
#	CFLAGS += -DCURSES_LOC="<curses.h>"
#endif
#endif
#endif

OBJS = checklist.o menubox.o textbox.o yesno.o inputbox.o \
       util.o lxdialog.o msgbox.o text_yesno.o menubox_back.o menu_text.o
SRCS = $(OBJS:.o=.c)


all: 	ncurses \
	lxdialog
	@echo "Ok."

lxdialog: $(OBJS)

ncurses:
	@x=`find $(ASTBROOT)/lib/ /lib/ /usr/lib/ /usr/lib64/ /usr/lib/i386-linux-gnu /usr/lib/x86_64-linux-gnu /usr/local/lib/ -maxdepth 1 -name 'libncurses.*' 2> /dev/null` ;\
	if [ ! "$$x" ]; then \
		echo -e "\007" ;\
		echo ">> Unable to find the Ncurses libraries." ;\
		echo ">>" ;\
		echo ">> You must have Ncurses installed" ;\
		echo ;\
		exit 1 ;\
	fi

clean:
	rm -f core *.o *~ lxdialog

install:
	install -D -m 755 lxdialog $(WAN_VIRTUAL)/usr/sbin/wanpipe_lxdialog

uninstall:
	rm -f $(WAN_VIRTUAL)/usr/sbin/wanpipe_lxdialog
