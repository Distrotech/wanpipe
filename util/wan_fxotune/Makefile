# ============================================================================
# Makefile
# Make script for building WAN router test programs in Linux and Windows
# ----------------------------------------------------------------------------
# Copyright (c) 1995-2010 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE=$(shell if [ 1 -eq `uname | grep -c "CYGWIN"` ]; then echo "__WINDOWS__"; else echo "__LINUX__"; fi)

##### LINUX

ifeq "$(OS_TYPE)" "__LINUX__"

DEBUG	= 2
SYSINC=$(shell if [ -f ../../.sysinclude ]; then cat ../../.sysinclude;  else echo ""; fi)
ifeq "$(SYSINC)" "" 
SYSINC=/common/include
endif
VPATH	= $(SYSINC)

# Tools options.
CFLAGS	= -Wall -O2 -D$(OS_TYPE) -DWANPIPE_TDM_API -D_DEBUG_=$(DEBUG) -D_GNUC_ -g 

CFLAGS	+= -I.
CFLAGS  += -I/usr/include/wanpipe


LDFLAGS  = -lsangoma
LDFLAGS += -lm
endif

###### WINDOWS
ifeq "$(OS_TYPE)" "__WINDOWS__"

CC = cl.exe

DEBUG	= 2
CFLAGS	= -D$(OS_TYPE) -DWANPIPE_TDM_API -D_DEBUG_=$(DEBUG) 
CFLAGS  += /Zi /Od
CFLAGS  += -DWIN32
CFLAGS  += -I.
CFLAGS  += -I"C:\Program Files\Microsoft Visual Studio 9.0\VC\INCLUDE" -I"C:\Program Files\Microsoft SDKs\Windows\v6.0A\include"
CFLAGS  += -I"C:\Program Files\Sangoma\include" 
LDFLAGS = /link "C:\Program Files\Sangoma\api\lib\x86\libsangoma.lib"
#LDFLAGS += kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

endif


TARGETS = wan_fxotune

####### RULES ################################################################

all:	$(TARGETS)  
	@echo "Ok."

wan_fxotune: wan_fxotune.c libteletone_generate.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) 

clean:
	rm -f $(TARGETS) *.ilk *.obj *.pdb


