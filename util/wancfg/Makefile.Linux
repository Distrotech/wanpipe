
# ============================================================================
# Makefile	Make script for building Linux WAN router utilities.
# ----------------------------------------------------------------------------
# Copyright (c) 1995-1998 Sangoma Technologies Inc.  All Rights Reserved.
# ============================================================================

####### MACROS ###############################################################

# Build options.
OS_TYPE	= __LINUX__
EBUG	= 2
#these protocols defined only for Linux, not for BSD:
PROTOCOL_DEFINES = -DCONFIG_PRODUCT_WANPIPE_LIP_ATM -DCONFIG_PRODUCT_WANPIPE_AFT_BRI
#PROTOCOL_DEFINES += -DZAPTEL_PARSER

WLINC=../../patches/kdrivers/include
WEC=../../patches/kdrivers/wanec
WECOCT=../../patches/kdrivers/wanec/oct6100_api/include

# Project file paths. Initilize to compile in develpment environment.
# Changed when Setup script is running.
WAN_VIRTUAL=
ENABLE_ZAPTEL_PARSER=
ZAPINC=/usr/src/zaptel

ifndef SYSINC
	SYSINC=$(WAN_VIRTUAL)/usr/include/wanpipe
endif

# Tools options.
#CFLAGS = -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I. \
         -D__LINUX__ -DSTATIC_LIBS=$(GD_STATIC) $(OGR_STATIC) $(PROTOCOL_DEFINES)

CXXFLAGS = -Wall  -Wno-write-strings -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I. -D__LINUX__  -I$(WLINC) -I$(WEC) -I$(WECOCT) -I$(SYSINC) $(PROTOCOL_DEFINES)

#uncomment this to compile from CVS, also may change to your zaptel installation directory
#CFLAGS += -I/usr/src/zaptel

#CFLAGS += -Wall -O2 -D$(OS_TYPE) -D_DEBUG_=$(DEBUG) -D_GNUC_ -I. \
         -DSTATIC_LIBS= $(GD_STATIC) $(OGR_STATIC) $(PROTOCOL_DEFINES)

CXXFLAGS += $(EXTRA_FLAGS)

#For gcc 4.6 to be added later
#-Wno-write-strings -Wno-unused-but-set-variable -Wno-int-to-pointer-cast

ifeq "${ENABLE_ZAPTEL_PARSER}" "YES"
CXXFLAGS += -DZAPTEL_PARSER -I$(ZAPINC)
endif

ifeq "${DAHDI_ISSUES}" "YES"
CXXFLAGS += -DDAHDI_ISSUES
endif

OBJS=cpp_string.o \
	conf_file_reader.o \
	text_box_yes_no.o \
	dialog_yes_no.o \
	text_box.o \
	menu_base.o \
	input_box.o \
	dialog_base.o \
	menu_main_configuration_options.o \
	menu_list_existing_wanpipes.o \
	menu_list_all_wanpipes.o \
	menu_new_device_configuration.o \
	menu_hardware_setup.o \
	menu_hardware_card_type.o \
	menu_select_card_type_manualy.o \
	menu_hardware_serial_card_advanced_options.o \
	menu_hardware_select_serial_clock_source.o \
	menu_hardware_te1_card_advanced_options.o \
	menu_hardware_te3_card_advanced_options.o \
	menu_hardware_select_comms_port.o \
	menu_te1_select_media.o \
	menu_te3_select_media.o \
	menu_hardware_serial_select_medium.o \
	menu_s508_io_port_select.o \
	menu_s508_irq_select.o \
	menu_s508_memory_addr.o \
	menu_te_select_line_decoding.o \
	menu_te_select_framing.o \
	menu_t1_lbo.o \
	input_box_active_channels.o \
	menu_te1_clock_mode.o \
	menu_hardware_probe.o \
	menu_aft_logical_channels_list.o \
	input_box_number_of_logical_channels.o \
	menu_aft_logical_channel_cfg.o \
	net_interface_file_reader.o \
	menu_advanced_pci_configuration.o \
	menu_hardware_cpu_number.o \
	menu_wan_channel_cfg.o \
	menu_frame_relay_basic_cfg.o \
	menu_frame_relay_signalling.o \
	input_box_number_of_dlcis.o \
	menu_frame_relay_dlci_configuration.o \
	menu_frame_relay_advanced_dlci_configuration.o \
	menu_frame_relay_cir_configuration.o \
	menu_frame_relay_arp.o \
	menu_frame_relay_manual_or_auto_dlci_cfg.o \
	input_box_frame_relay_dlci_number.o \
	menu_frame_relay_dlci_list.o \
	menu_frame_relay_advanced_global_configuration.o \
	menu_select_protocol.o \
	menu_ppp_basic_cfg.o \
	menu_ppp_ip_mode.o \
	menu_ppp_select_authentication_protocol.o \
	menu_net_interfaces_list.o \
	menu_net_interface_setup.o \
	menu_net_interface_ip_configuration.o \
	menu_net_interface_operation_mode.o \
	menu_net_interface_miscellaneous_options.o \
	menu_chdlc_basic_cfg.o \
	menu_chdlc_advanced_cfg.o \
	menu_device_miscellaneous_options.o \
	conf_file_writer.o \
	menu_adsl_encapsulation.o \
	menu_adsl_advanced_cfg.o \
	menu_adsl_standard.o \
	menu_adsl_trellis.o \
	menu_adsl_coding_gain.o \
	menu_adsl_rx_bin_adjust.o \
	menu_adsl_framing_struct.o \
	menu_adsl_exchange_type.o \
	menu_adsl_clock_type.o \
	wanrouter_rc_file_reader.o \
	menu_wan_channel_cfg_v1.o \
	menu_lapb_basic_cfg.o \
	menu_atm_basic_cfg.o \
	menu_atm_interface_configuration.o \
	message_box.o \
	menu_tdmv_law.o \
	menu_e1_lbo.o \
	main.o

ifeq "${ENABLE_ZAPTEL_PARSER}" "YES"
OBJS += zaptel_conf_file_reader.o \
	sangoma_card_list.o
endif

LDLIBS = -lm -lstdc++ -lncurses

####### RULES ################################################################

all:	wancfg
	@echo "Ok."

wancfg: $(OBJS)
	$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -f wancfg
	rm -f *.o

install:
	@echo "Installing WANCFG in $(WAN_VIRTUAL)/usr/sbin"
	install -D wancfg $(WAN_VIRTUAL)/usr/sbin/wancfg

uninstall:
	@echo "Uninstalling WANCFG in $(WAN_VIRTUAL)/usr/sbin"
	rm -f $(WAN_VIRTUAL)/usr/sbin/wancfg
