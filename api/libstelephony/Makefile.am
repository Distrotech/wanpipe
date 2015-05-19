 
WLINC=../../patches/kdrivers/include
WINC=/usr/include/wanpipe

AM_CFLAGS		= -fPIC -Wall -Wstrict-prototypes -Wmissing-prototypes -g -I$(WLINC) -I$(WINC)  -I. \
	-D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -O2 -D_DEBUG_=2 -D_GNUC_ -I../lib -Istel_tone 
#-x c++ 
AM_CXXFLAGS		= -fPIC -Wall -g -I$(WLINC) -I$(WINC)  -I. \
	-D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -O2 -D_DEBUG_=2 -D_GNUC_ -I../lib  -Istel_tone


lib_LTLIBRARIES = libstelephony.la
include_HEADERS = libstelephony.h 

libstelephony_la_SOURCES = libstelephony.cpp PToneDecoder.cpp PToneEncoder.cpp Q931EventsDecoder.cpp stelephony.cpp stel_tone/stel_tone.c stel_tone/wp_uart.c stel_tone/wp_libteletone_generate.c stel_tone/wp_libteletone_detect.c stel_tone/wp_fsk.c

libstelephony_la_CFLAGS    = $(AM_CFLAGS)
libstelephony_la_CXXFLAGS  = $(AM_CXXFLAGS)
libstelephony_la_LDFLAGS   = -version-info 1:0:0

