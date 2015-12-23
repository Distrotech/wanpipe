# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-1998 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= __LINUX__
DEBUG	= 2

LINK_ARGS= -lcurses -lpthread -LSangoma/Code/Sangoma -lSangoma

# Tools options.
CFLAGS	= -Wall -g -O0 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG)  -D_GNUC_
CFLAGS += -I. -ISangoma/Code -ISangoma/Code/Sangoma -I../../ -I../../../../patches/kdrivers/include -I../../wanec -I../../../../patches/kdrivers/wanec -I../../../../patches/kdrivers/wanec/oct6100_api/include
CFLAGS += -I/usr/include/wanpipe
CFLAGS += $(EXTRA_FLAGS) -D__LINUX__ -DLINUX
CFLAGS += -D_REENTRANT -D_GNU_SOURCE

CXX=g++ 

TARGETS=sample_data_tapping
LIBS=

####### RULES ################################################################
	#make -C Sangoma/Code/Sangoma

all: pre_build $(TARGETS)  
	@echo "Ok."

pre_build:
	make -f Makefile.Linux -C Sangoma/Code/Sangoma

sample_data_tapping:     sample_data_tapping.cpp  $(LIBS) $(LIBS_STEL) $(LIBS_TONE)
	$(CXX) $(CFLAGS) -o $@ $^  $(LINK_ARGS)

clean:
	rm -f $(TARGETS)
	make -f Makefile.Linux -C  Sangoma/Code/Sangoma clean
