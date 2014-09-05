
# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-1998 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= __LINUX__
DEBUG	= 2

# Project file paths.
SYSINC	= /usr/src/linux/include
VPATH	= $(SYSINC)

# Tools options.
CFLAGS = -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I$(SYSINC) -I. \
         -D__LINUX__ -DSTATIC_LIBS= $(GD_STATIC) $(OGR_STATIC) -lstdc++

####### RULES ################################################################

all:	wanctl
	@echo "Ok."

wanctl: cpp_string.cpp\
        conf_file_reader.cpp menu_select_trace_cmd.cpp menu_select_status_statistics_cmd.cpp\
        menu_net_interface_ip_configuration.cpp\
		    net_interface_file_reader.cpp menu_select_station_type.cpp\
    	  menu_protocol_mode_select.cpp\
        menu_main_configuration_options.cpp\
		    text_box_yes_no.cpp dialog_yes_no.cpp\
    		text_box.cpp menu_base.cpp\
		    input_box.cpp dialog_base.cpp main.cpp

	$(CXX) $(CFLAGS) -o $@ $^ -ll -lm

clean:
	rm -f wanctl
	rm -f *.o
	
install:
	install -D wanctl /usr/sbin/wanctl

uninstall:
	rm -f /usr/sbin/wanctl
