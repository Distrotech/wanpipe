
WLINC=../../patches/kdrivers/include
WEC=../../patches/kdrivers/wanec
WECOCT=../../patches/kdrivers/wanec/oct6100_api/include
REPODIR=../../include
REPODIR1=../../../code/include
REPOEC=../../wanec
REPOECOCT=$(REPOEC)/oct6100_api/include
WINC=/usr/include/wanpipe

AM_CFLAGS		= -fPIC -Wall -Wstrict-prototypes -Wmissing-prototypes -g -I./wanec -I$(WLINC) -I$(WEC) -I$(WECOCT) -I$(WINC) -I$(WINC)/oct6100_api  -I. -I$(REPODIR1) -I$(REPOECOCT) -I$(REPODIR)/aft_core -I$(REPOEC) -I$(REPODIR) \
	-D__LINUX__ -D_REENTRANT -D_GNU_SOURCE -O2 -D_DEBUG_=2 -D_GNUC_ -I../lib  -D__COMPILING_LIBSANGOMA__ 

# pedantic flags that will help us to keep clean code
# I feel sorry for the coders who like messy-warnings-all-over-the-place code
AM_CFLAGS += -Wall -Wwrite-strings -Wunused-variable -Wstrict-prototypes -Wmissing-prototypes

#ACLOCAL_AMFLAGS = -I m4 

LIB_SOURCES		= libsangoma.c libsangoma_utils.c libsangoma_config.c libsangoma_hwec.c libsangoma.h libhpsangoma.c libhpsangoma.h libhpsangoma_priv.c libhpsangoma_priv.h wanec/wanec_api.c wanec/wanec_api_lib.c wanec/wanec_api.h

if LIBPRI
LIB_SOURCES		+= sangoma_pri.c sangoma_pri.h
endif 

library_includedir      = $(includedir)
library_include_HEADERS = libsangoma.h libhpsangoma.h wanec/wanec_api.h

if LIBPRI
library_include_HEADERS	+= sangoma_pri.h
AM_CFLAGS               += -I@LIBPRI_PATH@
endif

lib_LTLIBRARIES         = libsangoma.la
libsangoma_la_CFLAGS    = $(AM_CFLAGS)
libsangoma_la_LDFLAGS   = -version-info @LIBSANGOMA_LT_CURRENT@:@LIBSANGOMA_LT_REVISION@:@LIBSANGOMA_LT_AGE@

if LIBPRI
libsangoma_la_CFLAGS	+= -I$(libpripath)
libsangoma_la_LDFLAGS	+= -L$(libpripath) -lpri
endif 

# We use powf in the gain code
libsangoma_la_LDFLAGS   += -lm

libsangoma_la_SOURCES   = $(LIB_SOURCES)

lib_LIBRARIES           = libsangoma.a
libsangoma_a_CFLAGS     = $(AM_CFLAGS)
libsangoma_a_SOURCES    = $(LIB_SOUCES)
